
#include "../../mobius_dll.h"

//#include "../../Modules/SolarRadiation.h"
#include "../../Modules/PET.h"
#define SIMPLYQ_GROUNDWATER          
#include "../../Modules/Simply/SimplyQ.h"
#include "../../Modules/Simply/SimplySed.h"
#include "../../Modules/Simply/SimplyP_v04.h"
#include "../../Modules/Simply/Alternate_versions_of_simplyC/SimplyC_DOC_creation_rate.h"
#include "../../Modules/Simply/SimplyN.h"
#include "../../Modules/SimplySoilTemperature.h"

mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("SimplyCNP", true);
	
	//AddMaxSolarRadiationModule(Model);
	//AddPriestleyTaylorPETModule2(Model);
	AddDegreeDayPETModule(Model);
	
	AddSimplyHydrologyModule(Model);
	AddSoilTemperatureModel(Model);
	
	AddSimplySedimentModule(Model);
	AddSimplyNModel(Model);
	AddSimplyCModel(Model);
	AddSimplyPModel(Model);
	
	return Model;
}
