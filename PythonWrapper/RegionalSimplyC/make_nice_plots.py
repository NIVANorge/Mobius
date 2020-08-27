

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from importlib.machinery import SourceFileLoader

# Initialise wrapper
wr = SourceFileLoader("mobius", r"../mobius.py").load_module()
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"..\mobius_calib_uncert_lmfit.py").load_module()

wr.initialize('../../Applications/SimplyC/simplyc_regional.dll')


param_file_prefix = 'optim_params_doc'

simmed   = ('Reach flow (daily mean, cumecs)', ['R0'])
observed = ('Observed flow', [])

simmeddoc = ('DOC flux from reach, daily mean', ['R0'])
observeddoc = ('Observed DOC', [])


def add_normalized_plot(ax, values, label):
	mn = np.mean(values)
	sd = np.std(values)
	
	ax.plot(range(1, 13), (values - mn)/sd, label=label)


def main() :
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	for index, row in catch_setup.iterrows():
		catch_no = row['met_index']
		catch_name = row['name']
		
		infile  = 'MobiusFiles/inputs_%d_%s.dat' % (catch_no, catch_name)
		parfile = 'MobiusFiles/%s_%d_%s.dat' % (param_file_prefix, catch_no, catch_name)
		
		try:
			dataset = wr.DataSet.setup_from_parameter_and_input_files(parfile, infile)
		except:
			print('Unable to load files for catchment %d %s' % (catch_no, catch_name))
			continue
		
		dataset.run_model()
		
		sim_df = cu.get_result_dataframe(dataset, [simmed, simmeddoc])
		obs_df = cu.get_input_dataframe(dataset, [observed, observeddoc], alignwithresults=True)
		
		df = pd.concat([obs_df, sim_df], axis=1)
		
		simname = cu.combine_name(simmed[0], simmed[1])
		obsname = cu.combine_name(observed[0], observed[1])
		simdocname = cu.combine_name(simmeddoc[0], simmeddoc[1])
		obsdocname = cu.combine_name(observeddoc[0], observeddoc[1])
		obsfluxname = 'Observed DOC flux'
		
		doc_df = df[[obsdocname]].copy()
		doc_df.interpolate(inplace=True)
		
		df[obsfluxname] = doc_df[obsdocname].values * df[obsname].values * 86400.0  # flux = concentration * flow
		
		df = df.resample('MS').mean()
		
		obs = np.zeros(12)
		sim = np.zeros(12)
		fluxobs = np.zeros(12)
		fluxsim = np.zeros(12)
		
		
		for index, row in df.iterrows() :
			sim[index.month-1] += row[simname]
			if not np.isnan(row[obsname]):
				obs[index.month-1] += row[obsname]
				
			fluxsim[index.month-1] += row[simdocname]
			if not np.isnan(row[obsfluxname]):
				fluxobs[index.month-1] += row[obsfluxname]

		fig, ax = plt.subplots(1, 2)
		fig.set_size_inches(10, 4.5)
		add_normalized_plot(ax[0], obs, 'obs')
		add_normalized_plot(ax[0], sim, 'sim')
		
		add_normalized_plot(ax[1], fluxobs, 'obs')
		add_normalized_plot(ax[1], fluxsim, 'sim')
		
		ax[0].set_title('Q')
		ax[0].legend()
		ax[0].set_xticks(range(1,13))
		ax[0].axhline(0, color='grey')
		
		ax[1].set_title('DOC flux')
		ax[1].legend()
		ax[1].set_xticks(range(1,13))
		ax[1].axhline(0, color='grey')
		
		fig.suptitle('%s by month (1985-2017)' % catch_name, fontsize=16)    #NOTE: Title is incorrect if we change run period
		
		plt.savefig('Figures/%d_%s_flow_month.png' % (catch_no, catch_name))
		
		plt.close()
		

if __name__ == "__main__":
	main()