
from importlib.machinery import SourceFileLoader
import lmfit
import numpy as np

cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"..\mobius_calib_uncert_lmfit.py").load_module()


def get_residuals(params, dataset, comparisons, norm=False, skip_timesteps=0) :
    
	dataset_copy = dataset.copy()
	cu.set_parameter_values(params, dataset_copy)
	
	#try :
	dataset_copy.run_model()
	#except :
	#	dataset_copy.delete()   
	#	return np.inf            #Sometimes the initialization of the selectivity coefficients crash.

	residuals = []
	for i, comparison in enumerate(comparisons):
		simname, simindexes, obsname, obsindexes, weight = comparison

		sim = dataset_copy.get_result_series(simname, simindexes)[skip_timesteps:]
		obs = dataset_copy.get_input_series(obsname, obsindexes, alignwithresults=True)[skip_timesteps:]

		if np.isnan(sim).any() :
			raise ValueError('Got a NaN in the simulated data')
		
		nvalues = np.sum(~np.isnan(obs))

		resid = np.sqrt(weight)*(sim - obs) / (np.nanmean(obs) * np.sqrt(nvalues))

		residuals.append(resid)
	

	dataset_copy.delete()   
    
	return residuals


def calib(dataset, params, comparisons) :
	
	mi, res = cu.minimize_residuals(params, dataset, comparisons, residual_method=get_residuals, method='nelder', iter_cb=None, norm=False)
	
	print(res.params)
	
	cu.set_parameter_values(res.params, dataset)
	dataset.run_model()
	
	return res.params
	

def get_params(dataset, wantparams, idx) :

	soilindex, waterindex = idx
	param_df = cu.get_double_parameters_as_dataframe(dataset, index_short_name={soilindex:'S', waterindex:'R'})
	
	calib_df = param_df[[any([sn.startswith(n) for n in wantparams]) for sn in param_df['short_name']]]
	
	params = cu.parameter_df_to_lmfit(calib_df)
	
	return params
	
	
def get_so4_setup(dataset, idx) :
	wantparams = ['SO4half_S', 'SO4max_S', 'WSO4_S']
	
	params = get_params(dataset, wantparams, idx)
	
	soilindex, waterindex = idx
	comparisons = [
                ('SO4(2-) ionic concentration', [waterindex], 'Observed runoff conc SO4', [], 1.0),
				]
	
	return params, comparisons
	

	
def get_base_cations_setup(dataset, idx) :
	
	wantparams = ['ECa_S', 'EMg_S', 'ENa_S', 'EK_S', 'WCa_S', 'WMg_S', 'WNa_S', 'WK_S']    #Maybe also OA_S
	
	params = get_params(dataset, wantparams, idx)
	
	#If we don't do this, we get selectivity coefficients of infinity
	params['ECa_S'].min = 1.0
	params['EMg_S'].min = 1.0
	params['ENa_S'].min = 1.0
	params['EK_S'].min  = 1.0
	
	#TODO: This is not ideal, but the important thing is that they never sum above 100. The user could reconfigure this themselves
	params['ECa_S'].max = 25.0
	params['EMg_S'].max = 25.0
	params['ENa_S'].max = 25.0
	params['EK_S'].max  = 25.0
	

	soilindex, waterindex = idx
	
	comparisons = [
				('Ca(2+) ionic concentration', [waterindex], 'Observed runoff conc Ca', [], 1.0),
				('Mg(2+) ionic concentration', [waterindex], 'Observed runoff conc Mg', [], 1.0),
				('Na(+) ionic concentration', [waterindex], 'Observed runoff conc Na', [], 1.0),
				('K(+) ionic concentration', [waterindex], 'Observed runoff conc K', [], 1.0),
				]
	
	return params, comparisons
	
	
def get_doc_setup(dataset, idx) :
	
	wantparams = ['OA_R']
	
	params = get_params(dataset, wantparams, idx)
	
	soilindex, waterindex = idx
	comparisons = [
		('pH', [waterindex], 'Observed runoff pH', [], 1.0),
	]
	
	return params, comparisons
	