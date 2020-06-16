
#define MOBIUS_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define MOBIUS_TEST_FOR_NAN 1
#define MOBIUS_EQUATION_PROFILING 1
#define MOBIUS_PRINT_TIMING_INFO 1
#define MOBIUS_INDEX_BOUNDS_TESTS 1

#include "../../mobius.h"

#include "../../Modules/Persist_Manning.h"
#include "../../Modules/WaterTemperature.h"
#include "../../Modules/INCA-Microplastics.h"

int main()
{
    const char *InputFile = "Beaver/BeaverInputFinal_mf.dat";
	const char *ParameterFile = "Beaver/BeaverManningsparameter_mnf.dat";
	
	mobius_model *Model = BeginModelDefinition("INCA-Microplastics");
	
	AddPersistModel(Model);
	AddWaterTemperatureModel(Model);
	AddINCAMicroplasticsModel(Model);
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
    
	ReadParametersFromFile(DataSet, ParameterFile);
	
    
	ReadInputsFromFile(DataSet, InputFile);
	
	//WriteParametersToFile(DataSet, "newparams.dat");
	
	//PrintResultStructure(Model);
	//PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);
	
	RunModel(DataSet);
}


