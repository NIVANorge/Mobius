

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius_dll.h"

#include "../../Modules/Persist.h"
#include "../../Modules/INCA-Sed.h"


mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("INCA-Sed");
	
	AddPersistModel(Model);
	AddINCASedModel(Model);
	
	return Model;
}
