// Consider replacing this entirely with something along the lines of:
// https://www.int-res.com/articles/cr/2/c002p183.pdf


#if !defined(SOIL_TEMPERATURE_MODEL_H)

static void
AddSoilTemperatureModel(mobius_model *Model)
{
	auto WattsPerMetrePerDegreeCelsius				= RegisterUnit(Model, "W/m/°C");
	auto MegaJoulesPerCubicMetrePerDegreeCelsius	= RegisterUnit(Model, "MJ/m3/°C");
	auto Dimensionless 								= RegisterUnit(Model);
	auto PerCm										= RegisterUnit(Model, "/cm");
	auto Metres										= RegisterUnit(Model, "m");
	auto Cm											= RegisterUnit(Model, "cm");
	auto Seconds									= RegisterUnit(Model, "s");
	auto DegreesCelsius								= RegisterUnit(Model, "°C");
	auto MetresSquaredPerSecondEMinus6				= RegisterUnit(Model, "1E-6 m2/s");
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units"); //NOTE: This must then be declared elsewhere
	
	// Soil temp params which are constant over land use or soil type
	auto SoilTempParamsGlobal = RegisterParameterGroup(Model, "Global soil temperature parameters");
	
	// To do: need this parameter?
	auto SnowDepthSoilTemperatureFactor	    = RegisterParameterDouble(Model, SoilTempParamsGlobal, "Snow depth / soil temperature factor", PerCm, -0.2, -3.0, -0.001, "Defines empirical relationship between snow depth and its insulating effect on soils, incorporating the conversion from snow depth in water equivalent to snow depth");
	// Note: Compared to orginal Rankinen et al. 2004 model, have grouped together the SnowDepthSoilTemperatureFactor together with
	// the snow water equivalent factor (as these two are just multiplied anyway, and one is a tuning param, so may as well incorporate the other)
	// Have adjusted the default, min and max param values accordingly.
	
	// Decided to fix this
//	auto SoilDepth							= RegisterParameterDouble(Model, SoilTempParamsGlobal, "Soil depth at which soil temperature is estimated", Metres, 0.2);
	
	// To do: empirical relationship between air T and soil T to give rule of thumb for ratio between the two, then remove this param and calculate within model
	auto InitialSoilTemperature		    	= RegisterParameterDouble(Model, SoilTempParamsGlobal, "Initial soil temperature", DegreesCelsius, 10.0, -30.0, 40.0);
	
	// Soil temp params which may vary with land use or soil type
	
	// To do: explore whether extra FreezeThaw parameter adds any explanatory power. Remove?
	auto SoilTempParamsLand = RegisterParameterGroup(Model, "Soil temperature parameters (varying by soil or land class)", LandscapeUnits);
	
	// 3 params used in original module:
	//auto ThermalConductivitySoil 			= RegisterParameterDouble(Model, SoilTempParamsLand, "Thermal conductivity of soil", WattsPerMetrePerDegreeCelsius,	0.7);
	//auto SpecificHeatCapacityFreezeThaw	    = RegisterParameterDouble(Model, SoilTempParamsLand, "Specific heat capacity due to freeze/thaw", MegaJoulesPerCubicMetrePerDegreeCelsius, 6.6, 1.0, 200.0, "Controls the energy released when water is frozen and energy consumed under melting");
	//auto SpecificHeatCapacitySoil			= RegisterParameterDouble(Model, SoilTempParamsLand, "Specific heat capacity of soil", MegaJoulesPerCubicMetrePerDegreeCelsius, 1.1);
	
	// Replaced with:
	auto SoilThermalConductivityOverHeatCapacity = RegisterParameterDouble(Model, SoilTempParamsLand, "Soil thermal conductivity (W/m/°C, range 0.4-0.8) divided by soil specific heat capacity (MJ/m3/°C, range 1.0-1.3; more like 4-15 if there is freeze-thaw)", MetresSquaredPerSecondEMinus6, 0.4, 0.01, 0.8);
	
	// Inputs
	auto AirTemperature = GetInputHandle(Model, "Air temperature");  //NOTE: This should be declared by the main model
	
	// Equations
	auto COUPSoilTemperature = RegisterEquation(Model, "COUP soil temperature", DegreesCelsius);
	SetInitialValue(Model, COUPSoilTemperature, InitialSoilTemperature);
	auto SoilTemperature     = RegisterEquation(Model, "Soil temperature corrected for insulating effect of snow",      DegreesCelsius);
	auto SnowAsWaterEquivalent = GetEquationHandle(Model, "Snow depth as water equivalent");

	
	EQUATION(Model, COUPSoilTemperature,
		auto Da = (1/1000000.0) * PARAMETER(SoilThermalConductivityOverHeatCapacity);
		return LAST_RESULT(COUPSoilTemperature)
			+ 86400.0
				* Da / Square( 2.0 * 0.15) //0.15m is the soil depth temp is estimated at. Decided to fix
				* ( INPUT(AirTemperature) - LAST_RESULT(COUPSoilTemperature) );
	)
	
	EQUATION(Model, SoilTemperature,
		return RESULT(COUPSoilTemperature)
			* std::exp(PARAMETER(SnowDepthSoilTemperatureFactor) * (RESULT(SnowAsWaterEquivalent)/10.0));
	)

}

#define SOIL_TEMPERATURE_MODEL_H
#endif