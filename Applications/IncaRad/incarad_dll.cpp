

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius_dll.h"

#include "../../Modules/INCA/Persist.h"
#include "../../Modules/INCA/INCA-Sed.h"
#include "../../Modules/SoilTemperature.h"
#include "../../Modules/WaterTemperature.h"
#include "../../Modules/INCA/INCA-Tox-C.h"
#include "../../Modules/INCA/INCA-Rad.h"


mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("INCA-Rad");
	
	AddPersistModel(Model);
	AddINCASedModel(Model);
	AddSoilTemperatureModel(Model);
	AddWaterTemperatureModel(Model);
	AddIncaToxDOCModule(Model);
	AddIncaRadModule(Model);
	
	return Model;
}
