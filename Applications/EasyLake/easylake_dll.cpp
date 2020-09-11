

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius_dll.h"

#define EASYLAKE_STANDALONE
#include "../../Modules/EasyLake.h"



mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("Easy-Lake", true);
	
	AddEasyLakePhysicalModule(Model);
	
	return Model;
}
