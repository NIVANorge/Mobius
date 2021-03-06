
import warnings
warnings.simplefilter(action='ignore', category=FutureWarning)

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from importlib.machinery import SourceFileLoader
from param_config import setup_calibration_params
from individual_calib import resid
from weighted_quantile import weighted_quantile


#import weighted_p_square as wp


# Initialise wrapper
wr = SourceFileLoader("mobius", r"../mobius.py").load_module()
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"..\mobius_calib_uncert_lmfit.py").load_module()

wr.initialize('../../Applications/SimplyC_regional/simplyc_regional.dll')


def collect_parameter_distributions(non_validation_only=False, reduced_only=True, num_lu=3, fast_high_c=False) :
	param_values = {}
	
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	for index, row in catch_setup.iterrows():
		catch_no = row['met_index']
		catch_name = row['name']
		
		if non_validation_only and row['validation_set']=='y' :
			continue
			
		if reduced_only and row['reduced_set']=='n' :
			continue
		
		infile  = 'MobiusFiles/inputs_%d_%s.dat' % (catch_no, catch_name)
		
		if num_lu == 1 :
			parfile = 'MobiusFiles/optim_DOC_1lu_%d_%s.dat' % (catch_no, catch_name)
			lu = {'F' : 'All'}
		elif num_lu == 2 :
			if fast_high_c :
				parfile = 'MobiusFiles/optim_DOC_2lu_fast_highC_%d_%s.dat' % (catch_no, catch_name)
			else :
				parfile = 'MobiusFiles/optim_DOC_2lu_%d_%s.dat' % (catch_no, catch_name)
			lu = {'F' : 'LowC', 'P' : 'HighC'}
		elif num_lu == 3 :
			parfile = 'MobiusFiles/norm4_optim_params_DOC_%d_%s.dat' % (catch_no, catch_name)
			lu = {'F' : 'Forest', 'S' : 'Shrubs', 'P' : 'Peat'}
			
		#parfile = 'MobiusFiles/optim_params_%d_%s.dat' % (catch_no, catch_name)
		
		dataset = wr.DataSet.setup_from_parameter_and_input_files(parfile, infile)
		
		params = setup_calibration_params(dataset, do_doc=True, num_lu=num_lu)
		
		
		
		for parname in params :
			if params[parname].expr is None :
				if not parname in param_values :
					param_values[parname] = {}
				if num_lu > 1 and parname[-2]=='_' :
					proportion = dataset.get_parameter_double('Land use proportions', ['R0', lu[parname[-1]]])
					if proportion < 0.01 : continue
				param_values[parname][catch_name] = params[parname].value
		
		
		dataset.delete()  #NOTE: Still leaks the model object, but no biggie.
	return param_values

def draw_random_parameter_set(params, param_values) :
	for parname in params :
		if parname in param_values :
			distr = param_values[parname]
			idx = np.random.randint(0, len(distr))
			params[parname].value = distr[list(distr)[idx]]


def nse(sim, obs, skip_timesteps=0) :
	simm = sim[skip_timesteps:]
	obss = obs[skip_timesteps:]

	nom   = np.nansum(np.square(obss - simm))
	denom = np.nansum(np.square(obss - np.nanmean(obss)))
	nse = 1.0 - nom / denom
	return nse
	
def nnse(sim, obs, skip_timesteps=0) :
	#Normalized nash-sutcliffe efficiency
	return 1.0 / (2.0 - nse(sim, obs, skip_timesteps))

def extrapolate_test() :
	param_values = collect_parameter_distributions(True)
	
	ndraws = 20000
	
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	fig, ax = plt.subplots(2, 3)
	fig.set_size_inches(80, 30)
	#ax = ax.flatten()
	
	plotindex = 0
	for index, row in catch_setup.iterrows():
		catch_no = row['met_index']
		catch_name = row['name']
		fullanme = row['fullname']
		
		if row['validation_set'] == '-' :
			continue
		
		print('Extrapolating for catchment %s' % catch_name)
		
		infile  = 'MobiusFiles/inputs_%d_%s.dat' % (catch_no, catch_name)
		#parfile = 'MobiusFiles/optim_params_%d_%s.dat' % (catch_no, catch_name)  # Using already-calibrated hydrology
		parfile = 'MobiusFiles/optim_params_%d_%s.dat' % (catch_no, catch_name)  # Using already-calibrated hydrology
		
		start_date = '1985-1-1'
		timesteps  = 12052
		
		dataset = wr.DataSet.setup_from_parameter_and_input_files(parfile, infile)
		dataset.set_parameter_time('Start date', [], start_date)
		dataset.set_parameter_uint('Timesteps', [], timesteps)
		
		skip_timesteps = 50      #Model 'burn-in' period
		
		
		
		obsseries = dataset.get_input_series('Observed DOC', [], alignwithresults=True)
		newobs = np.zeros(len(obsseries))
		newobs[:] = np.nan
		
		firstday = 0
		while firstday < len(obsseries) :
			#find the first day with value in the autumn and record that:
			for day in range(firstday+274, firstday+365):
				if day >= len(obsseries) :  break
				val = obsseries[day]
				if not np.isnan(val) :
					newobs[day] = val
					break
		
			firstday += 365  #disregards leap years, but not a big problem in this case
		
		dataset.set_input_series('Observed DOC', [], newobs, alignwithresults=True)
		
		params = setup_calibration_params(dataset, do_doc=True, do_hydro=False)
		
		
		pars   = []
		
		all_results = np.zeros((ndraws, timesteps))
		weights     = np.zeros(ndraws)
		
		dataset_copy = dataset.copy()
		
		for idx in range(ndraws) :
			draw_random_parameter_set(params, param_values)
			
			pars.append(params)
			
			cu.set_parameter_values(params, dataset_copy)
			
			dataset_copy.run_model()
			
			sim = dataset_copy.get_result_series('Reach DOC concentration (volume weighted daily mean)', ['R0'])
			
			weight = nnse(sim, newobs, skip_timesteps)
			
			all_results[idx, :] = sim
			weights[idx]        = weight
			
			#res = resid(params, dataset, comparisons, norm=True, skip_timesteps=skip_timesteps)
			#res = np.nansum(np.multiply(res, res))
			
			#resids.append(res)
		dataset_copy.delete()
		
		xvals = cu.get_date_index(dataset)
		
		quantiles = [0.05, 0.25, 0.50, 0.75, 0.95]
		#accum = wp.WeightedPSquareAccumulator(dataset.get_parameter_uint('Timesteps', []), quantiles)

		quant_dat = np.zeros((len(quantiles), timesteps))
		
		
		for ts in range(timesteps) :
			quant_dat[:, ts] = weighted_quantile(all_results[:, ts], quantiles, sample_weight=weights)
		
		for idx, quant in enumerate(quantiles) :
			col='#999999'
			thick = 2
			if quant == 0.5:
				col = '#8021A9'
				thick = 3
			ax[0,plotindex].plot(xvals, quant_dat[idx, :], label='%g%% percentile' % (quant*100.0), color=col, linewidth=thick)
			
		best_idx = np.argmax(weights)
		
		ax[0,plotindex].plot(xvals, all_results[best_idx, :], label='best', color='red')
			
		ax[0,plotindex].plot(xvals, obsseries, color='blue', marker='o', markersize=4, label='observed')
		ax[0,plotindex].plot(xvals, newobs, color='#DF6312', marker='o', markersize=10, label='reduced observed')
			
		ax[0,plotindex].legend()
		ax[0,plotindex].set_title('MC extrapolation')
		
		
		print('Extrapolation run:')
		print('N-S of extrapolated "best fit" vs the full observed data: %g' % nse(all_results[best_idx, :], obsseries, skip_timesteps))
		
		
		
		
		
		#Free optimization:
		
		dataset_copy = dataset.copy()
		
		# Clean the parameter set so that we are not biased by the above run
		params = setup_calibration_params(dataset_copy, do_doc=True, do_hydro=False)
		
		comparisons = [
					#('Reach flow (daily mean, cumecs)', ['R0'], 'Observed flow', [], 1.0),
					('Reach DOC concentration (volume weighted daily mean)', ['R0'], 'Observed DOC', [], 1.0),
				]
						
		mi, res = cu.minimize_residuals(params, dataset_copy, comparisons, residual_method=resid, method='nelder', iter_cb=None, norm=False, skip_timesteps=skip_timesteps)
		
		cu.set_parameter_values(res.params, dataset_copy)
		dataset_copy.run_model()
		
		free_optim_res = dataset_copy.get_result_series('Reach DOC concentration (volume weighted daily mean)', ['R0'])
		
		ax[1,plotindex].plot(xvals, free_optim_res, label='best', color='red')
		
		ax[1,plotindex].plot(xvals, obsseries, color='blue', marker='o', markersize=4)
		ax[1,plotindex].plot(xvals, newobs, color='#DF6312', marker='o', markersize=10)
		
		ax[1,plotindex].legend()
		ax[1,plotindex].set_title('Free optimization')
		
		print('Optimization run:')
		print('N-S of freely optimized "best fit" vs the full observed data: %g' % nse(free_optim_res, obsseries, skip_timesteps))
		
		dataset_copy.write_parameters_to_file('MobiusFiles/extrapolate_params_%d_%s.dat' % (catch_no, catch_name))
		
		dataset_copy.delete()
		
		
		plotindex = plotindex+1
		
		#break
	
	#fig.tight_layout()
	plt.savefig('Figures/extrapolations.png')
	plt.close()

	
def make_plots(num_lu=3, fast_high_c=False) :
	param_values = collect_parameter_distributions(num_lu=num_lu, fast_high_c=fast_high_c)
	
	#print(param_values)
	fig, ax = plt.subplots(5, 4)
	fig.set_size_inches(20, 20)
	ax = ax.flatten()
	
	for idx, parname in enumerate(param_values.keys()) :
	
		dict = param_values[parname]
		
		ax[idx].plot(range(len(dict)), dict.values(), label=parname)
		
		ax[idx].set_xticks(range(len(dict)))
		ax[idx].set_xticklabels(dict.keys())
		
		ax[idx].legend()
		
		plt.setp(ax[idx].get_xticklabels(), rotation=45, ha='right', rotation_mode='anchor')
		
	
	suffix = '%dlu%s' % (num_lu, 'fast_highC' if fast_high_c else '')
	
	fig.tight_layout()
	plt.savefig('Figures/parameters_%s.png' % suffix)
	plt.close()
	
	
	fig, ax = plt.subplots(5, 4)
	fig.set_size_inches(20, 20)
	ax = ax.flatten()
	
	for idx, parname in enumerate(param_values.keys()) :
	
		dict = param_values[parname]
		
		ax[idx].hist(dict.values(), label=parname)

		ax[idx].legend()

	fig.tight_layout()
	plt.savefig('Figures/parameter_distributions_%s.png' % suffix)
	plt.close()

def main() :
	make_plots(num_lu=2, fast_high_c=True)
	#extrapolate_test()
	

if __name__ == "__main__":
	main()