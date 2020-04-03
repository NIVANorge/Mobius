#define MOBIUS_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 1
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius.h"

#include "../../Modules/MAGIC/MAGIC_Core_Wrapper.h"
#include "../../Modules/MAGIC/MAGICBasic.h"



int main()
{
    const char *InputFile     = "testinputs.dat";
	const char *ParameterFile = "testparameters.dat";
	
	mobius_model *Model = BeginModelDefinition("MAGIC");
	
	AddMagicCoreModel(Model);
	AddMagicModel(Model);
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	SetTimestepSize(Model, "1M");
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
    
	ReadParametersFromFile(DataSet, ParameterFile);
	
	//AllocateParameterStorage(DataSet);
	//WriteParametersToFile(DataSet, "newparams.dat");

	ReadInputsFromFile(DataSet, InputFile);
	
	//PrintResultStructure(Model);
    
	RunModel(DataSet);
}