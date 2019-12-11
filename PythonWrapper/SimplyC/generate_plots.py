import warnings
import imp
import pandas as pd
import numpy as np
import matplotlib
import matplotlib.pyplot as plt

# Styling
warnings.filterwarnings("ignore")
plt.style.use('ggplot')


wrapper_fpath = (r"..\mobius.py")
wr = imp.load_source('mobius', wrapper_fpath)
wr.initialize('..\..\Applications\SimplyC\SimplyC_stage_1_4.dll')

# Calibration functions
calib_fpath = (r"..\mobius_calib_uncert_lmfit.py")
cu = imp.load_source('mobius_calib_uncert_lmfit', calib_fpath)

n = 4

fig_height = min(20, n*3.5)
fig, axes = plt.subplots(nrows=n, ncols=1, figsize=(15, fig_height)) 

for structure in range(1, n+1) :
	dataset = wr.DataSet.setup_from_parameter_and_input_files('structure%d_auto.dat' % structure, '../../Applications/SimplyC/Langtjern/langtjerninputs.dat')
	
	dataset.run_model()
	
	sim_df = cu.get_result_dataframe(dataset, [('Reach DOC concentration (volume weighted daily mean)', ['Inlet'])])
	obs_df = cu.get_input_dataframe(dataset, [('DOC', [])], alignwithresults=True)
	df = pd.concat([obs_df, sim_df], axis=1)
	
	df = df.rename(columns={'DOC []':'Observed [DOC]', 'Reach DOC concentration (volume weighted daily mean) [Inlet]':'Modeled [DOC]'})
	
	df.plot(ax=axes[structure-1], style=['o--', '-'])
	axes[structure-1].set_ylabel('$mg/l$')
	
	dataset.delete()

plt.tight_layout()  

plt.savefig('structures.png', dpi=200)