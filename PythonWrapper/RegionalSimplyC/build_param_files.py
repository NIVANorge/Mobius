import numpy as np
import pandas as pd

from importlib.machinery import SourceFileLoader

# Initialise wrapper
wr = SourceFileLoader("mobius", r"../mobius.py").load_module()
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"..\mobius_calib_uncert_lmfit.py").load_module()

wr.initialize('../../Applications/SimplyC_regional/simplyc_regional.dll')


def main(num_lu=3) :

	if num_lu == 1 :
		parfile = 'template_pars_1lu.dat'
	elif num_lu == 2 :
		parfile = 'template_pars_2lu.dat'
	elif num_lu == 3 :
		parfile = 'template_pars.dat'
		
	inputfile = 'template_inputs.dat'
	#template dataset:
	
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	for index, row in catch_setup.iterrows():
	
		islake = (row['lake_catch_km2'] > 0)
		if islake :
			if num_lu != 1 :
				raise Exception('Only one lu supported if using lakes, currently')
			parfile = 'template_pars_1lu_lake.dat'
		else :
			if num_lu == 1 :
				parfile = 'template_pars_1lu.dat'
			elif num_lu == 2 :
				parfile = 'template_pars_2lu.dat'
			elif num_lu == 3 :
				parfile = 'template_pars.dat'
	
		dataset = wr.DataSet.setup_from_parameter_and_input_files(parfile, inputfile)
	
		subcatch = dataset.get_indexes('Reaches')
	
		dataset.set_parameter_double('Latitude', [], row['lat'])
		dataset.set_parameter_double('Elevation', [], row['elev_m'])
		
		ca_river = row['area_km2']
		if islake :
			ca_lake = row['lake_catch_km2']
			ca_river = ca_river - ca_lake
			dataset.set_parameter_double('Catchment area', ['Lake'], ca_lake)
			
		dataset.set_parameter_double('Catchment area', ['River'], ca_river)
		
		reachlen = np.sqrt(row['area_km2'])*1000.0    #NOTE reach length is not sensitive for small catchments, so a rough estimate is ok
		if reachlen < 500.0 : reachlen = 500.0    #To not cause numeric instability
		dataset.set_parameter_double('Reach length', ['River'], reachlen)
		
		lu = [float(a)*0.01 for a in row['lu_FSPLO'].strip('[]').split(',')]   #Percent to fraction
		
		lu_forest = lu[0]
		lu_shrubs = lu[1] + lu[4] + lu[3]    #NOTE: classify 'other' AND 'lakes' as low-productive (only ok when few lakes)
		lu_peat   = lu[2]
		
		if islake :
			lake_area = row['area_km2']*lu[3]*1e6  # Area in m^2
			#TODO: Maybe use fancy retention time formula instead?
			lake_depth = row['lake_depth']     #Ooops, very rough estimate. Kind of ok since there is less mixing with bottom layers in case of deeper lakes?
			lake_volume = lake_area*lake_depth/3 # Pyramidal or conical shape.
			
			dataset.set_parameter_double('Lake surface area', ['Lake'], lake_area)
			dataset.set_parameter_double('Lake volume', ['Lake'], lake_volume)
			
		if row['mineral_layer'] == 'y' :
			dataset.set_parameter_enum('Deep soil/groundwater DOC computation', [], 'mass_balance')
		
		
		if num_lu == 1 :
			dataset.set_parameter_double('Land use proportions', ['River', 'All'], 1.0)
			if islake :
				dataset.set_parameter_double('Land use proportions', ['Lake', 'All'], 1.0)
		elif num_lu == 2 :
			dataset.set_parameter_double('Land use proportions', ['R0', 'LowC'], lu_forest + lu_shrubs)
			dataset.set_parameter_double('Land use proportions', ['R0', 'HighC'], lu_peat)
		elif num_lu == 3 :
			dataset.set_parameter_double('Land use proportions', ['R0', 'Forest'], lu_forest)
			dataset.set_parameter_double('Land use proportions', ['R0', 'Shrubs'], lu_shrubs)
			dataset.set_parameter_double('Land use proportions', ['R0', 'Peat'], lu_peat)
		
		catch_no = row['met_index']
		catch_name = row['name']
		
		if num_lu == 1 :
			dataset.write_parameters_to_file('MobiusFiles/template_params_1lu_%d_%s.dat' % (catch_no, catch_name))
		elif num_lu == 2 :
			dataset.write_parameters_to_file('MobiusFiles/template_params_2lu_%d_%s.dat' % (catch_no, catch_name))
		elif num_lu == 3 :
			dataset.write_parameters_to_file('MobiusFiles/template_params_%d_%s.dat' % (catch_no, catch_name))
	
if __name__ == "__main__":
	main(num_lu=1)