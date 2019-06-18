#define MOBIUS_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define MOBIUS_TEST_FOR_NAN 0 //0 or 1. Test for NaNs and if find print which equation, indexer and params associated with it. Slows model.
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 1

#include "../../mobius.h"

#include "../../Modules/UnitConversions.h"


#include "../../Modules/SimplyQ.h"
#include "../../Modules/SimplyC.h"
#include "../../Modules/SoilTemperature_simply.h"

#define READ_PARAMETER_FILE 1 //Read params from file? Or auto-generate using indexers defined below & defaults

int main()
{
	mobius_model *Model = BeginModelDefinition("SimplyC", "0.1"); //Name, version
	
	//Call functions declared earlier
	AddSimplyHydrologyModule(Model);
	AddSoilTemperatureModel(Model);
	AddSimplyCModel(Model);
	
	//Input .dat file can say whether inputs vary per reach/landscape unit/are global, etc., which affects eqns
	ReadInputDependenciesFromFile(Model, "../SimplyC/langtjerninputs.dat"); //NOTE: This has to happen here before EndModelDefinition
	
	EndModelDefinition(Model);  //"compiles" model - order eqns, etc.
	
	mobius_data_set *DataSet = GenerateDataSet(Model); //Create specific model dataset objext

#if READ_PARAMETER_FILE == 0 //If don't have param file already, auto-generate param file for you
	SetIndexes(DataSet, "Landscape units", {"Forest and bog"});
	SetBranchIndexes(DataSet, "Reaches", {  {"Inlet", {}} }  );
	
	AllocateParameterStorage(DataSet);
	WriteParametersToFile(DataSet, "new_SimplyC_params.dat");
#else
	ReadParametersFromFile(DataSet, "SimplyC_params_noGW.dat");

	ReadInputsFromFile(DataSet, "../SimplyC/langtjerninputs.dat");
	
	PrintResultStructure(Model);
	PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);
	
	
	//SetParameterValue(DataSet, "Timesteps", {}, (u64)1); //for testing

	RunModel(DataSet);
#endif

	// Can access results by name and indexes. Get this from results structure
//	PrintResultSeries(DataSet, "Soil water volume", {"Inlet"}, 10); //Print just first 10 values
//	PrintResultSeries(DataSet, "Soil water carbon flux", {"Inlet"}, 10);

	
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