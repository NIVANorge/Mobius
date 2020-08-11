import xarray as xr
import pandas as pd
import numpy as np
import os

location = 'D:/biowater_data'
climate_loc = 'climate_data'
flow_doc_loc = 'water_chemistry'

def get_time_index():
	filename = '%s/%s/downloads/biowater_metno_RR.nc' % (location, climate_loc)
	ds = xr.open_dataset(filename)

	time_index = [pd.Timestamp(x).date() for x in ds.time.data]

	ds.close()
	
	return time_index
	
def get_met_series(type, catch_no, lat, lon):
	filename = '%s/%s/downloads/biowater_metno_%s.nc' % (location, climate_loc, type)
	ds = xr.open_dataset(filename)
	
	#NOTE: Just check that the recorded lat and lon for the locations are not that far off from what we have from other sources.
	lat_ds = ds.latitude[0,catch_no]
	lon_ds = ds.longitude[0,catch_no]
	if np.abs(lat_ds - lat)>0.1 or np.abs(lon_ds - lon)>0.1 :
		print('Error in coordinates of catchment %d' % catch_no)
	
	values = ds[type][:, 0, catch_no].data
	if(type == 'TG') : values -= 273.15          #Kelvin to Celsius

	ds.close()
	
	return values
	
def get_era5_df(type, catch_no, lat, lon):
	#type should be tcc, ssr or rh
	
	filename = '%s/%s/downloads/era5_point_data.nc' % (location, climate_loc)
	ds = xr.open_dataset(filename)
	
	era5_df = pd.DataFrame()
	
	era5_df['Date'] = ds.time.data
	era5_df.set_index('Date', inplace=True)
	
	#NOTE: Just check that the recorded lat and lon for the locations are not that far off from what we have from other sources.
	lat_ds = ds.latitude[0,catch_no]
	lon_ds = ds.longitude[0,catch_no]
	if np.abs(lat_ds - lat)>0.1 or np.abs(lon_ds - lon)>0.1 :
		print('Error in coordinates of catchment %d in era5 data' % catch_no)
	
	for index, var in enumerate(['ssr', 'rh', 'tcc']) :
		name = ['Net shortwave radiation', 'Relative humidity', 'Cloud cover'][index]
	
		if var=='rh' :
			#Compute relative humidity from dewpoint temperature
			def es(t):
				return 611.21 * np.exp(17.502*( (t-273.16) / (t-32.19) ))
			
			t2m = ds['t2m'][:, 0, catch_no].data
			d2m = ds['d2m'][:, 0, catch_no].data

			values = 100.0 * es(d2m)/es(t2m)
		else :
			values = ds[var][:, 0, catch_no].data
		
		if var == 'ssr' : values *= 24*1e-6 #convert J to MJ   #the 24 is to compensate for taking mean below, which should be sum for ssr
		
		era5_df[name] = values
	
	era5_df = era5_df.resample('D').mean()
	
	ds.close()
	
	return era5_df
	
def get_flow_df(catch_no, catch_name, catch_area) :
	filename = '%s/%s/%d_%s.xlsx' % (location, flow_doc_loc, catch_no, catch_name)

	flow_df = pd.read_excel(filename, sheet_name='daily runoff')
	#NOTE: using the automatic format detection it sometimes switches day and month!!
	flow_df['Date'] = pd.to_datetime(flow_df['Date'], format='%d.%m.%Y')   #e.g. '09.07.1984'
	
	flow_df.set_index('Date', inplace=True)
	flow_df = flow_df[['Discharge L/s']]
	flow_df.rename(columns = {'Discharge L/s' : 'Observed flow'}, inplace = True)
	flow_df['Observed flow'] *= 0.001   #L/s to m3/s
	
	#NOTE: In some of the catchments the flow is scaled wrong, so we load the yearly values (which are correct), and rescale based on these:
	filename2 = '%s/%s/fluxes_%d_%s.xlsx' % (location, flow_doc_loc, catch_no, catch_name)
	try:
		flux_df = pd.read_excel(filename2, sheet_name='data-year', parse_dates=['Year ',])
		flux_df = flux_df[['Year ', 'Discharge_sum']].dropna()
		flux_df.set_index('Year ', inplace=True)
		
		flow_df2 = flow_df.resample('YS').sum()
		flow_df2['Observed flow'] *= (86.4 / catch_area)        #m3/s -> mm/yr
		
		flux_df = pd.concat([flow_df2, flux_df], axis=1)
		flux_df['conv'] = flux_df['Discharge_sum']/flux_df['Observed flow']
		
		#print(flux_df)
		
		conv = flux_df['conv'].dropna().values
		print('flux conv mean: %f, stddev: %f ' % (np.mean(conv), np.std(conv)))
	
		flow_df['Observed flow'] *= np.mean(conv)
	except BaseException as ex:
		print(ex)

	
	#NOTE: For some reason the flow was wrongly scaled for these catchments. The factor is a mystery...
	#if catch_name == 'Aneboda' :
	#	flow_df['Observed flow'] *= 4.762
	#elif catch_name == 'Lommabacken' :
	#	flow_df['Observed flow'] *= 0.0953
	#elif catch_name == 'Lilltjarnsbacken' :
	#	flow_df['Observed flow'] *= 0.1962
	#elif catch_name == 'Pipbacken' :
	#	flow_df['Observed flow'] *= 0.000543
	#elif catch_name == 'Svartberget' :
	#	flow_df['Observed flow'] *= 0.5972

	
	flow_df.dropna(inplace=True)

	return flow_df

def get_doc_df(catch_no, catch_name) :
	filename = '%s/%s/%d_%s.xlsx' % (location, flow_doc_loc, catch_no, catch_name)
	
	doc_df  = pd.read_excel(filename, sheet_name='DATA')
	
	try:
		#TODO: check that the default gets it correct when it works
		doc_df['Date_Time_start'] = doc_df['Date_Time_start'].astype('datetime64[ns]')
	except:
		#NOTE: Unfortunately some of the Swedish catchments have a format that the parser doesn't recognize
		doc_df['Date_Time_start'] = pd.to_datetime(doc_df['Date_Time_start'], format='%d.%m.%Y %H.%M')   #e.g. '09.07.1984 00.00'
	
	TOCcount = np.sum(~np.isnan(doc_df['TOC'].values))
	DOCcount = np.sum(~np.isnan(doc_df['DOC'].values))
	
	usevar = 'DOC'
	if TOCcount > DOCcount :
		usevar = 'TOC'
		
	meanval = np.nanmean(doc_df[usevar].values)
	
	if meanval > 800.0 :   #Indicating that the unit is probably kg/m3 instead of mg/l, so we have to unit convert
		doc_df[usevar] *= 0.001
	
	doc_df.set_index('Date_Time_start', inplace=True)
	doc_df = doc_df[[usevar]]
	doc_df = doc_df.resample('D').mean()
	
	doc_df.rename(columns = {usevar : 'Observed DOC'}, inplace = True)
	doc_df.dropna(inplace=True)
	
	return doc_df, usevar
	
def get_so4_df(catch_no) :
	filename = '%s/%s/sulfur_dep.csv' % (location, flow_doc_loc)
	df = pd.read_csv(filename, sep='\t')
	df.set_index('year', inplace=True)
	df = df[['catch_%d'%catch_no]]
	return df
	
def create_mobius_input_file(catch_no, catch_name, met_df, era5_df, flow_df, doc_df, so4_df, usevar):
	filename = 'MobiusFiles/inputs_%d_%s.dat' % (catch_no, catch_name)
	
	os.makedirs(os.path.dirname(filename), exist_ok=True)
	outfile = open(filename, 'w')
	
	#TODO: Add more metadata comments
	if usevar != 'DOC' : outfile.write('#NOTE: Using TOC as proxy for DOC\n\n')
	
	outfile.write('start_date: %s\n' % met_df.index.values[0])
	outfile.write('end_date: %s\n' % met_df.index.values[-1])

	outfile.write('\n')

	outfile.write('additional_timeseries:\n')
	outfile.write('"Observed flow" unit "m3/s"\n')
	outfile.write('"Observed DOC"  unit "mg/L"\n')

	outfile.write('\n')

	outfile.write('inputs:\n')
	for col in met_df.columns :
		outfile.write('\n')
		outfile.write('"%s":\n' % col)
		for val in met_df[col] :
			outfile.write('%g\n' % val)

	outfile.write('\n')
	
	for col in era5_df.columns :
		outfile.write('\n')
		outfile.write('"%s":\n' % col)
		for index, row in era5_df.iterrows() :
			outfile.write('%s %g\n' % (index, row[col]))

	outfile.write('\n')

	outfile.write('"SO4 deposition" linear_interpolate :\n')
	for index, row in so4_df.iterrows() :
		outfile.write('%d-1-1 %g\n' % (index, row['catch_%d' % catch_no]))

	outfile.write('\n')
			
	outfile.write('"Observed flow":\n')
	for index, row in flow_df.iterrows() :
		outfile.write('%s %g\n' % (index.date(), row['Observed flow']))
		
	outfile.write('\n')

	outfile.write('"Observed DOC":\n')
	for index, row in doc_df.iterrows() :
		outfile.write('%s %g\n' % (index.date(), row['Observed DOC']))
			
	outfile.close()

def main():
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	time_index = get_time_index()
	
	for index, row in catch_setup.iterrows():
		
		catch_no   = row['met_index']
		catch_name = row['name']
		lat        = row['lat']
		lon        = row['lon']
		catch_area = row['area_km2']
		
		#if(catch_name != 'Gardsjon') : continue
		
		print('Processing catchment: %s' % catch_name)
		
		
		met_df = pd.DataFrame(columns = ['Date'])
		met_df['Date'] = time_index
		met_df.set_index('Date', inplace=True)
		for var in ['RR', 'TG'] :
		
			values = get_met_series(var, catch_no, lat, lon)
		
			name = 'Precipitation' if var == 'RR' else 'Air temperature'
			
			met_df[name] = values
		
		era5_df = get_era5_df(type, index, lat, lon) #NOTE: Important to pass index rather than catch_no as catch_no here!
		
		flow_df = get_flow_df(catch_no, catch_name, catch_area)
		
		doc_df, usevar  = get_doc_df(catch_no, catch_name)
		
		so4_df = get_so4_df(catch_no)

		
		create_mobius_input_file(catch_no, catch_name, met_df, era5_df, flow_df, doc_df, so4_df, usevar)
	
	
if __name__ == "__main__":
	main()