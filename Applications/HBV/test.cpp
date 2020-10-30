#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius.h"

#include "../../Modules/HBV.h"


int main()
{
	mobius_model *Model = BeginModelDefinition("SimplyP");
	
	AddHBVModel(Model);
	
	ReadInputDependenciesFromFile(Model, "langtjerninputs.dat");
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);

	ReadParametersFromFile(DataSet, "langtjernparameters.dat");

	ReadInputsFromFile(DataSet, "langtjerninputs.dat");
	
	PrintResultStructure(Model);
	
	RunModel(DataSet);
}
