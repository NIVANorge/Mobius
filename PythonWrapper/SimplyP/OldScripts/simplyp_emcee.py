

import matplotlib, matplotlib.pyplot as plt, seaborn as sn, emcee, corner, imp, pandas as pd, datetime as dt, random
import numpy as np
import time
from scipy.stats import norm

wrapper_fpath = (r"..\inca.py")
optimize_funs_fpath = (r'..\inca_calibration.py')

wr = imp.load_source('inca', wrapper_fpath)
cf = imp.load_source('inca_calibration', optimize_funs_fpath)	

from multiprocessing import Pool


def log_prior(params, min, max) :
	"""
	params: list or array of parameter values
	min: list or array of minimum parameter values
	max: list or array of maximum parameter values
	"""
	if cf.check_min_max(params, min, max) :
		return 0
	return -np.inf

#IMPORTANT NOTE: If you want to run emcee multithreaded: The log_posterior function has to access the dataset as a global. It can not take it as an argument. This is because ctypes of pointer type can not be 'pickled',
#  which is what python does when it sends arguments to functions on separate threads. In that case, you will end up with a garbage value for the pointer.
def log_posterior(params, min, max, calibration, objective):
	"""
	params: list or array of parameter values
	min: list or array of minimum parameter values
	max: list or array of maximum parameter values
	calibration: 'calibration' structure is a list of tuples, each tuple containing (param name string, [index string])
	objective: tuple (- function to use as measure of model performance,
					  - list containing simulated and observed variables you want to compare. Each element in the list is a 4-element tuple, with    items (name of simulated result, [index result applies to], name of observation input data, [index for observed data])
					  - skiptimesteps: integer, the number of timesteps to discard from the start of the model run when comparing performance (i.e. warmup period)
	"""
	log_pri = log_prior(params, min, max)
	
	llfun = objective[0]
	
	if(np.isfinite(log_pri)):
		log_like = llfun(params, dataset, calibration, objective)
		return log_pri + log_like
	return -np.inf


	
def run_emcee(min, max, initial_guess, calibration, objective, n_walk, n_steps) :
	
	ll, comparisons, skiptimesteps = objective


	n_dim = len(initial_guess)

	starting_guesses = [initial_guess + 1e-4*np.random.randn(n_dim) for i in range(n_walk)]

	
	pool = Pool(8)
		
	sampler = emcee.EnsembleSampler(n_walk, n_dim, log_posterior, 
		#threads=8,
		pool = pool,
		args=[min, max, calibration, objective])

	start = time.time()
	pos, prob, state = sampler.run_mcmc(starting_guesses, n_steps)
	end = time.time()
	print('Time elapsed running emcee: %f' % (end - start))
	
	print('EMCEE average acceptance rate: %f' % np.mean(sampler.acceptance_fraction))

	samples = sampler.chain
	
	lnprobs = sampler.lnprobability
	
	pool.close()

	return samples, lnprobs

def chain_plot(samples, calibration, labels_long, filename):
	#NOTE: This one needs non-reshaped samples
	
	n_dim = samples.shape[2]

	fig, axes = plt.subplots(nrows=n_dim, ncols=1, figsize=(10, 20))    
	for idx, title in enumerate(labels_long):        
		axes[idx].plot(samples[:,:,idx].T, '-', color='k', alpha=0.3)
		axes[idx].set_title(title, fontsize=16) 
	plt.subplots_adjust(hspace=0.7)    
	
	fig.savefig(filename)
	
def reshape_samples(samples, lnprobs, n_burn):
	n_dim = samples.shape[2]
	s = samples[:, n_burn:, :].reshape((-1, n_dim))
	ln = lnprobs[:, n_burn:].reshape((-1))
	
	return s, ln
	
def best_sample(samplelist, lnproblist):
	#NOTE Needs reshaped samples
	index_max = np.argmax(lnproblist)
	best_sample = samplelist[index_max]
	return best_sample	
	
def triangle_plot(samplelist, labels_short, filename):
	#NOTE Needs reshaped samples
	tri = corner.corner(samplelist,
						labels=labels_short,
						#truths=truths,
						quantiles=[0.025, 0.5, 0.975],
						show_titles=True, 
						title_args={'fontsize': 24},
						label_kwargs={'fontsize': 20})

	tri.savefig(filename)
	
def do_n_random_simulations_from_sample_list(dataset, samplelist, calibration, objective, n_samples, comparison_idx) :

	llfun, comparisons, skiptimesteps = objective
	
	comparisontolookat = comparisons[comparison_idx] #TODO: Allow you to select multiple?
	simname, simindexes, obsname, obsindexes = comparisontolookat
	
	param_only    = []
	overall = []
	for it in range(1, n_samples) :       #TODO: Should be paralellized, really..
		
		random_index = random.randint(0, len(samplelist)-1)
		random_sample = samplelist[random_index]
		
		cf.set_values(dataset, random_sample, calibration)
		dataset.run_model()
	
		sim = dataset.get_result_series(simname, simindexes)
	
		param_only.append(sim)
		
		M = random_sample[len(calibration) + comparison_idx]
		
		stoch = norm.rvs(loc=0, scale=M*sim, size=len(sim))
		
		perturbed = sim + stoch

		overall.append(perturbed)
	
	return param_only, overall
	
	
def plot_simulations(dataset, best, simulationresults, calibration, objective, comparison_idx, filename) :

	llfun, comparisons, skiptimesteps = objective
	
	comparisontolookat = comparisons[comparison_idx] 
	simname, simindexes, obsname, obsindexes = comparisontolookat
	
	fig, ax = plt.subplots()
	
	start_date = dt.datetime.strptime(dataset.get_parameter_time('Start date', []),'%Y-%m-%d')
	timesteps = dataset.get_parameter_uint('Timesteps', [])
	date_idx = np.array(pd.date_range(start_date, periods=timesteps))
	
	#for sim in sims :
	for sim in simulationresults :
	
		a = 0.01
		ax.plot(date_idx, sim, color='black', alpha=a, label='_nolegend_') #,linewidth=1)
		
	obs = dataset.get_input_series(obsname, obsindexes, True)
	ax.plot(date_idx, obs, color = 'orange', label = 'observed')
	
	cf.set_values(dataset, best, calibration)
	dataset.run_model()
	
	bestsim = dataset.get_result_series(simname, simindexes)
	
	ax.plot(date_idx, bestsim, color = 'blue', label='best simulated')
	
	ax.legend()
	
	ax.set_xlabel('Date')
	ax.set_ylabel('%s $%s$' % (obsname, dataset.get_result_unit(simname)))
	
	fig.savefig(filename)

def plot_percentiles(dataset, simulationresults, calibration, objective, comparison_idx, perc, filename) :

	llfun, comparisons, skiptimesteps = objective
	comparisontolookat = comparisons[comparison_idx] 
	simname, simindexes, obsname, obsindexes = comparisontolookat
	
	percentiles = np.percentile(simulationresults, perc, axis=0)
	
	fig, ax = plt.subplots()
	
	start_date = dt.datetime.strptime(dataset.get_parameter_time('Start date', []),'%Y-%m-%d')
	timesteps = dataset.get_parameter_uint('Timesteps', [])
	date_idx = np.array(pd.date_range(start_date, periods=timesteps))
	
	ax.plot(date_idx, percentiles[1], color='red', label='median simulated')
	ax.fill_between(date_idx, percentiles[0], percentiles[2], color='red', alpha=0.3)
	
	obs = dataset.get_input_series(obsname, obsindexes, True)
	ax.plot(date_idx, obs, color = 'black', label = 'observed')
	
	ax.legend()
	
	fig.savefig(filename)
	
	
def simulation_of_median_parameters(dataset, samplelist, calibration, objective, comparison_idx) :
	median_sample = np.median(samplelist, axis=0)
	
	cf.set_values(dataset, median_sample, calibration)
	
	dataset.run_model()
	
	llfun, comparisons, skiptimesteps = objective
	comparisontolookat = comparisons[comparison_idx] 
	simname, simindexes, obsname, obsindexes = comparisontolookat
	
	obs = dataset.get_input_series(obsname, obsindexes, True)
	
	sim_med = dataset.get_result_series(simname, simindexes)
	
	err_raw = obs - sim_med
	
	M_med = median_sample[len(calibration) + comparison_idx]
	
	sigma_e = M_med*sim_med
	
	err_std = err_raw / sigma_e
	
	return median_sample, sim_med, err_std
	
	
wr.initialize('simplyp.dll')

dataset = wr.DataSet.setup_from_parameter_and_input_files('../../Applications/SimplyP/Tarland/TarlandParameters.dat', '../../Applications/SimplyP/Tarland/TarlandInputs.dat')


if __name__ == '__main__': #NOTE: This line is needed, or something goes horribly wrong with the paralellization

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

	labels_short = [r'$f_{quick}$', r'$\beta$', r'$T_g$', r'$a$', r'$b$', r'$T_s[A]$', r'$T_s[S]$', r'M']

	labels_long  = ['%s [%s] (%s)' % (cal[0], ', '.join(cal[1]), dataset.get_parameter_unit(cal[0])) 
						for cal in calibration]
	labels_long.append('M')

	initial_guess = cf.default_initial_guess(dataset, calibration)    #NOTE: This reads the initial guess that was provided by the parameter file.
	initial_guess.append(0.23)

	minval = [0.1 * x for x in initial_guess]
	maxval = [10.0 * x for x in initial_guess]

	cf.constrain_min_max(dataset, calibration, minval, maxval) #NOTE: Constrain to the min and max values recommended by the model in case we made our bounds too wide.

	skiptimesteps = 50   # Skip these many of the first timesteps in the objective evaluation

	comparisons = [
		('Reach flow (daily mean, cumecs)', ['Tarland1'], 'observed Q', []),
		#Put more here!
		]
		
	objective = (cf.log_likelyhood, comparisons, skiptimesteps)
	
	n_walk = 20
	n_steps = 200
	n_burn = 100
	
	n_random_samples = 100

	samp, lnprob = run_emcee(minval, maxval, initial_guess, calibration, objective, n_walk=n_walk, n_steps=n_steps)
	
	chain_plot(samp, calibration, labels_long, "simplyp_plots\\chains.png")
	
	samplelist, lnproblist = reshape_samples(samp, lnprob, n_burn)
	
	triangle_plot(samplelist, labels_short, "simplyp_plots\\triangle_plot.png")
	
	comparison_idx = 0 #TODO: Allow you to do multiple at the same time?
	
	param_only, overall = do_n_random_simulations_from_sample_list(dataset, samplelist, calibration, objective, n_random_samples, comparison_idx)
	
	best = best_sample(samplelist, lnproblist)
	#plot_simulations(dataset, best, param_only, calibration, objective, comparison_idx, "simplyp_plots\\random_samples.png")
	#plot_simulations(dataset, best, overall, calibration, objective, comparison_idx, "simplyp_plots\\random_samples2.png")
	
	perc = [2.5, 50, 97.5]
	plot_percentiles(dataset, overall, calibration, objective, comparison_idx, perc, "simplyp_plots\\percentiles.png")
	
	plot_percentiles(dataset, param_only, calibration, objective, comparison_idx, perc, "simplyp_plots\\percentiles2.png")
	
	median_sample, sim_med, err_std = simulation_of_median_parameters(dataset, samplelist, calibration, objective, comparison_idx)
	
	#TODO: This doesn't work because 1. there are nans in err_std (because of nans in the obs), and 2. the axes are wrong. Figure out what to do.
	err_std = [x for x in err_std if ~np.isnan(x)]
	fig,ax = plt.subplots()
	pd.tools.plotting.autocorrelation_plot(err_std)
	
	#print(err_std)
	
	plt.show()
	
	
