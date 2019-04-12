

//NOTE: This is an example of how to use the optimizer with the Persist model.


#include "../../mobius.h"
#include "../../Modules/Persist.h"

#define CALIBRATION_PRINT_DEBUG_INFO 1

#include "optimizer.h"

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
	
	optimization_setup Setup;
	
	ReadOptimizationSetup(&Setup, "optimization_setup.dat");
	
	auto Result = RunOptimizer(DataSet, &Setup);
	
	std::cout << std::endl;
	PrintOptimizationResult(&Setup, Result);
	
	WriteOptimalParametersToDataSet(DataSet, &Setup, Result);
	
	WriteParametersToFile(DataSet, "optimal_parameters.dat");
}