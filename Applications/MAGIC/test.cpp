#define MOBIUS_TIMESTEP_VERBOSITY 1
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define MOBIUS_TEST_FOR_NAN 1
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 1
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius.h"

#include "../../Modules/MAGIC/MAGIC_Core_Wrapper.h"
#include "../../Modules/MAGIC/MAGICBasic.h"
#include "../../Modules/MAGIC/MAGIC_CarbonNitrogen.h"


int main()
{
    const char *InputFile     = "testinputs.dat";
	const char *ParameterFile = "testparameters_microbial.dat";
	
	mobius_model *Model = BeginModelDefinition("MAGIC");
	
	AddMagicCoreModel(Model);
	AddMagicModel(Model);
	
	//Carbon and nitrogen
	//AddSimpleMagicCarbonNitrogenModel(Model);
	AddMicrobialMagicCarbonNitrogenModel(Model);
	
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	SetTimestepSize(Model, "1M");
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
    
	ReadParametersFromFile(DataSet, ParameterFile);
	
	SetParameterValue(DataSet, "Timesteps", {}, (u64)100);
	//AllocateParameterStorage(DataSet);
	//WriteParametersToFile(DataSet, "newparams.dat");

	ReadInputsFromFile(DataSet, InputFile);
	
	//PrintResultStructure(Model);
    
	RunModel(DataSet);
}