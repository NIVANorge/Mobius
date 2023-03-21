import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from importlib.machinery import SourceFileLoader

# Initialise wrapper
wr = SourceFileLoader("mobius", r"../mobius.py").load_module()
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"..\mobius_calib_uncert_lmfit.py").load_module()

wr.initialize('../../Applications/SimplyC_regional/simplyc_regional.dll')





def nash_sutcliffe(sim, obs) :
	residuals = sim - obs
	nonnan = np.count_nonzero(~np.isnan(residuals))
	sumsquare = np.nansum(np.square(residuals))
	meanob = np.nansum(obs) / nonnan
	
	return 1.0 - sumsquare / np.nansum(np.square(obs - meanob))

def main() :
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	skip_timesteps = 50
	
	reduced_only = True
	
	#setups = ['optim_DOC_1lu', 'optim_DOC_2lu', 'optim_DOC_2lu_fast_highC', 'optim_DOC_2lu_rel_conc', 'norm2_optim_params_DOC', 'norm4_optim_params_DOC']
	setups = ['optim_DOC_1lu_']
	
	print('%25s\t%25s\tNS(Q)  NS(DOC conc)  NS(DOC conc month)  NS(DOC flux)  NS(DOC flux month)  | ts_F  ts_P  ts_S  | DOC_F  DOC_P  DOC_S' % ('Catchment', 'Setup'))
	
	for index, row in catch_setup.iterrows():
		
		catch_no = row['met_index']
		catch_name = row['name']
		fullname = row['fullname']
		
		if reduced_only and row['reduced_set']=='n' : continue
		
		for setup in setups :
			
			infile  = 'MobiusFiles/inputs_%d_%s.dat' % (catch_no, catch_name)
			parfile = 'MobiusFiles/OptimResults/%s_%d_%s.dat' % (setup, catch_no, catch_name)
			
			try:
				dataset = wr.DataSet.setup_from_parameter_and_input_files(parfile, infile)
			except:
				#print('Unable to load files for %s %d %s' % (setup, catch_no, catch_name))
				continue
			
			dataset.run_model()
			
			simmedQ   = ('Reach flow (daily mean, cumecs)', ['River'])
			observedQ = ('Observed flow', [])

			simmeddocflux = ('DOC flux from reach, daily mean', ['River'])
			obsdocflux    = ('Observed DOC flux', [])

			simmeddocconc = ('Reach DOC concentration (volume weighted daily mean)', ['River'])
			observeddocconc = ('Observed DOC', [])
			
			sim_df = cu.get_result_dataframe(dataset, [simmedQ, simmeddocflux, simmeddocconc])
			obs_df = cu.get_input_dataframe(dataset, [observedQ, observeddocconc, obsdocflux], alignwithresults=True)
			
			#sim_df = cu.get_result_dataframe(dataset, [simmedQ])
			#obs_df = cu.get_input_dataframe(dataset, [observedQ], alignwithresults=True)
			
			df = pd.concat([obs_df, sim_df], axis=1)
			
			simQname = cu.combine_name(simmedQ[0], simmedQ[1])
			obsQname = cu.combine_name(observedQ[0], observedQ[1])
			simdocconcname = cu.combine_name(simmeddocconc[0], simmeddocconc[1])
			obsdocconcname = cu.combine_name(observeddocconc[0], observeddocconc[1])
			simfluxname    = cu.combine_name(simmeddocflux[0], simmeddocflux[1])
			obsfluxname    = cu.combine_name(obsdocflux[0], obsdocflux[1])
			
			month_df = df[skip_timesteps:]
			month_df = month_df.resample('M').mean()
			
			NS_Q = nash_sutcliffe(df[simQname].values[skip_timesteps:], df[obsQname].values[skip_timesteps:])
			
			NS_DOC_conc = nash_sutcliffe(df[simdocconcname].values[skip_timesteps:], df[obsdocconcname].values[skip_timesteps:])
			
			NS_DOC_conc_month = nash_sutcliffe(month_df[simdocconcname].values, month_df[obsdocconcname].values)
			
			NS_DOC_flux = nash_sutcliffe(df[simfluxname].values[skip_timesteps:], df[obsfluxname].values[skip_timesteps:])
			
			NS_DOC_flux_month = nash_sutcliffe(month_df[simfluxname].values, month_df[obsfluxname].values)
			
			indexes = dataset.get_indexes('Landscape units')
			ts_F = np.nan
			ts_P = np.nan
			ts_S = np.nan
			DOC_F = np.nan
			DOC_P = np.nan
			DOC_S = np.nan
			if 'Forest' in indexes :
				ts_F  = dataset.get_parameter_double('Soil water time constant', ['Forest'])
				DOC_F = dataset.get_parameter_double('Baseline Soil DOC concentration', ['Forest'])
			if 'Shrubs' in indexes :
				ts_S  = dataset.get_parameter_double('Soil water time constant', ['Shrubs'])
				DOC_S = dataset.get_parameter_double('Baseline Soil DOC concentration', ['Shrubs'])
			if 'Peat' in indexes :
				ts_P  = dataset.get_parameter_double('Soil water time constant', ['Peat'])
				DOC_P = dataset.get_parameter_double('Baseline Soil DOC concentration', ['Peat'])
			if 'LowC' in indexes :
				ts_F  = dataset.get_parameter_double('Soil water time constant', ['LowC'])
				DOC_F = dataset.get_parameter_double('Baseline Soil DOC concentration', ['LowC'])
			if 'HighC' in indexes :
				ts_P  = dataset.get_parameter_double('Soil water time constant', ['HighC'])
				DOC_P = dataset.get_parameter_double('Baseline Soil DOC concentration', ['HighC'])
			if 'All' in indexes :
				ts_F  = dataset.get_parameter_double('Soil water time constant', ['All'])
				DOC_F = dataset.get_parameter_double('Baseline Soil DOC concentration', ['All'])
				
			dataset.delete()
			
			print('%25s\t%25s\t%2.2f\t%2.2f\t%2.2f\t%2.2f\t%2.2f\t| %2.2f\t%2.2f\t%2.2f\t| %2.2f\t%2.2f\t%2.2f' % (fullname, setup, NS_Q, NS_DOC_conc, NS_DOC_conc_month, NS_DOC_flux, NS_DOC_flux_month, ts_F, ts_P, ts_S, DOC_F, DOC_P, DOC_S))
		print('')
		

if __name__ == "__main__":
	main()