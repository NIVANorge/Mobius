

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius_dll.h"

#define SIMPLYQ_GROUNDWATER    //NOTE: #defining this before the inclusion of the SimplyQ.h file turns on groundwater in SimplyQ.

#include "../../Modules/PET.h"
#include "../../Modules/Simply/SimplyQ.h"


mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("SimplyQ", true);
	
	//AddThornthwaitePETModule(Model);
	AddDegreeDayPETModule(Model);
	AddSimplyHydrologyModule(Model);
	
	return Model;
}
