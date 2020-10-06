import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from importlib.machinery import SourceFileLoader
from param_config import setup_calibration_params
from individual_calib import resid


# Initialise wrapper
wr = SourceFileLoader("mobius", r"../mobius.py").load_module()
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"..\mobius_calib_uncert_lmfit.py").load_module()

wr.initialize('../../Applications/SimplyC_regional/simplyc_regional.dll')


def collect_parameter_distributions(non_validation_only=False) :
	param_values = {}
	
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	for index, row in catch_setup.iterrows():
		catch_no = row['met_index']
		catch_name = row['name']
		
		if non_validation_only and row['validation_set']=='y' :
			continue
		
		infile  = 'MobiusFiles/inputs_%d_%s.dat' % (catch_no, catch_name)
		#parfile = 'MobiusFiles/optim_params_DOC_%d_%s.dat' % (catch_no, catch_name)
		parfile = 'MobiusFiles/norm2_optim_params_DOC_%d_%s.dat' % (catch_no, catch_name)
		
		dataset = wr.DataSet.setup_from_parameter_and_input_files(parfile, infile)
		
		params = setup_calibration_params(dataset, do_doc=True)
		
		lu = {'F' : 'Forest', 'S' : 'Shrubs', 'P' : 'Peat'}
		
		for parname in params :
			if params[parname].expr is None :
				if not parname in param_values :
					param_values[parname] = {}
				if parname[-2]=='_' :
					proportion = dataset.get_parameter_double('Land use proportions', ['R0', lu[parname[-1]]])
					if proportion < 0.01 : continue
				param_values[parname][catch_name] = params[parname].value
		
		
		dataset.delete()  #NOTE: Still leaks the model object, but no biggie.
	return param_values

def draw_random_parameter_set(params, param_values) :
	for parname in params :
		distr = param_values[parname]
		idx = np.random.randint(0, len(distr))
		params[parname].value = distr[distr.keys()[idx]]
		
def extrapolate_test() :
	param_values = collect_parameter_distributions(True)
	
	ndraws = 1000
	
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	for index, row in catch_setup.iterrows():
		catch_no = row['met_index']
		catch_name = row['name']
		
		if row['validation_set'] == '-' :
			continue
		
		print('Extrapolating for catchment %s' % catch_name)
		
		infile  = 'MobiusFiles/inputs_%d_%s.dat' % (catch_no, catch_name)
		parfile = 'MobiusFiles/template_params_%d_%s.dat' % (catch_no, catch_name)
		
		start_date = '1985-1-1'
		timesteps  = 12052
		
		dataset = wr.DataSet.setup_from_parameter_and_input_files(parfile, infile)
		dataset.set_parameter_time('Start date', [], start_date)
		dataset.set_parameter_uint('Timesteps', [], timesteps)
		
		skip_timesteps = 50      #Model 'burn-in' period
		
		comparisons = [
					('Reach flow (daily mean, cumecs)', ['R0'], 'Observed flow', [], 1.0),
					('Reach DOC concentration (volume weighted daily mean)', ['R0'], 'Observed DOC', [], 0.2),
				]
		
		obsseries = dataset.get_input_series('Observed DOC', [], alignwithresults=True)
		newobs = np.zeros(len(obsseries))
		newobs[:] = np.nan
		
		firstday = 0
		while firstday < len(obsseries) :
			#find the first day with value in the autumn and record that:
			for day in range(firstday+274, firstday+365):
				val = obsseries[day]
				if not np.isnan(val) :
					newobs[day] = val
					break
		
			firstday += 365  #disregards leap years, but not a big problem in this case
		
		dataset.set_input_series('Observed DOC', [], newobs, alignwithresults=True)
		
		params = setup_calibration_params(dataset, do_doc=True)
		
		for idx in range(nruns) :
			draw_random_parameter_set(params, param_values)
			res = resid(params, dataset, comparisons, norm=True, skip_timesteps=skip_timesteps)
		
		
	
def make_plots() :
	param_values = collect_parameter_distributions()
	
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
		
		
	fig.tight_layout()
	plt.savefig('Figures/parameters.png')
	plt.close()
	
	
	fig, ax = plt.subplots(5, 4)
	fig.set_size_inches(20, 20)
	ax = ax.flatten()
	
	for idx, parname in enumerate(param_values.keys()) :
	
		dict = param_values[parname]
		
		ax[idx].hist(dict.values(), label=parname)

		ax[idx].legend()

	fig.tight_layout()
	plt.savefig('Figures/parameter_distributions.png')
	plt.close()

def main() :
	make_plots()
	

if __name__ == "__main__":
	main()