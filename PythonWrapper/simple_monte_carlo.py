from joblib import Parallel, delayed
import numpy as np


#TODO: Allow for non-uniform distributions and result weights in this one. (Or result weights could be computed externally).

def monte_carlo_sample(dataset, ranges, set_pars, get_stats, nsamples, verbose=1) :
	'''
		Arguments:
			dataset  - A  Mobius DataSet object created using the Mobius python wrapper
			ranges   - A  dict {parname : (min, max)}, where parname is a (user defined) short name of a parameter and (min, max) is the range of this parameter
			set_pars - A  function (dataset, pars...), where dataset is a Mobius dataset, and pars is the parameters with the same name as in 'ranges'.
			           (Example: set_pars(ds, bfi, Tg), and ranges has the keys 'bfi' and 'Tg'.
			           This function must write the given parameters to the dataset in some user-defined way (there doesn't have to be a 1-1 correspondence between these
					   user defined parameters and the formal parameters in the Mobius model, so they could also e.g. be used to modify input series or set several different
					   formal parameters).
			get_statistic - A list of functions (dataset)->value, where dataset is a Mobius dataset. The functions have to compute a single value each.
			nsamples - How many times to sample the space for each statistic. The total number of model runs will be nsamples*(npars + 2)
			verbose  - Passed on to joblib.Parallel, about how it reports how many iterations are finished.
		Returns:
			distr     - A matrix with the first index corresponding to the monte carlo sample, and the second index to the statistic (result of the statistic function for that sample).
			pars      - A matrix with the parameters for the samples. First index is number of sample, second index is the number of the parameter.
	'''
	dim         = len(ranges)
	A = np.zeros((nsamples, dim))
	
	#TODO: allow specification of other priors here ?
	for i, par in enumerate(ranges) :
		A[:, i] = np.random.uniform(ranges[par][0], ranges[par][1], nsamples)
	
	def evaluate(pars) :
		pars2 = {}
		for i, par in enumerate(ranges) : pars2[par] = pars[i]
		ds = dataset.copy(copyresults=False, borrowinputs=True)
		set_pars(ds, **pars2)
		ds.run_model()
		stats = [get_s(ds) for get_s in get_stats]
		ds.delete()
		return stats

	fA = Parallel(n_jobs=-1, verbose=verbose, backend="threading")(map(delayed(evaluate), [A[j, :] for j in range(nsamples)]))
	
	return np.array(fA), A

def compute_effect_indexes(dataset, ranges, set_pars, get_statistic, nsamples=10000, verbose=1):
	'''
		Arguments:
			dataset  - A  Mobius DataSet object created using the Mobius python wrapper
			ranges   - A  dict {parname : (min, max)}, where parname is a (user defined) short name of a parameter and (min, max) is the range of this parameter
			set_pars - A  function (dataset, pars...), where dataset is a Mobius dataset, and pars is the parameters with the same name as in 'ranges'.
			           (Example: set_pars(ds, bfi, Tg), and ranges has the keys 'bfi' and 'Tg'.
			           This function must write the given parameters to the dataset in some user-defined way (there doesn't have to be a 1-1 correspondence between these
					   user defined parameters and the formal parameters in the Mobius model, so they could also e.g. be used to modify input series or set several different
					   formal parameters).
			get_statistic - A function (dataset)->value, where dataset is a Mobius dataset. The function has to compute a single value, which is the result statistic that you want to analyze
			           the distribution of.
			nsamples - How many times to sample the space for each statistic. The total number of model runs will be nsamples*(npars + 2)
			verbose  - Passed on to joblib.Parallel, about how it reports how many iterations are finished.
		Returns:
			main_effect  - The main effect index for each parameter, as a dict
			total_effect - The total effect index for each parameter, as a dict
			distr        - A distribution of the statistic sampled uniformly over the parameter space.
	'''
	dim         = len(ranges)

	A = np.zeros((nsamples, dim))
	B = np.zeros((nsamples, dim))

	for i, par in enumerate(ranges) :
		A[:, i] = np.random.uniform(ranges[par][0], ranges[par][1], nsamples)
		B[:, i] = np.random.uniform(ranges[par][0], ranges[par][1], nsamples)

	def evaluate(pars) :
		pars2 = {}
		for i, par in enumerate(ranges) : pars2[par] = pars[i]
		
		ds = dataset.copy(copyresults=False, borrowinputs=True)
		set_pars(ds, **pars2)
		ds.run_model()
		stat = get_statistic(ds)
		ds.delete()
		
		return stat

	fA = Parallel(n_jobs=-1, verbose=verbose, backend="threading")(map(delayed(evaluate), [A[j, :] for j in range(nsamples)]))
	fB = Parallel(n_jobs=-1, verbose=verbose, backend="threading")(map(delayed(evaluate), [B[j, :] for j in range(nsamples)]))

    #varfA = np.var(np.concatenate([fA,fB])))
	varfA = np.var(fA)

	main_effect = {}
	total_effect = {}

	for i, par in enumerate(ranges) :

		def fABij(j):
			pars = np.copy(A[j, :])
			pars[i] = B[j, i]
			return evaluate(pars)

		fABi = Parallel(n_jobs=-1, verbose=verbose, backend="threading")(map(delayed(fABij), range(nsamples)))
		fABi = np.array(fABi)

		main_effect[par] = np.sum([fB[j]*(fABi[j]-fA[j]) for j in range(nsamples)]) / (nsamples*varfA)
		total_effect[par] = np.sum([(fA[j] - fABi[j])**2 for j in range(nsamples)]) / (2.0*nsamples*varfA)
	return main_effect, total_effect, np.concatenate([fA,fB])