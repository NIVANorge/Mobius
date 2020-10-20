
from importlib.machinery import SourceFileLoader

# Initialise wrapper
cu = SourceFileLoader("mobius_calib_uncert_lmfit", r"..\mobius_calib_uncert_lmfit.py").load_module()


def configure_params(params, do_doc, do_hydro):
	
	if do_hydro :
		
		params['alphaPET_F'].min = 0.5
		params['alphaPET_F'].max = 2.0
		#TODO: Should this be different per land type or not?
		for lu in ['S', 'P']:
			params['alphaPET_%s' % lu].set(expr='alphaPET_F')
		
		params['fquick'].min = 0.0
		params['fquick'].max = 0.1
		
		params['Ts_F'].min = 1
		params['Ts_S'].min = 1
		params['Ts_P'].min = 2
		
		params['Ts_F'].max = 5
		params['Ts_S'].max = 5
		params['Ts_P'].max = 20

	if do_doc :
		
		params['fc_F'].min = 60
		params['fc_S'].min = 40
		params['fc_P'].min = 100
	
		params['fc_F'].max = 200
		params['fc_S'].max = 100
		params['fc_P'].max = 400
	
		params['depthST'].min = -1
		params['depthST'].max = -0.1
		
		params['STC_F'].min = 0.05
		params['STC_F'].max = 0.3
		params['STC_S'].set(expr='STC_F')
		params['STC_P'].set(expr='STC_F')
		
		params['kT'].min = 0.0
		params['kT'].max = 0.2

		params['kSO4'].min = 0.0
		params['kSO4'].max = 0.002
		
		params['baseDOC_F'].min = 3
		params['baseDOC_S'].min = 1
		params['baseDOC_P'].min = 6
		
		params['baseDOC_F'].max = 25
		params['baseDOC_S'].max = 4
		params['baseDOC_P'].max = 40
		

		params['cDOC_S'].set(expr='cDOC_F')
		params['cDOC_P'].set(expr='cDOC_F')
	
	
def setup_calibration_params(dataset, do_doc=True, do_hydro=True) :
	param_df = cu.get_double_parameters_as_dataframe(dataset, index_short_name={'Forest':'F', 'Shrubs':'S', 'Peat':'P', 'R0':'r0'})
	
	wantparams = []
	
	wantparams1 = [
			'alphaPET', 'DDFmelt', 
			'fquick', 'Ts',
	]
	wantparams2 = [
			'fc', 'depthST', 'STC',
			'kT', 'kSO4', 'cDOC', 'baseDOC',
	]
	
	if do_hydro :
		wantparams += wantparams1
	if do_doc :
		wantparams += wantparams2

	#Select all parameters with short_name starting with any of the names in 'wantparams'
	calib_df = param_df[[any([sn.startswith(n) for n in wantparams]) for sn in param_df['short_name']]]
	
	params = cu.parameter_df_to_lmfit(calib_df)
	
	configure_params(params, do_doc, do_hydro)
	
	return params