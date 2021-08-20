



static void
AddWASMODModel(mobius_model *Model)
{
	BeginModule(Model, "WASMOD", "0.1");
	
	SetModuleDescription(Model, R""""(
This is an implementation of the WASMOD model ("the Water And Snow balance MODeling system"). It is a simple monthly-timestep hydrology model.

This is based on the version that was published as a part of NOPEX:
[C-Y Xu and Sven Halldin (1997). The Effect of Climate Change on River Flow and Snow Cover in the NOPEX Area Simulated by a Simple Water Balance Model. Hydrology Research 28 (4-5), pp. 273–282.](https://doi.org/10.2166/nh.1998.19)
)"""");

	
	auto Dimensionless  = RegisterUnit(Model);
	auto DegreesCelsius = RegisterUnit(Model, "°C");
	auto Mm             = RegisterUnit(Model, "mm");
	auto MmPerTs        = RegisterUnit(Model, "mm/month");
	auto TsPerMm        = RegisterUnit(Model, "month/mm");
	auto PerTs          = RegisterUnit(Model, "1/month");
	
	
	auto WASMODPars     = RegisterParameterGroup(Model, "WASMOD");
	
	auto SnowMeltsAbove = RegisterParameterDouble(Model, WASMODPars, "Snow melts above this temperature", DegreesCelsius, 0.0, -4.0, 4.0, "", "a2");
	auto SnowFallOffset = RegisterParameterDouble(Model, WASMODPars, "Precipitation falls as snow below this temperature", DegreesCelsius, 0.1, 0.001, 10.0, "This is an offset from the snow melt temperature", "a1");
	auto ETDrynessAdjustment = RegisterParameterDouble(Model, WASMODPars, "Evapotranspiration sensitivity to dryness", TsPerMm, 0.1, 0.0, 1000.0);
	auto SlowFlowLinear = RegisterParameterDouble(Model, WASMODPars, "Slow flow linear scaling factor", PerTs, 1.0, 0.0001, 5.0);
	auto SlowFlowExp    = RegisterParameterDouble(Model, WASMODPars, "Slow flow exponent", Dimensionless, 1.0, 0.0, 2.0, "Arid: 0 or 0.5. Otherwise 1 or 2. Not to be adjusted freely in calibration");
	auto FastFlowLinear = RegisterParameterDouble(Model, WASMODPars, "Fast flow linear scaling factor", PerTs, 1.0, 0.0001, 5.0);
	auto FastFlowExp    = RegisterParameterDouble(Model, WASMODPars, "Fast flow exponent", Dimensionless, 1.0, 0.0, 2.0, "1 or 2. Not to be adjusted freely in calibration");
	auto InitialSnowPack = RegisterParameterDouble(Model, WASMODPars, "Initial snow pack", Mm, 0.0, 0.0, 10000.0);
	auto InitialSoilMoisture = RegisterParameterDouble(Model, WASMODPars, "Initial soil moisture", Mm, 10.0, 0.0, 10000.0);
	
	
	auto Precipitation  = RegisterInput(Model, "Precipitation", MmPerTs);
	auto AirTemperature = RegisterInput(Model, "Air temperature", DegreesCelsius);
	
	auto SnowFall       = RegisterEquation(Model, "Snow fall", MmPerTs);
	auto RainFall       = RegisterEquation(Model, "Rain fall", MmPerTs);
	auto SnowMelt       = RegisterEquation(Model, "Snow melt", MmPerTs);
	auto SnowPack       = RegisterEquation(Model, "Snow pack (water equivalents)", Mm);
	
	auto AvailableWater = RegisterEquation(Model, "Available water for evapotranspiration", Mm);
	auto AdjustedPET    = RegisterEquation(Model, "Adjusted potential evapotranspiration", MmPerTs);
	auto EvapoTranspiration = RegisterEquation(Model, "Evapotranspiration", MmPerTs);
	auto SlowFlow       = RegisterEquation(Model, "Slow flow", MmPerTs);
	auto ActiveRainfall = RegisterEquation(Model, "Active rainfall", MmPerTs);
	auto FastFlow       = RegisterEquation(Model, "Fast flow", MmPerTs);
	auto Runoff         = RegisterEquation(Model, "Runoff", MmPerTs);
	auto SoilMoisture   = RegisterEquation(Model, "Soil moisture", Mm);
	
	SetInitialValue(Model, SnowPack, InitialSnowPack);
	SetInitialValue(Model, SoilMoisture, InitialSoilMoisture);
	
	
	EQUATION(Model, SnowFall,
		double excess_temp = INPUT(AirTemperature) - (PARAMETER(SnowMeltsAbove) + PARAMETER(SnowFallOffset));
		excess_temp = std::min(excess_temp, 0.0);
		double rootexponent = excess_temp / PARAMETER(SnowFallOffset);
		double fraction = 1.0 - std::exp(-rootexponent*rootexponent);    //NOTE: The publications don't have a minus in the exponent, but the model doesn't work without it!
		//if(CURRENT_TIME().Year == 1980) WarningPrint("rootexp ", rootexponent, " fraction ", fraction, '\n');
		return INPUT(Precipitation)*Max(0.0, fraction); 
	)
	
	EQUATION(Model, RainFall,
		return INPUT(Precipitation) - RESULT(SnowFall);
	)
	
	EQUATION(Model, SnowMelt,
		double excess_temp = INPUT(AirTemperature) - PARAMETER(SnowMeltsAbove);
		excess_temp = std::max(0.0, excess_temp);
		double rootexponent = excess_temp / PARAMETER(SnowFallOffset);
		double fraction = 1.0 - std::exp(-rootexponent*rootexponent);
		return LAST_RESULT(SnowPack)*std::max(0.0, fraction); 
	)
	
	EQUATION(Model, SnowPack,
		return LAST_RESULT(SnowPack) + RESULT(SnowFall) - RESULT(SnowMelt);
	)

	EQUATION(Model, AdjustedPET,
		return 0.0; //TODO:
	)

	EQUATION(Model, AvailableWater,
		return RESULT(RainFall) + std::max(LAST_RESULT(SoilMoisture), 0.0);
	)
	
	EQUATION(Model, EvapoTranspiration,
		//NOTE: This is one of two possible formulas.
	
		double pet = RESULT(AdjustedPET);
		double w   = RESULT(AvailableWater);
		double a   = PARAMETER(ETDrynessAdjustment);
		return std::min(w * (1.0 - std::exp(-a*pet)), pet);
	)
	
	EQUATION(Model, SlowFlow,
		return PARAMETER(SlowFlowLinear) * std::pow(std::max(LAST_RESULT(SoilMoisture), 0.0), PARAMETER(SlowFlowExp));
	)

	EQUATION(Model, ActiveRainfall,
		double r = RESULT(RainFall);
		double pet = RESULT(AdjustedPET);
		return r - pet*(1.0 - std::exp(-r / std::max(pet, 1.0)));
	)
	
	EQUATION(Model, FastFlow,
		return PARAMETER(FastFlowLinear) * std::pow(std::max(LAST_RESULT(SoilMoisture), 0.0), PARAMETER(FastFlowExp))*(RESULT(SnowMelt) + RESULT(ActiveRainfall));
	)
	
	EQUATION(Model, Runoff,
		return RESULT(FastFlow) + RESULT(SlowFlow);
	)

	EQUATION(Model, SoilMoisture,
		return LAST_RESULT(SoilMoisture) + RESULT(RainFall) + RESULT(SnowMelt) - RESULT(EvapoTranspiration) - RESULT(Runoff);
	)


	EndModule(Model);
}