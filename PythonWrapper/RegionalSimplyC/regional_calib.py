

import numpy as np
import imp
import pickle
from scipy.stats import norm

# Initialise wrapper
wrapper_fpath = (r"..\mobius.py")
wr = imp.load_source('mobius', wrapper_fpath)
wr.initialize('../../Applications/SimplyC/simplyc_regional.dll')

# Calibration functions
calib_fpath = (r"..\mobius_calib_uncert_lmfit.py")
cu = imp.load_source('mobius_calib_uncert_lmfit', calib_fpath)

calib_locations = ['Langtjern', 'Storgama', 'Birkenes']

datasets = {}

for loc in calib_locations :
	parfile = '../../Applications/SimplyC/%s/params_%s_regional.dat' % (loc, loc)
	inpfile = '../../Applications/SimplyC/%s/inputs_%s.dat' % (loc, loc)
	datasets[loc] = wr.DataSet.setup_from_parameter_and_input_files(parfile, inpfile)