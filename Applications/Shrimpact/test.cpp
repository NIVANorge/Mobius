

#define MOBIUS_TEST_FOR_NAN 1
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 1
#define MOBIUS_INDEX_BOUNDS_TEST 1

#include "../../mobius.h"

#include "../../Modules/EcoTox/Shrimpact.h"

int main()
{
	mobius_model *Model = BeginModelDefinition("SHRIMPACT");
	
	AddShrimpactModel(Model);
	
	ReadInputDependenciesFromFile(Model, "testinputs.dat"); //NOTE: This has to happen here before EndModelDefinition
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);

	ReadParametersFromFile(DataSet, "testparameters.dat");

	ReadInputsFromFile(DataSet, "testinputs.dat");
	
	PrintResultStructure(Model);
	
	WriteParametersToFile(DataSet, "testparameters.dat");

	RunModel(DataSet);
}