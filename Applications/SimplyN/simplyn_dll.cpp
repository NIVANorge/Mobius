
#include "../../mobius_dll.h"

#include "../../Modules/UnitConversions.h"

//#include "../../Modules/SolarRadiation.h"
#include "../../Modules/PET.h"
#define SIMPLYQ_GROUNDWATER          
#include "../../Modules/SimplyQ.h"
//#include "../../Modules/SimplyQ_Langtjern.h"


#include "../../Modules/SimplyN.h"
#include "../../Modules/SimplySoilTemperature.h"

#include "../../Modules/SimplyQLakeAddon.h"

mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("SimplyN");
	
	//AddMaxSolarRadiationModule(Model);
	//AddPriestleyTaylorPETModule2(Model);
	
	AddDegreeDayPETModule(Model);
	AddSimplyHydrologyModule(Model);
	//AddSimplyQLakeAddon(Model);
	AddSoilTemperatureModel(Model);
	AddSimplyNModel(Model);
	
	return Model;
}
