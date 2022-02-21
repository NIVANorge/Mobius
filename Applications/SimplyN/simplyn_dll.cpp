
#include "../../mobius_dll.h"

//#include "../../Modules/SolarRadiation.h"
#include "../../Modules/PET.h"
#include "../../Modules/Simply/SimplySnow.h"
#define SIMPLYQ_GROUNDWATER          
#include "../../Modules/Simply/SimplyQ.h"
//#include "../../Modules/SimplyQ_Langtjern.h"

#include "../../Modules/Simply/SimplyN.h"
#include "../../Modules/SimplySoilTemperature.h"


mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("SimplyN", true);
	
	//AddMaxSolarRadiationModule(Model);
	//AddPriestleyTaylorPETModule2(Model);
	
	AddDegreeDayPETModule(Model);
	AddSimplySnowModule(Model);
	AddSimplyHydrologyModule(Model);
	AddSoilTemperatureModel(Model);
	AddSimplyNModel(Model);
	
	return Model;
}
