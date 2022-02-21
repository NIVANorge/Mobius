#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0

#include "../../mobius_dll.h"

#include "../../Modules/PET.h"
#include "../../Modules/Simply/SimplySnow.h"
//#define SIMPLYQ_GROUNDWATER    //NOTE: #define this before the inclusion of the SimplyQ.h file if you want SimplyQ to simulate groundwater
								 //Comment out this line if you don't want groundwater           
#include "../../Modules/Simply/SimplyQ.h"


//#include "../../Modules/SimplyC.h"
//#include "../../Modules/Alternate_versions_of_simplyC/SimplyC_exp_temp_SO4_groundwater_transport.h"
//#include "../../Modules/Alternate_versions_of_simplyC/SimplyC_polynomial_temp_SO4.h"
#include "../../Modules/Simply/Alternate_versions_of_simplyC/SimplyC_polynomial_temp_SO4_equilibrizing_DOC.h"
#include "../../Modules/SimplySoilTemperature.h"

mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("SimplyC");
	
	AddThornthwaitePETModule(Model);
	AddSimplySnowModule(Model);
	AddSimplyHydrologyModule(Model);
	AddSoilTemperatureModel2(Model);
	AddSimplyCModel(Model);
	
	return Model;
}
