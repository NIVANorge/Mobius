
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

	reduced_only = True      # Some catchments with large lakes or bad flow data are removed

	start_date = '1985-1-1'
	timesteps  = 12052       #NOTE: Some catchments have only late data. Could alternatively have individual periods per catchment
	
	skip_timesteps = 50      #Model 'burn-in' period
	
	comparisons = [
                ('Reach flow (daily mean, cumecs)', ['R0'], 'Observed flow', [], 1.0),
                ('Reach DOC concentration (volume weighted daily mean)', ['R0'], 'Observed DOC', [], 0.4),
               ]
	
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	for index, row in catch_setup.iterrows():
		catch_no = row['met_index']
		catch_name = row['name']
		
		if reduced_only and row['reduced_set']=='n' : continue
		
		#if catch_name != 'Langtjern03' : continue
		
		print('********** Processing location %s ***********' % catch_name)
		
		infile  = 'MobiusFiles/inputs_%d_%s.dat' % (catch_no, catch_name)
		#parfile = 'MobiusFiles/norm2_optim_params_DOC_%d_%s.dat' % (catch_no, catch_name)
		#parfile = 'MobiusFiles/optim_params_%d_%s.dat' % (catch_no, catch_name)
		parfile = 'MobiusFiles/template_params_2lu_%d_%s.dat' % (catch_no, catch_name)
		#parfile = 'MobiusFiles/optim_hydro_2lu_%d_%s.dat' % (catch_no, catch_name)
		#parfile = 'MobiusFiles/template_params_1lu_%d_%s.dat' % (catch_no, catch_name)
		
		dataset = wr.DataSet.setup_from_parameter_and_input_files(parfile, infile)
		
		dataset.set_parameter_uint('Timesteps', [], timesteps)
		dataset.set_parameter_time('Start date', [], start_date)
		
		dataset.run_model()
		
		params = setup_calibration_params(dataset, do_hydro=True, do_doc=True, num_lu=2, relative_conc=True)
		
		print('Initial GOF')
		cu.print_goodness_of_fit(dataset, comparisons, skip_timesteps=skip_timesteps)
		
		#mi, res = cu.minimize_residuals(params, dataset, comparisons, residual_method=resid, method='nelder', iter_cb=None, norm=False, skip_timesteps=skip_timesteps)
		mi, res = cu.minimize_residuals(params, dataset, comparisons, residual_method=resid_rel_conc, method='nelder', iter_cb=None, norm=False, skip_timesteps=skip_timesteps)
		
		#cu.set_parameter_values(res.params, dataset)
		set_rel_conc_params(res.params, dataset)
		
		
		dataset.run_model()
		print('Final GOF')
		cu.print_goodness_of_fit(dataset, comparisons, skip_timesteps=skip_timesteps)
		print('\n\n\n')
		
		#dataset.write_parameters_to_file('MobiusFiles/optim_params_%d_%s.dat' % (catch_no, catch_name))
		#dataset.write_parameters_to_file('MobiusFiles/norm4_optim_params_DOC_%d_%s.dat' % (catch_no, catch_name))
		#dataset.write_parameters_to_file('MobiusFiles/optim_DOC_1lu_%d_%s.dat' % (catch_no, catch_name))
		#dataset.write_parameters_to_file('MobiusFiles/optim_DOC_2lu_%d_%s.dat' % (catch_no, catch_name))
		dataset.write_parameters_to_file('MobiusFiles/optim_DOC_2lu_rel_conc_%d_%s.dat' % (catch_no, catch_name))
		
		dataset.delete()
		

if __name__ == "__main__":
	main()