import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from importlib.machinery import SourceFileLoader

# Initialise wrapper
wr = SourceFileLoader("mobius", r"../mobius.py").load_module()
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"..\mobius_calib_uncert_lmfit.py").load_module()

wr.initialize('../../Applications/SimplyC_regional/simplyc_regional.dll')


simmedQ   = ('Reach flow (daily mean, cumecs)', ['R0'])
observedQ = ('Observed flow', [])

simmeddocflux = ('DOC flux from reach, daily mean', ['R0'])

simmeddocconc = ('Reach DOC concentration (volume weighted daily mean)', ['R0'])
observeddocconc = ('Observed DOC', [])


def nash_sutcliffe(sim, obs) :
	residuals = sim - obs
	nonnan = np.count_nonzero(~np.isnan(residuals))
	sumsquare = np.nansum(np.square(residuals))
	meanob = np.nansum(obs) / nonnan
	
	return 1.0 - sumsquare / np.nansum(np.square(obs - meanob))

def main() :
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	skip_timesteps = 50
	
	print('Name NS(Q) NS(DOC conc) NS(DOC flux)')
	
	for index, row in catch_setup.iterrows():
		catch_no = row['met_index']
		catch_name = row['name']
		fullname = row['fullname']
		
		#if catch_name != 'Dalelva' : continue
		
		infile  = 'MobiusFiles/inputs_%d_%s.dat' % (catch_no, catch_name)
		parfile = 'MobiusFiles/optim_params_DOC_%d_%s.dat' % (catch_no, catch_name)
		
		try:
			dataset = wr.DataSet.setup_from_parameter_and_input_files(parfile, infile)
		except:
			print('Unable to load files for catchment %d %s' % (catch_no, catch_name))
			continue
		
		dataset.run_model()
		
		simmedQ   = ('Reach flow (daily mean, cumecs)', ['R0'])
		observedQ = ('Observed flow', [])

		simmeddocflux = ('DOC flux from reach, daily mean', ['R0'])

		simmeddocconc = ('Reach DOC concentration (volume weighted daily mean)', ['R0'])
		observeddocconc = ('Observed DOC', [])
		
		sim_df = cu.get_result_dataframe(dataset, [simmedQ, simmeddocflux, simmeddocconc])
		obs_df = cu.get_input_dataframe(dataset, [observedQ, observeddocconc], alignwithresults=True)
		
		#sim_df = cu.get_result_dataframe(dataset, [simmedQ])
		#obs_df = cu.get_input_dataframe(dataset, [observedQ], alignwithresults=True)
		
		df = pd.concat([obs_df, sim_df], axis=1)
		
		simQname = cu.combine_name(simmedQ[0], simmedQ[1])
		obsQname = cu.combine_name(observedQ[0], observedQ[1])
		simdocconcname = cu.combine_name(simmeddocconc[0], simmeddocconc[1])
		obsdocconcname = cu.combine_name(observeddocconc[0], observeddocconc[1])
		simfluxname    = cu.combine_name(simmeddocflux[0], simmeddocflux[1])
		obsfluxname = 'Observed DOC flux'
		
		doc_df = df[[obsdocconcname]].copy()
		doc_df.interpolate(inplace=True)
		
		df[obsfluxname] = doc_df[obsdocconcname].values * df[obsQname].values * 86.4  # flux = concentration * flow
		
		NS_Q = nash_sutcliffe(df[simQname].values[skip_timesteps:], df[obsQname].values[skip_timesteps:])
		
		NS_DOC_conc = nash_sutcliffe(df[simdocconcname].values[skip_timesteps:], df[obsdocconcname].values[skip_timesteps:])
		
		NS_DOC_flux = nash_sutcliffe(df[simfluxname].values[skip_timesteps:], df[obsfluxname].values[skip_timesteps:])
		
		print('%s %g %g %g' % (fullname, NS_Q, NS_DOC_conc, NS_DOC_flux))
		
		

if __name__ == "__main__":
	main()