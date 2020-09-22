

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from importlib.machinery import SourceFileLoader

# Initialise wrapper
wr = SourceFileLoader("mobius", r"../mobius.py").load_module()
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"..\mobius_calib_uncert_lmfit.py").load_module()

wr.initialize('../../Applications/SimplyC_regional/simplyc_regional.dll')


param_file_prefix = 'optim_params_doc'

simmed   = ('Reach flow (daily mean, cumecs)', ['R0'])
observed = ('Observed flow', [])

simmeddoc = ('DOC flux from reach, daily mean', ['R0'])
observeddoc = ('Observed DOC', [])


def add_normalized_plot(ax, xvals, values, label):
	mn = np.nanmean(values)
	sd = np.nanstd(values)
	
	ax.plot(xvals, (values - mn)/sd, label=label)


def main() :
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	for index, row in catch_setup.iterrows():
		catch_no = row['met_index']
		catch_name = row['name']
		
		#if catch_name != 'Dalelva' : continue
		
		infile  = 'MobiusFiles/inputs_%d_%s.dat' % (catch_no, catch_name)
		parfile = 'MobiusFiles/%s_%d_%s_2.dat' % (param_file_prefix, catch_no, catch_name)
		
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
		
		df_month = df.resample('MS').mean()
		
		obs = np.zeros(12)
		sim = np.zeros(12)
		fluxobs = np.zeros(12)
		fluxsim = np.zeros(12)
		
		
		for index, row in df_month.iterrows() :
			sim[index.month-1] += row[simname]
			if not np.isnan(row[obsname]):
				obs[index.month-1] += row[obsname]
				
			fluxsim[index.month-1] += row[simdocname]
			if not np.isnan(row[obsfluxname]):
				fluxobs[index.month-1] += row[obsfluxname]

		fig, ax = plt.subplots(1, 2)
		fig.set_size_inches(10, 4.5)
		
		month_x = range(1, 13)
		
		add_normalized_plot(ax[0], month_x, obs, 'obs')
		add_normalized_plot(ax[0], month_x, sim, 'sim')
		
		add_normalized_plot(ax[1], month_x, fluxobs, 'obs')
		add_normalized_plot(ax[1], month_x, fluxsim, 'sim')
		
		ax[0].set_title('Q')
		ax[0].legend()
		ax[0].set_xticks(month_x)
		ax[0].axhline(0, color='grey')
		
		ax[1].set_title('DOC flux')
		ax[1].legend()
		ax[1].set_xticks(month_x)
		ax[1].axhline(0, color='grey')
		
		fig.suptitle('%s by month (1985-2017)' % catch_name, fontsize=16)    #NOTE: Title is incorrect if we change run period
		plt.savefig('Figures/month_%d_%s_2.png' % (catch_no, catch_name))
		plt.close()
		
		df_year = df.resample('Y').mean()
		
		#print(df_year)
		
		fig, ax = plt.subplots(1, 2)
		fig.set_size_inches(10, 4.5)
		
		year_x = range(1985, 2018)
		
		add_normalized_plot(ax[0], year_x, df_year[obsname].values, 'obs')
		add_normalized_plot(ax[0], year_x, df_year[simname].values, 'sim')
		
		add_normalized_plot(ax[1], year_x, df_year[obsfluxname].values, 'obs')
		add_normalized_plot(ax[1], year_x, df_year[simdocname].values, 'sim')
		
		ax[0].set_title('Q')
		ax[0].legend()
		#ax[0].set_xticks(year_x)
		ax[0].axhline(0, color='grey')
		
		ax[1].set_title('DOC flux')
		ax[1].legend()
		#ax[1].set_xticks(year_x)
		ax[1].axhline(0, color='grey')
		
		fig.suptitle('%s by year (1985-2017)' % catch_name, fontsize=16)    #NOTE: Title is incorrect if we change run period
		plt.savefig('Figures/year_%d_%s_2.png' % (catch_no, catch_name))
		plt.close()

if __name__ == "__main__":
	main()