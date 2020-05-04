#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../../mobius_dll.h"

#include "../../../Modules/PET.h"

#define SIMPLYQ_GROUNDWATER
#include "../../../Modules/SimplyQ.h"
#include "../../../Modules/SimplySed.h"
#include "../../../Modules/SimplyP_v04.h"


void
DllBuildModel(mobius_model *Model)
{
	Model->Name = "SimplyP";
	
	AddThornthwaitePETModule(Model);
	AddSimplyHydrologyModule(Model);
	AddSimplySedimentModule(Model);
	AddSimplyPModel(Model);
}
