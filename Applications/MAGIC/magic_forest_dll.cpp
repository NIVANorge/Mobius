

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 1
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius_dll.h"

#include "../../Modules/MAGIC/MAGIC_Core_Wrapper.h"
#include "../../Modules/MAGIC/MAGICForest.h"
//#include "../../Modules/MAGIC/WASMOD.h"
#include "../../Modules/MAGIC/ABCD.h"
//#include "../../Modules/MAGIC/MAGIC_CarbonNitrogen.h"
#include "../../Modules/MAGIC/MagicForestCNP.h"


mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("MAGIC Forest", true, "1M");
	
	//AddWASMODModel(Model);
	AddABCDModel(Model);
	AddMagicCoreModel(Model);
	AddMagicForestModule(Model);
	AddSimpleMagicForestCNPModel(Model);
	

	return Model;
}
