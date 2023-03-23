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
	
	setup = 'optim_DOC_1lu_'

	catch_setup = catch_setup.loc[catch_setup['reduced_set']=='y']
	
	results = catch_setup[['fullname', 'lat', 'lon', 'elev_m', 'area_km2', 'lake_catch_km2', 'lu_FSPLO']].copy()
	
	#print('%25s\t%25s\tNS(Q)  NS(DOC conc)  NS(DOC conc month)  NS(DOC flux)  NS(DOC flux month)  | ts_F  ts_P  ts_S  | DOC_F  DOC_P  DOC_S' % ('Catchment', 'Setup'))
	
	nrows = len(catch_setup)
	results['NS(Q)'] = np.zeros(nrows)
	results['NS(DOC conc)'] = np.zeros(nrows)
	results['NS(DOC conc month)'] = np.zeros(nrows)
	results['NS(DOC flux)'] = np.zeros(nrows)
	results['NS(DOC flux month)'] = np.zeros(nrows)
	results['Ts_s'] = np.zeros(nrows)
	results['fc'] = np.zeros(nrows)
	results['DOCbase'] = np.zeros(nrows)
	results['kSO4'] = np.zeros(nrows)
	results['kT1'] = np.zeros(nrows)
	results['kT2'] = np.zeros(nrows)
	results['hlGW'] = np.full(nrows, np.nan)
	results['lakeRad'] = np.full(nrows, np.nan)
	
	for index, row in catch_setup.iterrows():
		
		catch_no = row['met_index']
		catch_name = row['name']
		fullname = row['fullname']

		infile  = 'MobiusFiles/inputs_%d_%s.dat' % (catch_no, catch_name)
		parfile = 'MobiusFiles/OptimResults/%s_%d_%s.dat' % (setup, catch_no, catch_name)
		
		try:
			dataset = wr.DataSet.setup_from_parameter_and_input_files(parfile, infile)
		except:
			print('Unable to load files for %s %d %s' % (setup, catch_no, catch_name))
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
		
		reaches = dataset.get_indexes('Reaches')
		gwmethod = dataset.get_parameter_enum('Deep soil/groundwater DOC computation', [])
		
		haslake = ('Lake' in reaches)
		hasmineral = (gwmethod == 'mass_balance')
		
		Ts_s    = dataset.get_parameter_double('Soil water time constant', ['All'])
		fc      = dataset.get_parameter_double('Soil field capacity', ['All'])
		DOCbase = dataset.get_parameter_double('Baseline Soil DOC concentration', ['All'])
		kSO4    = dataset.get_parameter_double('Soil carbon solubility response to SO4 deposition', [])
		kT1     = dataset.get_parameter_double('Soil temperature DOC creation linear coefficient', [])
		kT2     = dataset.get_parameter_double('Soil temperature DOC creation square coefficient', [])
		hlGW    = np.nan if not hasmineral else dataset.get_parameter_double('Deep soil/groundwater DOC half life', [])
		lakeRad = np.nan if not haslake else    dataset.get_parameter_double('Lake DOC radiation breakdown', [])
		
		results.loc[index, 'NS(Q)'] = NS_Q
		results.loc[index, 'NS(DOC conc)'] = NS_DOC_conc
		results.loc[index, 'NS(DOC conc month)'] = NS_DOC_conc_month
		results.loc[index, 'NS(DOC flux)'] = NS_DOC_flux
		results.loc[index, 'NS(DOC flux month)'] = NS_DOC_flux_month
		results.loc[index, 'Ts_s'] = Ts_s
		results.loc[index, 'fc'] = fc
		results.loc[index, 'DOCbase'] = DOCbase
		results.loc[index, 'kSO4'] = kSO4
		results.loc[index, 'kT1'] = kT1
		results.loc[index, 'kT2'] = kT2
		results.loc[index, 'hlGW'] = hlGW
		results.loc[index, 'lakeRad'] = lakeRad
			
		dataset.delete()
		
		#print('%25s\t%25s\t%2.2f\t%2.2f\t%2.2f\t%2.2f\t%2.2f\t| %2.2f\t%2.2f\t%2.2f\t| %2.2f\t%2.2f\t%2.2f' % (fullname, setup, NS_Q, NS_DOC_conc, NS_DOC_conc_month, NS_DOC_flux, NS_DOC_flux_month, ts_F, ts_P, ts_S, DOC_F, DOC_P, DOC_S))
	
	results.to_excel('result_summary.xlsx', 'Results', index=False)

if __name__ == "__main__":
	main()