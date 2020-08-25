
#define MOBIUS_TIMESTEP_VERBOSITY 0
//NOTE: the g++ compiler flag ffast-math will make it so that isnan does not work correctly, so don't use that flag.
#define MOBIUS_TEST_FOR_NAN 0
#define MOBIUS_EQUATION_PROFILING 0
#define MOBIUS_PRINT_TIMING_INFO 1

#include "../../mobius.h"

#include "../../Modules/Persist.h"
#include "../../Modules/INCA-Sed.h"

int main()
{
    const char *InputFile = "tarlandinputs.dat";
	const char *ParameterFile = "tarlandparameters.dat";
	
	
	mobius_model *Model = BeginModelDefinition("INCA-Sed");
	
	AddPersistModel(Model);
	AddINCASedModel(Model);
	
	ReadInputDependenciesFromFile(Model, InputFile);
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
    
	ReadParametersFromFile(DataSet, ParameterFile);
    
	ReadInputsFromFile(DataSet, InputFile);
	
	//WriteParametersToFile(DataSet, "newparams.dat");
	
	PrintResultStructure(Model);
	//PrintParameterStorageStructure(DataSet);
	//PrintInputStorageStructure(DataSet);
	
	
	SetParameterValue(DataSet, "Timesteps", {}, (u64)485);
	SetParameterValue(DataSet, "Start date", {}, "2004-1-1"); //No water input in january, so difficult to see any effect of erosion or splash detachment
    
	RunModel(DataSet);
	
    
	PrintResultSeries(DataSet, "Runoff to reach", {"Tarland", "Arable", "Direct runoff"}, 10);
	PrintResultSeries(DataSet, "Sediment mobilised via splash detachment", {"Tarland", "Arable"}, 10);
	PrintResultSeries(DataSet, "Flow erosion K factor", {"Tarland", "Arable"}, 10);
	PrintResultSeries(DataSet, "Sediment transport capacity", {"Tarland", "Arable"}, 10);
	PrintResultSeries(DataSet, "Sediment mobilised via flow erosion", {"Tarland", "Arable"}, 10);
	PrintResultSeries(DataSet, "Surface sediment store", {"Tarland", "Arable"}, 10);
	PrintResultSeries(DataSet, "Sediment delivery to reach", {"Tarland", "Arable"}, 10);
	
	PrintResultSeries(DataSet, "Total sediment delivery to reach", {"Tarland"}, 10);
	
	std::vector<const char *> SizeIndexes = {"Clay", "Silt", "Fine sand", "Medium sand", "Coarse sand"};
	for(const char *Idx : SizeIndexes)
	{
		PrintResultSeries(DataSet, "Suspended sediment mass", {"Tarland", Idx}, 100);
		PrintResultSeries(DataSet, "Mass of bed sediment per unit area", {"Tarland", Idx}, 100);
	}
}


