#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../../mobius_dll.h"

#include "../../../Modules/PET.h"
#include "../../../Modules/Simply/SimplySnow.h"

#define SIMPLYQ_GROUNDWATER
#include "../../../Modules/Simply/SimplyQ.h"
//#include "../../../Modules/Simply/SimplyQ_Ballycanew.h"
#include "../../../Modules/Simply/SimplySed.h"
//#include "../../../Modules/Simply/SimplySed_Ballycanew.h"
#include "../../../Modules/Simply/SimplyP.h"   //NOTE: can change this again when starting a new version


mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("SimplyP", true);   //'true' signifies that we want an "End date" parameter instead of a "Timesteps" parameter
	
	AddThornthwaitePETModule(Model);
	AddSimplySnowModule(Model);
	AddSimplyHydrologyModule(Model);
	AddSimplySedimentModule(Model);
	AddSimplyPModel(Model);
	
	return Model;
}
