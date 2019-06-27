

#define MOBIUS_TEST_FOR_NAN 1
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 0

#include "../../mobius.h"

#include "../../Modules/SimplyP.h"

#define READ_PARAMETER_FILE 1 //Read params from file? Or auto-generate using indexers defined below & defaults

int main()
{
	mobius_model *Model = BeginModelDefinition("SimplyP", "0.3");
	
	AddSimplyPHydrologyModule(Model);
	AddSimplyPSedimentModule(Model);
	AddSimplyPPhosphorusModule(Model);
	AddSimplyPInputToWaterBodyModule(Model);
	
	ReadInputDependenciesFromFile(Model, "Tarland/TarlandInputs_TEST.dat"); //NOTE: This has to happen here before EndModelDefinition
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);

#if READ_PARAMETER_FILE == 0
	SetIndexes(DataSet, "Landscape units", {"Arable", "Improved grassland", "Semi-natural"});
	SetBranchIndexes(DataSet, "Reaches", {{"R1",{}}, {"R2", {"R1"}}}  );
	
	AllocateParameterStorage(DataSet);
	WriteParametersToFile(DataSet, "newparams.dat");
#else
	ReadParametersFromFile(DataSet, "Tarland/TarlandParameters_v0-3_TEST.dat");

	ReadInputsFromFile(DataSet, "Tarland/TarlandInputs_TEST.dat");
	
	PrintResultStructure(Model);
	//PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);
	
	
	//SetParameterValue(DataSet, "Timesteps", {}, (u64)1);

	RunModel(DataSet);
#endif

	//PrintResultSeries(DataSet, "Agricultural soil water volume", {"Coull"}, 10); //Print just first 10 values
	//PrintResultSeries(DataSet, "Agricultural soil water flow", {"Coull"}, 10);

	
	//PrintResultSeries(DataSet, "Agricultural soil water EPC0", {"Coull"}, 1000);
	//PrintResultSeries(DataSet, "Agricultural soil labile P mass", {"Coull"}, 1000);
	//PrintResultSeries(DataSet, "Agricultural soil TDP mass", {"Coull"}, 1000);
	
	//PrintResultSeries(Dataset, "Reach flow input", {Coull}, 10);
	//PrintResultSeries(Dataset, "Reach flow (end-of-day)", {Coull}, 10);
	//PrintResultSeries(Dataset, "Reach flow (daily mean, mm/day)", {Coull}, 10);
	//PrintResultSeries(Dataset, "Reach volume", {Coull}, 10);
	
	/*
	DlmWriteResultSeriesToFile(DataSet, "results.dat",
		{"Agricultural soil water volume", "Agricultural soil net P sorption", "Agricultural soil water EPC0", "Agricultural soil labile P mass", "Agricultural soil TDP mass"}, 
		{{"Coull"},                     {"Coull"},                   {"Coull"},                      {"Coull"},                       {"Coull"}},
		'\t'
	);
	*/
}