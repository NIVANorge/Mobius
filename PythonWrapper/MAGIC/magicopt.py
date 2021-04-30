
from importlib.machinery import SourceFileLoader
import lmfit
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

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
	
	#print(res.params)
	
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
	
	
def get_monte_carlo_setup(dataset, idx) :

	wantparams = ['DEP_S', 'PV_S', 'BD_S', 'CEC_S', 'OA_S']
	
	params = get_params(dataset, wantparams, idx)
	
	for par in params :
		params[par].min = 0.9*params[par].value
		params[par].max = 1.1*params[par].value
		
	return params
	
def draw_params(params, num) :
	vals = np.zeros((len(params), num))
	
	#TODO: We should maybe do a more robust latin squares draw here..
	for idx, par in enumerate(params) :
		vals[idx, :] = np.random.uniform(params[par].min, params[par].max, num)

	return vals
	
	
def monte_carlo_run(dataset, mc_params, idx, num=10) :
	
	vals = draw_params(mc_params, num)
	
	def set_mc_val(ds, mc_pars, values):
		for idx, par in enumerate(mc_pars) :
			mc_pars[par].value = values[idx]
		cu.set_parameter_values(mc_pars, ds)
	
	bc_pars, bc_comp = get_base_cations_setup(dataset, idx)
	
	result_datasets = []
	
	for idx in range(0, num) :
		
		dataset_copy = dataset.copy()
		
		set_mc_val(dataset_copy, mc_params, vals[:, idx])
		
		opt_bc_pars = calib(dataset_copy, bc_pars, bc_comp)
		
		result_datasets.append(dataset_copy)
		
	
	return result_datasets, bc_comp
	
	
def plot_mc(result_datasets, comparisons) :
	fig_height = min(30, len(comparisons)*3.5)
	fig, axes = plt.subplots(nrows=len(comparisons), ncols=1, figsize=(15, fig_height)) 
	
	for idx2, comparison in enumerate(comparisons):
	
		simname, simindexes, obsname, obsindexes, *_ = comparison
		
		ds = result_datasets[0]
		unit = ds.get_result_unit(simname) # Assumes that the unit is the same for obs and sim
		df = cu.get_input_dataframe(ds, [(obsname, obsindexes)], alignwithresults=True)
		
		for ds in result_datasets :
			
			sim_df = cu.get_result_dataframe(ds, [(simname, simindexes)])
			
			df = pd.concat([df, sim_df], axis=1)
			
		if len(comparisons) > 1:
			df.plot(ax=axes[idx2], style=['o--', '-'])
			axes[idx2].set_ylabel('$%s$' % unit)
		else :
			df.plot(ax=axes, style=['o--', '-'])
			axes.set_ylabel('$%s$' % unit)

	plt.legend(loc='upper left')
	plt.tight_layout()
	
	
	#for ds in result_datasets :
	#	ds.delete()