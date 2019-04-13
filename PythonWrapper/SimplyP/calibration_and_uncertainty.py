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

# Initialise wrapper
wrapper_fpath = (r"..\mobius.py")
wr = imp.load_source('mobius', wrapper_fpath)
wr.initialize('simplyp.dll')

def get_double_parameters_as_dataframe(dataset):
    """ Get all 'double' parameters declared in the model 'dataset' object and returns a dataframe
        summarising parameters names, units, prior ranges etc.
        
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
        'vary=True'. The provided dataframe should be (as subset of) the dataframe
        returned by get_double_parameters_as_dataframe().
        
        NOTE: At present, you need to manually add column named 'short_name' to 
        this dataframe, as it is not defined in the 'dataset' object itself.
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
    
    Args:
        params:      Obj. LMFit 'Parameters' object
        dataset:     Obj. Mobius 'dataset' object
        
    Returns:
        None. Parameter values in 'dataset' are changed.
    """
    for key in params.keys():
        name = params[key].user_data['name']
        index =  params[key].user_data['index']
        val = params[key].value
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
    
def log_likelihood(params, error_param_dict, dataset, comparisons, skip_timesteps=0):
    """
    """
    # Separate model params from error params
    model_params = lmfit.Parameters()
    error_params = lmfit.Parameters()
    
    for par in params.keys():
        if par.split('_')[0] == 'err':
            error_params.add(params[par])
        else:
            model_params.add(params[par]) 
    
    # Update parameters and run model
    dataset_copy = dataset.copy()
    set_parameter_values(model_params, dataset_copy)
    dataset_copy.run_model()
    
    ll_tot = 0
    
    for i, comparison in enumerate(comparisons):
        simname, simindexes, obsname, obsindexes = comparison
    
        sim = dataset_copy.get_result_series(simname, simindexes)
        obs = dataset_copy.get_input_series(obsname, obsindexes, alignwithresults=True)
        
        sim = sim[skip_timesteps:]
        obs = obs[skip_timesteps:]
        
        error_par = error_params[error_param_dict[obsname]].value
        sigma_e = error_par*sim
      
        ll = norm(sim, sigma_e).logpdf(obs)
        
        ll_tot += np.nansum(ll)
    
    dataset_copy.delete()
    
    #print(ll_tot)    
    
    return ll_tot

def run_mcmc(params, error_param_dict, dataset, comparisons, skip_timesteps=0, nworkers=8, 
             ntemps=1, nsteps=1000, nwalk=100, nburn=500, thin=5):
    """
    """
    # Check user input
    error_params = [i for i in params.keys() if i.split('_')[0]=='err']
    assert len(error_params) == len(comparisons), \
        'The number of error terms does not match the number of comparisons.' 
    
    for error_param in error_params:
        if np.isfinite(params[error_param].min):
            assert params[error_param].min > 0, \
                'Minimum bound for %s must be >0.' % error_param
        
    # Run MCMC
    mcmc = lmfit.Minimizer(log_likelihood, 
                           params, 
                           fcn_args=(error_param_dict, dataset, comparisons),
                           fcn_kws={'skip_timesteps':skip_timesteps},
                           nan_policy='omit',
                          )

    result = mcmc.emcee(params=params,
                        burn=nburn, 
                        steps=nsteps, 
                        nwalkers=nwalk, 
                        thin=thin, 
                        ntemps=ntemps,                 
                        workers=nworkers,                  
                        float_behavior='posterior',
                       )
    
    return (mcmc, result)

if __name__ == '__main__': 

    # Unpack options from pickled file
    with open('pickled\\mcmc_settings.pkl', 'rb') as handle:
        settings_dict = pickle.load(handle)
    
    params = settings_dict['params']
    error_param_dict = settings_dict['error_param_dict']
    dataset = settings_dict['dataset']
    comparisons = settings_dict['comparisons']
    skip_timesteps = settings_dict['skip_timesteps']
    nworkers = settings_dict['nworkers']
    ntemps = settings_dict['ntemps']
    nsteps = settings_dict['nsteps']
    nwalk = settings_dict['nwalk']
    nburn = settings_dict['nburn']
    thin = settings_dict['thin']
    result_path = settings_dict['result_path'] 
    chain_path = settings_dict['chain_path']
    corner_path = settings_dict['corner_path']
    
    # Perform sampling
    mcmc, result = run_mcmc(params, error_param_dict, dataset, comparisons, skip_timesteps=skip_timesteps, 
                            nworkers=nworkers, ntemps=ntemps, nsteps=nsteps, nwalk=nwalk, nburn=nburn, 
                            thin=thin)

    # Save results
    with open(result_path, 'wb') as output:
        pickle.dump([mcmc, result], output)
        
    # Plotting
    result.flatchain.plot(subplots=True, figsize=(15, 10), color='k', alpha=0.3)
    plt.savefig(chain_path, dpi=200)
    
    tri = corner.corner(result.flatchain,
                        labels=result.var_names,
                        quantiles=[0.025, 0.5, 0.975],
                        show_titles=True, 
                        title_args={'fontsize':20},
                        label_kwargs={'fontsize':18},
                        verbose=True,
                       )
    plt.savefig(corner_path, dpi=200)

    #GoF_stats_single_sample(samplelist, lnproblist, dataset, calibration, objective, n_ms)    
    
    