from importlib.machinery import SourceFileLoader
import pandas as pd
import numpy  as np

wr = SourceFileLoader("mobius", r"../../mobius.py").load_module()
#cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"../../mobius_calib_uncert_lmfit.py").load_module()

multi = SourceFileLoader("multiloop", r"../multiloop.py").load_module()
magicopt = SourceFileLoader("magicopt",  r"../magicopt.py").load_module()

wr.initialize('../../../Applications/MAGIC/magic_forest.dll')

parfile = 'templateparameters.dat'
infile  = 'templateinputs.dat'


for lake_id in [....] :

	ds = wr.DataSet.setup_from_parameter_and_input_files(parfile, infile)
	
	
	ds.set_parameter_double("Depth", ["Soil"], 0.5)
	
	
	
	
	
	
	
	
	
	ds.write_parameters_to_file("initial_data/lake_%d.dat"%lake_id)
	ds.write_inputs_to_file("inputs/lake_%d.dat"%lake_id)
	
	ds.delete()

