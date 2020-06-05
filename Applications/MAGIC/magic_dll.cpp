

#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius_dll.h"

#include "../../Modules/MAGIC/MAGIC_Core_Wrapper.h"
#include "../../Modules/MAGIC/MAGICBasic.h"
#include "../../Modules/MAGIC/MAGIC_CarbonNitrogen.h"


void
DllBuildModel(mobius_model *Model)
{
	Model->Name = "MAGIC";
	
	AddMagicCoreModel(Model);
	AddMagicModel(Model);
	AddSimpleMagicCarbonNitrogenModel(Model);

	SetTimestepSize(Model, "1M");
}
