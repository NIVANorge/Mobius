import imp
import numpy as np
import scipy.integrate
import time


def convert_m3_per_second_to_mm_per_day(m3_per_second, catchment_area) :
	return m3_per_second * 86499.0 / (1000.0 * catchment_area)


def convert_mm_per_day_to_m3_per_second(mm_per_day, catchment_area) :
	return mm_per_day * catchment_area / 86.4
	
	
def activation_control(x, threshold, reld) :
	if(x < threshold) :
		return 0.0
	dist = threshold * reld
	if(x > threshold + dist) :
		return 1.0
	xx = (x - threshold) / dist
	return (3.0 - 2.0*xx)*xx*xx
	
def snow_step(timestep, inputs, snow_results, parameters) :
	ddfmelt = parameters[0]
	pquick  = parameters[1]
	
	precip = inputs[0, timestep]
	airt   = inputs[1, timestep]
	
	last_snow_depth = snow_results[4, timestep-1]
	
	if airt < 0 :
		psnow = precip
		prain = 0.0
	else :
		psnow = 0.0
		prain = precip
	
	pot_melt = max(0.0, ddfmelt*airt)
	melt = min(last_snow_depth, pot_melt)
	snow_depth = last_snow_depth + psnow - melt
	hydrol_input_soil = melt + prain
	inf = (1.0 - pquick)*hydrol_input_soil
	inf_ex = pquick*hydrol_input_soil
	
	snow_results[0, timestep] = psnow
	snow_results[1, timestep] = prain
	snow_results[2, timestep] = pot_melt
	snow_results[3, timestep] = melt
	snow_results[4, timestep] = snow_depth
	snow_results[5, timestep] = hydrol_input_soil
	snow_results[6, timestep] = inf
	snow_results[7, timestep] = inf_ex
	
	
def land_step(lu, timestep, inputs, land_results, parameters, tc, inf) :
	y0 = [land_results[lu, 0, timestep-1], 0.0]
	
	pet  = inputs[2, timestep]
	mpet = parameters[2]
	fc   = parameters[3]
	
	def land_ode(t, y) :
		v = y[0]
		smd = fc - v
		qsw = -smd * activation_control(v, fc, 0.01) / tc[lu]
		return [inf - mpet*pet*(1.0 - np.exp(np.log(0.01)*v / fc)) - qsw, qsw]
		
	res = scipy.integrate.solve_ivp(land_ode, [0,1], y0, method='RK45', t_eval=[1], first_step=0.01)
	
	land_results[lu, 0, timestep] = res.y[0, 0]
	land_results[lu, 1, timestep] = res.y[1, 0]
	
def reach_step(timestep, reach_results, parameters, inf_ex, qsw) :
	y0 = [
		reach_results[0, timestep-1],
		reach_results[1, timestep-1],
		0.0
	]
	
	bfi     = parameters[4]
	tg      = parameters[5]
	qg_min  = parameters[6]
	c_m     = parameters[7]
	a_catch = parameters[8]
	rlen    = parameters[9]
	slope   = parameters[10]
	
	def reach_ode(t, y) :
		vg = y[0]
		vr = y[1]
		
		qg0 = vg/tg
		tt = activation_control(qg0, qg_min, 0.01)
		
		qg = (1.0 - tt)*qg_min + tt*qg0
		
		qland0 = inf_ex + (1.0 - bfi)*qsw + qg
		qland = convert_mm_per_day_to_m3_per_second(qland0, a_catch)
		
		val = vr * np.sqrt(slope) / (rlen * c_m)
		qr = 0.28 * val * np.sqrt(val)
		
		return [
			bfi * qsw - qg,
			86400.0 * (qland - qr),
			qr
		]
		
	res = scipy.integrate.solve_ivp(reach_ode, [0,1], y0, method='RK45', t_eval=[1], first_step=0.1)
	
	vg = res.y[0, 0]
	qg0 = vg/tg
	tt = activation_control(qg0, qg_min, 0.01)
	qg = (1.0-tt)*qg_min + tt*qg0
	
	reach_results[0, timestep] = qg*tg
	reach_results[1, timestep] = res.y[1, 0]
	reach_results[2, timestep] = res.y[2, 0]


def run_hardcoded_model(timesteps, parameters, tc, lup, inputs, snow_results, land_results, reach_results) :
	for timestep in range(1, timesteps+1) :
		snow_step(timestep, inputs, snow_results, parameters)
	
		inf = snow_results[6, timestep]
		inf_ex = snow_results[7, timestep]
		qsw = 0.0
		for lu in range(land_results.shape[0]) :
			land_step(lu, timestep, inputs, land_results, parameters, tc, inf)
			
			qsw = qsw + land_results[lu, 1, timestep] * lup[lu]
		
		reach_step(timestep, reach_results, parameters, inf_ex, qsw)
	

# Initialise wrapper
wrapper_fpath = (r"..\PythonWrapper\mobius.py")
wr = imp.load_source('mobius', wrapper_fpath)
wr.initialize('simplyq.dll')


dataset = wr.DataSet.setup_from_parameter_and_input_files('testparameters.dat', 'tarlandinputs.dat')

dataset.run_model() # We have to do this once first for the 'Potential evapotranspiration' timeseries to be correctly initialized

landscape_units = dataset.get_indexes('Landscape units')
reaches         = dataset.get_indexes('Reaches')         #Note: this hard code test only works for one reach right now!

parameters = np.zeros((11))

parameters[0] = dataset.get_parameter_double('Degree-day factor for snowmelt', [])
parameters[1] = dataset.get_parameter_double('Proportion of precipitation that contributes to quick flow', [])
parameters[2] = dataset.get_parameter_double('PET multiplication factor', [])
parameters[3] = dataset.get_parameter_double('Soil field capacity', [])
parameters[4] = dataset.get_parameter_double('Baseflow index', [])
parameters[5] = dataset.get_parameter_double('Groundwater time constant', [])
parameters[6] = dataset.get_parameter_double('Minimum groundwater flow', [])
parameters[7] = dataset.get_parameter_double('Manning\'s coefficient', [])
parameters[8] = dataset.get_parameter_double('Catchment area', [reaches[0]])
parameters[9] = dataset.get_parameter_double('Reach length', [reaches[0]]) * 0.5 #Effective reach length
parameters[10] = dataset.get_parameter_double('Reach slope', [reaches[0]])

tc = np.zeros((3))
lup = np.zeros((3))

for idx, lu in enumerate(landscape_units) :
	tc[idx]  = dataset.get_parameter_double('Soil water time constant', [lu])
	lup[idx] = dataset.get_parameter_double('Land use proportions', [reaches[0], lu])
	
timesteps = dataset.get_parameter_uint('Timesteps', [])

inputs = np.zeros((3, timesteps+1))

inputs[0, 1:] = dataset.get_input_series('Precipitation', [], alignwithresults=True)
inputs[1, 1:] = dataset.get_input_series('Air temperature', [], alignwithresults=True)
inputs[2, 1:] = dataset.get_input_series('Potential evapotranspiration', [], alignwithresults=True)

snow_results  = np.zeros((8, timesteps+1))
land_results  = np.zeros((3, 2, timesteps+1))
reach_results = np.zeros((3, timesteps+1))

#initial values
snow_results[4,0] = dataset.get_parameter_double('Initial snow depth as water equivalent', [])

for idx, lu in enumerate(landscape_units) :
	land_results[idx, 0, 0] = dataset.get_parameter_double('Soil field capacity', [])


qr0 = dataset.get_parameter_double('Initial in-stream flow', [reaches[0]])	
reach_results[0, 0] = convert_m3_per_second_to_mm_per_day(qr0, parameters[8]) * parameters[4] * parameters[5]
reach_results[1, 0] = 0.349 * (qr0**0.34) * 2.71 * (qr0**0.557) * parameters[9] * 2.0

run_hardcoded_model(timesteps, parameters, tc, lup, inputs, snow_results, land_results, reach_results)

#Evaluate correctness:

mobius_snow_depth = dataset.get_result_series('Snow depth', [])
mobius_soil_water_volume = dataset.get_result_series('Soil water volume', [landscape_units[0]])
mobius_mean_reach_flow = dataset.get_result_series('Reach flow (daily mean, cumecs)', [reaches[0]])

if not np.allclose(mobius_snow_depth, snow_results[4, 1:], rtol=1e-6) :
	print('Snow results were different')
else :
	print('Snow results were correct')
	
if not np.allclose(mobius_soil_water_volume, land_results[0, 0, 1:], rtol=1e-3) :   #Note: we are using a different ODE solver, so we don't get better precision than this
	print('Land results were different')
else :
	print('Land results were correct')
	
if not np.allclose(mobius_mean_reach_flow, reach_results[2, 1:], rtol=2e-2) : #Note: we are using a different ODE solver, so we don't get better precision than this
	print('Reach results were different')
else :
	print('Reach results were correct')
	
summobius = 0
sumhardcode = 0
for run in range(1000) :
	before = time.perf_counter()
	dataset.run_model()
	after = time.perf_counter()
	summobius += (after - before)
	
	before = time.perf_counter()
	run_hardcoded_model(timesteps, parameters, tc, lup, inputs, snow_results, land_results, reach_results)
	after = time.perf_counter()
	sumhardcode += (after - before)
	
print('Mobius time (1000 runs):')
print(summobius)
print('Hardcoded time (1000 runs):')
print(sumhardcode)
print('Ratio:')
print(sumhardcode / summobius)
	
