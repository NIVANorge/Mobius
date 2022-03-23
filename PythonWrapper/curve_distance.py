import numpy as np
import matplotlib.pyplot as plt

def vertical_distance(obs, sim) :
	assert len(obs) == len(sim)
	resid = sim - obs
	sumabs = np.nansum(np.abs(resid))
	sumsq  = np.nansum(np.multiply(resid, resid))
	return sumabs, sumsq

def curve_distance(obs, sim, t_scale=1.0, maxshift=10, plotit=False) :
	
	assert len(obs) == len(sim)
	
	t_scale2 = t_scale * np.nanmean(obs)
	
	sumabsdist = 0.0
	sumsqdist  = 0.0
	
	matches_x = np.full(len(sim), np.nan)
	matches_y = np.full(len(sim), np.nan)
	
	for idx in range(len(sim)) :
		ob = obs[idx]
		if np.isfinite(ob) :
			mindist = np.inf
			pt = np.array([t_scale2*idx, ob])
			
			searchidx_min = max(0, idx-maxshift)
			searchidx_max = min(idx+maxshift+1, len(sim))
			for idx2 in range(searchidx_min, searchidx_max-1) :
				p0 = np.array([idx2*t_scale2, sim[idx2]])
				p1 = np.array([(idx2+1)*t_scale2, sim[idx2+1]])
				len_seg_sq = np.dot(p1-p0, p1-p0)
				
				assert len_seg_sq > 0.0
				t = max(0, min(1, np.dot(pt-p0, p1-p0)/len_seg_sq))
				proj = p0 + t*(p1-p0)
				dist = np.sqrt(np.dot(pt-proj, pt-proj))
				
				if dist < mindist :
					mindist = dist
					
					matches_x[idx] = proj[0]/t_scale2
					matches_y[idx] = proj[1]
				
			sumabsdist += mindist
			sumsqdist  += mindist*mindist
	
	if plotit:
		fig,ax = plt.subplots(1,1)
		fig.set_size_inches(20, 10)
		ax.plot(obs, linestyle='--', marker='o')
		ax.plot(sim)
		for idx in range(len(sim)) :
			if np.isfinite(obs[idx]):
				ax.plot([idx, matches_x[idx]], [obs[idx], matches_y[idx]], color='grey')
	
	return sumabsdist, sumsqdist