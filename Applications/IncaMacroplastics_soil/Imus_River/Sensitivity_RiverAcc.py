#!/usr/bin/env python
# coding: utf-8

import imp
import numpy as np
import matplotlib.pyplot as plt
from datetime import datetime
import os

import uncertainty as un

wrapper_fpath = (r'C:/MOBIUS/Mobius/PythonWrapper/mobius.py')
mobius = imp.load_source('mobius', wrapper_fpath)

calib_fpath = (r'C:/MOBIUS/Mobius/PythonWrapper/mobius_calib_uncert_lmfit.py')
calib = imp.load_source('calib', calib_fpath)

mobius.initialize(r'C:/MOBIUS/Mobius/Applications/IncaMacroplastics/incamacroplastics_soil.dll')


infile = 'inputs_Ismus_Best_optimistic_Remediation2006_soil.dat'
parfile = 'params_Ismus_MACRO_2lu_RiverAcc_Soil_Agromet.dat'


dataset = mobius.DataSet.setup_from_parameter_and_input_files(parfile, infile)


def set_pars(ds, alph, bur_f, rain, wind, extrem_u, det, bfi_u, tear, P_T_f, P_T_u) :
    ds.set_parameter_double('Mismanagement adjustment factor', [], alph)
    ds.set_parameter_double('Burial proportion in each Land use type', ['Forest'], bur_f)
    ds.set_parameter_double('Rain mobilization probability', [], rain)
    ds.set_parameter_double('Wind mobilization probability', [], wind)
    ds.set_parameter_double('Mobilization probability during extreme rain events in each Land use type', ['Urban'], extrem_u)
#    ds.set_parameter_double('Bank attachment rate tuning parameter', [], att)
    ds.set_parameter_double('Detachment rate tuning parameter', [], det)
    ds.set_parameter_double('Percolation matrix', ['Urban', 'Soil water', 'Soil water'], 1.0-bfi_u)
    ds.set_parameter_double('Percolation matrix', ['Urban', 'Soil water', 'Groundwater'], bfi_u)
#    ds.set_parameter_double('Percolation matrix', ['Forest', 'Soil water', 'Soil water'], 1.0-bfi_f)
#    ds.set_parameter_double('Percolation matrix', ['Forest', 'Soil water', 'Groundwater'], bfi_f)
    ds.set_parameter_double('Bag tearing tuning parameter', [], tear)
#    ds.set_parameter_double('Micro-grinding tuning parameter', [], grind)
    ds.set_parameter_double('Probability to reach the river in each Land use type', ['Forest'], P_T_f)
    ds.set_parameter_double('Probability to reach the river in each Land use type', ['Urban'], P_T_u)
    
ranges = {
    'alph' : (0.0, 0.5),
    'bur_f' : (0.0005, 0.01),
    'rain' : (1.0, 3.0),
    'wind' : (0.01, 0.05),
    'extrem_u' : (0.002, 0.1),
#    'att' : (0.5, 5.0),
    'det' : (0.05, 0.5),
    'bfi_u' : (0.6, 0.8),
#    'bfi_f' : (0.8, 0.95),
    'tear' : (0.01, 0.001),
#    'grind' : (0.0001, 0.00001),
    'P_T_f' : (0.05, 0.34),
    'P_T_u' : (0.37, 0.95),
}

def plastic_output(ds) :
    return np.sum(ds.get_result_series('Total floating litter output (mass)', ['Das_main4']))

main_effect, total_effect, dist = un.compute_effect_indexes(dataset, ranges, set_pars, plastic_output, 140)

print('Main effect:')
print(main_effect)
print('Total effect:')
print(total_effect)

# datetime object containing current date and time
now = datetime.now()

dt_string = now.strftime("%d_%m_%Y_%H_%M_%S")


results_path = (r'./Stopping_at_%s' % dt_string)
os.mkdir(results_path)

content = str(dist)
with open(r'%s/dist.dat' % results_path, 'w') as f:
    f.write(content)

content1 = str(main_effect)
with open(r'%s/main_effect.dat' % results_path, 'w') as f:
    f.write(content1)

content2 = str(total_effect)
with open(r'%s/total_effect.dat' % results_path, 'w') as f:
    f.write(content2)
