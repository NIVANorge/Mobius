

#include "../../mobius.h"

int main()
{
	mobius_model *Model = BeginModelDefinition();
	
	//NOTE: Every parameter has to belong to a parameter group.
	auto System = RegisterParameterGroup(Model, "System");
	
	//There are two special parameters "Timesteps" and "Start date" that all models should have.
	//We will eventually add timestep size, but for now that is hard coded to a day.
	
	auto Days = RegisterUnit(Model, "days"); //NOTE: Units have no effect on how the model runs, but they are displayed for instance in the INCAView gui. They are just a note that lets you remember the unit of a parameters or equation when calibrating the model.
	auto Dimensionless = RegisterUnit(Model);
	
	//ParameterUInt is of type (64 bit) unsigned integer, i.e {0, 1, 2, ..., 2^64-1}.
	// The value 100 is the default value, and will be the value of this parameter unless it is overwritten later (usually when loading a parameter file, but in this example we just set parameter values manually below)
	RegisterParameterUInt(Model, System, "Timesteps", Days, 100);
	
	RegisterParameterDate(Model, System, "Start date", "1980-1-1");
	
	//NOTE: An index set is something that allows you to have several instances of the same parameters and equations. Typically the index set is something like "Landscape units", and say you want to have a different "Initial snow depth" parameter per landscape unit. In that case, put it in a parameter group that indexes over the landscape units index set. This will also make the snow depth equation have a separate instance per landscape unit.
	auto Domain = RegisterIndexSet(Model, "Domain"); //Domain is just a dummy name, ignore it.
	
	auto Group = RegisterParameterGroup(Model, "Group", Domain); //NOTE: The group "Group" indexes over the index set "Domain". This means that there will be a separate instance of every parameter in the group for every index in "Domain". The indexes are set later. 
	
	//ParameterDouble will be of type double, i.e. a standard 64 bit floating point number.
	//NOTE: The A that is returned from RegisterParameterDouble below does not itself contain a value, it is just a handle that you use to reference this parameter inside an equation. See below.
	auto A = RegisterParameterDouble(Model, Group, "A", Dimensionless, 1.0);
	auto B = RegisterParameterDouble(Model, Group, "B", Dimensionless, 0.0);
	
	//An input is a timeseries that is typically read in from a file, but in this tutorial we just set it explicitly below.
	auto X = RegisterInput(Model, "X");
	
	//Tell the model that we want an equation named "A times X plus B"
	auto ATimesXPlusB = RegisterEquation(Model, "A times X plus B", Dimensionless);
	
	//Set the equation body of this equation.
	// If there were multiple index sets, you could end up with the equation being evaluated for each combination of indexes over all the index sets it depends on.
	//NOTE: This heavily uses macros. What this expands to is a C++11 lambda function that is added to a vector stored in the model. The model builder should not have to worry about the underlying implementation here.
	//Since this equation references parameters that index over the index set Domain, the equation will automatically be evaluated one time for each index in that index set.
	EQUATION(Model, ATimesXPlusB,
		return PARAMETER(A) * INPUT(X) + PARAMETER(B);
	)
	
	
	//EndModelDefinitiion tells the model that we are finished defining the structure of the model. We can not add any more parameters, equations, index sets etc beyond this point.
	EndModelDefinition(Model);
	
	
	//A data set is a specialisation of a model where one can set the indexes of the index sets and the values of the corresponding instances of all the parameters and inputs.
	mobius_data_set *DataSet = GenerateDataSet(Model);
	
	//All indexes of all index sets have to be set before we specify parameter values or input series.
	SetIndexes(DataSet, "Domain", {"First", "Second"});
	
	u64 DesiredTimesteps = 10; //u64 is the type of our unsigned integers.
	SetParameterValue(DataSet, "Timesteps", {}, DesiredTimesteps); // The {} is because timesteps does not index over any index sets
	
	SetParameterValue(DataSet, "A", {"Second"}, 10.0);
	SetParameterValue(DataSet, "B", {"First"}, 0.0);
	SetParameterValue(DataSet, "B", {"Second"}, 5.0);
	//NOTE: We have not set all the parameter values of all the instances, for example we did not set the value of "A" corresponding to the "First" index. This just means that "A" will get its default value of 1.0 for this index.
	
	// Create a vector of values for the input series "X". Typically these will be read in from a file, but in this first tutorial we just set them manually.
	std::vector<double> XValues;
	XValues.resize(DesiredTimesteps); // You can make the input series longer than the amount of timesteps you want to run for, but not shorter.
	for(u64 I = 0; I < DesiredTimesteps; ++I)
	{
		XValues[I] = (double)I;
	}
	SetInputSeries(DataSet, "X", {}, XValues.data(), XValues.size());
	
	//Run the model with this dataset
	RunModel(DataSet);
	
	// Print the result series of the equation "A times X plus B" that was calculated with parameter values of index "First".
	PrintResultSeries(DataSet, "A times X plus B", {"First"}, DesiredTimesteps);
	// For index "Second":
	PrintResultSeries(DataSet, "A times X plus B", {"Second"}, DesiredTimesteps); //You could put in a number N smaller than DesiredTimesteps here if you just wanted to print the first N values, not all of them.
	
	// Alternatively one can extract result values into a vector and do stuff with it.
	/*
		std::vector<double> MyResults;
		MyResults.resize(DesiredTimesteps);
		GetResultSeries(DataSet, "A times X plus B", {"First"}, MyResults.data(), MyResults.size());
		for(double R : MyResults) std::cout << R << std::endl;
	*/
	
}