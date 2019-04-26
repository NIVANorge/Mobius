

#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0

#include "../../mobius.h"

#include "../../Modules/SimplyP.h"

#define READ_PARAMETER_FILE 1 //Read params from file? Or auto-generate using indexers defined below & defaults

int main()
{
	mobius_model *Model = BeginModelDefinition("SimplyP", "0.0");
	
	auto Days 	        = RegisterUnit(Model, "days");
	auto System = RegisterParameterGroup(Model, "System");
	RegisterParameterUInt(Model, System, "Timesteps", Days, 10957);
	RegisterParameterDate(Model, System, "Start date", "1981-1-1");

	AddSimplyPHydrologyModule(Model);
	AddSimplyPSedimentModule(Model);
	AddSimplyPPhosphorusModule(Model);
	AddSimplyPInputToWaterBodyModule(Model);
	
	ReadInputDependenciesFromFile(Model, "Tarland/TarlandInputs.dat"); //NOTE: This has to happen here before EndModelDefinition
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);

#if READ_PARAMETER_FILE == 0
	SetIndexes(DataSet, "Landscape units", {"Arable", "Improved grassland", "Semi-natural"});
	SetBranchIndexes(DataSet, "Reaches", {  {"Tarland1", {}} }  );
	
	AllocateParameterStorage(DataSet);
	WriteParametersToFile(DataSet, "newparams.dat");
#else
	ReadParametersFromFile(DataSet, "Tarland/TarlandParameters.dat");

	ReadInputsFromFile(DataSet, "Tarland/TarlandInputs.dat");
	
	PrintResultStructure(Model);
	//PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);
	
	
	//SetParameterValue(DataSet, "Timesteps", {}, (u64)1);

	RunModel(DataSet);
#endif

	//PrintResultSeries(DataSet, "Agricultural soil water volume", {"Tarland1"}, 10); //Print just first 10 values
	//PrintResultSeries(DataSet, "Agricultural soil water flow", {"Tarland1"}, 10);

	
	//PrintResultSeries(DataSet, "Agricultural soil water EPC0", {"Tarland1"}, 1000);
	//PrintResultSeries(DataSet, "Agricultural soil labile P mass", {"Tarland1"}, 1000);
	//PrintResultSeries(DataSet, "Agricultural soil TDP mass", {"Tarland1"}, 1000);
	
	/*
	DlmWriteResultSeriesToFile(DataSet, "results.dat",
		{"Agricultural soil water volume", "Agricultural soil net P sorption", "Agricultural soil water EPC0", "Agricultural soil labile P mass", "Agricultural soil TDP mass"}, 
		{{"Tarland1"},                     {"Tarland1"},                   {"Tarland1"},                      {"Tarland1"},                       {"Tarland1"}},
		'\t'
	);
	*/
}