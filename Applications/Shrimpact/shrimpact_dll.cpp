

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius_dll.h"

#include "../../Modules/EcoTox/Shrimpact.h"


mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("SHRIMPACT", false, "3M");
	
	AddShrimpactModel(Model);
	
	return Model;
}
