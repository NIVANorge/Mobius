

import matplotlib, matplotlib.pyplot as plt, seaborn as sn, emcee, corner, imp, pandas as pd, datetime as dt, random, pickle
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

	# Starting locations for walkers. Either:
	# 1. Start from small Gaussian 'ball' located near the MAP, or
	#starting_guesses = [initial_guess + 1e-4*np.random.randn(n_dim) for i in range(n_walk)]
	
	# 2. Start from random locations samples uniformly from prior
	starting_guesses = list(np.random.uniform(low=min, high=max, size=(n_walk, n_dim)))

	
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

def chain_plot(samples, labels_long, filename):
	#NOTE: This one needs non-reshaped samples
	
	n_dim = samples.shape[2]
	
	figHeight = max(30, len(labels_long)*3.5)

	fig, axes = plt.subplots(nrows=n_dim, ncols=1, figsize=(10, figHeight))    
	for idx, title in enumerate(labels_long):        
		axes[idx].plot(samples[:,:,idx].T, '-', color='k', alpha=0.3)
		axes[idx].set_title(title, fontsize=12) 
	plt.subplots_adjust(hspace=0.5)    
	
	fig.savefig(filename)
	plt.close()
	
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
	tri_fig = corner.corner(samplelist,
						labels=labels_short,
						#truths=truths,
						quantiles=[0.025, 0.5, 0.975],
						show_titles=True, 
						title_args={'fontsize': 28},
						label_kwargs={'fontsize': 26})

	tri_fig.savefig(filename)
	plt.close()
	
def do_n_random_simulations_from_sample_list(dataset, samplelist, calibration, objective, n_samples, comparison_idx) :

	llfun, comparisons, skiptimesteps = objective
	
	comparisontolookat = comparisons[comparison_idx] #TODO: Allow you to select multiple?
	simname, simindexes, obsname, obsindexes = comparisontolookat
	
	n_comparisons = len(comparisons)
	
	param_only    = []
	overall = []
	for it in range(1, n_samples) :       #TODO: Should be paralellized, really..
		
		random_index = random.randint(0, len(samplelist)-1)
		random_sample = samplelist[random_index]
		
		cf.set_values(dataset, random_sample, calibration, n_comparisons)
		dataset.run_model()
	
		sim = dataset.get_result_series(simname, simindexes)
	
		param_only.append(sim)
		
		if comparison_idx<3: # Q, SS, TP
			M = random_sample[len(calibration) + comparison_idx]
		else: # TDP, PP same as TP
			M = random_sample[-1]
		
		stoch = norm.rvs(loc=0, scale=M*sim, size=len(sim))
		
		perturbed = sim + stoch

		overall.append(perturbed)
	
	return param_only, overall

	
def plot_simulations(dataset, best, simulationresults, calibration, objective, comparison_idx, filename) :

	llfun, comparisons, skiptimesteps = objective
	
	n_comparisons = len(comparisons)
	
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
	
	cf.set_values(dataset, best, calibration, n_comparisons)
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
	
	# If lots of missing data in obs (i.e. not daily), plot as scatter too
	if len(obs[~np.isnan(obs)])< 0.5*len(percentiles[1]):
		marker='o'
		ax.plot(date_idx, obs, color = 'black', marker='o', ms=3, label = 'observed')
		ax.plot(date_idx, obs, color = 'black')
	
	else:
		ax.plot(date_idx, obs, color = 'black', label = 'observed')
	
	plt.ylim(bottom=0) # Stochastic error may cause negative flows; mask for now (N.B. need to revisit)
	
	ax.legend()

	plt.setp(ax.get_xticklabels(), visible=True, rotation=30, ha='right')
	
	plt.savefig(filename)
	plt.close()
	
	
def simulation_of_median_parameters(dataset, samplelist, calibration, objective, comparison_idx) :

	llfun, comparisons, skiptimesteps = objective
	comparisontolookat = comparisons[comparison_idx] 
	simname, simindexes, obsname, obsindexes = comparisontolookat
	n_comparisons = len(comparisons)

	median_sample = np.median(samplelist, axis=0)
	
	cf.set_values(dataset, median_sample, calibration, n_comparisons)
	
	dataset.run_model()
	
	obs = dataset.get_input_series(obsname, obsindexes, True)
	
	sim_med = dataset.get_result_series(simname, simindexes)
	
	err_raw = obs - sim_med
	
	#M_med = median_sample[len(calibration) + comparison_idx]
	M_med = median_sample[len(calibration) + comparison_idx]
	
	sigma_e = M_med*sim_med
	
	err_std = err_raw / sigma_e
	
	return median_sample, sim_med, err_std
	
def GoF_stats_single_sample(samplelist, lnproblist, dataset, calibration, objective):
	bestSample = best_sample(samplelist, lnproblist)
	cf.set_values(dataset, bestSample, calibration)
	dataset.run_model()
	print('\nBest sample (max log likelihood):')
	cf.print_goodness_of_fit(dataset, objective)

#######################################################################

wr.initialize('simplyp.dll')

dataset = wr.DataSet.setup_from_parameter_and_input_files('../../Applications/SimplyP/Morsa/MorsaParameters.dat', '../../Applications/SimplyP/Morsa/MorsaInputs.dat')


if __name__ == '__main__': #NOTE: This line is needed, or something goes horribly wrong with the paralellization

	# List of simulated and observed variables to include in likelihood
	# comparisons = [('Reach flow (daily mean, cumecs)', ['Kure'],
                 # 'Observed Q', []),
                 # ('Reach TP concentration', ['Kure'],
                 # 'Observed TP at Kure', [])]
				 
	comparisons = [
					('Reach flow (daily mean, cumecs)', ['Kure'], 'Observed Q', []),
					('Reach suspended sediment concentration', ['Kure'], 'Observed SS at Kure', []),
					('Reach TP concentration', ['Kure'], 'Observed TP at Kure', []),
					('Reach TDP concentration', ['Kure'], 'Observed TDP at Kure', []),
					('Reach PP concentration', ['Kure'], 'Observed PP at Kure', []),            
				]
				   
	n_comparisons = len(comparisons)

	# Read in csv with parameters to vary, short names and param ranges
	#fpath = fpath = r'C:\Data\GitHub\INCABuilder\PythonWrapper\SimplyP\SimplyP_calParams_ranges_morsa_v3.csv'
	fpath = fpath = r'SimplyP_calParams_ranges_morsa_v3.csv'
	
	param_df = pd.read_csv(fpath)
	
	# Make calibration variable (list of (param, index) tuples to calibrate)
	calibration = [
		('Degree-day factor for snowmelt', []),
		('Proportion of precipitation that contributes to quick flow', []),
		('PET multiplication factor', []),
		('Baseflow index', []),
		('Groundwater time constant', []),
		('Soil water time constant', ['Semi-natural']),
		('Reach sediment input scaling factor', []),
		('Initial soil water TDP concentration and EPC0', ['Arable']),
		('Groundwater TDP concentration', []),
		('Particulate P enrichment factor', []),
		('Reach effluent TDP inputs', ['Kure']),
	]
	

	initial_guess = cf.default_initial_guess(dataset, calibration)
	#NOTE: This reads the initial guess that was provided by the parameter file, excluding any error term parameters
	# Initial guess for the residual error term(s)
	for i in range(0,3):
		initial_guess.append(0.5) #3 terms, 
	
	# Read max & min values from file
	minval = list(param_df['Min'].dropna().values)
	maxval = list(param_df['Max'].dropna().values)

	cf.constrain_min_max(dataset, calibration, minval, maxval) #NOTE: Constrain to the min and max values recommended by the model in case we made our bounds too wide.
	
	# Labels
	labels_short = param_df['ShortName']
	labels_long  = ['%s, %s' % (cal[0], cal[1]) 
						for cal in calibration]
	labels_long.append('M_Q')
	labels_long.append('M_SS')
	labels_long.append('M_P')
		
	# Objective
	skiptimesteps = 30   # Skip this many first timesteps in the objective evaluation
	objective = (cf.log_likelyhood, comparisons, skiptimesteps)
	
	###########################################################
	# emcee setup params and run
	n_walk = 100
	n_steps = 10000
	n_burn = 5000
	
	# n_walk = 36
	# n_steps = 10
	# n_burn = 0

	samp, lnprob = run_emcee(minval, maxval, initial_guess, calibration, objective, n_walk=n_walk, n_steps=n_steps)
	
	
	# ##########################################################
	# Post-processing of results

	chain_plot(samp, labels_long, "simplyp_plots\\chains.png")
	# TO DO: chain plot of how log likelihood evolves for each chain (all chains on one plot)
	
	samplelist, lnproblist = reshape_samples(samp, lnprob, n_burn)
	
	triangle_plot(samplelist, labels_short, "simplyp_plots\\triangle_plot.png")

	n_random_samples = 100
	#varlist = ['Q','TP']
	varlist = ['Q','SS','TP','TDP','PP']
	
	GoF_stats_single_sample(samplelist, lnproblist, dataset, calibration, objective, n_comparisons)
	
	for comparison_idx in range(0, n_comparisons):
		
		print('Starting random simulations for %s' % varlist[comparison_idx])
		
		#NOTE: this function customized to tie Ms for P vars!
		param_only, overall = do_n_random_simulations_from_sample_list(dataset, samplelist, calibration, objective, n_random_samples, comparison_idx)
		
		print('Finished random simulations for %s' % varlist[comparison_idx])
		
		#best = best_sample(samplelist, lnproblist)
		#plot_simulations(dataset, best, param_only, calibration, objective, comparison_idx, "simplyp_plots\\random_samples.png")
		#plot_simulations(dataset, best, overall, calibration, objective, comparison_idx, "simplyp_plots\\random_samples2.png")
		
		perc = [2.5, 50, 97.5]
		plot_percentiles(dataset, overall, calibration, objective, comparison_idx, perc,
		"simplyp_plots\\percentiles_fullUncertainty_%s.png" % varlist[comparison_idx])
		
		plot_percentiles(dataset, param_only, calibration, objective, comparison_idx, perc,
		"simplyp_plots\\percentiles_paramUncertainty_%s.png" % varlist[comparison_idx])
		
		#median_sample, sim_med, err_std = simulation_of_median_parameters(dataset, samplelist, calibration, objective, comparison_idx)
	
		#TODO: This doesn't work because 1. there are nans in err_std (because of nans in the obs), and 2. the axes are wrong. Figure out what to do.
		#pd.tools.plotting.autocorrelation_plot(err_std)
		#print(err_std)

	emcee_result_li = [samp, lnprob]
	
	pickle_fpath = r'C:\Data\GitHub\INCABuilder\PythonWrapper\SimplyP\pickled\emcee_results_Morsa_v3.pkl'
	with open(pickle_fpath, 'wb') as output:
		pickle.dump(emcee_result_li, output)
	
