#define MOBIUS_TIMESTEP_VERBOSITY 3
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define MOBIUS_TEST_FOR_NAN 1
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 1
#define MOBIUS_INDEX_BOUNDS_TESTS 1

#include "../../mobius.h"

#include "../../Modules/PET.h"

#define SIMPLYQ_GROUNDWATER
#include "../../Modules/SimplyQ.h"

#define EASYLAKE_SIMPLYQ
#include "../../Modules/EasyLake.h"



int main()
{
    const char *InputFile     = "langtjerninputs.dat";
	const char *ParameterFile = "testparameters_simplyq_debugging.dat";
	
	mobius_model *Model = BeginModelDefinition("SimplyQ with Easy-Lake", true);
	
	AddDegreeDayPETModule(Model);
	AddSimplyHydrologyModule(Model);
	AddEasyLakePhysicalModule(Model);
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
    
	ReadParametersFromFile(DataSet, ParameterFile);
	
	//AllocateParameterStorage(DataSet);
	//WriteParametersToFile(DataSet, "newparams.dat");

	ReadInputsFromFile(DataSet, InputFile);
	
	PrintResultStructure(Model);
    
	RunModel(DataSet);
}