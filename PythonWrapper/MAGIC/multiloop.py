
from importlib.machinery import SourceFileLoader
import pandas as pd

#wr = SourceFileLoader("mobius", r"../../mobius.py").load_module()
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"../../mobius_calib_uncert_lmfit.py").load_module()

def do_magic_loop(dataset, excelfile, loopfun) :
	
	soil_df = pd.read_excel(excelfile, sheet_name='CCE-soil', header=16, skiprows=[17], index_col=1)
	
	for id, row in soil_df.iterrows() :
		print('Running lake %s (ID %d)\n' % (row['*Name'], id))
		
		ds = dataset.copy()
		
		def set_single_series_value(ds, name, year, value) :
			in_df = cu.get_input_dataframe(ds, [(name, [])])
			series = in_df['%s []' % name]
			series['%s-1-1'%year] = value
			ds.set_input_series(name, [], series.values)
			
		year = row['Year']
		set_single_series_value(ds, 'Observed ECa', year, row['ExCa'])
		set_single_series_value(ds, 'Observed EMg', year, row['ExMg'])
		set_single_series_value(ds, 'Observed ENa', year, row['ExNa'])
		set_single_series_value(ds, 'Observed EK',  year, row['ExK'])
		set_single_series_value(ds, 'Observed soil pH', year, row['Soil_pH'])
		
		#ds.set_parameter_double('Initial organic C', ['Soil'], row['C'])
		#ds.set_parameter_double('Initial organic N', ['Soil'], row['N'])
		
		ds.set_parameter_double('Depth', ['Soil'], row['Depth'])
		ds.set_parameter_double('Porosity', ['Soil'], row['Porosity'])
		ds.set_parameter_double('Bulk density', ['Soil'], row['BulkDens'])
		ds.set_parameter_double('Cation exchange capacity', ['Soil'], row['CEC'])
		ds.set_parameter_double('Soil sulfate adsorption capacity, half saturation', ['Soil'], row['HlfSat'])
		ds.set_parameter_double('Soil sulfate adsorption max capacity', ['Soil'], row['Emx'])
		ds.set_parameter_double('(log10) Al(OH)3 dissociation equilibrium constant', ['Soil'], row['KAl'])
		
		ds.set_parameter_double('Temperature', ['Soil'], row['Temp'])
		ds.set_parameter_double('CO2 partial pressure', ['Soil'], row['PCO2'])
		ds.set_parameter_double('Organic acid concentration', ['Soil'], row['DOC']*1e-3)
		
		ds.set_parameter_double('Nitrification', ['Soil'], -row['Nitrif'])
		
		elems = ['Ca', 'Mg', 'Na', 'K', 'SO4', 'NH4', 'NO3']
		
		for elem in elems :
			ds.set_parameter_double('%s sinks' % elem, ['Soil'], row['Upt%s'%elem])
		
		#TODO: Other C/N params
		
		elems = ['Ca', 'Mg', 'Na', 'K', 'SO4']
		
		for elem in elems :
			ds.set_parameter_double('%s weathering' % elem, ['Soil'], row['We%s'%elem])
		
		
		elems = ['Ca', 'Mg', 'Na', 'K']
		
		for elem in elems :
			ds.set_parameter_double('Initial exchangeable %s on soil as %% of CEC' % elem, ['Soil'], row['E%s-0'%elem])
		
		ds.set_parameter_double('Runoff', ['Soil'], row['runoff'])
		
		for elem in elems :
			set_single_series_value(ds, 'Observed %s'%elem, year, row['Soil %s'%elem])
		
		
		
		ds.write_parameters_to_file('testparams.dat')
		
		ds.delete()
		
		break
	
	#print(depo_df)