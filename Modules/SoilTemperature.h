

#if !defined(SOIL_TEMPERATURE_MODEL_H)

static void
AddSoilTemperatureModel(mobius_model *Model)
{
	BeginModule(Model, "INCA Soil temperature", "1.0");
	SetModuleDescription(Model, R"DESC(
This is an implementation of the soil temperature model developed for the INCA models in	
Rankinen K. T. Karvonen and D. Butterfield (2004), A simple model for predicting soil temperature in snow covered and seasonally frozen soil; Model description and testing, Hydrol. Earth Syst. Sci., 8, 706-716
)DESC");
	
	auto WattsPerMetrePerDegreeCelsius				= RegisterUnit(Model, "W/m/°C");
	auto MegaJoulesPerCubicMetrePerDegreeCelsius	= RegisterUnit(Model, "MJ/m3/°C");
	auto Dimensionless 								= RegisterUnit(Model);
	auto PerCm										= RegisterUnit(Model, "/cm");
	auto Metres										= RegisterUnit(Model, "m");
	auto Cm											= RegisterUnit(Model, "cm");
	auto Seconds									= RegisterUnit(Model, "s");
	auto DegreesCelsius								= RegisterUnit(Model, "°C");
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto SoilTemp = RegisterParameterGroup(Model, "Soil temperature", LandscapeUnits);
	
	auto ThermalConductivitySoil 			= RegisterParameterDouble(Model, SoilTemp, "Thermal conductivity of soil", 				WattsPerMetrePerDegreeCelsius,			 0.7, 0.1, 2.0);
	auto SpecificHeatCapacityFreezeThaw	    = RegisterParameterDouble(Model, SoilTemp, "Specific heat capacity due to freeze/thaw", MegaJoulesPerCubicMetrePerDegreeCelsius, 6.6, 1.0, 200.0, "Controls the energy released when water is frozen and energy consumed under melting");
	auto WaterEquivalentFactor	            = RegisterParameterDouble(Model, SoilTemp, "Water equivalent factor", 	   Dimensionless, 	  0.3,  0.01,   1.0, "1mm of snow would produce this amount of water when melted");
	auto SnowDepthSoilTemperatureFactor	    = RegisterParameterDouble(Model, SoilTemp, "Snow depth / soil temperature factor", 		PerCm, 								    -0.02, -0.03, -0.001, "Defines an empirical relationship between snow pack depth and its insulating effect on soils");
	auto SpecificHeatCapacitySoil			= RegisterParameterDouble(Model, SoilTemp, "Specific heat capacity of soil",			MegaJoulesPerCubicMetrePerDegreeCelsius, 1.1, 0.5, 2.0);
	auto SoilDepth							= RegisterParameterDouble(Model, SoilTemp, "Soil depth",								Metres,									 0.2, 0.0, 20.0, "Depth at which soil temperature is predicted.");
	auto InitialSoilTemperature		    	= RegisterParameterDouble(Model, SoilTemp, "Initial soil temperature",					DegreesCelsius,							20.0, -30.0, 50.0);
	
	auto AirTemperature = GetInputHandle(Model, "Air temperature");  //NOTE: This should be declared by the main model
	
	auto Da                  = RegisterEquation(Model, "Da",                    Dimensionless);
	auto COUPSoilTemperature = RegisterEquation(Model, "COUP soil temperature", DegreesCelsius);
	SetInitialValue(Model, COUPSoilTemperature, InitialSoilTemperature);
	auto SoilTemperature     = RegisterEquation(Model, "Soil temperature",      DegreesCelsius);
	auto SnowDepth = RegisterEquation(Model, "Snow depth", Cm);
	
	auto SnowAsWaterEquivalent = GetEquationHandle(Model, "Snow depth as water equivalent");
	
	EQUATION(Model, Da,
		if ( LAST_RESULT(COUPSoilTemperature) > 0.0)
		{
			return PARAMETER(ThermalConductivitySoil)
					/ ( 1000000.0 * PARAMETER(SpecificHeatCapacitySoil) );
		}

		return PARAMETER(ThermalConductivitySoil)
				/ ( 1000000.0 * ( PARAMETER(SpecificHeatCapacitySoil) + PARAMETER(SpecificHeatCapacityFreezeThaw) ) );
	)
	
	EQUATION(Model, COUPSoilTemperature,
		return LAST_RESULT(COUPSoilTemperature)
			+ 86400.0
				* RESULT(Da) / Square( 2.0 * PARAMETER(SoilDepth))
				* ( INPUT(AirTemperature) - LAST_RESULT(COUPSoilTemperature) );
	)
	
	EQUATION(Model, SoilTemperature,
		return RESULT(COUPSoilTemperature)
			* std::exp(PARAMETER(SnowDepthSoilTemperatureFactor) * RESULT(SnowDepth));
	)
	
	EQUATION(Model, SnowDepth,
		return RESULT(SnowAsWaterEquivalent)
					/ 10.0 / PARAMETER(WaterEquivalentFactor);
	)
	
	EndModule(Model);
}

#define SOIL_TEMPERATURE_MODEL_H
#endif