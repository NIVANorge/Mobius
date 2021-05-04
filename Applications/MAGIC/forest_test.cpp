#define MOBIUS_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define MOBIUS_TEST_FOR_NAN 1
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius.h"

#include "../../Modules/MAGIC/MAGIC_Core_Wrapper.h"
#include "../../Modules/MAGIC/MagicForestDecompUptake.h"
#include "../../Modules/MAGIC/MAGICForest.h"
//#include "../../Modules/MAGIC/WASMOD.h"
#include "../../Modules/MAGIC/ABCD.h"
//#include "../../Modules/MAGIC/MAGIC_CarbonNitrogen.h"
#include "../../Modules/MAGIC/MagicForestCNP.h"


int main()
{
    const char *InputFile     = "forest_test_inputs.dat";
	const char *ParameterFile = "forest_test_parameters2.dat";
	
	mobius_model *Model = BeginModelDefinition("MAGIC Forest", true, "1M");
	
	//AddWASMODModel(Model);
	AddABCDModel(Model);
	AddMagicCoreModel(Model);
	AddMagicForestModule(Model);
	AddMAGICForestDecompUptakeModel(Model);
	AddSimpleMagicForestCNPModel(Model);
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	EndModelDefinition(Model);
	
	//PrintInitialValueOrder(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
    
	ReadParametersFromFile(DataSet, ParameterFile);
	
	SetParameterValue(DataSet, "End date", {}, "1850-1-1");
	//AllocateParameterStorage(DataSet);
	//WriteParametersToFile(DataSet, "newparams.dat");

	ReadInputsFromFile(DataSet, InputFile);
	
	//PrintResultStructure(Model);
    
	RunModel(DataSet);
}