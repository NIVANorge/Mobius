import itertools
import pandas as pd
import numpy as np
import lmfit
import matplotlib
import matplotlib.pyplot as plt
import emcee
import corner
import imp
import datetime as dt
import random
import pickle
import time
from scipy.stats import norm
from multiprocessing import Pool

def get_double_parameters_as_dataframe(dataset):
    """ Get all 'double' parameters declared in the model 'dataset' object and return a dataframe
        summarising parameter names, units, prior ranges etc.
        
    Args:
        dataset: A Mobius 'dataset' object.
        
    Returns:
        Dataframe of model parameters.
    """
    # Get basic dataset properties
    index_sets = dataset.get_index_sets()
    pars = dataset.get_parameter_list()
    
    # Get 'double' params
    pars = [i[0] for i in pars if i[1]=='double']

    # Container for results
    par_dict = {'name':[],
                'unit':[],
                'index':[],
                'min_value':[],
                'initial_value':[],
                'max_value':[]}

    for par in pars:
        # Get par properties
        unit = dataset.get_parameter_unit(par)
        par_min, par_max = dataset.get_parameter_double_min_max(par)

        par_index_sets = dataset.get_parameter_index_sets(par)
        par_indexes = [dataset.get_indexes(i) for i in par_index_sets]

        for par_index in list(itertools.product(*par_indexes)):
            par_val = dataset.get_parameter_double(par, par_index)

            # Add to results
            par_dict['name'].append(par)
            par_dict['unit'].append(unit)
            par_dict['index'].append(par_index)
            par_dict['min_value'].append(par_min)
            par_dict['initial_value'].append(par_val)
            par_dict['max_value'].append(par_max)        

    return pd.DataFrame(par_dict)

def parameter_df_to_lmfit(df):
    """ Converts a dataframe of model parameters to an LMFit 'Parameters' object.
        All parameters are initialised with 'parameter_type=model_parameter' and 
        'vary=True'. The provided dataframe should be (a subset of) the dataframe
        returned by get_double_parameters_as_dataframe().
        
        NOTE: At present, you need to manually add a column named 'short_name' to 
        this dataframe, to succinctly identify each parameter (this is not 
        currently defined in the 'dataset' object itself).
    """
    params = lmfit.Parameters()

    for idx, row in df.iterrows():
        param = lmfit.Parameter(row['short_name'],
                                value=row['initial_value'],
                                min=row['min_value'],
                                max=row['max_value'],
                                vary=True,
                                user_data={'index':row['index'],
                                           'name':row['name'],
                                           'parameter_type':'model_parameter',
                                          }
                                )
        params.add(param)

    return params 
    
def set_parameter_values(params, dataset):
    """ Set the current parameter values in 'dataset' to those specified by 'params'.
        NOTE: Ignores error parameters with names beginning 'err_'
    
    Args:
        params:      Obj. LMFit 'Parameters' object
        dataset:     Obj. Mobius 'dataset' object
        
    Returns:
        None. Parameter values in 'dataset' are changed.
    """
    for key in params.keys():
        if key.split('_')[0] != 'err':
            name = params[key].user_data['name']
            index =  params[key].user_data['index']
            val = params[key].value
            #print('%s %s %s' % (name, index, val))
            dataset.set_parameter_double(name, index, val)
        
def calculate_residuals(params, dataset, comparisons, norm=False, skip_timesteps=0):
    """ Set the parameters of 'dataset' to 'params' and run the model. For each data
        series in 'comparisons', calculate the residual vector (model - simulated), 
        optionally normalised to the mean of the observations. Returns a single 1D
        vector of concatenated residuals.
        
    Args:
        params:         Obj. LMFit 'Parameters' object
        dataset:        Obj. Mobius 'dataset' object
        comparisons:    List. Datasets to be compared
        norm:           Bool. Whether to normalise residuals by dividing by the mean of 
                        the observations for each data series
        skip_timesteps: Int. Number of steps to skip before performing comparison
                     
    Returns:
        Array of residuals.
    """   
    # Update parameters and run model
    dataset_copy = dataset.copy()
    set_parameter_values(params, dataset_copy)
    dataset_copy.run_model()
    
    residuals = []
    
    for i, comparison in enumerate(comparisons):
        simname, simindexes, obsname, obsindexes = comparison
    
        sim = dataset_copy.get_result_series(simname, simindexes)
        obs = dataset_copy.get_input_series(obsname, obsindexes, alignwithresults=True)
        
        sim = sim[skip_timesteps:]
        obs = obs[skip_timesteps:]
        
        if norm:
            sim = sim/np.nanmean(obs)
            obs = obs/np.nanmean(obs)

        resid = sim - obs
        
        residuals.append(resid)

    dataset_copy.delete()
    
    #print(np.nansum(np.concatenate(residuals)**2))    
    
    return np.concatenate(residuals)

def minimize_residuals(params, dataset, comparisons, method='nelder', norm=False, 
                       skip_timesteps=0):
    """ Lest squares minimisation of the residuals. See
            
            https://lmfit.github.io/lmfit-py/fitting.html#
            
        for further details.
        
    Args:
        params:         Obj. LMFit 'Parameters' object
        dataset:        Obj. Mobius 'dataset' object
        comparisons:    List. Datasets to be compared
        method:         Str. Optimiser to use. See table here for options:
                            
                            https://lmfit-py.readthedocs.io/en/0.8.3/fitting.html#choosing-different-fitting-methods
                        
                        Note: Using "method='nelder'" corresponds to scipy.optimize.fmin()
        norm:           Bool. Whether to normalise residuals by dividing by the mean of 
                        the observations for each data series
        skip_timesteps: Int. Number of steps to skip before performing comparison
        
    Returns:
        Tuple (mi, res). The former is an LMFit minimizer and the latter the 'result' object.
    """
    # Check all params are 'model_parameters'
    for par in params.keys():
        assert params[par].user_data['parameter_type'] == 'model_parameter', \
               ("Parameter %s has type %s. All parameters must be of type 'model_parameter'."
               % (par, params[par].user_data['parameter_type']))
        
    # Create minimizer
    mi = lmfit.Minimizer(calculate_residuals, 
                         params, 
                         fcn_args=(dataset, comparisons),
                         fcn_kws={'norm':norm,
                                  'skip_timesteps':skip_timesteps,
                                 },
                         nan_policy='omit',                         
                        )
    
    # Run minimizer
    res = mi.minimize(method=method)
    
    return (mi, res)

def run_mcmc(log_like_fcn, params, error_param_dict, comparisons, nworkers=8, ntemps=1, nsteps=1000, 
             nwalk=100, nburn=500, thin=5, start='optimum', fcn_args=None, fcn_kws=None):
    """ Sample from the posterior using emcee. 
    
        NOTE: The code save the "raw" chains (i.e. no burning or thinning) for later use.
    
    Args:
        log_like_fcn:     Obj. Function returning total likelihood as a Float. The first argument must
                          accept an LMFit 'Parameters' object
        params:           Obj. LMFit 'Parameters' object
        error_param_dict: Dict. Maps observed series to error terms e.g.
                              
                              {'Observed Q':'err_q'}
                          
                          Error terms must be named 'err_XXX'
        comparisons:      List. Datasets to be compared
        skip_timesteps:   Int. Number of steps to skip before performing comparison
        nworkers:         Int. Number of processes to use for parallelisation
        ntemps:           Int. Number of temperature for parallel-tempering. Use 1
                          to run the standard 'ensemble sampler'
        nsteps:           Int. Number of steps per chain
        nwalk:            Int. Number of chains/walkers
        nburn:            Int. Number of steps to discrad from the start of each chain as 'burn-in'
        thin:             Int. Keep only every 'thin' steps
        start:            Str. Either 'optimum' or 'uniform'. If 'optimum', MCMC chains will be 
                          randomly initialised from within a small "Gaussian ball" in the vicinty of
                          the supplied parameter values; if 'uniform', the chains will start from 
                          random locations sampled uniformly from the prior parameter ranges
        fcn_args:         List. Additional positional arguments to pass to log_like_fcn
        fcn_kws:          Dict. Additional keyword arguments to pass to log_like_fcn
        
    Returns:
        LMFit emcee result object.
    """
    # Check user input
    assert start in ('optimum', 'uniform'), "'start' must be either 'optimum' or 'uniform'."
    
    error_params = [i for i in params.keys() if i.split('_')[0]=='err']  
    for error_param in error_params:
        if np.isfinite(params[error_param].min):
            assert params[error_param].min > 0, \
                'Minimum bound for %s must be >0.' % error_param

    # Set starting locations for chains
    varying = np.asarray([par.vary for par in params.values()])
    lower_bounds = np.asarray([i.min for i in params.values()])[varying]
    upper_bounds = np.asarray([i.max for i in params.values()])[varying]
    
    if (ntemps == 1) and (start == 'uniform'):
        starting_guesses = list(np.random.uniform(low=lower_bounds,
                                                  high=upper_bounds,
                                                  size=(nwalk, len(lower_bounds))))
    elif (ntemps > 1) and (start == 'uniform'):
        starting_guesses = list(np.random.uniform(low=lower_bounds,
                                                  high=upper_bounds,
                                                  size=(ntemps, nwalk, len(lower_bounds))))
    else:
        starting_guesses = None
    
    # Run MCMC
    start = time.time()

    mcmc = lmfit.Minimizer(log_like_fcn, 
                           params, 
                           fcn_args=fcn_args,
                           fcn_kws=fcn_kws,
                           nan_policy='omit',
                          )

    result = mcmc.emcee(params=params,
                        burn=nburn, 
                        steps=nsteps, 
                        nwalkers=nwalk, 
                        thin=thin, 
                        ntemps=ntemps,                 
                        workers=nworkers,  
                        pos=starting_guesses,
                        float_behavior='posterior',
                       )

    end = time.time()
    print('Time elapsed running emcee: %.2f minutes.\n' % ((end - start)/60))
    
    #print('EMCEE average acceptance rate: %.2f' % np.mean(sampler.acceptance_fraction))
    
    return result

def chain_plot(result, file_name=None):
    """ Plot 'raw' chains for each variable.
    
    Args:
        result:    LMFit emcee result object
        file_name: Raw str. Path for plot to be created
        
    Returns:
        None.
    """  
    chain = result.chain
    ndim = result.chain.shape[-1]
    labels = result.var_names
    
    # Take the zero'th PTsampler temperature for the parameter estimators
    if len(result.chain.shape) == 4:
        # Parallel tempering
        samples = result.chain[0, ...]
    else:
        samples = result.chain

    # Plot
    fig_height = max(30, len(labels)*3.5)

    fig, axes = plt.subplots(nrows=ndim, ncols=1, figsize=(10, fig_height))    
    for idx, label in enumerate(labels):        
        axes[idx].plot(samples[..., idx].T, '-', color='k', alpha=0.3)
        axes[idx].set_title(label, fontsize=12) 
    plt.subplots_adjust(hspace=0.5)   
    plt.tight_layout()
    
    if file_name:
        fig.savefig(file_name, dpi=300)

def triangle_plot(result, nburn, thin, file_name=None, truths=None):
    """ Triange/corner plot of MCMC results. Removes burn-in period and thins
        before plotting.
        
    Args:
        result:    LMFit emcee result object
        nburn:     Int. Number of steps to discard as burn-in
        thin:      Int. Keep only every 'thin' steps
        file_name: Raw str. Path for plot to be created
        truths:    Array-like or None. True values, if known 
    """ 
    chain = result.chain[..., nburn::thin, :]
    ndim = result.chain.shape[-1]
    labels = result.var_names
    
    # Take the zero'th PTsampler temperature for the parameter estimators
    if len(result.chain.shape) == 4:
        # Parallel tempering
        samples = chain[0, ...].reshape((-1, ndim))
    else:
        samples = chain.reshape((-1, ndim))
        
    corner.corner(samples,
                  labels=result.var_names,
                  quantiles=[0.025, 0.5, 0.975],
                  show_titles=True, 
                  truths=truths,
                  title_args={'fontsize':20},
                  label_kwargs={'fontsize':18},
                  verbose=True,
                 )
    
    if file_name:
        plt.savefig(file_name, dpi=200)
        
def plot_objective(dataset, comparisons, skip_timesteps=0, file_name=None):
    """ Plot the results the data series defined in 'comparisons' for a sinlge model run.
    
    Args:
        dataset:        Obj. Mobius 'dataset' object
        comparisons:    List. Datasets to be compared
        skip_timesteps: Int. Number of steps to skip before performing comparison
        file_name:      Raw str. Path for plot to be created
        
    Returns:
        None.
    """
    fig_height = max(30, len(comparisons)*3.5)

    fig, axes = plt.subplots(nrows=len(comparisons), ncols=1, figsize=(15, fig_height)) 
    
    for idx, comparison in enumerate(comparisons):
        
        simname, simindexes, obsname, obsindexes = comparison
        
        sim = dataset.get_result_series(simname, simindexes)
        obs = dataset.get_input_series(obsname, obsindexes, alignwithresults=True)
       
        start_date = dt.datetime.strptime(dataset.get_parameter_time('Start date', []),'%Y-%m-%d')
        timesteps = dataset.get_parameter_uint('Timesteps', [])
        date_idx = np.array(pd.date_range(start_date, periods=timesteps))
        
        sim = sim[skip_timesteps:]
        obs = obs[skip_timesteps:]
        date_idx = date_idx[skip_timesteps:]

        df = pd.DataFrame({'Date':date_idx,
                           '%s [%s]' % (obsname, ', '.join(obsindexes)):obs,
                           '%s [%s]' % (simname, ', '.join(simindexes)):sim,
                          })
        df.set_index('Date', inplace=True)

        unit = dataset.get_result_unit(simname) # Assumes that the unit is the same for obs and sim
		
        if len(comparisons) > 1:
            df.plot(ax=axes[idx], style=['o--', '-'])
            axes[idx].set_ylabel('$%s$' % unit)
        else :
            df.plot(ax=axes, style=['o--', '-'])
            axes.set_ylabel('$%s$' % unit)

    plt.tight_layout()
            
    if file_name:
        plt.savefig(filename, dpi=200)
    
def gof_stats_map(result, dataset, comparisons, skip_timesteps):
    """ Run the model with the 'best' (i.e. MAP) parameter set and print
        goodness-of-fit statistics.
    
    Args:
        result:         LMFit emcee result object
        dataset:        Obj. Mobius 'dataset' object
        comparisons:    List. Datasets to be compared
        skip_timesteps: Int. Number of steps to skip before performing comparison
        
    Returns:
        None. 
    """
    set_parameter_values(result.params, dataset)
    dataset.run_model()
    print('\nBest sample (max log likelihood):')
    print_goodness_of_fit(dataset, comparisons, skip_timesteps)

def print_goodness_of_fit(dataset, comparisons, skip_timesteps=0):
    """ Print various goodness-of-fit statistics for the datasets compared in
        'comparisons'.
        
    Args:
        dataset:        Obj. Mobius 'dataset' object
        comparisons:    List. Datasets to be compared
        skip_timesteps: Int. Number of steps to skip before performing comparison
        
    Returns:
        None.
    """
    for comparison in comparisons :
        simname, simindexes, obsname, obsindexes = comparison

        sim = dataset.get_result_series(simname, simindexes)
        obs = dataset.get_input_series(obsname, obsindexes, alignwithresults=True)

        sim = sim[skip_timesteps:]
        obs = obs[skip_timesteps:]
        
        residuals = sim - obs
        nonnan = np.count_nonzero(~np.isnan(residuals))
        
        bias = np.nansum(residuals) / nonnan
        meanabs = np.nansum(np.abs(residuals)) / nonnan
        sumsquare = np.nansum(np.square(residuals))
        meansquare = sumsquare / nonnan
        
        meanob = np.nansum(obs) / nonnan
        
        nashsutcliffe = 1 - sumsquare / np.nansum(np.square(obs - meanob))
        
        print('\nGoodness of fit for %s [%s] vs %s [%s]:' % (simname, ', '.join(simindexes), 
                                                             obsname, ', '.join(obsindexes)))
        print('Mean error (bias): %f' % bias)
        print('Mean absolute error: %f' % meanabs)
        print('Mean square error: %f' % meansquare)
        print('Nash-Sutcliffe coefficient: %f' % nashsutcliffe)
        print('Number of observations: %s\n' % nonnan)