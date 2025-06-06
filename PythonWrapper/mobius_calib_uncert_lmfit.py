import itertools
import pandas as pd
import numpy as np
import lmfit
import matplotlib
import matplotlib.pyplot as plt
import emcee
import corner
#import imp
import datetime as dt
import random
import pickle
import time
import networkx as nx
from scipy.stats import norm
from multiprocessing import Pool

def reaches_from_adjacency_table(excel_path, sheet_name='Sheet1', reach_index_set='Reaches'):
    """ Generate text to define the 'Reaches' index_set in a Mobius parameter file from
        an adjacency table.
        
        The syntax to define reach structures in Mobius is concise, but can be confusing for
        complex river networks. This function takes an "adjacency table" defining the reach
        structure and converts it to Mobius syntax, which can be copied into a Mobius
        parameter file.
        
    Args:
        excel_path:      Raw str. Path to Excel adjacency table. Must contain two columns:
                  
                             ['reach_id', 'next_down_id']
                             
                         Each reach should appear only ONCE in the first column. The second 
                         column should contain the reach_id for the next reach DOWNSTREAM. 
                         Each reach must flow into ONE (and only one) downstream reach
        sheet_name:      Str. Name of Excel sheet containing the adjacency table
        reach_index_set: Str. The name of the index_set for reaches
        
    Returns:
        Str. Reaches defined using Mobius syntax.
    """
    df = pd.read_excel(excel_path, sheet_name=sheet_name)
    
    assert list(df.columns) == ['reach_id', 'next_down_id'], "Column headings must be ['reach_id', 'next_down_id']."
    
    data = []
    
    for idx, row in df.iterrows():
        reach = row['reach_id']

        upstream_reaches = df.query('next_down_id == @reach')['reach_id'].values

        if len(upstream_reaches) == 0:
            data.append('"%s"' % reach)

        else:
            # Build Mobius string for this reach
            reach_str = ['"%s"' % i for i in upstream_reaches]
            reach_str = ['"%s"' % reach] + reach_str
            reach_str = ' '.join(reach_str)
            reach_str = '{%s}' % reach_str
            data.append(reach_str)

    # Build final string
    total_str = ' '.join(data)
    total_str = '"%s" : {%s}' % (reach_index_set, total_str)
    
    print(total_str)   
    
    return total_str 
    
def plot_reach_structure(dataset, reach_index_set='Reaches'):
    """ Display the model's reach/river network as a directed graph.
    
    Args:
        dataset:         Obj. Mobius dataset object
        reach_index_set: Str. The name of the index_set representing river reaches
                         
    Returns:
        NetworkX graph. Can be displayed/saved using 
            
            from nxpd import draw
            draw(g, show='ipynb', filename=None)
    """  
    assert reach_index_set in dataset.get_index_sets(), "The specified 'reach_index_set' is not recognised."
    
    reaches = dataset.get_indexes(reach_index_set)

    g = nx.DiGraph()

    # Add reaches as nodes
    for reach in reaches:
        g.add_node(reach, label=reach)

    # Add edges linking reaches
    for reach in reaches:
        # Get upstream reaches
        upstream_reaches = dataset.get_branch_inputs(reach_index_set, reach)

        for upstream_reach in upstream_reaches:       
            g.add_edge(upstream_reach, reach)

    return g
    
def get_double_parameters_as_dataframe(dataset, use_model_short_names=True, index_short_name={}):
    """ Get all 'double' parameters declared in the model 'dataset' object and return a dataframe
        summarising parameter names, units, prior ranges etc.
        
    Args:
        dataset: A Mobius 'dataset' object.
        use_model_short_names : bool (default:True). Set to true if parameter short names should be generated based on data registered in the model (if possible).
        index_short_name : Dictionary. Used to determine short names of indexes. For instance index_short_dict={'Forest' : 'F', 'Reach0' : 'r0'} etc. Generated short names will have the form parname_idx0_idx1 etc., where parname is a short name of the parameter, and idx0, idx1 etc. are short names of the associated indexes.
        
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
                'short_name':[],
                'unit':[],
                'index':[],
                'min_value':[],
                'initial_value':[],
                'max_value':[]}

    for par in pars:
        # Get par properties
        unit = dataset.get_parameter_unit(par)
        par_min, par_max = dataset.get_parameter_double_min_max(par)
        short_name0 = dataset.get_parameter_short_name(par)

        par_index_sets = dataset.get_parameter_index_sets(par)
        par_indexes = [dataset.get_indexes(i) for i in par_index_sets]

        for par_index in list(itertools.product(*par_indexes)):
            par_val = dataset.get_parameter_double(par, par_index)
            
            if use_model_short_names and len(short_name0) > 0:
                short_name = short_name0
                if len(par_index) > 0 :
                    short_name += '_' + '_'.join([(index_short_name[idx] if idx in index_short_name else idx) for idx in par_index])
            else:
                short_name = ''
            
            # Add to results
            par_dict['name'].append(par)
            par_dict['unit'].append(unit)
            par_dict['index'].append(par_index)
            par_dict['min_value'].append(par_min)
            par_dict['initial_value'].append(par_val)
            par_dict['max_value'].append(par_max)			
            par_dict['short_name'].append(short_name)

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

        # For gradient-based optimisers to work, the initial values should not be 
        # exactly equal to either min or max (the algorithm needs to be able to 
        # "explore" around the current value to estimate the gradient).
        # Warn user if (value == min) or (value == max)
        #if ((row['initial_value'] == row['min_value']) or 
        #    (row['initial_value'] == row['max_value'])):
            #print("WARNING: Parameter '%s' was initialised at the limit of its range. "
            #      "Gradient-based optimisers find this difficult.\n"
            #      "         Consider setting (min < value < max) instead?" % row['short_name'])
			#NOTE: I found the above error too annoying when I do large batch runs -MDN 01.06.2021
	
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
    
def set_parameter_values(params, dataset, use_stat=None):
    """ Set the current parameter values in 'dataset' to those specified by 'params'.
        NOTE: Ignores error parameters with names beginning 'err_' or 'aux_'
    
    Args:
        params:      Obj. LMFit 'Parameters' object
        dataset:     Obj. Mobius 'dataset' object
        use_stat:    Str. (None, 'median' or 'map'). Only relevant if 'params' is part 
                     of an MCMC result object; otherwise pass None. Specifies whether to
                     update the model to use the median or the MAP estimate from the 
                     posterior
        
    Returns:
        None. Parameter values in 'dataset' are changed.
    """
    assert use_stat in (None, 'median', 'map'), "'use_stat' must be None, 'median' or 'map'."
    
    for key in params.keys():
        if key.split('_')[0] != 'err' and key.split('_')[0] != 'aux':
            name = params[key].user_data['name']
            index =  params[key].user_data['index']
            
            if use_stat:
                val = params[key].user_data[use_stat]                
            else:
                val = params[key].value
            #print('%s %s %s' % (name, index, val))
            dataset.set_parameter_double(name, index, val)


def get_date_index(dataset) :
    """ Get a vector of datetimes that matches the time steps in the dataset.
	"""
    start_date = dataset.get_parameter_time('Start date', [])
    timesteps  = dataset.get_next_timesteps()
	
    (type, magnitude) = dataset.get_timestep_size()
	
    return pd.date_range(start=start_date, periods=timesteps, freq='%d%s%s' % (magnitude, type, 'S' if type=='M' else ''))

def get_input_date_index(dataset) :
    start_date = dataset.get_input_start_date()
    timesteps  = dataset.get_input_timesteps()
	
    (type, magnitude) = dataset.get_timestep_size()
	
    return pd.date_range(start=start_date, periods=timesteps, freq='%d%s%s' % (magnitude, type, 'S' if type=='M' else ''))


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
        simname, simindexes, obsname, obsindexes, *_ = comparison
    
        sim = dataset_copy.get_result_series(simname, simindexes)
        obs = dataset_copy.get_input_series(obsname, obsindexes, alignwithresults=True)
        
        sim = sim[skip_timesteps:]
        obs = obs[skip_timesteps:]
		
        if norm:
            sim = sim/np.nanmean(obs)
            obs = obs/np.nanmean(obs)
		
		
        if np.isnan(sim).any() :
            raise ValueError('Got a NaN in the simulated data')
		

        resid = sim - obs
        
        residuals.append(resid)

    dataset_copy.delete()
    
    #print(np.nansum(np.concatenate(residuals)**2))    
    
    return np.concatenate(residuals)

def minimize_residuals(params, dataset, comparisons, residual_method=calculate_residuals, iter_cb=None, method='nelder', norm=False, 
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
    mi = lmfit.Minimizer(residual_method, 
                         params, 
                         fcn_args=(dataset, comparisons),
                         fcn_kws={'norm':norm,
                                  'skip_timesteps':skip_timesteps,
                                 },
                         nan_policy='omit',
						 iter_cb=iter_cb,
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

def update_mcmc_results(result, nburn, thin):
    """ The summary statistics contained in the LMFit result object do not account for
        the burn-in period. This function discards the burn-in, thins and then re-calculates
        parameter medians and standard errors. The 'user_data' attribute for each parameter
        is then updated to include two new values: 'median' and 'map'. These can be passed to
        set_parameter_values() via the 'use_stat' kwarg.        
        
    Args:
        result: Obj. LMFit result object with method='emcee'
        nburn:  Int. Number of steps to discrad from the start of each chain as 'burn-in'
        thin:   Int. Keep only every 'thin' steps
        
    Returns:
        Updated LMFit result object.    
    """
    # Discard the burn samples and thin
    chain = result.chain[..., nburn::thin, :]
    ndim = result.chain.shape[-1]
	
    # Take the zero'th PTsampler temperature for the parameter estimators
    if len(result.chain.shape) == 4:
        # Parallel tempering
        flatchain = chain[0, ...].reshape((-1, ndim))
    else:
        flatchain = chain.reshape((-1, ndim))

    # 1-sigma quantile, estimated as half the difference between the 15 and 84 percentiles
    quantiles = np.percentile(flatchain, [15.87, 50, 84.13], axis=0)

    for i, var_name in enumerate(result.var_names):
        std_l, median, std_u = quantiles[:, i]
        result.params[var_name].value = median
        result.params[var_name].stderr = 0.5 * (std_u - std_l)
        result.params[var_name].correl = {}

    result.params.update_constraints()

    # Work out correlation coefficients
    corrcoefs = np.corrcoef(flatchain.T)

    for i, var_name in enumerate(result.var_names):
        for j, var_name2 in enumerate(result.var_names):
            if i != j:
                result.params[var_name].correl[var_name2] = corrcoefs[i, j]

    # Add both the median and the MAP as additonal 'user_data' pars in the 'result' object
    lnprob = result.lnprob[..., nburn::thin]
    highest_prob = np.argmax(lnprob)
    hp_loc = np.unravel_index(highest_prob, lnprob.shape)
    map_soln = chain[hp_loc]
    
    for i, var_name in enumerate(result.var_names):
        std_l, median, std_u = quantiles[:, i]
        result.params[var_name].user_data['median'] = median
        result.params[var_name].user_data['map'] = map_soln[i]
        
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


def combine_name(name, indexes) :
	return '%s [%s]' % (name, ', '.join(indexes))

def get_input_dataframe(dataset, list, alignwithresults=False) :
    """ Get a dataframe with a date column together with model input columnts
	
    Args:
        dataset:       Obj. Mobius 'dataset' object
        list:          List of pairs (name, indexes) where 'name' is the name of an input timeseries and indexes are associated indexes
        alignwithresults: Bool. Whether the time series should be extracted starting at the model run start date or the input start date.
    
    Returns : A pandas dataframe
    """

    if alignwithresults:
        start_date = dataset.get_parameter_time('Start date', [])
        timesteps  = dataset.get_next_timesteps()
    else :
        start_date = dataset.get_input_start_date()
        timesteps  = dataset.get_input_timesteps()
	
    (type, magnitude) = dataset.get_timestep_size()
	
    dates = np.array(pd.date_range(start=start_date, periods=timesteps, freq='%d%s%s' % (magnitude, type, 'S' if type=='M' else '')))
	
    df = pd.DataFrame({'Date' : dates})
	
    for name, indexes in list :
        series = dataset.get_input_series(name, indexes, alignwithresults)
        full_name = combine_name(name, indexes)
        df[full_name] = series
		
    df.set_index('Date', inplace=True)
	
    return df
	
def get_result_dataframe(dataset, list) :
    """ Get a dataframe with a date column together with model result columnts
	
	Args:
		dataset:       Obj. Mobius 'dataset' object
		list:          List of pairs (name, indexes) where 'name' is the name of a result timeseries and indexes are associated indexes
    
	Returns : A pandas dataframe
	"""

    dates = get_date_index(dataset)
	
    df = pd.DataFrame({'Date' : dates})
	
    for name, indexes in list :
        series = dataset.get_result_series(name, indexes)
        full_name = combine_name(name, indexes)
        df[full_name] = series

    df.set_index('Date', inplace=True)
	
    return df

       
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
    fig_height = min(30, len(comparisons)*3.5)

    fig, axes = plt.subplots(nrows=len(comparisons), ncols=1, figsize=(15, fig_height)) 
    
    for idx, comparison in enumerate(comparisons):
        
        simname, simindexes, obsname, obsindexes, *_ = comparison
        
        sim_df = get_result_dataframe(dataset, [(simname, simindexes)])
        obs_df = get_input_dataframe(dataset, [(obsname, obsindexes)], alignwithresults=True)
		
        df = pd.concat([obs_df, sim_df], axis=1)

        unit = dataset.get_result_unit(simname) # Assumes that the unit is the same for obs and sim
		
        if len(comparisons) > 1:
            df.plot(ax=axes[idx], style=['o--', '-'])
            #axes[idx].set_ylabel('$%s$' % unit)   #NOTE: The $ causes a problem when the unit is %
            axes[idx].set_ylabel('%s' % unit)
        else :
            df.plot(ax=axes, style=['o--', '-'])
            #axes.set_ylabel('$%s$' % unit)    #NOTE: The $ causes a problem when the unit is %
            axes.set_ylabel('%s' % unit)   

    plt.tight_layout()
            
    if file_name:
        plt.savefig(filename, dpi=200)
	
    return axes
    
def gof_stats(result, dataset, comparisons, skip_timesteps, use_stat='map'):
    """ Run the model with the 'best' (i.e. MAP or median) parameter set and print
        goodness-of-fit statistics.
    
    Args:
        result:         LMFit emcee result object
        dataset:        Obj. Mobius 'dataset' object
        comparisons:    List. Datasets to be compared
        skip_timesteps: Int. Number of steps to skip before performing comparison
        use_stat:       Str. 'map' or 'median'
        
    Returns:
        None. 
    """
    set_parameter_values(result.params, dataset, use_stat=use_stat)
    dataset.run_model()
    print('\nBest sample (%s):' % use_stat)
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
        simname, simindexes, obsname, obsindexes, *_ = comparison

        sim = dataset.get_result_series(simname, simindexes)
        obs = dataset.get_input_series(obsname, obsindexes, alignwithresults=True)

        sim = sim[skip_timesteps:]
        obs = obs[skip_timesteps:]
        
        residuals = sim - obs
        nonnan = np.count_nonzero(~np.isnan(residuals))
        
        bias = np.nansum(residuals) / nonnan
        meanabs = np.nansum(np.abs(residuals)) / nonnan
        sumsquare = np.nansum(np.square(residuals))
        rootmeansquare = np.sqrt(sumsquare / nonnan)
        
        meanob = np.nansum(obs) / nonnan
        
        nashsutcliffe = 1 - sumsquare / np.nansum(np.square(obs - meanob))
        
        print('\nGoodness of fit for %s [%s] vs %s [%s]:' % (simname, ', '.join(simindexes), 
                                                             obsname, ', '.join(obsindexes)))
        print('Mean error (bias): %f' % bias)
        print('Mean absolute error: %f' % meanabs)
        print('Root mean square error: %f' % rootmeansquare)
        print('Nash-Sutcliffe coefficient: %f' % nashsutcliffe)
        print('Number of observations: %s\n' % nonnan)