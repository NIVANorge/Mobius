
#include "../../mobius.h"
#include "../../Modules/MAGIC/MAGIC_Core.h"



int main()
{
	magic_input Input = {};
	magic_param Param = {};
	magic_output Result = {};
	
	bool IsSoil = true;
	double Conv = 1.0;
	double H_estimate = 1.0;
	double IonicStrength = 0.0;
	
	MagicCore(Input, Param, Result, IsSoil, Conv, H_estimate, IonicStrength);
	
	//TODO: set proper in-values and test out-values
}