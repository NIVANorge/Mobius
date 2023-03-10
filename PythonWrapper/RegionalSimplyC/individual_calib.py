
import numpy as np
import pandas as pd

from param_config import setup_calibration_params

from importlib.machinery import SourceFileLoader

# Initialise wrapper
wr = SourceFileLoader("mobius", r"../mobius.py").load_module()
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"..\mobius_calib_uncert_lmfit.py").load_module()

wr.initialize('../../Applications/SimplyC_regional/simplyc_regional.dll')


def resid_main(params, dataset, comparisons, norm=False, skip_timesteps=0):
	
	residuals = []
	for i, comparison in enumerate(comparisons):
		simname, simindexes, obsname, obsindexes, weight = comparison

		sim = dataset.get_result_series(simname, simindexes)[skip_timesteps:]
		obs = dataset.get_input_series(obsname, obsindexes, alignwithresults=True)[skip_timesteps:]

		if np.isnan(sim).any() :
			raise ValueError('Got a NaN in the simulated data')
		
		nvalues = np.sum(~np.isnan(obs))

		resid = np.sqrt(weight)*(sim - obs) / (np.nanmean(obs) * np.sqrt(nvalues))

		residuals.append(resid)
	
	return residuals

def resid(params, dataset, comparisons, norm=False, skip_timesteps=0) :
    
	dataset_copy = dataset.copy()
	cu.set_parameter_values(params, dataset_copy)
	dataset_copy.run_model()

	residuals = resid_main(params, dataset_copy, comparisons, norm, skip_timesteps)

	dataset_copy.delete()   
    
	return residuals


def set_rel_conc_params(params, dataset) :
	cu.set_parameter_values(params, dataset)
	
	peat_conc = params['baseDOC_F'].value * params['aux_baseDOC_P'].value
	dataset.set_parameter_double('Baseline Soil DOC concentration', ['HighC'], peat_conc)

def resid_rel_conc(params, dataset, comparisons, norm=False, skip_timesteps=0) :
	
	dataset_copy = dataset.copy()
	
	set_rel_conc_params(params, dataset_copy)
	
	dataset_copy.run_model()

	residuals = resid_main(params, dataset_copy, comparisons, norm, skip_timesteps)

	dataset_copy.delete()   
    
	return residuals
	


def main() :

	# Configuration.
	
	reduced_only = True      # Some catchments with bad discharge data are removed
	rel_conc = False         # Calibrate one base DOC concentrations, and set base conc in other lu based on that. Only makes sense for >1 lu
	n_lu     = 1             # Only works for one land use class now.
	
	skip_timesteps = 50      #Model 'burn-in' period
	
	do_doc   = False         # If false, only calibrate on hydrology. Can be beneficial to do one run of that first.
	do_iter_callback = False   # For debug porpoises.

	start_date = '1985-1-1'
	end_date   = '2017-12-31'
	
	do_single_only = -1      # Set the catch_no of the catchment you want to run if you only want to run one. Set to -1 to run all.
	
	
	comparisons = [('Reach flow (daily mean, cumecs)', ['River'], 'Observed flow', [], 1.0)]
	if do_doc :
		comparisons.append(('Reach DOC concentration (volume weighted daily mean)', ['River'], 'Observed DOC', [], 0.4))
			   
	
	def iter_callback(params, iter, resid, *args, **kws) :
		if iter % 100 == 0 :
			print('Optim iteration %d' % iter)
	
	iter_cb = None
	if do_iter_callback :
		iter_cb = iter_callback
	
	
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	for index, row in catch_setup.iterrows():
		catch_no = row['met_index']
		catch_name = row['name']
		
		if reduced_only and row['reduced_set']=='n' : continue
		
		if do_single_only >= 0 and catch_no != do_single_only : continue    # Debug option to just run one catchment
		
		print('********** Processing location %s ***********' % catch_name)
		
		infile  = 'MobiusFiles/inputs_%d_%s.dat' % (catch_no, catch_name)

		if n_lu == 1 :
			if do_doc :
				parfile = 'MobiusFiles/OptimResults/optim_hydro_params_1lu_%d_%s.dat' % (catch_no, catch_name)
			else :
				parfile = 'MobiusFiles/template_params_1lu_%d_%s.dat' % (catch_no, catch_name)
		else :
			raise Exception('Only one land use type is functional now. The script is not updated for the latest version of more lu types')
		
		dataset = wr.DataSet.setup_from_parameter_and_input_files(parfile, infile)
		
		dataset.set_parameter_time('Start date', [], start_date)
		dataset.set_parameter_time('End date', [], end_date)
		
		dataset.run_model()
		
		params = setup_calibration_params(dataset, do_hydro=True, do_doc=do_doc, relative_conc=rel_conc)
		
		print('Initial parameter setup')
		params.pretty_print()
		
		print('Initial GOF')
		cu.print_goodness_of_fit(dataset, comparisons, skip_timesteps=skip_timesteps)
		
		res_method = resid_rel_conc if rel_conc else resid
		
		mi, res = cu.minimize_residuals(params, dataset, comparisons, residual_method=res_method, method='nelder', iter_cb=iter_cb, norm=False, skip_timesteps=skip_timesteps)
		
		if rel_conc :
			set_rel_conc_params(res.params, dataset)
		else :
			cu.set_parameter_values(res.params, dataset)
		
		dataset.run_model()
		print('Final GOF')
		cu.print_goodness_of_fit(dataset, comparisons, skip_timesteps=skip_timesteps)
		print('\n\n\n')
		
		if do_doc :
			dataset.write_parameters_to_file('MobiusFiles/OptimResults/optim_DOC_%dlu_%s_%d_%s.dat' % (n_lu, 'rel_conc' if rel_conc else '', catch_no, catch_name))
		else :
			dataset.write_parameters_to_file('MobiusFiles/OptimResults/optim_hydro_params_%dlu_%d_%s.dat' % (n_lu, catch_no, catch_name))
		
		dataset.delete()
		

if __name__ == "__main__":
	main()