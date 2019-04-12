



// NOTE: This is an example of a GLUE setup for Persist.


#define MOBIUS_TIMESTEP_VERBOSITY 0
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0

#include "../../mobius.h"

#include "../../Modules/Persist.h"

#define CALIBRATION_PRINT_DEBUG_INFO 1
#define GLUE_MULTITHREAD 0

#include "glue.h"

int main()
{
	const char *ParameterFile = "../../Applications/IncaN/tovdalparametersPersistOnly.dat";
	const char *InputFile     = "../../Applications/IncaN/tovdalinputs.dat";
	
	mobius_model *Model = BeginModelDefinition();
	
	auto Days 	      = RegisterUnit(Model, "days");
	auto System       = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1999-1-1");
	
	AddPersistModel(Model);
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
	
	//NOTE: For model structure as well as parameter values that we are not going to change:
	ReadParametersFromFile(DataSet, ParameterFile);
	ReadInputsFromFile(DataSet, InputFile);
	
	glue_setup Setup;
	glue_results Results;
	
	ReadGLUESetupFromFile(&Setup, "GLUE_setup.dat");
	
	timer RunGlueTimer = BeginTimer();
	RunGLUE(DataSet, &Setup, &Results);
	u64 Ms = GetTimerMilliseconds(&RunGlueTimer);
	
	std::cout << "GLUE finished. Running the model " << Setup.NumRuns << " times with " << Setup.NumThreads << " threads took " << Ms << " milliseconds." << std::endl;
	

	WriteGLUEResultsToDatabase("GLUE_results.db", &Setup, &Results, DataSet);
}