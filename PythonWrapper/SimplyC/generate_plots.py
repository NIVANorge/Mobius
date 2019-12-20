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

n = 6



dataset = wr.DataSet.setup_from_parameter_and_input_files('structure1_auto.dat', '../../Applications/SimplyC/Langtjern/langtjerninputs.dat')
dataset.run_model() # Have to do this due to a bug in get_input_series(). TODO: Fix that!
df = cu.get_input_dataframe(dataset, [('DOC', [])], alignwithresults=True)
df.rename(columns={'DOC []':'Observed'}, inplace = True)

dataset.delete()



#for structure in range(1, n+1) :
for structure in [1, 3, 5] :
	if structure == 5 :
		wr.initialize('..\..\Applications\SimplyC\SimplyC_stage_5.dll')
	elif structure == 6 :
		wr.initialize('..\..\Applications\SimplyC\SimplyC_stage_6.dll')

	dataset = wr.DataSet.setup_from_parameter_and_input_files('structure%d_auto.dat' % structure, '../../Applications/SimplyC/Langtjern/langtjerninputs.dat')
	
	dataset.run_model()
	
	sim_df = cu.get_result_dataframe(dataset, [('Reach DOC concentration (volume weighted daily mean)', ['Inlet'])])
	
	sim_df.rename(columns={'Reach DOC concentration (volume weighted daily mean) [Inlet]':'Modelled, structure %d' % structure}, inplace=True)
	
	dataset.delete()
	
	df = pd.concat([df, sim_df], axis=1)


fig_height = 6
fig_width  = 15
fig, axes = plt.subplots(nrows=1, ncols=1, figsize=(fig_width, fig_height)) 

#df.plot(ax=axes, style=['o--', '-', '-', '-', '-', '-', '-'])
sub_df = df[['Modelled, structure %d' % structure for structure in [1, 3, 5]]]
#sub_df.plot(ax=axes, style=['-r', '-b', '-g'])

line, = axes.plot_date(df.index.values, df['Observed'], 'o-', fillstyle='none', color='#777777', xdate=True)
line.set_label('Observed')
axes.legend()
sub_df.plot(ax=axes, style=['#e41a1c', '#377eb8', '#4daf4a'])

axes.set_ylabel('Stream DOC concentration [mg/l]')

plt.tight_layout()  

plt.savefig('structures.png', dpi=200)
plt.show()


'''
fig_height = n*4
fig_width  = 15
fig, axes = plt.subplots(nrows=n, ncols=1, figsize=(15, fig_height)) 

for structure in range(1, n+1) :
	sub_df = df[['Observed [DOC]', 'Modelled [DOC] structure %d' % structure]]

	sub_df.plot(ax=axes[structure-1], style=['o--', '-'])
	axes[structure-1].set_ylabel('mg/l')

plt.tight_layout()  

plt.savefig('structures_alt.png', dpi=200)
plt.show()
'''

