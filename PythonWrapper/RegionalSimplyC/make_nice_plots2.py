

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from importlib.machinery import SourceFileLoader

# Initialise wrapper
wr = SourceFileLoader("mobius", r"../mobius.py").load_module()
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"..\mobius_calib_uncert_lmfit.py").load_module()

wr.initialize('../../Applications/SimplyC_regional/simplyc_regional.dll')


simmed   = ('Reach flow (daily mean, cumecs)', ['R0'])
observed = ('Observed flow', [])

simmeddoc = ('DOC flux from reach, daily mean', ['R0'])
observeddoc = ('Observed DOC', [])


#def add_normalized_plot(ax, xvals, values, label, color):
#	mn = np.nanmean(values)
#	sd = np.nanstd(values)
	
#	ax.plot(xvals, (values - mn)/sd, label=label, color=color)

def add_normalized_plots(ax, xvals, obsvals, simvals, obscolor, simcolor):
	mn = np.nanmean(obsvals)
	sd = np.nanstd(obsvals)
	
	ax.plot(xvals, (obsvals - mn)/sd, label='obs', color=obscolor)
	ax.plot(xvals, (simvals - mn)/sd, label='sim', color=simcolor)



def main() :
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	fig_month, ax_month = plt.subplots(8, 6)
	fig_month.set_size_inches(20, 30)
	
	ax_month = ax_month.flatten()
	
	fig_year, ax_year = plt.subplots(8, 6)
	fig_year.set_size_inches(20, 30)
	
	ax_year = ax_year.flatten()
	
	for index, row in catch_setup.iterrows():
		catch_no = row['met_index']
		catch_name = row['name']
		fullname = row['fullname']
		
		#if catch_name != 'Dalelva' : continue
		
		#param_file_prefix = 'optim_params_DOC'
		param_file_prefix = 'norm3_optim_params_DOC'
		
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
		
		#TODO: This was put in the input file, so we don't have to recompute it here.
		doc_df = df[[obsdocname]].copy()
		doc_df.interpolate(inplace=True, limit_area='inside', limit=60)   # limit: don't fill in more than two missing months
		
		df[obsfluxname] = doc_df[obsdocname].values * df[obsname].values * 86.4  # flux = concentration * flow
		
		df_month = df.resample('MS').mean()
		
		obs = np.zeros(12)
		sim = np.zeros(12)
		fluxobs = np.zeros(12)
		fluxsim = np.zeros(12)
		
		
		for idx, row in df_month.iterrows() :
			if not np.isnan(row[obsname]):
				sim[idx.month-1] += row[simname]
				obs[idx.month-1] += row[obsname]
				
			if not np.isnan(row[obsfluxname]):
				fluxsim[idx.month-1] += row[simdocname]
				fluxobs[idx.month-1] += row[obsfluxname]
		
		month_x = range(1, 13)
		
		add_normalized_plots(ax_month[2*index], month_x, obs, sim, '#0082C8', '#E6194B')
		add_normalized_plots(ax_month[2*index+1], month_x, fluxobs, fluxsim, '#3CB44B', '#F58230')
		
		ax_month[2*index].set_title('%s Q' % fullname)
		ax_month[2*index].legend()
		ax_month[2*index].set_xticks(month_x)
		ax_month[2*index].axhline(0, color='grey')
		
		ax_month[2*index+1].set_title('%s DOC flux' % fullname)
		ax_month[2*index+1].legend()
		ax_month[2*index+1].set_xticks(month_x)
		ax_month[2*index+1].axhline(0, color='grey')
		
		#df_year = df.resample('Y').mean()    #TODO: problematic beginning or ending years with many lacking values...
		df_year = df.resample('Y').agg(lambda x: np.nanmean(x.values) if np.isnan(x.values).sum() <= 60 else np.nan)   #Skip years with more than 60 missing values
		
		#print(df_year)
		
		year_x = range(1985, 2018)
		
		add_normalized_plots(ax_year[2*index], year_x, df_year[obsname].values, df_year[simname].values, '#0082C8', '#E6194B')
		
		add_normalized_plots(ax_year[2*index+1], year_x, df_year[obsfluxname].values, df_year[simdocname].values, '#3CB44B', '#F58230')
		
		ax_year[2*index].set_title('%s Q' % fullname)
		ax_year[2*index].legend()
		#ax[0].set_xticks(year_x)
		ax_year[2*index].axhline(0, color='grey')
		
		ax_year[2*index+1].set_title('%s DOC flux' % fullname)
		ax_year[2*index+1].legend()
		#ax[1].set_xticks(year_x)
		ax_year[2*index+1].axhline(0, color='grey')
		
	#fig_month.suptitle('Q and DOC flux by month (1985-2017)', fontsize=48)    #NOTE: Title is incorrect if we change run period
	#fig_year.suptitle('Q and DOC flux by year (1985-2017)', fontsize=48)    #NOTE: Title is incorrect if we change run period
	
	fig_month.tight_layout()
	fig_year.tight_layout()
	#Apparently they close in opposite order of creation
	
	
	plt.savefig('Figures/year.png')
	plt.close()
	
	plt.savefig('Figures/month.png')
	plt.close()

if __name__ == "__main__":
	main()