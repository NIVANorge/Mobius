

#if !defined(SNOW_MELT_MODEL_H)

static void
AddSnowMeltModel(mobius_model *Model)
{
	auto Mm 						= RegisterUnit(Model, "mm");
	auto MmPerDegreeCelsiusPerDay 	= RegisterUnit(Model, "mm/°C/day");
	auto Dimensionless 				= RegisterUnit(Model);
	auto DegreesCelsius 			= RegisterUnit(Model, "°C");
	auto MmPerDay 					= RegisterUnit(Model, "mm/day");
	auto Cm                         = RegisterUnit(Model, "cm");
	
	auto LandscapeUnits = RegisterIndexSet(Model, "Landscape units");
	auto Land = RegisterParameterGroup(Model, "Landscape units", LandscapeUnits);
	
	auto InitialSnowPackDepth     = RegisterParameterDouble(Model, Land, "Initial snow pack depth",                           Mm,                       0.0);
	auto DegreeDayFactorSnowmelt  = RegisterParameterDouble(Model, Land, "Degree-day factor for snowmelt",                    MmPerDegreeCelsiusPerDay, 0.0);
	auto TempBelowAllPrecipSnow   = RegisterParameterDouble(Model, Land, "Temperature below which all precipitation is snow", Mm,                      -1.5);
	auto TempAboveAllPrecipRain   = RegisterParameterDouble(Model, Land, "Temperature above which all precipitation is rain", DegreesCelsius,           1.0);
	auto SnowfallCorrectionFactor = RegisterParameterDouble(Model, Land, "Snowfall correction factor",                        Dimensionless,            1.23);
	auto RainfallCorrectionFactor = RegisterParameterDouble(Model, Land, "Rainfall correction factor",                        Dimensionless,            1.08);
	auto TempAtWhichSnowMelts     = RegisterParameterDouble(Model, Land, "Temperature at which snow melts",                   DegreesCelsius,           0.5);
	auto EvaporationFromSnow      = RegisterParameterDouble(Model, Land, "Evaporation from snow",                             MmPerDay,                 0.09);
    
	auto ActualPrecipitation = RegisterInput(Model, "Actual precipitation");
	auto AirTemperature      = RegisterInput(Model, "Air temperature");
    
	auto PrecipitationFallingAsSnow = RegisterEquation(Model, "Precipitation falling as snow", MmPerDay);
	auto PrecipitationFallingAsRain = RegisterEquation(Model, "Precipitation falling as rain", MmPerDay);
	auto SnowPack                   = RegisterEquation(Model, "Snow pack",                     Mm);
	auto SnowMelt                   = RegisterEquation(Model, "Snow melt",                     MmPerDay);
	auto SnowPackWithMelt           = RegisterEquation(Model, "Snow pack with melt",           Mm);
	auto SnowEvaporation            = RegisterEquation(Model, "Snow evaporation",              MmPerDay);
	auto SnowAsWaterEquivalent      = RegisterEquation(Model, "Snow depth as water equivalent",      Mm);
	SetInitialValue(Model, SnowAsWaterEquivalent, InitialSnowPackDepth);
	
	EQUATION(Model, PrecipitationFallingAsSnow,
		double Snow = 0.0;
		double tempPrecipRain = PARAMETER(TempAboveAllPrecipRain);
		double tempPrecipSnow = PARAMETER(TempBelowAllPrecipSnow);
		double precip = INPUT(ActualPrecipitation);
		
		if ( INPUT(AirTemperature) < tempPrecipSnow )
		{
			Snow = precip;
		}
		else if ( INPUT(AirTemperature) < tempPrecipRain )
		{
			Snow = precip
                * ( tempPrecipRain - INPUT(AirTemperature) )
                / ( tempPrecipRain - tempPrecipSnow );
		}
        
		return Snow * PARAMETER(SnowfallCorrectionFactor);
	)

	EQUATION(Model, PrecipitationFallingAsRain,
		return ( INPUT(ActualPrecipitation) - RESULT(PrecipitationFallingAsSnow) ) * PARAMETER(RainfallCorrectionFactor);
	)
    
	EQUATION(Model, SnowPack,
		return LAST_RESULT(SnowAsWaterEquivalent) + RESULT(PrecipitationFallingAsSnow);
	)
	
	EQUATION(Model, SnowMelt,
		if ( INPUT(AirTemperature) < PARAMETER(TempAtWhichSnowMelts) ) return 0.0;

		return Min(RESULT(SnowPack),
                   PARAMETER(DegreeDayFactorSnowmelt)
                   * ( INPUT(AirTemperature) - PARAMETER(TempAtWhichSnowMelts) ) );
	)
	
	EQUATION(Model, SnowPackWithMelt,
		return RESULT(SnowPack)	- RESULT(SnowMelt);
	)
	
	EQUATION(Model, SnowEvaporation,
		return Min( RESULT(SnowPackWithMelt), PARAMETER(EvaporationFromSnow) );
	)
	
	EQUATION(Model, SnowAsWaterEquivalent,
		return RESULT(SnowPackWithMelt)
            - RESULT(SnowEvaporation);
	)
    
}



#define SNOW_MELT_MODEL_H
#endif