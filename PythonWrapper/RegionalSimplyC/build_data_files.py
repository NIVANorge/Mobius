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
	
def get_flow_df(catch_no, catch_name) :
	filename = '%s/%s/%d_%s.xlsx' % (location, flow_doc_loc, catch_no, catch_name)

	flow_df = pd.read_excel(filename, sheet_name='daily runoff')
	#NOTE: using the automatic format detection it sometimes switches day and month!!
	flow_df['Date'] = pd.to_datetime(flow_df['Date'], format='%d.%m.%Y')   #e.g. '09.07.1984'
	
	flow_df.set_index('Date', inplace=True)
	flow_df = flow_df[['Discharge L/s']]
	flow_df.rename(columns = {'Discharge L/s' : 'Observed flow'}, inplace = True)
	flow_df['Observed flow'] *= 0.001   #L/s to m3/s
	
	if catch_name == 'Aneboda' :
		flow_df['Observed flow'] *= 4.762         #NOTE: For some reason the flow was wrongly scaled for this catchment. The factor is a mystery...
	
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
	
def create_mobius_input_file(catch_no, catch_name, met_df, flow_df, doc_df, usevar):
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
		#if not (catch_no==15) : continue 
		catch_name = row['name']
		lat        = row['lat']
		lon        = row['lon']
		
		print('Processing catchment: %s' % catch_name)
		
		
		met_df = pd.DataFrame(columns = ['Date'])
		met_df['Date'] = time_index#
		met_df.set_index('Date', inplace=True)
		for var in ['RR', 'TG'] :
		
			values = get_met_series(var, catch_no, lat, lon)
		
			name = 'Precipitation' if var == 'RR' else 'Air temperature'
			
			met_df[name] = values
			
		flow_df = get_flow_df(catch_no, catch_name)
		
		doc_df, usevar  = get_doc_df(catch_no, catch_name)
			
		create_mobius_input_file(catch_no, catch_name, met_df, flow_df, doc_df, usevar)
	
	
if __name__ == "__main__":
	main()