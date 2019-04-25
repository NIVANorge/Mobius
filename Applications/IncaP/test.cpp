#define MOBIUS_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 1
#define MOBIUS_INDEX_BOUNDS_TESTS 1

#include "../../mobius.h"

#include "../../Modules/Persist.h"
#include "../../Modules/SoilTemperature.h"
#include "../../Modules/WaterTemperature.h"
#include "../../Modules/SolarRadiation.h"
#include "../../Modules/INCA-Sed.h"
#include "../../Modules/INCA-P.h"



int main()
{
    const char *InputFile     = "testinputs.dat";
	const char *ParameterFile = "testparameters.dat";
	
	
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
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
    
	ReadParametersFromFile(DataSet, ParameterFile);
	
	WriteParametersToFile(DataSet, "newparams.dat");

	ReadInputsFromFile(DataSet, InputFile);
	
	PrintResultStructure(Model);
    
	RunModel(DataSet);
}