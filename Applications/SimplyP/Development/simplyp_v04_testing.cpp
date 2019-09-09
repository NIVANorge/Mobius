

#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 1
#define MOBIUS_PRINT_TIMING_INFO 1
#define MOBIUS_INDEX_BOUNDS_TESTS 0

#include "../../../mobius.h"

#define SIMPLYQ_GROUNDWATER
#include "../../../Modules/SimplyQ.h"
#include "../../../Modules/SimplySed.h"
#include "../../../Modules/SimplyP_v04.h"

int main()
{
	mobius_model *Model = BeginModelDefinition("SimplyP", "0.3");
	
	AddSimplyHydrologyModule(Model);
	AddSimplySedimentModule(Model);
	AddSimplyPModel(Model);
	
	ReadInputDependenciesFromFile(Model, "../Tarland/TarlandInputs.dat"); //NOTE: This has to happen here before EndModelDefinition
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);


	ReadParametersFromFile(DataSet, "../Tarland/TarlandParameters_v0-4.dat");

	ReadInputsFromFile(DataSet, "../Tarland/TarlandInputs.dat");
	
	//PrintResultStructure(Model);
	//PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);
	

	RunModel(DataSet);

}