



static void
AddWASMODModel(mobius_model *Model)
{
	BeginModule(Model, "WASMOD", "0.1");
	
	SetModuleDescription(Model, R""""(
This is (going to be) an implementation of the WASMOD model ("the Water And Snow balance MODeling system"). It is a simple monthly-timestep hydrology model.

This is based on the version that was published as a part of NOPEX:
[^ https://doi.org/10.2166/nh.1998.19^ C-Y Xu and Sven Halldin (1997). The Effect of Climate Change on River Flow and Snow Cover in the NOPEX Area Simulated by a Simple Water Balance Model. Hydrology Research 28 (4-5), pp. 273–282.
)"""");

	
	
	auto DegreesCelsius = RegisterUnit(Model, "°C");
	auto Mm             = RegisterUnit(Model, "mm");
	auto MmPerTs        = RegisterUnit(Model, "mm/month");
	
	
	auto WASMODPars     = RegisterParameterGroup(Model, "WASMOD");
	
	auto SnowMeltsAbove = RegisterParameterDouble(Model, WASMODPars, "Snow melts above this temperature", DegreesCelsius, 0.0, -4.0, 4.0, "", "a2");
	auto SnowFallsBelow = RegisterParameterDouble(Model, WASMODPars, "Precipitation falls as snow below this temperature", DegreesCelsius, 0.0, -4.0, 4.0, "Must be higher than the snow melt threshold", "a1");
	
	auto Precipitation  = RegisterInput(Model, "Precipitation", MmPerTs);
	auto AirTemperature = RegisterInput(Model, "Air temperature", DegreesCelsius);
	
	auto SnowFall       = RegisterEquation(Model, "Snow fall", MmPerTs);
	auto RainFall       = RegisterEquation(Model, "Rain fall", MmPerTs);
	auto SnowMelt       = RegisterEquation(Model, "Snow melt", MmPerTs);
	auto SnowPack       = RegisterEquation(Model, "Snow pack (water equivalents)", Mm);
	auto FlowInput      = RegisterEquation(Model, "Water input to soil", MmPerTs);
	
	
	EQUATION(Model, SnowFall,
		double excess_temp = INPUT(AirTemperature) - PARAMETER(SnowFallsBelow);
		excess_temp = Min(excess_temp, 0.0);
		double rootexponent = excess_temp / (PARAMETER(SnowFallsBelow) - PARAMETER(SnowMeltsAbove));
		double fraction = 1.0 - std::exp(-rootexponent*rootexponent);    //NOTE: The publications don't have a minus in the exponent, but the model doesn't work without it!
		//if(CURRENT_TIME().Year == 1980) WarningPrint("rootexp ", rootexponent, " fraction ", fraction, '\n');
		return INPUT(Precipitation)*Max(0.0, fraction); 
	)
	
	EQUATION(Model, RainFall,
		return INPUT(Precipitation) - RESULT(SnowFall);
	)
	
	EQUATION(Model, SnowMelt,
		double excess_temp = INPUT(AirTemperature) - PARAMETER(SnowMeltsAbove);
		excess_temp = Max(0.0, excess_temp);
		double rootexponent = excess_temp / (PARAMETER(SnowFallsBelow) - PARAMETER(SnowMeltsAbove));
		double fraction = 1.0 - std::exp(-rootexponent*rootexponent);
		return LAST_RESULT(SnowPack)*Max(0.0, fraction); 
	)
	
	EQUATION(Model, SnowPack,
		return LAST_RESULT(SnowPack) + RESULT(SnowFall) - RESULT(SnowMelt);
	)
	
	EQUATION(Model, FlowInput,
		return RESULT(RainFall) + LAST_RESULT(SnowMelt);
	)


	EndModule(Model);
}