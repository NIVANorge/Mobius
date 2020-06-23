

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius_dll.h"

#include "../../Modules/Persist.h"
#include "../../Modules/SoilTemperature.h"
#include "../../Modules/WaterTemperature.h"
#include "../../Modules/INCA-N.h"



mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("INCA-N");
	
	AddPersistModel(Model);
	//RegisterParameterGroup(Model, "Percolation", GetIndexSetHandle(Model, "Reaches")); //NOTE: Uncomment to make percolation matrices be per reach instead of per L.U.
	AddSoilTemperatureModel(Model);
	AddWaterTemperatureModel(Model);
	AddIncaNModel(Model);
	
	return Model;
}
