

#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 1

#include "../../mobius.h"


#define SIMPLYQ_GROUNDWATER    //NOTE: #defining this before the inclusion of the SimplyQ.h file turns on groundwater in SimplyQ.

#include "../../Modules/SimplyQ.h"

#define READ_PARAMETER_FILE 1 //Read params from file? Or auto-generate using indexers defined below & defaults

int main()
{
	mobius_model *Model = BeginModelDefinition("SimplyQ");
	
	AddSimplyHydrologyModule(Model);
	
	ReadInputDependenciesFromFile(Model, "tarlandinputs.dat"); //NOTE: This has to happen here before EndModelDefinition
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);

#if READ_PARAMETER_FILE == 0
	SetIndexes(DataSet, "Landscape units", {"Arable", "Improved grassland", "Semi-natural"});
	SetBranchIndexes(DataSet, "Reaches", {  {"Tarland1", {}} }  );
	
	AllocateParameterStorage(DataSet);
	WriteParametersToFile(DataSet, "newparams.dat");
#else
	ReadParametersFromFile(DataSet, "testparameters.dat");

	ReadInputsFromFile(DataSet, "tarlandinputs.dat");
	
	PrintResultStructure(Model);
	PrintParameterStorageStructure(DataSet);
	PrintInputStorageStructure(DataSet);
	
	
	//SetParameterValue(DataSet, "Timesteps", {}, (u64)1);

	RunModel(DataSet);
#endif

	//PrintResultSeries(DataSet, "Soil water volume", {"Tarland1"}, 10); //Print just first 10 values
	//PrintResultSeries(DataSet, "Soil water flow", {"Tarland1"}, 10);
	
	/*
	DlmWriteResultSeriesToFile(DataSet, "results.dat",
		{"Soil water volume"}, 
		{{"Tarland1"},                     {"Tarland1"},                   {"Tarland1"},                      {"Tarland1"},                       {"Tarland1"}},
		'\t'
	);
	*/
}