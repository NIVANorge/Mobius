
#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 1
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 1
#define MOBIUS_INDEX_BOUNDS_TESTS 1

#define MOBIUS_TEST_INDEX_OVERFLOW

#include "../../mobius.h"

#include "../../Modules/Persist.h"
#include "../../Modules/INCA-Sed.h"
#include "../../Modules/SoilTemperature.h"
#include "../../Modules/WaterTemperature.h"
#include "../../Modules/SolarRadiation.h"
#include "../../Modules/INCA-Tox-C.h"
#include "../../Modules/INCA-Tox.h"
#include "../../Modules/INCA-Tox-Lake.h"

int main()
{
    const char *InputFile = "COWI/inputs_COWI.dat";
	const char *ParameterFile = "COWI/params_COWI.dat";
	
	mobius_model *Model = BeginModelDefinition("INCA-Tox");
	
	AddPersistModel(Model);
	AddINCASedModel(Model);
	AddSoilTemperatureModel(Model);
	AddWaterTemperatureModel(Model);
	AddMaxSolarRadiationModule(Model);
	AddIncaToxDOCModule(Model);
	AddIncaToxModule(Model);
	AddIncaToxLakeModule(Model);
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
    
	ReadParametersFromFile(DataSet, ParameterFile);
	
    
	ReadInputsFromFile(DataSet, InputFile);
	
	//WriteParametersToFile(DataSet, "newparams.dat");
	
	PrintResultStructure(Model);
	//PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);
	
	RunModel(DataSet);
}


