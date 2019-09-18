#include "../../mobius.h"
#include "GibletThrasherModel.h"

int main()
{
	mobius_model *Model = BeginModelDefinition("Giblet-Thrasher model");  //You can name the model if you want to.
	
	AddGibletThrasherModel(Model);
	
	//NOTE: Check the input file to see what index sets the input series depend on.
	ReadInputDependenciesFromFile(Model, "inputs.dat"); //Unfortunately we have to call this before EndModelDefinition, because after that the model structure is frozen.
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
	
/*
	SetIndexes(DataSet, "Geographical location", {"Plateau", "Forest", "Nasty bogs"});
	AllocateParameterStorage(DataSet);
	WriteParametersToFile(DataSet, "newparameters.dat");
*/
	
	ReadParametersFromFile(DataSet, "parameters.dat");
	ReadInputsFromFile(DataSet, "inputs.dat");
	
	PrintResultStructure(Model);
	
	RunModel(DataSet);
	
	u64 Timesteps = GetParameterUInt(DataSet, "Timesteps", {});
	
	//NOTE: Writes the result series to a (tab delimited) csv file. It should be easy to load the file from python or matlab to plot the series.
	DlmWriteResultSeriesToFile(DataSet, "results.dat", 
		{"Giblet population", "Giblet population", "Giblet population", "Thrasher population", "Thrasher population", "Thrasher population"}, 
		{{"Plateau"},         {"Forest"},          {"Nasty bogs"},      {"Plateau"},           {"Forest"},            {"Nasty bogs"}},
		'\t'
	);
	
	//NOTE: Check the first 100 migration sums just to see that they are 0 (up to some numerical error) and no giblets are destroyed or created by migration.
	std::vector<double> NetMigration;
	NetMigration.resize(100);
	GetResultSeries(DataSet, "Net giblet migration", {}, NetMigration.data(), NetMigration.size());
	for(double D : NetMigration)
	{
		if(std::abs(D) > 1e-6)
		{
			std::cout << "Something went wrong with the migration setup." << std::endl;
			break;
		}
	}
}