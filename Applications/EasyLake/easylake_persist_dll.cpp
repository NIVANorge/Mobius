

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 1

#include "../../mobius_dll.h"

#include "../../Modules/INCA/PERSiST.h"

#define EASYLAKE_PERSIST
#include "../../Modules/EasyLake/EasyLake.h"



mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("PERSiST with Easy-Lake");
	
	AddPersistModel(Model);
	AddEasyLakePhysicalModule(Model);
	
	return Model;
}
