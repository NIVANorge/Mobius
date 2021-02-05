

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os



def year_range(begin_year, end_year, monthly=True) :
	start = pd.to_datetime('%s-1-1' % begin_year, yearfirst=True)
	end   = pd.to_datetime('%s-12-1' % end_year if monthly else '%s-1-1' % end_year, yearfirst=True)
	
	#return np.array(pd.date_range(start = start, end = end, freq='MS' if monthly else 'AS'))
	return pd.date_range(start = start, end = end, freq='MS' if monthly else 'AS')

def empty_input_df(begin_year, end_year) :
	return pd.DataFrame(index = year_range(begin_year, end_year))


def add_ts(input_df, source, dest_name, aggr_method='mean', patch_ref_range = None) :
	
	if aggr_method == 'sum' :
		monthly = source.resample('MS').sum(min_count=28)  #This min count is not ideal, we could also use mean, and multiply with length of month
	elif aggr_method == 'mean' :
		monthly = source.resample('MS').mean()
		
	distr = np.zeros(12)
	count = np.zeros(12)
	
	if patch_ref_range is not None:
		patch_range = year_range(patch_ref_range[0], patch_ref_range[1], monthly=True)
	
		for index, val in monthly.iteritems() :
			if not np.isnan(val) :
				if index in patch_range :
					distr[index.month-1] += val
					count[index.month-1] += 1
	
		if aggr_method == 'sum' :
			distr /= np.nansum(monthly[patch_range].values)
			assert np.abs(np.sum(distr) - 1.0) < 1e-8
		elif aggr_method == 'mean' :
			distr /= count
	
	#print('Monthly distribution:')
	#print(distr)
	#print('distr sum (should be 1) %g' % np.sum(distr))
	
	if aggr_method == 'sum' :
		yearly = source.resample('AS').sum(min_count=365)
	elif aggr_method == 'mean' :
		yearly = source.resample('AS').sum(min_count=365)
		# We still want the min count to not get thrown off by a year where there are only december values. However, the mean() function does not provide this
		#TODO: Does NOT work for time series that are not daily!!!!
		yearly /= 365.0
		#yearly = source.resample('AS').mean()
	
	all_mean = np.nanmean(yearly.values)
	#print('all mean %g' % all_mean)
	
	yearly = yearly.reindex(year_range(input_df.index[0].year, input_df.index[-1].year, False))
	
	if patch_ref_range is not None :
		ref_dates = year_range(patch_ref_range[0], patch_ref_range[1], monthly=False)
		ref_val = np.nanmean([yearly[ref_date] for ref_date in ref_dates])
		
		first_idx = yearly.first_valid_index()
		idx = np.searchsorted(yearly.index, first_idx)
		yearly[yearly.index[idx-1]] = ref_val
	
		#TODO: Provide different methods for filling in holes
		yearly = yearly.interpolate(limit_area=None, limit_direction='both')
		#print(yearly)
	
	input_df[dest_name] = np.zeros(len(input_df.index))
	for index, row in input_df.iterrows() :
		if index in monthly.index :
			input_df.loc[index, dest_name] = monthly[index]
		else :
			yr_val = yearly[pd.to_datetime('%s-1-1' % index.year, yearfirst=True)]
			m_val  = distr[index.month-1]
			if aggr_method == 'sum' :
				input_df.loc[index, dest_name] = yr_val * m_val
			elif aggr_method == 'mean' :
				input_df.loc[index, dest_name] = m_val + yr_val - all_mean
	

def add_precip_ts(input_df, source, patch_ref_range=None) :#, source_is_mm=True) :
	add_ts(input_df, source, 'Precipitation', 'sum', patch_ref_range)
	#if source_is_mm : input_df['Precipitation'] *= 1e-3     #NOTE MAGIC wants this in m/month instead of mm/month
	

def add_air_temperature_ts(input_df, source, patch_ref_range=None) :
	add_ts(input_df, source, 'Air temperature', 'mean', patch_ref_range)
	
def add_runoff_ts(input_df, source) :
	add_ts(input_df, source, 'Runoff', 'sum', patch_ref_range=None)
	#TODO: Re-scale in case it is given in m3/s or something like that.
	
	
def add_deposition_ts(
	input_df, source_df, runoff_df, source_is_mg_l=True, runoff_is_mg_l=True, n_dep_given_as_n=True, s_dep_given_as_s=True,
	sea_salt_ref='Cl', sea_salt_ref_range=None, 
	patch_scale=None, patch_ref_range=(1989,1992), 
	so4_weathering=0.0, ca_excess_weight=0.5, ca_ref_range=None, n_dry_deposition_factor=1.0) :
	'''
	input_df  : The dataframe you want to write the inputs to. It must already have a 'Precipitation' and a 'Runoff' column
	source_df : A date-indexed dataframe with observed concentrations in precipitation, with columns ['Ca', 'Mg', 'Na', 'K', 'NH4', 'SO4', 'Cl', 'NO3', 'F']   F is optional
	runoff_df : A date-indexed dataframe with observed concentrations in runoff, with the same columns as source_df.
	source_is_mg_l : True if the unit of the source_df is mg/l, False if they are meq/m3=µeq/l
	runoff_is_mg_l : True if the unit of the runoff_df is mg/l, False if they are meq/m3=µeq/l
	n_dep_given_as_n : Only relevant if source is mg/l: True if the mass of NO3 and NH4 deposition is given as the mass of N, False if it is given as the mass of the full molecule.
	s_dep_given_as_s : Only relevant if source is mg/l: True if the mass of SO4 deposition is given as the mass of S, False if it is given as the mass of the full molecule.
	sea_salt_ref : The reference deposition series for sea salt correction. Usually 'Cl', but can also use 'Mg', 'Na', or 'K'.
	sea_salt_ref_range : pair (start_year, end_year). This is the range that is used to compute the sea_salt_factor. Also SO4*, Ca* and N factors.
	patch_scale : A date-indexed dataframe containing some of the columns of the input_df. Should have some values of historical deposition relative to a reference year. Example : get_emep_deposition_scales()
	patch_ref_range : pair (start_year, end_year). Range of years to compute the value for the reference year to scale the values used for patching.
	so4_weathering : how much of the so4 runoff should be accounted for by weathering and not deposition
	ca_ref_range : If not None, compute Ca excess deposition relative to SO4 excess deposition with reference to this range.
	ca_excess_weight : Ca excess deposition relative to SO4 excess deposition is multiplied with this value
	n_dry_deposition_factor : Multiply NH4 and NO3 deposition by this value
	'''
	
	
	source_df = source_df.resample('MS').mean()
	source_df = source_df.interpolate(limit_area='inside')
	
	runoff_df2 = runoff_df.resample('MS').mean()
	runoff_df2 = runoff_df2.interpolate(limit_area='inside')
	
	if sea_salt_ref_range is not None:
		cl_range = year_range(sea_salt_ref_range[0], sea_salt_ref_range[1], monthly=True)
	else:
		cl_range = source_df.index.intersection(runoff_df2.index)
	
	def get_converted_conc(df, element, convert) :
		charge = {
			'Ca' : 2.0,
			'Mg' : 2.0,
			'Na' : 1.0,
			'K'  : 1.0,
			'NH4': 1.0,
			'SO4': 2.0,
			'Cl' : 1.0,
			'NO3': 1.0,
			'F'  : 1.0,
		}
		atomic_mass = {
			'Ca' : 40.0,
			'Mg' : 24.3,
			'Na' : 23.0,
			'K'  : 39.0,
			'NH4': 14.0 if n_dep_given_as_n else 18.0,
			'SO4': 32.1 if s_dep_given_as_s else 96.0,
			'Cl' : 35.5,
			'NO3': 14.0 if n_dep_given_as_n else 62.0,
			'F'  : 19.0,
		}
		if convert :
			#Convert conc from [mg/l] to [meq/m3]
			return (df[element] * charge[element] / atomic_mass[element]) * 1000.0
		else :
			#Assume source is in [meq/m3] = [µeq/l]
			return df[element]
	
	sea_salt_ratio = {
		'Ca' : 0.037,
		'Mg' : 0.196,
		'Na' : 0.856,
		'K'  : 0.018,
		#'NH4':
		'SO4': 0.103,
		#'Cl' :
		#'NO3':
		#'F'  :    #TODO!!! What to do about F?
	}
	
	def add_single(obs, name, nonmarine=None) :
		add_single_deposition_ts(input_df, obs, nonmarine, name, None if (patch_scale is None or name not in patch_scale) else patch_scale[name], patch_ref_range)
	
	obs_ss_conc = get_converted_conc(source_df, sea_salt_ref, source_is_mg_l)
	
	if sea_salt_ref is 'Cl' :
		obs_cl_conc = obs_ss_conc
	else :
		obs_cl_conc = obs_ss_conc / sea_salt_ratio[sea_salt_ref]	
	
	obs_cl_dep    = obs_cl_conc * input_df['Precipitation'] * 1e-3      #TODO: 1e-3 is if precip is given in mm. Otherwise, remove that factor!
	
	#print(obs_cl_dep.resample('AS').sum())
	
	
	#TODO: This is probably correct, because we can't assume the sea salt ref runs straight through??
	obs_cl_runoff_conc = get_converted_conc(runoff_df2, 'Cl', runoff_is_mg_l)
	obs_cl_runoff = obs_cl_runoff_conc * input_df['Runoff'] * 1e-3             #TODO: Measured in mm or m?
	
	sea_salt_factor = np.sum(obs_cl_runoff[cl_range].values) / np.sum(obs_cl_dep[cl_range].values)
	
	print('The sea salt factor was: %g' % sea_salt_factor)
	
	## SO4 computation:
	sea_salt_so4_conc = obs_cl_conc * 0.103 * sea_salt_factor
	sea_salt_so4_dep = obs_cl_dep * 0.103 * sea_salt_factor
	obs_so4_conc = get_converted_conc(source_df, 'SO4', source_is_mg_l)
	
	obs_so4_dep = obs_so4_conc * input_df['Precipitation'] * 1e-3
	
	obs_so4_runoff_conc = get_converted_conc(runoff_df2, 'SO4', runoff_is_mg_l)
	obs_so4_runoff = obs_so4_runoff_conc * input_df['Runoff'] * 1e-3
	
	#print(input_df['Runoff'][cl_range])
	#print(obs_so4_runoff)
	#print(obs_so4_dep)
	
	computed_excess_so4_dep = obs_so4_runoff - so4_weathering/12.0 - sea_salt_so4_dep   #TODO: instead of dividing by 12.0, that should really be sensitive to month length
	obs_excess_so4_dep = obs_so4_dep - sea_salt_so4_dep
	
	so4_excess_factor = np.sum(computed_excess_so4_dep[cl_range].values) / np.sum(obs_excess_so4_dep[cl_range].values)
	
	print('The SO4* factor was %g' % so4_excess_factor)
	
	projected_so4_excess = so4_excess_factor*(obs_so4_conc - sea_salt_so4_conc)
	add_single(sea_salt_so4_conc + projected_so4_excess, 'SO4')
	
	
	add_single(obs_ss_conc*sea_salt_factor, sea_salt_ref)
	if sea_salt_ref is not 'Cl' :
		add_single(obs_cl_conc * sea_salt_factor, 'Cl')

	for element in ['Ca', 'Mg', 'Na', 'K'] :
		if element is sea_salt_ref : continue
		
		ss_conc = obs_cl_conc * sea_salt_factor * sea_salt_ratio[element]   #NOTE: The sea_salt_ratio is in eq/eq, so no need to use charge and molar mass here
		
		if (ca_ref_range is not None) and element is 'Ca' :
			ca_range = year_range(ca_ref_range[0], ca_ref_range[1], monthly=True)
		
			ss_ca_dep = ss_conc * input_df['Precipitation'] * 1e-3
			
			#NOTE: No runoff correction for Ca deposition possible due to retention!
			obs_ca_conc = get_converted_conc(source_df, 'Ca', source_is_mg_l)
			obs_ca_dep = obs_ca_conc * input_df['Precipitation'] * 1e-3
			
			excess_ca_dep = obs_ca_dep - ss_ca_dep
			
			ca_excess_factor = np.nansum(excess_ca_dep[ca_range].values) / np.nansum(computed_excess_so4_dep[ca_range].values)  #TODO: ratio to computed or to obs? We scale with comp. later, so should use that?
			#ca_excess_factor = 0.1
			
			print('The Ca* factor was %g' % ca_excess_factor)
			
			projected_ca_excess = projected_so4_excess*ca_excess_factor*ca_excess_weight
			ca_conc = ss_conc + projected_ca_excess
			
			add_single(ca_conc, 'Ca', projected_ca_excess)
		else :
			add_single(ss_conc, element)
		
		#Computing NH4 and NO3:
		
		#TODO: NH4 and NO3 dry deposition!
		
		nh4_conc = get_converted_conc(source_df, 'NH4', source_is_mg_l)*n_dry_deposition_factor
		add_single(nh4_conc, 'NH4')
		
		no3_conc = get_converted_conc(source_df, 'NO3', source_is_mg_l)*n_dry_deposition_factor
		add_single(no3_conc, 'NO3')
		
		
		#TODO: F !
	
	
	
	#Adding observed values too:
	
	for element in runoff_df :
		if element not in ['Ca', 'Mg', 'Na', 'K', 'NH4', 'SO4', 'Cl', 'NO3', 'F', 'H+'] : continue
		if np.isnan(runoff_df[element]).all() : continue
		
		input_df['Observed runoff conc %s' % element] = runoff_df[element]
	
	
	
	
def add_single_deposition_ts(input_df, source, nonmarine=None, element_name='Cl', patch_scale=None, patch_ref_range=(1989,1992)) :

	#NOTE: Only works if source is monthly. Should only be called by the above function!
	#NOTE: We have a separate patch source for Ca, which is the non-marine deposition only..
	
	ref_dates = year_range(patch_ref_range[0], patch_ref_range[1], monthly=True)

	source = source.reindex(input_df.index)
	if nonmarine is not None :
		nonmarine = nonmarine.reindex(input_df.index)
		patch_offset = np.nanmean((source-nonmarine)[ref_dates])    #NOTE: For Ca we still want to use the marine deposition as a base.
		scale_val    = np.nanmean(nonmarine[ref_dates])
	else :
		patch_offset = 0.0
		scale_val = np.nanmean(source[ref_dates])
	
	
	if patch_scale is not None :
		patch = patch_offset + patch_scale * scale_val
	
		for index, val in source.iteritems() :
			if np.isnan(val) and (index in patch.index) :
				source[index] = patch[index]
	else :
		#NOTE: This makes everything before const. equal to this value when we later interpolate
		first_idx = source.first_valid_index()
		idx = np.searchsorted(source.index, first_idx)
		source[source.index[idx-1]] = patch_offset + scale_val  #patch_scale is supposed to be constantly equal to 1
			
	source = source.interpolate(limit_area=None, limit_direction='both')
	
	input_df['%s conc in precip' % element_name] = source

	
def get_emep_deposition_scales() :
	#NOTE: This is really just correct for an area around Birkenes. We should make something more general.

	emep = {
		'Date' : pd.to_datetime(['1850-1-1', '1915-1-1', '1925-1-1', '1940-1-1', '1955-1-1', '1960-1-1', '1965-1-1', '1970-1-1', '1975-1-1', '1980-1-1', '1985-1-1', '1990-1-1'], yearfirst=True),
		#'Ca'   : [0.5, 0.81, 0.82, 0.89, 1.1, 1.13, 1.24, 1.30, 1.20, 1.17, 1.06, 1.0],
		'Ca'   : [0.05, 0.61, 0.64, 0.78, 1.2, 1.26, 1.48, 1.60, 1.41, 1.34, 1.13, 1.0],  #Since the scaling for Ca is only applied to 0.5*(excess deposition) now, we use the same series for Ca as for SO4
		'SO4'  : [0.05, 0.61, 0.64, 0.78, 1.2, 1.26, 1.48, 1.60, 1.41, 1.34, 1.13, 1.0],
		'NH4'  : [0.0, np.nan, np.nan, np.nan, np.nan, 0.99, 1.0, 1.02, 1.12, 1.14, 1.12, 1.0],
		'NO3'  : [0.0, np.nan, np.nan, np.nan, np.nan, 0.53, 0.63, 0.74, 0.76, 0.84, 0.84, 1.0],
	}
	
	emep_df = pd.DataFrame(emep)
	emep_df.set_index('Date', inplace=True)
	
	return emep_df
	
	
def plot_inputs(input_df) :
	num = len(input_df.columns)
	
	fig, ax = plt.subplots(num)
	fig.set_size_inches(20, 5*num)
	
	for idx, col in enumerate(input_df) :
		if col.startswith('Observed') :
			input_df[col].plot(ax=ax[idx], legend=col, marker='o')
		else :	
			input_df[col].plot(ax=ax[idx], legend=col)
	
	
def write_as_input_file(input_df, filename) :
	
	try :
		os.makedirs(os.path.dirname(filename), exist_ok=True)
	except :
		#Do nothing. We expect a failure if there is no directory in the path name
		dum=0
		
	of = open(filename, 'w')
	
	of.write('start_date: %s\n' % pd.to_datetime(input_df.index.values[0]).date())
	of.write('end_date: %s\n' % pd.to_datetime(input_df.index.values[-1]).date())
	
	of.write('\n')
	
	
	accepted = ['Air temperature', 'Precipitation', 
		'Ca conc in precip', 'Mg conc in precip', 'Na conc in precip', 'K conc in precip', 'NH4 conc in precip', 'SO4 conc in precip', 'Cl conc in precip', 'NO3 conc in precip', 'F conc in precip',
		]
		
	def get_unit(name) :
		units = {
			'Runoff' : 'mm/month',
			'Observed runoff conc' : 'meq/m3',
		}
		for key in units :
			if name.startswith(key) :
				return units[key]
		return None
	
	additional = []
	for col in input_df :
		if col not in accepted : additional.append(col)
	
	if len(additional) > 0 :
		of.write('additional_timeseries:\n')
		for col in additional :
			of.write('"%s"' % col)
			unit = get_unit(col)
			if unit is not None :
				of.write(' unit "%s"' % unit)
			of.write('\n')
		of.write('\n')
	
	of.write('inputs:\n')
	
	for ts in input_df :
		of.write('"%s" :\n' % ts)
		for index, row in input_df.iterrows() :
			val = row[ts]
			if not np.isnan(val) :
				of.write('%s\t%g\n' % (index.date(), val))
		of.write('\n\n\n')
			
	of.close()
	
	
	
	
	
	
	