
#define MOBIUS_TEST_FOR_NAN 0

#include "../../mobius_dll.h"


//#include "../../Modules/SolarRadiation.h"
#include "../../Modules/PET.h"
#include "../../Modules/Simply/SimplySnow.h"
#define SIMPLYQ_GROUNDWATER          
#include "../../Modules/Simply/SimplyQ.h"
#include "../../Modules/Simply/SimplySed.h"
#include "../../Modules/Simply/SimplyP.h"
#include "../../Modules/Simply/SimplyC.h"
#include "../../Modules/Simply/SimplyN.h"
#include "../../Modules/SimplySoilTemperature.h"

#define EASYLAKE_SIMPLYQ
#include "../../Modules/EasyLake/EasyLake.h"

#include "../../Modules/EasyLake/EasyLakeCNP.h"

mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("SimplyCNP", true);
	
	//TODO: Since EasyLake uses complex climate variables, we should just use proper Priestley-Taylor anyway
	
	//AddMaxSolarRadiationModule(Model);
	//AddPriestleyTaylorPETModule2(Model);
	AddDegreeDayPETModule(Model);
	AddSimplySnowModule(Model);
	
	AddSimplyHydrologyModule(Model);
	AddSoilTemperatureModel(Model);
	
	AddSimplySedimentModule(Model);
	AddSimplyNModel(Model);
	AddSimplyCModel(Model);
	AddSimplyTOCModule(Model);
	AddSimplyPModel(Model);
	
	AddEasyLakePhysicalModule(Model);
	AddEasyLakeCNPModule(Model);
	
	return Model;
}
