

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 1

#include "../../mobius_dll.h"

#include "../../Modules/INCA/Persist.h"
#include "../../Modules/INCA/INCA-Sed.h"
#include "../../Modules/SoilTemperature.h"
#include "../../Modules/WaterTemperature.h"
#include "../../Modules/SolarRadiation.h"
#include "../../Modules/INCA/INCA-Tox-C.h"
#include "../../Modules/INCA/INCA-Tox.h"

#define EASYLAKE_PERSIST
#include "../../Modules/EasyLake/EasyLake.h"
#include "../../Modules/INCA/INCA-Tox-Lake.h"


mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("INCA-Tox + Lake");
	
	AddPersistModel(Model);
	AddINCASedModel(Model);
	AddSoilTemperatureModel(Model);
	AddWaterTemperatureModel(Model);
	AddMaxSolarRadiationModule(Model);
	AddIncaToxDOCModule(Model);
	AddIncaToxModule(Model);
	AddEasyLakePhysicalModule(Model);
	AddIncaToxLakeModule(Model);
	
	return Model;
}
