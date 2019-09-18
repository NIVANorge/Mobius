#include "../../mobius_dll.h"

#include "GibletThrasherModel.h"



DLLEXPORT void *
DllSetupModel(char *ParameterFilename, char *InputFilename)
{
	//NOTE: This is just an example of creating a model .dll that can be used with the python wrapper or MobiView.
	
	CHECK_ERROR_BEGIN
	
	mobius_model *Model = BeginModelDefinition("Giblet-Thrasher model");
	
	AddGibletThrasherModel(Model);
	
	ReadInputDependenciesFromFile(Model, InputFilename);
	
	EndModelDefinition(Model);
	
	mobius_data_set *DataSet = GenerateDataSet(Model);
	
	ReadParametersFromFile(DataSet, ParameterFilename);
	ReadInputsFromFile(DataSet, InputFilename);
	
	return (void *)DataSet;
	
	CHECK_ERROR_END
	
	return 0;
}
