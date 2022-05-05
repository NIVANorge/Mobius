

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius_dll.h"

#define SIMPLYQ_GROUNDWATER    //NOTE: #defining this before the inclusion of the SimplyQ.h file turns on groundwater in SimplyQ.

#include "../../Modules/PET.h"
#include "../../Modules/Simply/SimplySnow_Nordic.h"
//#include "../../Modules/Simply/HBVSnow.h"
#include "../../Modules/Simply/SimplyQ_Ballycanew.h"
//#include "../../Modules/Simply/SimplyQ_SoilFrost.h"
//#include "../../Modules/Simply/SimplySed.h"
#include "../../Modules/Simply/SimplyC.h"
#include "../../Modules/SimplySoilTemperature.h"


mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("SimplyC (quantom)", true);
	
	//AddThornthwaitePETModule(Model);
	AddSimplySnowNordicModule(Model);
	//AddHBVSnowModule(Model);
	AddDegreeDayPETModule(Model);
	AddSoilTemperatureModel(Model);
	AddSimplyHydrologyModule(Model);
	//AddSimplySedimentModule(Model);
	AddSimplyCModel(Model);
	//AddSimplyTOCModule(Model);
	
	return Model;
}
