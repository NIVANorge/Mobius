
from importlib.machinery import SourceFileLoader
import pandas as pd
import numpy as np

#wr = SourceFileLoader("mobius", r"../../mobius.py").load_module()
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"../../mobius_calib_uncert_lmfit.py").load_module()


def set_single_series_value(ds, name, year, value) :
	in_df = cu.get_input_dataframe(ds, [(name, [])])
	series = in_df['%s []' % name]
	series['%s-6-1'%year] = value
	ds.set_input_series(name, [], series.values)

def do_magic_loop(dataset, excelfiles, loopfun, limit_num=-1, do_id=-1, do_year=0) :
	soilfile1, soilfile2, lakefile1, lakefile2, depofile1, depofile2, seqfile = excelfiles
	
	##OOOOPS, hard coded sheet names...
	if do_year == 0 or do_year == -1 :
		soil_df = pd.read_excel(soilfile1, sheet_name='Tusen1995-soil', header=16, skiprows=[17], index_col=1)
		lake_df = pd.read_excel(lakefile1, sheet_name='Tusen1995-lake', header=16, skiprows=[17], index_col=1)
		lake_df_2 = pd.read_excel(lakefile2, sheet_name='Tusen2019-lake', header=16, skiprows=[17], index_col=1)
		depo_df = pd.read_excel(depofile1, sheet_name='Tusen1995-depo', header=16, skiprows=[17], index_col=1)
	else :
		soil_df = pd.read_excel(soilfile2, sheet_name='Tusen2019-soil', header=16, skiprows=[17], index_col=1)
		lake_df = pd.read_excel(lakefile2, sheet_name='Tusen2019-lake', header=16, skiprows=[17], index_col=1)
		lake_df_2 = pd.read_excel(lakefile1, sheet_name='Tusen1995-lake', header=16, skiprows=[17], index_col=1)
		depo_df = pd.read_excel(depofile2, sheet_name='Tusen2019-depo', header=16, skiprows=[17], index_col=1)
	nh4_df  = pd.read_excel(seqfile, sheet_name='Tusen1995-WC-NH4', header=17, index_col=1)
	no3_df  = pd.read_excel(seqfile, sheet_name='Tusen1995-WC-NO3', header=17, index_col=1)
	so4_df  = pd.read_excel(seqfile, sheet_name='Tusen1995-WC-SO4', header=17, index_col=1)
	cl_df   = pd.read_excel(seqfile, sheet_name='Tusen1995-WC-Cl', header=17, index_col=1)
	
	
	#TODO: These should have been read in from the file!
	no3_sequence_years = [1800, 1880, 1930, 1940, 1950, 1970, 1980, 1985, 1992, 1998, 2005, 2012, 2020]
	nh4_sequence_years = [1800, 1880, 1920, 1950, 1970, 1975, 1980, 1985, 1992, 1999, 2004, 2014, 2020]
	so4_sequence_years = [1800, 1880, 1940, 1945, 1970, 1975, 1985, 1991, 1999, 2013, 2014, 2016, 2020]
	cl_sequence_years  = [1800, 1993, 1994, 1996, 1997, 2017, 2018, 2020, 2021]
	
	input_index = cu.get_input_date_index(dataset)
	full_index = pd.date_range(pd.to_datetime('1800-1-1'), '2100-1-1', freq='MS') #TODO: this is not that good...
	
	
	loop_idx = 0
	for id, soil in soil_df.iterrows() :
	
		if do_id!=-1 and id!=do_id : continue
		
		if limit_num >= 0 and loop_idx >= limit_num : break
		loop_idx += 1
	
		#if id not in lake_df_2.index : continue                        #TODO: maybe run 1-point sample then instead!
	
		name_name = '*Name'
		#name_name = "'    each year (depend on lake chem)"    # caused by new formatting of the excel sheet
		print('Running lake %s (ID %d)\n' % (soil[name_name], id))
		
		lake = lake_df.loc[id, :]
		lake2 = lake_df_2.loc[id, :]
		depo = depo_df.loc[id, :]
		
		nh4 = nh4_df.loc[id, :]
		no3 = no3_df.loc[id, :]
		so4 = so4_df.loc[id, :]
		cl  = cl_df.loc[id, :]
		
		ds = dataset.copy()
		
		
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
		ds.set_parameter_double('Porosity', ['Soil'], soil['Porosity']*0.01)
		ds.set_parameter_double('Bulk density', ['Soil'], soil['BulkDens'])
		ds.set_parameter_double('Cation exchange capacity', ['Soil'], soil['CEC'])
		ds.set_parameter_double('Soil sulfate adsorption capacity, half saturation', ['Soil'], soil['HlfSat'])
		ds.set_parameter_double('Soil sulfate adsorption max capacity', ['Soil'], soil['Emx'])
		ds.set_parameter_double('(log10) Al(OH)3 dissociation equilibrium constant', ['Soil'], soil['KAl'])
		
		ds.set_parameter_double('(-log10) pK 1st equilibrium constant for triprotic organic acid', ['Soil'], soil['pK1'])
		ds.set_parameter_double('(-log10) pK 2nd equilibrium constant for triprotic organic acid', ['Soil'], soil['pK2'])
		ds.set_parameter_double('(-log10) pK 3rd equilibrium constant for triprotic organic acid', ['Soil'], soil['pK3'])
		
		ds.set_parameter_double('Temperature', ['Soil'], soil['Temp'])
		ds.set_parameter_double('CO2 partial pressure', ['Soil'], soil['PCO2'])
		#ds.set_parameter_double('Organic acid concentration', ['Soil'], soil['DOC'])
		ds.set_parameter_double('Organic acid concentration', ['Soil'], 100.0)
	
		ds.set_parameter_double('Nitrification', ['Soil'], -soil['Nitrif'])
		
		elems = ['Ca', 'Mg', 'Na', 'K', 'SO4', 'NH4', 'NO3']
		
		for elem in elems :
			#ds.set_parameter_double('%s sinks' % elem, ['Soil'], soil['Upt%s'%elem])
			ds.set_parameter_double('%s sinks' % elem, ['Soil'], soil[elem])
		
		#TODO: Other C/N params
		
		elems = ['Ca', 'Mg', 'Na', 'K', 'SO4']
		
		for elem in elems :
			ds.set_parameter_double('%s weathering' % elem, ['Soil'], soil['We%s'%elem])
			#print('%s weathering: %f' % (elem, soil['We%s'%elem]))
		
		elems = ['Ca', 'Mg', 'Na', 'K']
		
		sumexch = 0.0
		for elem in elems :
			sumexch += soil['E%s-0'%elem]
			
		#mult = 1.0 if sumexch <= 90.0 else 90.0/sumexch   #NOTE: Correct ECa, EMg, ENa, EK so that they sum to < 100. Ideally this should be done in-model?
		mult = 1.0
		
		for elem in elems :
			ds.set_parameter_double('Initial exchangeable %s on soil as %% of CEC' % elem, ['Soil'], soil['E%s-0'%elem]*mult)
		
		
		#for elem in elems :
		#	set_single_series_value(ds, 'Observed %s'%elem, year, soil['Soil %s'%elem])
		
		
		#### LAKE sheet:
		
		
		# TODO: What to do about discharge when having two different values (for two different years)
		ds.set_parameter_double('Discharge', ['Soil'], lake['Runoff'])
		ds.set_parameter_double('Discharge', ['Lake'], lake['Runoff'])
		
		elems = ['Ca', 'Mg', 'Na', 'K', 'NH4', 'SO4', 'Cl', 'NO3']
		
		year = lake['Year']
		for elem in elems :
			set_single_series_value(ds, 'Observed %s'%elem, year, lake['%s'%elem])
			
		set_single_series_value(ds, 'Observed lake pH', year, lake['pH'])
		#set_single_series_value(ds, 'TotMon-Al', year, lake['TotMon-Al'])
	
		ds.set_parameter_double('Relative area', ['Lake'], lake['RelArea']*0.01)
		ds.set_parameter_double('Relative area', ['Soil'], 1.0-lake['RelArea']*0.01)
		ds.set_parameter_double('Depth', ['Lake'], lake['RetTime']*lake['Runoff']/(lake['RelArea']*0.01))
		
		ds.set_parameter_double('(log10) Al(OH)3 dissociation equilibrium constant', ['Lake'], lake['KAl'])
		
		ds.set_parameter_double('Temperature', ['Lake'], lake['Temp'])
		ds.set_parameter_double('CO2 partial pressure', ['Lake'], lake['pCO2'])
		ds.set_parameter_double('Organic acid concentration', ['Lake'], lake['DOC'])
		
		ds.set_parameter_double('Nitrification', ['Lake'], -lake['Nitrif'])
		
		ds.set_parameter_double('(-log10) pK 1st equilibrium constant for triprotic organic acid', ['Lake'], lake['pK1'])
		ds.set_parameter_double('(-log10) pK 2nd equilibrium constant for triprotic organic acid', ['Lake'], lake['pK2'])
		ds.set_parameter_double('(-log10) pK 3rd equilibrium constant for triprotic organic acid', ['Lake'], lake['pK3'])
		
		#NOTE NO3 retention as in OKA's study.
		S_N = 5.0
		q_s = lake['Runoff'] / (lake['RelArea']*0.01)
		ds.set_parameter_double('NO3 sinks', ['Lake'], -100.0 * S_N / (q_s + S_N))
		
		#### LAKE sheet 2 :
		
		if do_year==-1 :
			elems = ['Ca', 'Mg', 'Na', 'K', 'NH4', 'SO4', 'Cl', 'NO3']
			
			year = lake2['Year']
			for elem in elems :
				set_single_series_value(ds, 'Observed %s'%elem, year, lake2['%s'%elem])
				
			set_single_series_value(ds, 'Observed lake pH', year, lake2['pH'])
		
		
		#### SEQUENCE sheet
		
		scale_df = pd.DataFrame({
			'Dates' : full_index,
			'NH4' : np.full(len(full_index), np.nan),
			'NO3' : np.full(len(full_index), np.nan),
			'SO4' : np.full(len(full_index), np.nan),
			'Cl'  : np.full(len(full_index), np.nan),
		})
		
		scale_df.set_index('Dates', inplace=True)
		
		for yr in range(0, len(no3_sequence_years)) :
			hdr = 'ScalFact' if yr==0 else 'ScalFact.%d'%yr
			scale_df.loc[pd.to_datetime('%d-6-1' % nh4_sequence_years[yr]),   'NH4'] = nh4[hdr]
			scale_df.loc[pd.to_datetime('%d-6-1' % no3_sequence_years[yr]),   'NO3'] = no3[hdr]
			if yr < len(so4_sequence_years) :
				scale_df.loc[pd.to_datetime('%d-6-1' % so4_sequence_years[yr]), 'SO4'] = so4[hdr]
			if yr < len(cl_sequence_years) :
				scale_df.loc[pd.to_datetime('%d-6-1' % cl_sequence_years[yr]), 'Cl'] = cl[hdr]
		#sdf = scale_df.dropna()
		#print(sdf)
		
		scale_df.interpolate(inplace=True, limit_direction='both')
		
		mask = (scale_df.index >= input_index[0]) & (scale_df.index <= input_index[-1])
		ds.set_input_series('NH4 wet deposition scaling factor', [], scale_df['NH4'].values[mask], alignwithresults=True)
		ds.set_input_series('NO3 wet deposition scaling factor', [], scale_df['NO3'].values[mask], alignwithresults=True)
		ds.set_input_series('SO4 wet deposition scaling factor', [], scale_df['SO4'].values[mask], alignwithresults=True)
		for elem in ['Ca', 'Mg', 'Na', 'K', 'Cl'] :
			ds.set_input_series('%s wet deposition scaling factor'%elem, [], scale_df['Cl'].values[mask], alignwithresults=True)
		
		#### DEPON sheet:
		
		elems = ['Ca', 'Mg', 'Na', 'K', 'NH4', 'SO4', 'Cl', 'NO3']
		
		depo_ref_year = depo['Year']
		
		for elem in elems :
			index_elem = elem
			if elem == 'Cl': index_elem = 'CL'   #sigh
			scale = 1.0
			if elem in scale_df.columns :
				invscale = scale_df[elem]['%d-1-1'%depo_ref_year]    # Compensate for the fact that the reference year may not have been scaled as 1.0 in the sequence
				#print('%s scale in %d was %f'%(elem,depo_ref_year,invscale))
				scale = 1.0 / invscale
				
				#scale2 = scale_df[elem]['1990-1-1']
				#print('%s 1990 scale was %f' %(elem, scale2))
			value = depo[index_elem]*scale
			if elem=='SO4':
				sea_salt = depo['CL']*0.103
				excess0 = depo['SO4']-sea_salt
				excess1 = excess0*scale
				value = sea_salt + excess1
			ds.set_parameter_double('%s wet deposition' % elem, [], value)    
		
		ds.set_parameter_double('Precipitation', [], depo['Precip'])
		
		try :
			loopfun(ds, id, soil, depo, lake)
		except RuntimeError as err :
			print('Got an error for lake %d :' % id, err)
            
		
		ds.delete()
	