
# Runs the various climate scenarios that we generated input files from in 'extract.py'
# Aggregates the data to generate plots.

import imp
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

from pandas.plotting import register_matplotlib_converters
register_matplotlib_converters()


wrapper_fpath = (r'../../../PythonWrapper/mobius.py')
mobius = imp.load_source('mobius', wrapper_fpath)
mobius.initialize('../incan.dll')


scenarios = ['rcp85', 'rcp45']
models = ['CNRM_CCLM', 'CNRM_RCA', 'EC-EARTH_CCLM', 'EC-EARTH_HIRHAM', 'EC-EARTH_RACMO', 'EC-EARTH_RCA', 'HADGEM_RCA', 'IPSL_RCA', 'MPI_CCLM', 'MPI_RCA']
#models = ['HADGEM_RCA']

start_date = '1990-01-01'
timesteps = 40542
date_idx = np.array(pd.date_range(start_date, periods=timesteps))

df = pd.DataFrame({'Date' : date_idx})
df.set_index('Date', inplace=True)

for scenario in scenarios :

	for model in models :
		
		parfile = 'incan_params_Storelva_to2100.dat'
		inputfile = 'inputs_Storelva_%s_%s.dat' % (model, scenario)
		
		dataset = mobius.DataSet.setup_from_parameter_and_input_files(parfile, inputfile)
		
		dataset.run_model()
		
		df['Q_%s_%s' % (model, scenario)]   = dataset.get_result_series('Reach flow', ['Outlet'])
		df['NO3_%s_%s' % (model, scenario)] = dataset.get_result_series('Reach nitrate concentration', ['Nes Verk'])
		df['P_%s_%s' % (model, scenario)] = dataset.get_input_series('Actual precipitation', [])
		df['T_%s_%s' % (model, scenario)] = dataset.get_input_series('Air temperature', [])
		
		dataset.delete()
		
#print(df)
dataset = mobius.DataSet.setup_from_parameter_and_input_files(parfile, 'incan_inputs_Storelva.dat')
start_date = '1990-01-01'
timesteps =  10591
date_idx2 = np.array(pd.date_range(start_date, periods=timesteps))
obs_df = pd.DataFrame({'Date' : date_idx2})
obs_df.set_index('Date', inplace=True)

obs_df['NO3_obs'] = dataset.get_input_series('Observed NO3', ['Nes Verk'])
obs_df['Q_obs']   = dataset.get_input_series('Observed discharge outlet', [])
obs_df['P_obs']   = dataset.get_input_series('Actual precipitation', [])
obs_df['T_obs']   = dataset.get_input_series('Air temperature', [])
dataset.delete()

df = pd.concat([df, obs_df], axis=1)

#aggr_df = df.resample('Y').mean()    # 'Y' for yearly, 'Q' for quarterly
aggr = df.resample('QS-DEC').mean()


for idx, season in enumerate(['DJF', 'MAM', 'JJA', 'SON']) :
	
	aggr_df = aggr.iloc[(idx+1)::4]
	
	colors = ['red', 'blue']

	variables = ['Q', 'NO3', 'P', 'T']
	names = ['Flow [m3/s]', 'Nitrate [mg/l]', 'Precipitation [mm/day]', 'Air temperature [°C]']

	fig, ax = plt.subplots(len(variables), 1, figsize=(6,10))

	for idx_var, variable in enumerate(variables):
		for idx_scn, scenario in enumerate(scenarios) :

			cols = ['%s_%s_%s' % (variable, model, scenario) for model in models]

			disp_df = aggr_df[cols]

			ax[idx_var].fill_between(disp_df.index, disp_df.min(axis=1).values, disp_df.max(axis=1).values, facecolor=colors[idx_scn], alpha = 0.5)
			ax[idx_var].plot(disp_df.index, disp_df.mean(axis=1).values, color=colors[idx_scn], label=scenario)
		
		
		ax[idx_var].plot(aggr_df.index, aggr_df['%s_obs' % variable].values, color='black', label='observed')
		
		ax[idx_var].set_ylabel(names[idx_var])
		ax[idx_var].legend()

	fig.savefig('plot_%s.png' % season)


