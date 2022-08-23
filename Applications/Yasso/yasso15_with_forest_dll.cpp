

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius_dll.h"

#define FOREST_STANDALONE
#include "../../Modules/MAGIC/MAGICForestDecompUptake.h"

#define YASSO_USE_FOREST_MODEL
#include "../../Modules/MAGIC/Yasso15.h"


mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("Yasso", true, "1Y");
	
	AddMAGICForestDecompUptakeModel(Model);
	AddYasso15Model(Model);

	return Model;
}
