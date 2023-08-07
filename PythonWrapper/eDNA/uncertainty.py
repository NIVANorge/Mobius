from joblib import Parallel, delayed
import numpy as np
from datetime import datetime
import os
from csv import writer
from csv import reader
import pandas as pd


def monte_carlo_sample(dataset, ranges, set_pars, get_stats, nsamples, verbose=1) :
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
	
	return np.array(fA)

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
		Returns:
			main_effect  - The main effect index for each parameter, as a dict
			total_effect - The total effect index for each parameter, as a dict
			distr        - A distribution of the statistic sampled uniformly over the parameter space.
	'''
	dim         = len(ranges)

	A = np.zeros((nsamples, dim))
	B = np.zeros((nsamples, dim))

	now = datetime.now()
	dt_string = now.strftime("%d_%m_%Y_%H_%M_%S")
	results_path = (r'./Starting_at_%s' % dt_string)
	os.mkdir(results_path)
#	with open(r'%s/Soil_BuriedOrNot.csv' % results_path, 'x') as csv:
#		pass
	#RiverFlow = [];
	eDNA = [];
	biomass = [];
	Stats = [];
	params = [];
	eDNA2 = [];
	biomass2 = [];
	Stats2 = [];
	params2 = [];
	eDNA3 = [];
	biomass3 = [];
	Stats3 = [];
	params3 = [];
	eDNA4 = [];
	biomass4 = [];
	Stats4 = [];
	params4 = [];
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
		ts_last = ds.get_last_timesteps()
		res = np.zeros(1156);
		bio_res = np.zeros(1156);
		res2 = np.zeros(1156);
		bio_res2 = np.zeros(1156);
		res3 = np.zeros(1156);
		bio_res3 = np.zeros(1156);
		res4 = np.zeros(1156);
		bio_res4 = np.zeros(1156);
		param = np.zeros(5);
		param2 = np.zeros(3);
		param3 = np.zeros(3);
		param4 = np.zeros(3);
		for i in range(1156) :
			res[i] = ds.get_result_series("eDNA concentration", ['oncgor', str(i)])[ts_last-1]
			bio_res[i] = ds.get_result_series("Fish biomass", ['oncgor', str(i)])[ts_last-1]
			res2[i] = ds.get_result_series("eDNA concentration", ['salalp', str(i)])[ts_last-1]
			bio_res2[i] = ds.get_result_series("Fish biomass", ['salalp', str(i)])[ts_last-1]
			res3[i] = ds.get_result_series("eDNA concentration", ['salsal', str(i)])[ts_last-1]
			bio_res3[i] = ds.get_result_series("Fish biomass", ['salsal', str(i)])[ts_last-1]
			res4[i] = ds.get_result_series("eDNA concentration", ['saltru', str(i)])[ts_last-1]
			bio_res4[i] = ds.get_result_series("Fish biomass", ['saltru', str(i)])[ts_last-1]

		
		param[0] = ds.get_parameter_double("Biomass background", ['oncgor'])
		param[1] = ds.get_parameter_double("Biomass at outlet", ['oncgor'])
		param[2] = ds.get_parameter_double("Biomass distribution linear coeff", ['oncgor'])
		param[3] = ds.get_parameter_double("eDNA shedding rate per fish biomass", [])
		param[4] = ds.get_parameter_double("eDNA decay rate", [])

		param2[0] = ds.get_parameter_double("Biomass background", ['salalp'])
		param2[1] = ds.get_parameter_double("Biomass at outlet", ['salalp'])
		param2[2] = ds.get_parameter_double("Biomass distribution linear coeff", ['salalp'])
		param3[0] = ds.get_parameter_double("Biomass background", ['salsal'])
		param3[1] = ds.get_parameter_double("Biomass at outlet", ['salsal'])
		param3[2] = ds.get_parameter_double("Biomass distribution linear coeff", ['salsal'])
		param4[0] = ds.get_parameter_double("Biomass background", ['saltru'])
		param4[1] = ds.get_parameter_double("Biomass at outlet", ['saltru'])
		param4[2] = ds.get_parameter_double("Biomass distribution linear coeff", ['saltru'])

		det = np.zeros(5);
		det2 = np.zeros(4);
		det3 = np.zeros(4);
		det4 = np.zeros(4);
		if 17.7 <= res[1155] <= 29.5:
			det[0] = det[0] + 1
		if 1.18 <= res[955] <= 8.18:
			det[0] = det[0] + 1
		if 1.70e-01 <= res[640] <= 3.12:
			det[0] = det[0] + 1       
		if 0 <= res[405] <= 7.12e-01:
			det[0] = det[0] + 1
		if 0 <= res[202] <= 5.39e-02:
			det[0] = det[0] + 1
		if 0 <= res[1] <= 5.02e-03:
			det[0] = det[0] + 1
		det[1] = ((res[1155]- 23.62846414)**2+(res[955]-4.680833597)**2+(res[640]-1.642588328)**2+(res[405]-0.263301157)**2+(res[202]-0.036923)**2+(res[1]-0.001709934)**2)**(1/2)
		det2[0] = ((res2[1155]- 0.000523492)**2+(res2[955]-0.000207198)**2+(res2[640]-0.000337655)**2+(res2[405]-0.000167338)**2+(res2[202]-0.003847995)**2+(res2[1]-0.000307665)**2)**(1/2)
		det3[0] = ((res3[1155]- 0.033336732)**2+(res3[955]-0.019870038)**2+(res3[640]-0.020235988)**2+(res3[405]-0.008144054)**2+(res3[202]-0.009435042)**2+(res3[1]-0.001248652)**2)**(1/2)
		det4[0] = ((res4[1155]- 0.02146758)**2+(res4[955]-0.009874342)**2+(res4[640]-0.008561764)**2+(res4[405]-0.00544136)**2+(res4[202]-0.009339731)**2+(res4[1]-0.004048413)**2)**(1/2)
		E = np.zeros(6)
		E2 = np.zeros(6)
		E3 = np.zeros(6)
		E4 = np.zeros(6)
		SSE = np.zeros(6)
		SSE2 = np.zeros(6)
		SSE3 = np.zeros(6)
		SSE4 = np.zeros(6)
		SSU = np.zeros(6)
		obs = np.array([23.62846414, 4.680833597,1.642588328,0.263301157,0.036923,0.001709934])
		obs2 = np.array([0.000523492,0.000207198,0.000337655,0.000167338,0.003847995,0.000307665])
		obs3 = np.array([0.033336732,0.019870038,0.020235988,0.008144054,0.009435042,0.001248652])
		obs4 = np.array([0.02146758,0.009874342,0.008561764,0.00544136,0.009339731,0.004048413])

		sim = res[[1155, 955, 640, 405, 202, 1],]
		sim2 = res2[[1155, 955, 640, 405, 202, 1],]
		sim3 = res3[[1155, 955, 640, 405, 202, 1],]
		sim4 = res4[[1155, 955, 640, 405, 202, 1],]
		E = obs - sim;
		SSE = np.sum(E**2);
		u = np.mean(obs);
		SSU = np.sum((obs - u)**2);
		corr = np.zeros(1)
		corr_matrix = np.corrcoef(obs, sim)
		corr = corr_matrix[0,1]
		E2 = obs2 - sim2;
		SSE2 = np.sum(E2**2);
		u2 = np.mean(obs2);
		SSU2 = np.sum((obs2 - u2)**2);
		corr2 = np.zeros(1)
		corr_matrix2 = np.corrcoef(obs2, sim2)
		corr2 = corr_matrix2[0,1]
		E3 = obs3 - sim3;
		SSE3 = np.sum(E3**2);
		u3 = np.mean(obs3);
		SSU3 = np.sum((obs3 - u3)**2);
		corr3 = np.zeros(1)
		corr_matrix3 = np.corrcoef(obs3, sim3)
		corr3 = corr_matrix3[0,1]
		E4 = obs4 - sim4;
		SSE4 = np.sum(E4**2);
		u4 = np.mean(obs4);
		SSU4 = np.sum((obs4 - u4)**2);
		corr4 = np.zeros(1)
		corr_matrix4 = np.corrcoef(obs4, sim4)
		corr4 = corr_matrix4[0,1]

		det[2] = 1 - SSE/SSU;
		det[3] = corr**2
		det[4] = SSE
		det2[1] = 1 - SSE2/SSU2;
		det2[2] = corr2**2
		det2[3] = SSE2
		det3[1] = 1 - SSE3/SSU3;
		det3[2] = corr3**2
		det3[3] = SSE3
		det4[1] = 1 - SSE4/SSU4;
		det4[2] = corr4**2
		det4[3] = SSE4

		params.append(param.T)
		Stats.append(det.T)
		eDNA.append(res.T)
		biomass.append(bio_res.T)

		params2.append(param2.T)
		Stats2.append(det2.T)
		eDNA2.append(res2.T)
		biomass2.append(bio_res2.T)

		params3.append(param3.T)
		Stats3.append(det3.T)
		eDNA3.append(res3.T)
		biomass3.append(bio_res3.T)

		params4.append(param4.T)
		Stats4.append(det4.T)
		eDNA4.append(res4.T)
		biomass4.append(bio_res4.T)

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

	with open(r'%s/eDNA_conc.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(eDNA).T,delimiter=',',fmt='%s', comments='')
	with open(r'%s/Biomass_conc.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(biomass).T,delimiter=',',fmt='%s', comments='')
	with open(r'%s/Stats_conc.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(Stats).T,delimiter=',',fmt='%s', comments='')
	with open(r'%s/Params.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(params).T,delimiter=',',fmt='%s', comments='')

	with open(r'%s/eDNA2_conc.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(eDNA2).T,delimiter=',',fmt='%s', comments='')
	with open(r'%s/Biomass2_conc.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(biomass2).T,delimiter=',',fmt='%s', comments='')
	with open(r'%s/Stats2_conc.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(Stats2).T,delimiter=',',fmt='%s', comments='')
	with open(r'%s/Params2.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(params2).T,delimiter=',',fmt='%s', comments='')

	with open(r'%s/eDNA3_conc.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(eDNA3).T,delimiter=',',fmt='%s', comments='')
	with open(r'%s/Biomass3_conc.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(biomass3).T,delimiter=',',fmt='%s', comments='')
	with open(r'%s/Stats3_conc.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(Stats3).T,delimiter=',',fmt='%s', comments='')
	with open(r'%s/Params3.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(params3).T,delimiter=',',fmt='%s', comments='')

	with open(r'%s/eDNA4_conc.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(eDNA4).T,delimiter=',',fmt='%s', comments='')
	with open(r'%s/Biomass4_conc.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(biomass4).T,delimiter=',',fmt='%s', comments='')
	with open(r'%s/Stats4_conc.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(Stats4).T,delimiter=',',fmt='%s', comments='')
	with open(r'%s/Params4.csv' % results_path, 'ab') as csvfile:
		np.savetxt(csvfile,pd.DataFrame(params4).T,delimiter=',',fmt='%s', comments='')
	return main_effect, total_effect, np.concatenate([fA,fB])