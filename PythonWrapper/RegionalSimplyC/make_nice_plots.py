

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

from importlib.machinery import SourceFileLoader

# Initialise wrapper
wr = SourceFileLoader("mobius", r"../mobius.py").load_module()
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"..\mobius_calib_uncert_lmfit.py").load_module()

wr.initialize('../../Applications/SimplyC/simplyc_regional.dll')


param_file_prefix = 'optim_params'

simmed   = ('Reach flow (daily mean, cumecs)', ['R0'])
observed = ('Observed flow', [])

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
		
		dataset = wr.DataSet.setup_from_parameter_and_input_files(parfile, infile)
		
		dataset.run_model()
		
		sim_df = cu.get_result_dataframe(dataset, [simmed])
		obs_df = cu.get_input_dataframe(dataset, [observed], alignwithresults=True)
		
		df = pd.concat([obs_df, sim_df], axis=1)
		
		df = df.resample('MS').mean()
		
		obs = np.zeros(12)
		sim = np.zeros(12)
		
		simname = cu.combine_name(simmed[0], simmed[1])
		obsname = cu.combine_name(observed[0], observed[1])
		
		for index, row in df.iterrows() :
			sim[index.month-1] += row[simname]
			if not np.isnan(row[obsname]):
				obs[index.month-1] += row[obsname]

		fig, ax = plt.subplots(1, 1)
		add_normalized_plot(ax, obs, 'obs')
		add_normalized_plot(ax, sim, 'sim')
		
		ax.set_title('Q by month (1985-2017), %s' % catch_name)          #NOTE: Title is incorrect if we change run period
		ax.legend()
		
		plt.axhline(0, color='grey')
		plt.xticks(range(1, 13))
		
		plt.savefig('Figures/%d_%s_flow_month.png' % (catch_no, catch_name))
		
		plt.close()
		

if __name__ == "__main__":
	main()