

static void
AddHBVSnowModule(mobius_model *Model)
{
	BeginModule(Model, "HBV-Snow", "0.0");
	
	SetModuleDescription(Model, R""""(
This is an adaption of the hydrology module from HBV-Nordic (TODO: add reference)
)"""");

	auto Mm                = RegisterUnit(Model, "mm");
	auto MmPerDay          = RegisterUnit(Model, "mm/day");
	auto DegC              = RegisterUnit(Model, "°C");
	auto MmPerDegreePerDay = RegisterUnit(Model, "mm/°C/day");
	auto Dimensionless     = RegisterUnit(Model);
	
	
	auto Precipitation  = RegisterInput(Model, "Precipitation", MmPerDay);
	auto AirTemperature = RegisterInput(Model, "Air temperature", DegC);
	
	auto SnowPar        = RegisterParameterGroup(Model, "Snow");
	
	auto SnowFallTemp        = RegisterParameterDouble(Model, SnowPar, "Temperature below which precipitation falls as snow", DegC, 0.0, -4.0, 4.0);
	auto SnowMeltTemp        = RegisterParameterDouble(Model, SnowPar, "Temperature above which snow melts", DegC, 0.0, -4.0, 4.0);
	auto DegreeDaySnowMelt   = RegisterParameterDouble(Model, SnowPar, "Degree-day factor for snow melt", MmPerDegreePerDay, 2.74, 0.0, 5.0, "", "DDFmelt");
	auto LiquidWaterFraction = RegisterParameterDouble(Model, SnowPar, "Liquid water fraction", Dimensionless, 0.1, 0.0, 1.0, "Amount of melt water each unit of snow can hold before it is released");
	auto RefreezeEfficiency  = RegisterParameterDouble(Model, SnowPar, "Refreeze efficiency", Dimensionless, 0.5, 0.0, 1.0);
	auto InitialSnowDepth    = RegisterParameterDouble(Model, SnowPar, "Initial snow depth as water equivalent", Mm, 0.0, 0.0, 50000.0);
	
	
	auto PrecipFallingAsRain = RegisterEquation(Model, "Precipitation falling as rain", MmPerDay);
	auto PrecipFallingAsSnow = RegisterEquation(Model, "Precipitation falling as snow", MmPerDay);
	
	auto SnowMelt            = RegisterEquation(Model, "Snow melt", MmPerDay);
	auto Refreeze            = RegisterEquation(Model, "Refreeze", MmPerDay);
	auto MeltWaterToSoil     = RegisterEquation(Model, "Melt water to soil", MmPerDay);
	auto HydrolInputToSoil   = RegisterEquation(Model, "Hydrological input to soil box", MmPerDay);
	
	auto WaterInSnow         = RegisterEquation(Model, "Water in snow", Mm);
	auto SnowDepth           = RegisterEquation(Model, "Snow depth as water equivalent", Mm);
	SetInitialValue(Model, SnowDepth, InitialSnowDepth);
	SetInitialValue(Model, WaterInSnow, 0.0);
	
	
	
	EQUATION(Model, PrecipFallingAsRain,
		double pr = INPUT(Precipitation);
		if(INPUT(AirTemperature) <= PARAMETER(SnowFallTemp))
			return 0.0;
		return pr;
	)
	
	EQUATION(Model, PrecipFallingAsSnow,
		double pr = INPUT(Precipitation);
		if(INPUT(AirTemperature) > PARAMETER(SnowFallTemp))
			return 0.0;
		return pr;
	)
	
	EQUATION(Model, SnowMelt,
		double snow = LAST_RESULT(SnowDepth);
		double ddf  = PARAMETER(DegreeDaySnowMelt);   // TODO: Could be made more complex
		double potential_melt = (INPUT(AirTemperature) - PARAMETER(SnowMeltTemp))*ddf;
		potential_melt = std::max(potential_melt, 0.0);
		potential_melt = std::min(potential_melt, snow);
		return potential_melt;
	)
	
	EQUATION(Model, Refreeze,
		double snow_water = LAST_RESULT(WaterInSnow);
		double ddf = PARAMETER(DegreeDaySnowMelt) * PARAMETER(RefreezeEfficiency);  // Should degree-day be split in the more advanced version here too?
		double potential_refreeze = (PARAMETER(SnowMeltTemp) - INPUT(AirTemperature))*ddf;
		potential_refreeze = std::max(potential_refreeze, 0.0);
		potential_refreeze = std::min(potential_refreeze, snow_water);
		return potential_refreeze;
	)
	
	EQUATION(Model, MeltWaterToSoil,
		double threshold = RESULT(SnowDepth) * PARAMETER(LiquidWaterFraction);
		double snow_water = LAST_RESULT(WaterInSnow) + RESULT(SnowMelt);
		return std::max(0.0, snow_water - threshold);
	)
	
	EQUATION(Model, SnowDepth,
		return LAST_RESULT(SnowDepth) + RESULT(PrecipFallingAsSnow) + RESULT(Refreeze) - RESULT(SnowMelt);
	)
	
	EQUATION(Model, WaterInSnow,
		return LAST_RESULT(WaterInSnow) + RESULT(SnowMelt) - RESULT(Refreeze) - RESULT(MeltWaterToSoil);
	)
	
	EQUATION(Model, HydrolInputToSoil,
		return RESULT(PrecipFallingAsRain) + RESULT(MeltWaterToSoil);
	)


	EndModule(Model);
}