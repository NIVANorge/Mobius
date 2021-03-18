#define MOBIUS_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define MOBIUS_TEST_FOR_NAN 1
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 1
#define MOBIUS_INDEX_BOUNDS_TESTS 1

#include "../../mobius.h"

#include "../../Modules/MAGIC/Yasso.h"



int main()
{
    const char *InputFile     = "testinputs.dat";
	const char *ParameterFile = "testparameters.dat";
	
	mobius_model *Model = BeginModelDefinition("Yasso", true, "1Y");
	
	AddYassoModel(Model);
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	EndModelDefinition(Model);
	
	//PrintInitialValueOrder(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
    
	//std::cout << "dataset generated\n";
	
	ReadParametersFromFile(DataSet, ParameterFile);
	
	//std::cout << "parameters read\n";
	//SetParameterValue(DataSet, "End date", {}, "1850-1-1");
	//AllocateParameterStorage(DataSet);
	//WriteParametersToFile(DataSet, "newparams.dat");

	ReadInputsFromFile(DataSet, InputFile);
	
	//std::cout << "inputs read\n";
	//PrintResultStructure(Model);
    
	RunModel(DataSet);
}