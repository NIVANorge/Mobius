

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius_dll.h"

//#include "../../Modules/MAGIC/MAGIC_Core_Wrapper.h"
#include "../../Modules/MAGIC/MAGICForest.h"
//#include "../../Modules/MAGIC/MAGIC_CarbonNitrogen.h"


mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("MAGIC", false, "1M");
	
	//AddMagicCoreModel(Model);
	AddMagicForestModule(Model);
	
	//Carbon and nitrogen
	//AddSimpleMagicCarbonNitrogenModel(Model);
	//AddRatioMagicCarbonNitrogenModel(Model);
	//AddMicrobialMagicCarbonNitrogenModel(Model);

	return Model;
}