import matplotlib
import matplotlib.pyplot as plt
import seaborn as sn
import emcee
import corner
import imp
import pandas as pd 
import datetime as dt
import random
import pickle
import numpy as np
import time
from scipy.stats import norm
from multiprocessing import Pool

wrapper_fpath = (r"..\inca.py")
optimize_funs_fpath = (r'..\inca_calibration.py')

wr = imp.load_source('inca', wrapper_fpath)
cf = imp.load_source('inca_calibration', optimize_funs_fpath)	


def log_prior(params, min, max) :
    """
    params: list or array of parameter values
    min: list or array of minimum parameter values
    max: list or array of maximum parameter values
    """
    if cf.check_min_max(params, min, max):
        return 0
    return -np.inf

# IMPORTANT NOTE: If you want to run emcee multithreaded: The log_posterior function has to access the dataset 
# as a global. It can not take it as an argument. This is because ctypes of pointer type can not be 'pickled',
# which is what python does when it sends arguments to functions on separate threads. In that case, you will end 
# up with a garbage value for the pointer.
def log_posterior(params, min, max, calibration, objective, n_ms):
    """
    params: list or array of parameter values
    min: list or array of minimum parameter values
    max: list or array of maximum parameter values
    calibration: 'calibration' structure is a list of tuples, each tuple containing (param name string, [index string])
    objective: tuple (- function to use as measure of model performance,
                      - list containing simulated and observed variables you want to compare. 
                        Each element in the list is a 4-element tuple, with items (name of simulated result,
                        [index result applies to], name of observation input data, [index for observed data])
                      - skiptimesteps: integer, the number of timesteps to discard from the start of the model
                        run when comparing performance (i.e. warmup period)
    """
    log_pri = log_prior(params, min, max)
    
    llfun = objective[0]
    
    if(np.isfinite(log_pri)):
        log_like = llfun(params, dataset, calibration, objective, n_ms)
        return log_pri + log_like
    return -np.inf
    
def run_emcee(min, max, initial_guess, calibration, objective, n_walk, n_steps, n_ms):
    """
    """    
    n_dim = len(initial_guess)

    # Starting locations for walkers. Either:
    # 1. Start from small Gaussian 'ball' located near the MAP, or
    #starting_guesses = [initial_guess + 1e-4*np.random.randn(n_dim) for i in range(n_walk)]
    
    # 2. Start from random locations samples uniformly from prior
    starting_guesses = list(np.random.uniform(low=min, high=max, size=(n_walk, n_dim)))
    
    pool = Pool(8)
        
    sampler = emcee.EnsembleSampler(n_walk, 
                                    n_dim, 
                                    log_posterior, 
                                    #threads=8,
                                    pool=pool,
                                    args=[min, max, calibration, objective, n_ms])

    start = time.time()
    pos, prob, state = sampler.run_mcmc(starting_guesses, n_steps)
    end = time.time()
    print('Time elapsed running emcee: %.2f minutes' % ((end - start)/60))
    
    print('EMCEE average acceptance rate: %.2f' % np.mean(sampler.acceptance_fraction))

    samples = sampler.chain
    
    lnprobs = sampler.lnprobability
    
    pool.close()

    return samples, lnprobs

def chain_plot(samples, labels_long, filename):
    """ NOTE: This one needs non-reshaped samples
    """    
    n_dim = samples.shape[2]
    
    figHeight = max(30, len(labels_long)*3.5)

    fig, axes = plt.subplots(nrows=n_dim, ncols=1, figsize=(10, figHeight))    
    for idx, title in enumerate(labels_long):        
        axes[idx].plot(samples[:,:,idx].T, '-', color='k', alpha=0.3)
        axes[idx].set_title(title, fontsize=12) 
    plt.subplots_adjust(hspace=0.5)    
    
    fig.savefig(filename, dpi=300)
    #plt.close()
    
def reshape_samples(samples, lnprobs, n_burn):
    """
    """
    n_dim = samples.shape[2]
    s = samples[:, n_burn:, :].reshape((-1, n_dim))
    ln = lnprobs[:, n_burn:].reshape((-1))
    
    return s, ln
    
def best_sample(samplelist, lnproblist):
    """ NOTE Needs reshaped samples.
    """
    index_max = np.argmax(lnproblist)
    best_sample = samplelist[index_max]
    return best_sample	
    
def triangle_plot(samplelist, labels_short, filename, truths=None):
    """ NOTE Needs reshaped samples.
    """
    tri_fig = corner.corner(samplelist,
                            labels=labels_short,
                            truths=truths,
                            quantiles=[0.025, 0.5, 0.975],
                            show_titles=True, 
                            title_args={'fontsize': 28},
                            label_kwargs={'fontsize': 26})

    tri_fig.savefig(filename, dpi=300)
    #plt.close()
    
def plot_simulations(dataset, best, simulationresults, calibration, objective, comparison_idx, filename, n_ms):
    """ Not currently used?
    """
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
    
    cf.set_values(dataset, best, calibration, n_ms)
    dataset.run_model()
    
    bestsim = dataset.get_result_series(simname, simindexes)
    
    ax.plot(date_idx, bestsim, color = 'blue', label='best simulated')
    
    ax.legend()
    
    ax.set_xlabel('Date')
    ax.set_ylabel('%s $%s$' % (obsname, dataset.get_result_unit(simname)))
    
    fig.savefig(filename)

def simulation_of_median_parameters(dataset, samplelist, calibration, objective, comparison_idx, n_ms):
    """ Not currently used?
    """
    llfun, comparisons, skiptimesteps = objective
    comparisontolookat = comparisons[comparison_idx] 
    simname, simindexes, obsname, obsindexes = comparisontolookat

    median_sample = np.median(samplelist, axis=0)
    
    cf.set_values(dataset, median_sample, calibration, n_ms)
    
    dataset.run_model()
    
    obs = dataset.get_input_series(obsname, obsindexes, True)
    
    sim_med = dataset.get_result_series(simname, simindexes)
    
    err_raw = obs - sim_med
    
    M_med = median_sample[len(calibration)-n_ms+comparison_idx]
    
    sigma_e = M_med*sim_med
    
    err_std = err_raw / sigma_e
    
    return median_sample, sim_med, err_std
    
def GoF_stats_single_sample(samplelist, lnproblist, dataset, calibration, objective, n_ms):
    """
    """
    bestSample = best_sample(samplelist, lnproblist)
    cf.set_values(dataset, bestSample, calibration, n_ms)
    dataset.run_model()
    print('\nBest sample (max log likelihood):')
    cf.print_goodness_of_fit(dataset, objective)

#######################################################################

wr.initialize('simplyp.dll')

dataset = wr.DataSet.setup_from_parameter_and_input_files('../../Applications/SimplyP/Morsa/MorsaParameters.dat', 
                                                          '../../Applications/SimplyP/Morsa/MorsaInputs.dat')

if __name__ == '__main__': #NOTE: This line is needed, or something goes horribly wrong with the paralellization
    
    # Unpack options from pickled file
    with open('pickled\\mcmc_settings.pkl', 'rb') as handle:
        settings_dict = pickle.load(handle)
        
    comparisons = settings_dict['comparisons']
    skiptimesteps = settings_dict['skiptimesteps']
    calibration = settings_dict['calibration']
    n_ms = settings_dict['n_ms']
    minval = settings_dict['param_min']
    initial_guess = settings_dict['initial_guess']
    maxval = settings_dict['param_max']
    labels_short = settings_dict['labels_short']
    labels_long = settings_dict['labels_long']
    n_walk = settings_dict['n_walk']
    n_steps = settings_dict['n_steps']
    n_burn = settings_dict['n_burn']

    objective = (cf.log_likelyhood, comparisons, skiptimesteps)
    
    # Perform sampling
    samp, lnprob = run_emcee(minval, maxval, initial_guess, calibration, objective, 
                             n_walk=n_walk, n_steps=n_steps, n_ms=n_ms)

    # Save results
    emcee_result_li = (samp, lnprob)
    pickle_fpath = r'pickled\\emcee_results_morsa_v3.pkl'
    with open(pickle_fpath, 'wb') as output:
        pickle.dump(emcee_result_li, output)
        
    # Post-processing of results
    chain_plot(samp, labels_long, "simplyp_plots\\chains.png")
    # TO DO: chain plot of how log likelihood evolves for each chain (all chains on one plot)
    
    samplelist, lnproblist = reshape_samples(samp, lnprob, n_burn)
    
    triangle_plot(samplelist, labels_short, "simplyp_plots\\triangle_plot.png")

    GoF_stats_single_sample(samplelist, lnproblist, dataset, calibration, objective, n_ms)