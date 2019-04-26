#define MOBIUS_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define MOBIUS_TEST_FOR_NAN 1
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../mobius.h"

#include "../../Modules/Persist.h"
#include "../../Modules/SoilTemperature.h"
#include "../../Modules/WaterTemperature.h"
#include "../../Modules/SolarRadiation.h"
#include "../../Modules/INCA-Sed.h"
#include "../../Modules/INCA-P.h"

#include "../../incaview_compatibility.h"

int main(int argc, char **argv)
{
	incaview_commandline_arguments Args;
	ParseIncaviewCommandline(argc, argv, &Args);
	
	mobius_model *Model = BeginModelDefinition("INCA-P", "0.0");
	
	auto Days   = RegisterUnit(Model, "days");
	auto System = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 10957);
	RegisterParameterDate(Model, System, "Start date", "1981-1-1");
	
	AddPersistModel(Model);
	AddSoilTemperatureModel(Model);
	AddWaterTemperatureModel(Model);
	AddSolarRadiationModule(Model);
	AddINCASedModel(Model);
	AddINCAPModel(Model);
	
	EnsureModelComplianceWithIncaviewCommandline(Model, &Args);
	
	EndModelDefinition(Model);
	
	//PrintResultStructure(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
	
	RunDatasetAsSpecifiedByIncaviewCommandline(DataSet, &Args);
}