
import numpy as np, imp

# Path to folder containing wrapper modules
wrapper_fpath = (r"..\mobius.py")
optimize_funs_fpath = (r'..\mobius_calibration.py')

wr = imp.load_source('mobius', wrapper_fpath)
cf = imp.load_source('mobius_calibration', optimize_funs_fpath)	

wr.initialize('simplyp.dll')

dataset = wr.DataSet.setup_from_parameter_and_input_files('../../Applications/SimplyP/Tarland/TarlandParameters.dat', '../../Applications/SimplyP/Tarland/TarlandInputs.dat')

#NOTE: The 'calibration' structure is a list of (indexed) parameters that we want to calibrate
calibration = [
	('Proportion of precipitation that contributes to quick flow', []),
	('Baseflow index',                                             []),
	('Groundwater time constant',                                  []),
	('Gradient of reach velocity-discharge relationship',          []),
	('Exponent of reach velocity-discharge relationship',          []),
	('Soil water time constant',                                   ['Arable']),
	('Soil water time constant',                                   ['Semi-natural']),
	]
	
print(dataset.get_branch_inputs('Reaches', 'Tarland1'))
	
initial_guess = cf.default_initial_guess(dataset, calibration)    #NOTE: This reads the initial guess that was provided by the parameter file.
initial_guess.append(0.5)

min = [0.1 * x for x in initial_guess]
max = [10.0 * x for x in initial_guess]

cf.constrain_min_max(dataset, calibration, min, max) #NOTE: Constrain to the min and max values recommended by the model in case we made our bounds too wide.

skiptimesteps = 50   # Skip these many of the first timesteps in the objective evaluation

comparisons = [
	('Reach flow (daily mean, cumecs)', ['Tarland1'], 'observed Q', []),
	#put more here!
	]

objective = (cf.log_likelyhood, comparisons, skiptimesteps)


param_est = cf.run_optimization(dataset, min, max, initial_guess, calibration, objective, minimize=False)
#param_est = param_est[0]

print('\n')
for idx, cal in enumerate(calibration) :
	name, indexes = cal
	print('Estimated %-60s %-20s %5.2f (range [%5.2f, %5.2f])' %  (name, ', '.join(indexes), param_est[idx], min[idx], max[idx]))
if len(param_est) > len(calibration) :
	print('M: %f' % param_est[len(calibration)])

# Computing the Hessian at the optimal point:
hess = cf.compute_hessian(dataset, param_est, calibration, objective)
print('\nHessian matrix at optimal parameters:')
cf.print_matrix(hess)
inv_hess = np.linalg.inv(hess)
print('\nInverse Hessian:')
cf.print_matrix(inv_hess)

# NOTE: Write the optimal values back to the dataset and then generate a new parameter file that has these values.
cf.set_values(dataset, param_est, calibration)
dataset.write_parameters_to_file('optimal_parameters.dat')

# NOTE: Run the model one more time with the optimal parameters to get the correct values in the dataset, then print goodness of fit and plot.
dataset.run_model()
cf.print_goodness_of_fit(dataset, objective)
cf.plot_objective(dataset, objective, "simplyp_plots\\optimizer_MAP.png")



