#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0

#include "../../mobius_dll.h"

#include "../../Modules/SolarRadiation.h"
#include "../../Modules/PET.h"
          
#include "../../Modules/Simply/SimplySnow_Langtjern.h"
//#define SIMPLYQ_GROUNDWATER
#include "../../Modules/Simply/SimplyQ.h"
#include "../../Modules/Simply/Alternate_versions_of_SimplyC/SimplyC_DOC_creation_rate.h"

#include "../../Modules/SimplySoilTemperature.h"


mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("SimplyC (regional)");
	
	AddMaxSolarRadiationModule(Model);
	AddPriestleyTaylorPETModule2(Model);
	AddSimplySnowModule(Model);
	AddSimplyHydrologyModule(Model);
	AddSoilTemperatureModel(Model);
	AddSimplyCModel(Model);
	
	return Model;
}
