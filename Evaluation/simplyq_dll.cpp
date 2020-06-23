

//NOTE: This is for compiling the evaluation copy of SimplyQ to a dll that can be tested along with the python version of the hardcoded simplyq.



#include "../mobius_dll.h"

#define SIMPLYQ_GROUNDWATER    //NOTE: #defining this before the inclusion of the SimplyQ.h file turns on groundwater in SimplyQ.

#include "SimplyQ_eval_copy.h"



mobius_model *
DllBuildModel()
{
	mobius_model *Model = BeginModelDefinition("SimplyQ");
	
	AddSimplyHydrologyModule(Model);

	return Model;
}
