
from importlib.machinery import SourceFileLoader
import pandas as pd
import numpy as np

#wr = SourceFileLoader("mobius", r"../../mobius.py").load_module()
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"../../mobius_calib_uncert_lmfit.py").load_module()

def do_magic_loop(dataset, excelfile, loopfun, limit_num=-1) :
	
	soil_df = pd.read_excel(excelfile % 'soil', sheet_name='CCE-soil', header=16, skiprows=[17], index_col=1)
	lake_df = pd.read_excel(excelfile % 'lake', sheet_name='CCE-lake', header=16, skiprows=[17], index_col=1)
	depo_df = pd.read_excel(excelfile % 'depo', sheet_name='CCE-depo', header=16, skiprows=[17], index_col=1)
	nh4_df  = pd.read_excel(excelfile % 'seqs', sheet_name='CCE-NH4', header=15, skiprows=[16,17], index_col=1)
	no3_df  = pd.read_excel(excelfile % 'seqs', sheet_name='CCE-NO3', header=15, skiprows=[16,17], index_col=1)
	so4_df  = pd.read_excel(excelfile % 'seqs', sheet_name='CCE-SO4', header=15, skiprows=[16,17], index_col=1)
	
	
	#TODO: These should have been read in from the file!
	n_sequence_years   = [1800, 1880, 1920, 1940, 1950, 1960, 1970, 1975, 1980, 1985, 1990, 1995, 2000]
	so4_sequence_years = [1800, 1880, 1920, 1940, 1945, 1955, 1960, 1970, 1975, 1980, 1985, 1990, 2000]
	
	input_index = cu.get_date_index(dataset)
	full_index = pd.date_range(pd.to_datetime('1800-1-1'), input_index[-1], freq='MS') #TODO: this is not that good...
	
	
	loop_idx = 0
	for id, soil in soil_df.iterrows() :
		
		if limit_num >= 0 and loop_idx >= limit_num : break
		loop_idx += 1
	
		print('Running lake %s (ID %d)\n' % (soil['*Name'], id))
		
		lake = lake_df.loc[id, :]
		depo = depo_df.loc[id, :]
		
		nh4 = nh4_df.loc[id, :]
		no3 = no3_df.loc[id, :]
		so4 = so4_df.loc[id, :]
		
		
		
		ds = dataset.copy()
		
		def set_single_series_value(ds, name, year, value) :
			in_df = cu.get_input_dataframe(ds, [(name, [])])
			series = in_df['%s []' % name]
			series['%s-1-1'%year] = value
			ds.set_input_series(name, [], series.values)
		
		
		#### SOIL sheet
		
		year = soil['Year']
		set_single_series_value(ds, 'Observed ECa', year, soil['ExCa'])
		set_single_series_value(ds, 'Observed EMg', year, soil['ExMg'])
		set_single_series_value(ds, 'Observed ENa', year, soil['ExNa'])
		set_single_series_value(ds, 'Observed EK',  year, soil['ExK'])
		set_single_series_value(ds, 'Observed soil pH', year, soil['Soil_pH'])
		
		#ds.set_parameter_double('Initial organic C', ['Soil'], row['C'])
		#ds.set_parameter_double('Initial organic N', ['Soil'], row['N'])
		
		ds.set_parameter_double('Depth', ['Soil'], soil['Depth'])
		ds.set_parameter_double('Porosity', ['Soil'], soil['Porosity'])
		ds.set_parameter_double('Bulk density', ['Soil'], soil['BulkDens'])
		ds.set_parameter_double('Cation exchange capacity', ['Soil'], soil['CEC'])
		ds.set_parameter_double('Soil sulfate adsorption capacity, half saturation', ['Soil'], soil['HlfSat'])
		ds.set_parameter_double('Soil sulfate adsorption max capacity', ['Soil'], soil['Emx'])
		ds.set_parameter_double('(log10) Al(OH)3 dissociation equilibrium constant', ['Soil'], soil['KAl'])
		
		ds.set_parameter_double('Temperature', ['Soil'], soil['Temp'])
		ds.set_parameter_double('CO2 partial pressure', ['Soil'], soil['PCO2'])
		#ds.set_parameter_double('Organic acid concentration', ['Soil'], soil['DOC'])
		ds.set_parameter_double('Organic acid concentration', ['Soil'], 100.0)
	
		ds.set_parameter_double('Nitrification', ['Soil'], -soil['Nitrif'])
		
		elems = ['Ca', 'Mg', 'Na', 'K', 'SO4', 'NH4', 'NO3']
		
		for elem in elems :
			ds.set_parameter_double('%s sinks' % elem, ['Soil'], soil['Upt%s'%elem])
		
		#TODO: Other C/N params
		
		elems = ['Ca', 'Mg', 'Na', 'K', 'SO4']
		
		for elem in elems :
			ds.set_parameter_double('%s weathering' % elem, ['Soil'], soil['We%s'%elem])
			#print('%s weathering: %f' % (elem, soil['We%s'%elem]))
		
		elems = ['Ca', 'Mg', 'Na', 'K']
		
		sumexch = 0.0
		for elem in elems :
			sumexch += soil['E%s-0'%elem]
			
		mult = 1.0 if sumexch <= 90.0 else 90.0/sumexch   #NOTE: Correct ECa, EMg, ENa, EK so that they sum to < 100. Ideally this should be done in-model?
		
		for elem in elems :
			ds.set_parameter_double('Initial exchangeable %s on soil as %% of CEC' % elem, ['Soil'], soil['E%s-0'%elem]*mult)
		
		for elem in elems :
			set_single_series_value(ds, 'Observed %s'%elem, year, soil['Soil %s'%elem])
		
		
		#### LAKE sheet:
		
		ds.set_parameter_double('Discharge', ['Soil'], lake['Qs'])
		ds.set_parameter_double('Discharge', ['Lake'], lake['Qs'])
		
		elems = ['Ca', 'Mg', 'Na', 'K', 'NH4', 'SO4', 'Cl', 'NO3']
		
		year = lake['Year']
		for elem in elems :
			set_single_series_value(ds, 'Observed %s'%elem, year, lake['%s'%elem])  #TODO: check that this works, or some columns have to be renamed
			
		set_single_series_value(ds, 'Observed lake pH', year, lake['pH'])
		#set_single_series_value(ds, 'TotMon-Al', year, lake['TotMon-Al'])
	
		ds.set_parameter_double('Relative area', ['Lake'], lake['RelArea']*0.01)
		ds.set_parameter_double('Depth', ['Lake'], lake['RetTime']*lake['Qs'])
		
		ds.set_parameter_double('(log10) Al(OH)3 dissociation equilibrium constant', ['Lake'], lake['KAl'])
		
		ds.set_parameter_double('Temperature', ['Lake'], lake['Temp'])
		ds.set_parameter_double('CO2 partial pressure', ['Lake'], lake['pCO2'])
		ds.set_parameter_double('Organic acid concentration', ['Lake'], lake['DOC'])
		
		ds.set_parameter_double('Nitrification', ['Lake'], -lake['Nitrif'])
		
		
		#### DEPON sheet:
		
		for elem in elems :
			index_elem = elem
			if elem == 'Cl': index_elem = 'CL'   #sigh
			ds.set_parameter_double('%s wet deposition' % elem, [], depo[index_elem])    #TODO: does this work, or do we need column renaming?
		
		ds.set_parameter_double('Precipitation', [], depo['Ppt'])
		
		
		#### SEQUENCE sheet
		
		scale_df = pd.DataFrame({
			'Dates' : full_index,
			'NH4' : np.full(len(full_index), np.nan),
			'NO3' : np.full(len(full_index), np.nan),
			'SO4' : np.full(len(full_index), np.nan),
		})
		
		scale_df.set_index('Dates', inplace=True)
		
		for yr in range(0, len(n_sequence_years)) :
			scale_df.loc[pd.to_datetime('%d-1-1' % n_sequence_years[yr]),   'NH4'] = nh4['(year%d)' %(yr+1)]
			scale_df.loc[pd.to_datetime('%d-1-1' % n_sequence_years[yr]),   'NO3'] = no3['(year%d)' %(yr+1)]
			scale_df.loc[pd.to_datetime('%d-1-1' % so4_sequence_years[yr]), 'SO4'] = so4['(year%d)' %(yr+1)]
		
		
		scale_df.interpolate(inplace=True, limit_direction='both')
		
		mask = (scale_df.index >= input_index[0]) & (scale_df.index <= input_index[-1])
		ds.set_input_series('NH4 wet deposition scaling factor', [], scale_df['NH4'].values[mask], alignwithresults=True)
		ds.set_input_series('NO3 wet deposition scaling factor', [], scale_df['NO3'].values[mask], alignwithresults=True)
		ds.set_input_series('SO4 wet deposition scaling factor', [], scale_df['SO4'].values[mask], alignwithresults=True)
		
		
		loopfun(ds)
		#ds.write_parameters_to_file('testparams.dat')
		
		ds.delete()
	
	#print(depo_df)