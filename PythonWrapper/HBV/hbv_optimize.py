
import imp

# Path to folder containing wrapper modules
wrapper_fpath = (r"..\inca.py")
optimize_funs_fpath = (r'..\inca_calibration.py')

wr = imp.load_source('inca', wrapper_fpath)
cf = imp.load_source('inca_calibration', optimize_funs_fpath)

wr.initialize('hbv.dll')

dataset = wr.DataSet.setup_from_parameter_and_input_files('../../Applications/HBV/langtjernparameters.dat', '../../Applications/HBV/langtjerninputs.dat')


dataset.set_parameter_uint('Timesteps', [], 730)
dataset.set_parameter_time('Start date', [], '2010-1-1')


#NOTE: The 'calibration' structure is a list of (indexed) parameters that we want to calibrate
calibration = [
	('Degree day evapotranspiration',                                           ['Forest']),
	('Fraction of field capacity where evapotranspiration reaches its maximal', ['Forest']),
	('Beta recharge exponent',                                                  ['Forest']),
	('Recession coefficient for groundwater slow flow (K1)',                    ['R1']),
	('Recession coefficient for groundwater fast flow (K0)',                    ['R1']),
	('Threshold for second runoff in groundwater storage (UZL)',                ['R1']),
	('a',                                                                       ['R1']),
	('b',                                                                       ['R1']),
	]


initial_guess = cf.default_initial_guess(dataset, calibration)    #NOTE: This reads the initial guess that was provided by the parameter file.
initial_guess.append(0.5)

min = [0.1 * x for x in initial_guess]
max = [10.0 * x for x in initial_guess]

cf.constrain_min_max(dataset, calibration, min, max) #NOTE: Constrain to the min and max values recommended by the model in case we made our bounds too wide.

skiptimesteps = 100   # Skip these many of the first timesteps in the objective evaluation

objective = (cf.log_likelyhood, 'Reach flow', ['R1'], 'Discharge', [], skiptimesteps)


param_est = cf.run_optimization(dataset, min, max, initial_guess, calibration, objective, minimize=False)

print('\n')
for idx, cal in enumerate(calibration) :
	name, indexes = cal
	print('Estimated %-60s %-20s %5.2f (range [%5.2f, %5.2f])' %  (name, ', '.join(indexes), param_est[idx], min[idx], max[idx]))
if len(param_est) > len(calibration) :
	print('M: %f' % param_est[len(calibration)])


# NOTE: Write the optimal values back to the dataset and then generate a new parameter file that has these values.
cf.set_values(dataset, param_est, calibration)
dataset.write_parameters_to_file('hbv_optimal_parameters.dat')

# NOTE: Run the model one more time with the optimal parameters and plot them
dataset.run_model()
cf.plot_objective(dataset, objective, "hbv_plots\\optimizer_MAP_langtjern.png")



