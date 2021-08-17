

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 1
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 1

#include "../../mobius_dll.h"

#include "../../Modules/PET.h"

#define SIMPLYQ_GROUNDWATER
#include "../../Modules/Simply/SimplyQ.h"

#define EASYLAKE_SIMPLYQ
#include "../../Modules/EasyLake/EasyLake.h"


mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("SimplyQ with Easy-Lake", true);
	
	AddDegreeDayPETModule(Model);
	AddSimplyHydrologyModule(Model);
	AddEasyLakePhysicalModule(Model);
	
	return Model;
}
