#include "../../mobius_dll.h"

#include "GibletThrasherModel.h"



mobius_model *
DllBuildModel()
{
	//NOTE: This is just an example of creating a model .dll that can be used with the python wrapper or MobiView.
	
	mobius_model *Model = BeginModelDefinition("Giblet-Thrasher model");
	
	AddGibletThrasherModel(Model);
	
	return Model;
}
