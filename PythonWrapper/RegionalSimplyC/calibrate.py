import imp
import lmfit
import pandas as pd
import numpy as np
#import matplotlib
#import matplotlib.pyplot as plt


def normal_weight(series) :
	size = np.count_nonzero(~np.isnan(series))
	return series / np.sqrt(size)

# Custom residual function that takes into account yearly sums as well as daily values
def resid(params, dataset, comparisons, norm=False, skip_timesteps=0) :
	dataset_copy = dataset.copy()
	cu.set_parameter_values(params, dataset_copy)
	dataset_copy.run_model()
    
	residuals = []
    
	for i, comparison in enumerate(comparisons):
		simname, simindexes, obsname, obsindexes, weighsums = comparison
    
		sim = dataset_copy.get_result_series(simname, simindexes)[skip_timesteps:]
		obs = dataset_copy.get_input_series(obsname, obsindexes, alignwithresults=True)[skip_timesteps:]
        
		if np.isnan(sim).any() :
			raise ValueError('Got a NaN in the simulated data')

		resid = sim - obs
        
		residuals.append(normal_weight(resid))
        
		if weighsums :
			date_index = cu.get_date_index(dataset)[skip_timesteps:]
			yearly_resid = pd.Series(data=resid, index=date_index, copy=False)
			yearly_resid = yearly_resid.resample('Y').sum()
			residuals.append(normal_weight(yearly_resid.values))

		dataset_copy.delete()   
    
	return np.concatenate(residuals)





wrapper_fpath = (r"..\mobius.py")
wr = imp.load_source('mobius', wrapper_fpath)
wr.initialize('..\..\Applications\SimplyC\SimplyC.dll')

# Calibration functions
calib_fpath = (r"..\mobius_calib_uncert_lmfit.py")
cu = imp.load_source('mobius_calib_uncert_lmfit', calib_fpath)

sites = ['Langtjern', 'Storgama', 'Birkenes']
infiles = ['langtjerninputs', 'inputs_Storgama', 'inputs_Birkenes']
parfiles = ['params_DOC_dissolution2', 'params_Storgama_conc_based_creation', 'params_Birkenes']

for idx, site in enumerate(sites) :
	dataset = wr.DataSet.setup_from_parameter_and_input_files('..\..\Applications\SimplyC\%s\%s.dat' % (site, parfiles[idx]), '..\..\Applications\SimplyC\%s\%s.dat' % (site, infiles[idx]))
	
	param_df = cu.get_double_parameters_as_dataframe(dataset)
	
	comparisons = [
		#('Reach flow (daily mean, cumecs)', ['Inlet'], 'Discharge', []),
		('Reach DOC concentration (volume weighted daily mean)', [site], 'Observed DOC', [], False),
		#('DOC flux from reach, daily mean', [site], 'DOC flux', [], True)
	]	
	
	
	# Ooops, this is VERY volatile to changes in model structure!
	vars = [
		0,1,3,4,5,6,13,
		24,25,26,27
	]
	
	calib_df = param_df.loc[vars].reset_index(drop=True)
	
	calib_df['short_name'] = [
		'DDPET',
		'minPET',
		'DDFmelt',
		'Tmelt',
		'msnow',
		'fquick',
		'Ts',
		'kT',
		'kSO4',
		'DOCc',
		'DOCb',
	]
	
	params = cu.parameter_df_to_lmfit(calib_df)


	params['kT'].min = 0.0
	params['kT'].max = 0.1

	params['kSO4'].min = 0.0
	params['kSO4'].max = 1.0

	params['DOCc'].max = 1.0
	params['DOCb'].min = 1.0
	params['DOCb'].max = 15.0

	params['DDFmelt'].max = 7.0
	
	mi, res = cu.minimize_residuals(params, dataset, comparisons, residual_method=resid, method='nelder', norm=False, skip_timesteps=365)
	cu.set_parameter_values(res.params, dataset)
	dataset.run_model()
	cu.print_goodness_of_fit(dataset, comparisons, skip_timesteps=365)
	
	dataset.write_parameters_to_file('Result\params_%s_optimized.dat' % site)
	
	dataset.delete()
	
	