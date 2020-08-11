
import imp

# Calibration functions
calib_fpath = (r"..\mobius_calib_uncert_lmfit.py")
cu = imp.load_source('mobius_calib_uncert_lmfit', calib_fpath)


def configure_params(params, do_doc):
	params['fquick'].max = 0.1
	
	params['fc'].min = 40
	
	params['Ts_F'].max = 12
	
	for lu in ['F', 'S', 'P']:
		params['alphaPET_%s' % lu].max = 3.0

	if do_doc :
		params['kT'].min = 0.0
		params['kT'].max = 0.2

		params['kSO4'].min = 0.0
		params['kSO4'].max = 0.2

		params['STC_S'].set(expr='STC_F')
		params['STC_P'].set(expr='STC_F')

		for lu in ['F', 'S', 'P'] :
			params['baseDOC_%s' % lu].min = 1.0
			params['baseDOC_%s' % lu].max = 30.0
			
			params['cDOC_%s' % lu].max = 3.0

		params['cDOC_S'].set(expr='cDOC_F')
		params['cDOC_P'].set(expr='cDOC_F')
	
	
def setup_calibration_params(dataset, do_doc) :
	param_df = cu.get_double_parameters_as_dataframe(dataset, index_short_name={'Forest':'F', 'Shrubs':'S', 'Peat':'P', 'R0':'r0'})
	
	wantparams = [
			'alphaPET', 'DDFmelt', 
			'fquick', 'Ts', 'fc',
	]
	wantparams2 = [
			'depthST', 'STC',
			'kT', 'kSO4', 'cDOC', 'baseDOC',
		]
	
	if do_doc :
		wantparams += wantparams2

	#Select all parameters with short_name starting with any of the names in 'wantparams'
	calib_df = param_df[[any([sn.startswith(n) for n in wantparams]) for sn in param_df['short_name']]]
	
	params = cu.parameter_df_to_lmfit(calib_df)
	
	configure_params(params, do_doc)
	
	return params