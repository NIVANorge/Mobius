#include "../../mobius.h"
#include "tutorial2model.h"

int main()
{
	mobius_model *Model = BeginModelDefinition();
	
	auto System = RegisterParameterGroup(Model, "System");

	auto Days = RegisterUnit(Model, "days");

	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	RegisterParameterDate(Model, System, "Start date", "1980-1-1");
	
	AddTutorial2Model(Model); //NOTE: We have put all of the model definition in the file tutorial2model.h
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
	
	//NOTE: It is useful to run the following commented-out code the first time you set up the model to autogenerate a parameter file so that you don't have to type all of it yourself:
/*
	SetIndexes(DataSet, "Landscape units", {"Forest", "Peatland"});
	SetIndexes(DataSet, "Soil boxes", {"Upper", "Lower"});
	AllocateParameterStorage(DataSet);
	WriteParametersToFile(DataSet, "newparameters.dat");
*/
	
	ReadParametersFromFile(DataSet, "parameters.dat");
	ReadInputsFromFile(DataSet, "inputs.dat"); //Input files always have to be read after parameter files since the indexes of the index sets are set in the parameter file, and the inputs could potentially depend on the index sets (though they don't in this particular example).
	
	PrintResultStructure(Model); //NOTE: This tells you the order of execution for the equations as well as which index sets they index over. This is very useful for debugging when building a new model, and it tells you in which order you should put indexes when extracting multi-indexed result values.
	
	RunModel(DataSet);
	
	u64 Timesteps = GetParameterUInt(DataSet, "Timesteps", {});
	
	PrintResultSeries(DataSet, "A times X plus B", {"Forest"}, Timesteps);
	PrintResultSeries(DataSet, "A times X plus B", {"Peatland"}, Timesteps);
	
	PrintResultSeries(DataSet, "Another equation", {"Forest", "Upper"}, Timesteps); // "Another equation" indexes over both Landscape units and Soil boxes.
	PrintResultSeries(DataSet, "Another equation", {"Forest", "Lower"}, Timesteps);
	PrintResultSeries(DataSet, "Another equation", {"Peatland", "Upper"}, Timesteps);
	PrintResultSeries(DataSet, "Another equation", {"Peatland", "Lower"}, Timesteps);
}