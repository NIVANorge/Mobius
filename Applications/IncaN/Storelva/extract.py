

# Extracts projected climate data from netCDF files downloaded from https://nedlasting.nve.no/klimadata/kss and generates input files for Mobius INCA-N.
# weights.txt weighs how much each 1x1km grid cell contributes to the specific catchment.


from netCDF4 import Dataset
import numpy as np

scenarios = ['rcp85', 'rcp45']
models = ['CNRM_CCLM', 'CNRM_RCA', 'EC-EARTH_CCLM', 'EC-EARTH_HIRHAM', 'EC-EARTH_RACMO', 'EC-EARTH_RCA', 'HADGEM_RCA', 'IPSL_RCA', 'MPI_CCLM', 'MPI_RCA']

weights = np.loadtxt('weights.txt')
weights = weights / np.sum(weights)


for scenario in scenarios :
	for model in models :
	
		print('%s_%s' % (scenario, model))

		filenames_p = ['KSSData/%s_RR_DAY/%s_%s_RR_daily_mm_%d.nc' % (model, 'hist' if year <= 2005 else scenario, model, year) for year in range(1990, 2101)]
		filenames_t = ['KSSData/%s_TM_DAY/%s_%s_TM_daily_K_%d.nc'  % (model, 'hist' if year <= 2005 else scenario, model, year) for year in range(1990, 2101)]

		precipitation = np.array([])
		air_temperature = np.array([])

		for ii in range(0, len(filenames_p)) :

			year = 1990 + ii
			
			#print(year)

			data = Dataset(filenames_p[ii], 'r', format='NETCDF4')

			precip = np.array(data['/precipitation__map_%s_daily' % ('hist' if year <= 2005 else scenario)]) * 0.1
	
			precipitation = np.concatenate((precipitation, [np.average(precip[ii], weights=weights) for ii in range(precip.shape[0])]))

			data.close()

			data = Dataset(filenames_t[ii], 'r', format='NETCDF4')
			
			air_t = np.array(data['/air_temperature__map_%s_daily' % ('hist' if year <= 2005 else scenario)]) * 0.1 - 273.15
			
			air_temperature = np.concatenate((air_temperature, [np.average(air_t[ii], weights=weights) for ii in range(air_t.shape[0])]))

			data.close()
			
		output = open('inputs_Storelva_%s_%s.dat' % (model, scenario), 'w')

		print('start_date : 1990-01-01', file=output)
		print('timesteps : %d' % len(air_temperature), file=output)
		print('', file=output)
		print('index_set_dependencies :\n "Fertilizer ammonium" : {"Landscape units"}\n"Fertilizer nitrate"  : {"Landscape units"}\n', file=output)

		print('inputs :', file=output)
		print('include_file "N_supplies_Storelva_extrapolated.dat"', file=output)
		print('', file=output)
		print('"Actual precipitation" : ', file=output)
		for prec in precipitation :
			print('%g' % prec, file=output)
		print('', file=output)
		print('"Air temperature" : ', file=output)
		for temp in air_temperature :
			print('%g' % temp, file=output)

		output.close()

