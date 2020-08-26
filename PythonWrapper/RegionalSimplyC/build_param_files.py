import numpy as np
import pandas as pd

from importlib.machinery import SourceFileLoader

# Initialise wrapper
wr = SourceFileLoader("mobius", r"../mobius.py").load_module()
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"..\mobius_calib_uncert_lmfit.py").load_module()

wr.initialize('../../Applications/SimplyC/simplyc_regional.dll')

parfile = 'template_pars.dat'
inputfile = 'template_inputs.dat'


def main() :
	#template dataset:
	dataset = wr.DataSet.setup_from_parameter_and_input_files(parfile, inputfile)
	
	catch_setup = pd.read_csv('catchment_organization.csv', sep='\t')
	
	for index, row in catch_setup.iterrows():
		dataset.set_parameter_double('Latitude', [], row['lat'])
		dataset.set_parameter_double('Elevation', [], row['elev_m'])
		
		
		dataset.set_parameter_double('Catchment area', ['R0'], row['area_km2'])
		
		reachlen = np.sqrt(row['area_km2'])*1000.0    #NOTE reach length is not sensitive for small catchments, so a rough estimate is ok
		if reachlen < 500.0 : reachlen = 500.0    #To not cause numeric instability
		dataset.set_parameter_double('Reach length', ['R0'], reachlen)
		
		lu = [float(a)*0.01 for a in row['lu_FSPLO'].strip('[]').split(',')]   #Percent to fraction
		
		lu_forest = lu[0]
		lu_shrubs = lu[1] + lu[4]     #NOTE: classify 'other' as low-productive shrubs
		lu_peat   = lu[2]
		
		dataset.set_parameter_double('Land use proportions', ['R0', 'Forest'], lu_forest)
		dataset.set_parameter_double('Land use proportions', ['R0', 'Shrubs'], lu_shrubs)
		dataset.set_parameter_double('Land use proportions', ['R0', 'Peat'], lu_peat)
		
		catch_no = row['met_index']
		catch_name = row['name']
		
		dataset.write_parameters_to_file('MobiusFiles/template_params_%d_%s.dat' % (catch_no, catch_name))
	
	
	
	
if __name__ == "__main__":
	main()