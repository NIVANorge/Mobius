#include "../../mobius_dll.h"

#include "GibletThrasherModel.h"



void
DllBuildModel(mobius_model *Model)
{
	//NOTE: This is just an example of creating a model .dll that can be used with the python wrapper or MobiView.
	
	Model->Name = "Giblet-Thrasher model";
	
	AddGibletThrasherModel(Model);
}
