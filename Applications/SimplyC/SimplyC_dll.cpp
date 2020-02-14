#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0

#include "../../mobius_dll.h"

#include "../../Modules/UnitConversions.h"

#include "../../Modules/PET.h"
//#define SIMPLYQ_GROUNDWATER    //NOTE: #define this before the inclusion of the SimplyQ.h file if you want SimplyQ to simulate groundwater
								 //Comment out this line if you don't want groundwater           
//#include "../../Modules/SimplyQ.h"
#include "../../Modules/SimplyQ_Langtjern.h"


//#include "../../Modules/SimplyC.h"
//#include "../../Modules/Alternate_versions_of_simplyC/SimplyC_exp_temp_SO4_groundwater_transport.h"
//#include "../../Modules/Alternate_versions_of_simplyC/SimplyC_polynomial_temp_SO4.h"
//#include "../../Modules/Alternate_versions_of_simplyC/SimplyC_polynomial_temp_SO4.h"
#include "../../Modules/Alternate_versions_of_simplyC/SimplyC_DOC_creation_rate.h"
#include "../../Modules/SimplySoilTemperature.h"

#include "../../Modules/SimplyQLakeAddon.h"

DLLEXPORT void *
DllSetupModel(char *ParameterFilename, char *InputFilename)
{
	CHECK_ERROR_BEGIN
	
	mobius_model *Model = BeginModelDefinition("SimplyC");
	
	//AddThornthwaitePETModule(Model);
	AddDegreeDayPETModule(Model);
	AddSimplyHydrologyModule(Model);
	AddSimplyQLakeAddon(Model);
	AddSoilTemperatureModel(Model);
	AddSimplyCModel(Model);
	
	ReadInputDependenciesFromFile(Model, InputFilename);
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
	
	ReadParametersFromFile(DataSet, ParameterFilename);
	ReadInputsFromFile(DataSet, InputFilename);
	
	return (void *)DataSet;
	
	CHECK_ERROR_END
	
	return 0;
}
