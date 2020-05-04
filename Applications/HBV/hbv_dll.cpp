

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0

#include "../../mobius_dll.h"

#include "../../Modules/HBV.h"


void DllBuildModel(mobius_model *Model)
{
	Model->Name = "HBV";
	
	AddHBVModel(Model);
}
