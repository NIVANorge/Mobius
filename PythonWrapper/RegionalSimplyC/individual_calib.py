
import numpy as np
import imp
import pandas as pd

from param_config import setup_calibration_params

# Initialise wrapper
wrapper_fpath = (r"..\mobius.py")
wr = imp.load_source('mobius', wrapper_fpath)
wr.initialize('../../Applications/SimplyC/simplyc_regional.dll')

# Calibration functions
calib_fpath = (r"..\mobius_calib_uncert_lmfit.py")
cu = imp.load_source('mobius_calib_uncert_lmfit', calib_fpath)




def resid(params, dataset, comparisons, norm=False, skip_timesteps=0) :
    
	residuals = []
    
	dataset_copy = dataset.copy()
	cu.set_parameter_values(params, dataset_copy)
	dataset_copy.run_model()

	for i, comparison in enumerate(comparisons):
		simname, simindexes, obsname, obsindexes = comparison

		sim = dataset_copy.get_result_series(simname, simindexes)[skip_timesteps:]
		obs = dataset_copy.get_input_series(obsname, obsindexes, alignwithresults=True)[skip_timesteps:]

		if np.isnan(sim).any() :
			raise ValueError('Got a NaN in the simulated data')

		resid = sim - obs

		residuals.append(resid)

	dataset_copy.delete()   
    
	return residuals



def main() :

	start_date = '1985-1-1'
	timesteps  = 12052       #NOTE: Some catchments have only late data. Could alternatively have individual periods per catchment
	
	skip_timesteps = 50      #Model 'burn-in' period
	
	comparisons = [
                ('Reach flow (daily mean, cumecs)', ['R0'], 'Observed flow', []),
                #('Reach DOC concentration (volume weighted daily mean)', ['R0'], 'Observed DOC', []),
               ]
	
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	for index, row in catch_setup.iterrows():
		catch_no = row['met_index']
		catch_name = row['name']
		
		print('********** Processing location %s ***********' % catch_name)
		
		infile  = 'MobiusFiles/inputs_%d_%s.dat' % (catch_no, catch_name)
		parfile = 'MobiusFiles/template_params_%d_%s.dat' % (catch_no, catch_name)
		
		dataset = wr.DataSet.setup_from_parameter_and_input_files(parfile, infile)
		
		dataset.set_parameter_uint('Timesteps', [], timesteps)
		dataset.set_parameter_time('Start date', [], start_date)
		
		dataset.run_model()
		
		params = setup_calibration_params(dataset)
		
		print('Initial GOF')
		cu.print_goodness_of_fit(dataset, comparisons, skip_timesteps=skip_timesteps)
		
		mi, res = cu.minimize_residuals(params, dataset, comparisons, residual_method=resid, method='leastsq', iter_cb=None, norm=False, skip_timesteps=skip_timesteps)
		
		cu.set_parameter_values(res.params, dataset)
		dataset.run_model()
		print('Final GOF')
		cu.print_goodness_of_fit(dataset, comparisons, skip_timesteps=skip_timesteps)
		print('\n\n\n')
		
		dataset.write_parameters_to_file('MobiusFiles/optim_params_%d_%s.dat' % (catch_no, catch_name))
		
		dataset.delete()
		

if __name__ == "__main__":
	main()