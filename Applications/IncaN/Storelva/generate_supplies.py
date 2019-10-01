
# Extrapolate input Nitrate and Ammonium supplies to generate an input file of these.

import imp
import numpy as np

wrapper_fpath = (r'../../../PythonWrapper/mobius.py')
mobius = imp.load_source('mobius', wrapper_fpath)
calib_fpath = (r"../../../PythonWrapper/mobius_calib_uncert_lmfit.py")
cu = imp.load_source('mobius_calib_uncert_lmfit', calib_fpath)

mobius.initialize('../incan.dll')

dataset = mobius.DataSet.setup_from_parameter_and_input_files('incan_params_Storelva_to2018.dat', 'incan_inputs_Storelva.dat')

df = cu.get_input_dataframe(dataset, [('Ammonium wet deposition', []), ('Nitrate wet deposition', []), ('Ammonium dry deposition', []),	('Nitrate dry deposition', [])] )

df = df.resample('Y').mean()
vars = ['Ammonium wet deposition', 'Nitrate wet deposition', 'Ammonium dry deposition', 'Nitrate dry deposition']


fert_df = cu.get_input_dataframe(dataset, [('Fertilizer nitrate', ['Agricultural']), ('Fertilizer ammonium', ['Agricultural'])])


output = open('N_supplies_Storelva_extrapolated.dat', 'w')

print('"Fertilizer nitrate" {"Agricultural"}:', file=output)
for year in range(1990, 2101) :
	val1 = fert_df['Fertilizer nitrate [Agricultural]']['%d-04-25' % year if year <= 2018 else '2018-04-25']
	val2 = fert_df['Fertilizer nitrate [Agricultural]']['%d-09-01' % year if year <= 2018 else '2018-09-01']
	print('%d-04-25 to %d-05-04 %f' % (year, year, val1), file = output)
	print('%d-06-10 to %d-06-19 %f' % (year, year, val1), file = output)
	print('%d-09-01 to %d-09-10 %f' % (year, year, val2), file = output)

print('', file=output)
print('"Fertilizer ammonium" {"Agricultural"} :', file=output)
for year in range(1990, 2101) :
	val1 = fert_df['Fertilizer ammonium [Agricultural]']['%d-04-25' % year if year <= 2018 else '2018-04-25']
	val2 = fert_df['Fertilizer ammonium [Agricultural]']['%d-09-01' % year if year <= 2018 else '2018-09-01']
	print('%d-04-25 to %d-05-04 %f' % (year, year, val1), file = output)
	print('%d-06-10 to %d-06-19 %f' % (year, year, val1), file = output)
	print('%d-09-01 to %d-09-10 %f' % (year, year, val2), file = output)
print('', file=output)


for var in vars :
	print('', file=output)
	print('"%s" :' % var, file=output)
	initial_values = df['%s []' % var]
	meanval = np.mean(initial_values[-3:])
	for year in range(1990, 2101) :
		print('%d-01-01 to %d-12-31 %f' % (year, year, initial_values[year-1990] if year <= 2017 else meanval), file=output)

output.close()