

import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import os



def year_range(begin_year, end_year, monthly=True) :
	start = pd.to_datetime('%s-1-1' % begin_year, yearfirst=True)
	end   = pd.to_datetime('%s-12-1' % end_year if monthly else '%s-1-1' % end_year, yearfirst=True)
	
	return np.array(pd.date_range(start = start, end = end, freq='MS' if monthly else 'AS'))

def empty_input_df(begin_year, end_year) :
	return pd.DataFrame(index = year_range(begin_year, end_year))


def add_ts(input_df, source_df, dest_name, source_name, aggr_method='mean') :
	
	source = source_df[source_name]
	
	if aggr_method == 'sum' :
		monthly = source.resample('MS').sum(min_count=28)  #This min count is not ideal, we could also use mean, and multiply with length of month
	elif aggr_method == 'mean' :
		monthly = source.resample('MS').mean()
		
	distr = np.zeros(12)
	count = np.zeros(12)
	
	for index, val in monthly.iteritems() :
		if not np.isnan(val) :
			distr[index.month-1] += val
			count[index.month-1] += 1
	
	if aggr_method == 'sum' :
		distr /= np.nansum(monthly.values)
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
	

def add_precip_ts(input_df, source_df, source_name) :#, source_is_mm=True) :
	add_ts(input_df, source_df, 'Precipitation', source_name, 'sum')
	#if source_is_mm : input_df['Precipitation'] *= 1e-3     #NOTE MAGIC wants this in m/month instead of mm/month
	

def add_air_temperature_ts(input_df, source_df, source_name) :
	add_ts(input_df, source_df, 'Air temperature', source_name, 'mean')
	
def add_deposition_ts(input_df, source_df, element_name, patch_scale=None, patch_ref_range=range(1989,1992)) :
	
	source = source_df[element_name]

	#TODO: Currently only works if source_df is yearly and has values on january 1st for its range
	ref_dates = pd.to_datetime(['%s-1-1' % ref_year for ref_year in patch_ref_range])

	ref_val = np.nanmean([source[ref_date] for ref_date in ref_dates])
	
	source = source.reindex(input_df.index)
	
	if patch_scale is not None :
		patch = patch_scale * ref_val
	
		for index, val in source.iteritems() :
			if np.isnan(val) and (index in patch.index) :
				source[index] = patch[index]
	else :
		#NOTE: This makes everything before const. equal to this value when we later interpolate
		first_idx = source.first_valid_index()
		idx = np.searchsorted(source.index, first_idx)
		source[source.index[idx-1]] = ref_val
			
	source = source.interpolate(limit_area=None, limit_direction='both')
	
	input_df['%s conc in precip' % element_name] = source

	
def get_emep_deposition_scales() :
	emep = {
		'Date'        : pd.to_datetime(['1850-1-1', '1915-1-1', '1925-1-1', '1940-1-1', '1955-1-1', '1960-1-1', '1965-1-1', '1970-1-1', '1975-1-1', '1980-1-1', '1985-1-1', '1990-1-1'], yearfirst=True),
		'half-SO4' : [0.5, 0.81, 0.82, 0.89, 1.1, 1.13, 1.24, 1.30, 1.20, 1.17, 1.06, 1.0],
		'SO4'      : [0.05, 0.61, 0.64, 0.78, 1.2, 1.26, 1.48, 1.60, 1.41, 1.34, 1.13, 1.0],
		'NH4'      : [0.0, np.nan, np.nan, np.nan, np.nan, 0.99, 1.0, 1.02, 1.12, 1.14, 1.12, 1.0],
		'NO3'      : [0.0, np.nan, np.nan, np.nan, np.nan, 0.53, 0.63, 0.74, 0.76, 0.84, 0.84, 1.0],
	}
	
	emep_df = pd.DataFrame(emep)
	emep_df.set_index('Date', inplace=True)
	
	return emep_df
	
	
def plot_inputs(input_df) :
	num = len(input_df.columns)
	
	fig, ax = plt.subplots(num)
	fig.set_size_inches(20, 5*num)
	
	for idx, col in enumerate(input_df) :
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
	
	#TODO: Additional time series
	
	of.write('inputs:\n')
	
	for ts in input_df :
		of.write('"%s" :\n' % ts)
		for index, row in input_df.iterrows() :
			val = row[ts]
			if not np.isnan(val) :
				of.write('%s\t%g\n' % (index.date(), val))
		of.write('\n\n\n')
			
	of.close()
	
	
	
	
	
	
	