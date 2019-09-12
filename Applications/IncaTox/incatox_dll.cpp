

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius_dll.h"

#include "../../Modules/Persist.h"
#include "../../Modules/INCA-Microplastics.h"
#include "../../Modules/SoilTemperature.h"
#include "../../Modules/INCA-Tox-C.h"
#include "../../Modules/INCA-Tox.h"


DLLEXPORT void *
DllSetupModel(char *ParameterFilename, char *InputFilename) {
    
	CHECK_ERROR_BEGIN
	
	mobius_model *Model = BeginModelDefinition("INCA-Tox", "0.0");
	
	AddPersistModel(Model);
	//AddINCAMicroplasticsModel(Model); //TODO: Add back in when we get that far.
	AddSoilTemperatureModel(Model);
	AddIncaToxDOCModule(Model);
	AddIncaToxModule(Model);
	
	ReadInputDependenciesFromFile(Model, InputFilename);
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
	
	ReadParametersFromFile(DataSet, ParameterFilename);
	ReadInputsFromFile(DataSet, InputFilename);
	
	return (void *)DataSet;
	
	CHECK_ERROR_END
	
	return 0;
}