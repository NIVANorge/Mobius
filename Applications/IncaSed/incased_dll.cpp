

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius_dll.h"

#include "../../Modules/Persist_Manning.h"
#include "../../Modules/INCA-Sed.h"


void
DllBuildModel(mobius_model *Model)
{
	Model->Name = "INCA-Sed";
	
	AddPersistModel(Model);
	AddINCASedModel(Model);
}
