
#include "../../mobius_dll.h"

#include "../../Modules/UnitConversions.h"

//#include "../../Modules/SolarRadiation.h"
#include "../../Modules/PET.h"
#define SIMPLYQ_GROUNDWATER          
#include "../../Modules/SimplyQ.h"
#include "../../Modules/SimplySed.h"
#include "../../Modules/SimplyP_v04.h"
#include "../../Modules/Alternate_versions_of_simplyC/SimplyC_DOC_creation_rate.h"
#include "../../Modules/SimplyN.h"
#include "../../Modules/SimplySoilTemperature.h"

#define EASYLAKE_SIMPLYQ
#include "../../Modules/EasyLake.h"

#include "../../Modules/EasyLakeCNP.h"

mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("SimplyCNP", true);
	
	//TODO: Since EasyLake uses complex climate variables, we should just use proper Priestley-Taylor anyway
	
	//AddMaxSolarRadiationModule(Model);
	//AddPriestleyTaylorPETModule2(Model);
	AddDegreeDayPETModule(Model);
	
	AddSimplyHydrologyModule(Model);
	AddSoilTemperatureModel(Model);
	
	AddSimplySedimentModule(Model);
	AddSimplyNModel(Model);
	AddSimplyCModel(Model);
	AddSimplyPModel(Model);
	
	AddEasyLakePhysicalModule(Model);
	AddEasyLakeCNPModule(Model);
	
	return Model;
}
