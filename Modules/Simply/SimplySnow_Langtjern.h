


static void
AddSimplySnowModule(mobius_model *Model)
{
	
	BeginModule(Model, "SimplySnow (Langtjern)", "0.1");
	
	SetModuleDescription(Model, R""""(
The basic simple snow module factored out from SimplyQ. Development version Langtjern.
)"""");


	auto Mm                = RegisterUnit(Model, "mm");
	auto MmPerDay          = RegisterUnit(Model, "mm/day");
	auto DegreesCelsius    = RegisterUnit(Model, "°C");
	auto MmPerDegreePerDay = RegisterUnit(Model, "mm/°C/day");
	auto Dimensionless     = RegisterUnit(Model);

	// Global snow parameters
	auto Snow = RegisterParameterGroup(Model, "Snow");
	
	auto InitialSnowDepth           = RegisterParameterDouble(Model, Snow, "Initial snow depth as water equivalent", Mm, 0.0, 0.0, 50000.0);
	auto DegreeDayFactorSnowmelt    = RegisterParameterDouble(Model, Snow, "Degree-day factor for snowmelt", MmPerDegreePerDay, 2.74, 0.0, 5.0, "", "DDFmelt");
	auto SnowMeltOffsetTemperature  = RegisterParameterDouble(Model, Snow, "Snow melt offset temperature", DegreesCelsius, 0.0, -4.0, 4.0, "Snow begins melting above this temperature");
	auto TemperatureAtWhichPrecipFallsAsSnow = RegisterParameterDouble(Model, Snow, "Temperature at which precipitation falls as snow", DegreesCelsius, 0.0, -4.0, 4.0, "Precipitation falls as snow below this temperature");
	auto SnowMultiplier             = RegisterParameterDouble(Model, Snow, "Snow fall multiplier", Dimensionless, 1.0, 0.5, 1.5, "Adjustment factor to take into account possible inaccuracies in snow fall measurements");
	//auto UseComplexMeltRate         = RegisterParameterBool(Model, Snow, "Use complex melt rate", false, "Base the melt rate on albedo, radiation and precipitation in addition to temperature");

	// Inputs
	auto Precipitation  = RegisterInput(Model, "Precipitation", MmPerDay);
	auto AirTemperature = RegisterInput(Model, "Air temperature", DegreesCelsius);


	auto PrecipitationFallingAsSnow = RegisterEquation(Model, "Precipitation falling as snow", MmPerDay);
	auto PrecipitationFallingAsRain = RegisterEquation(Model, "Precipitation falling as rain", MmPerDay);
	auto PotentialDailySnowmelt     = RegisterEquation(Model, "Potential daily snowmelt", MmPerDay);
	auto SnowMelt                   = RegisterEquation(Model, "Snow melt", MmPerDay);
	auto SnowDepth                  = RegisterEquation(Model, "Snow depth as water equivalent", Mm);
	SetInitialValue(Model, SnowDepth, InitialSnowDepth);
	auto HydrologicalInputToSoilBox = RegisterEquation(Model, "Hydrological input to soil box", MmPerDay);
	
	EQUATION(Model, PrecipitationFallingAsSnow,
		double precip = INPUT(Precipitation) * PARAMETER(SnowMultiplier);
		return (INPUT(AirTemperature) <= PARAMETER(TemperatureAtWhichPrecipFallsAsSnow)) ? precip : 0.0;
	)
	
	EQUATION(Model, PrecipitationFallingAsRain,
		double precip = INPUT(Precipitation);
		return (INPUT(AirTemperature) > PARAMETER(TemperatureAtWhichPrecipFallsAsSnow)) ? precip : 0.0;
	)
	
	EQUATION(Model, PotentialDailySnowmelt,
		return Max(0.0, PARAMETER(DegreeDayFactorSnowmelt) * (INPUT(AirTemperature) - PARAMETER(SnowMeltOffsetTemperature)));
	)
	
	EQUATION(Model, SnowMelt,
		return Min(LAST_RESULT(SnowDepth), RESULT(PotentialDailySnowmelt));
	)
	
	EQUATION(Model, SnowDepth,
		return LAST_RESULT(SnowDepth) + RESULT(PrecipitationFallingAsSnow) - RESULT(SnowMelt);
	)
	
	EQUATION(Model, HydrologicalInputToSoilBox,
		return RESULT(SnowMelt) + RESULT(PrecipitationFallingAsRain);
	)

}