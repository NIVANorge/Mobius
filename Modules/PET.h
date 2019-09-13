


// In this file we will put some alternative PET modules that could be used by other models.


static void
AddPriestleyTaylorPET(mobius_model *Model)
{
	auto AirTemperature             = GetInputHandle(Model, "Air temperature");
	auto SnowDepthAsWaterEquivalent = GetResultHandle(Model, "Snow depth as water equivalent");
	
	auto Dimensionless  = RegisterUnit(Model);
	auto M              = RegisterUnit(Model, "m");
	auto kPa            = RegisterUnit(Model, "kPa");
	auto kPaPerDegreesC = RegisterUnit(Model, "kPa/°C");
	auto MJPerM2        = RegisterUnit(Model, "MJ/m2");
	auto MJPerKg        = RegisterUnit(Model, "MJ/kg");
	auto Mm             = RegisterUnit(Model, "mm");
	
	//TODO: Provide ways to estimate these too?
	auto RelativeHumidity     = RegisterInput(Model, "Relative humidity", Dimensionless);
	auto SolarRadiation       = RegisterInput(Model, "Solar radiation", MJPerM2);
	auto MaxPossibleRadiation = RegisterInput(Model, "Max possible radiation", MJPerM2); //TODO: At least this one should be something like what is computed in SolarRadiation.h
	
	auto System                         = GetParameterGroupHandle(Model, "System");
	auto Elevation                      = RegisterParameterDouble(Model, System, "Elevation", M, 0.0, 0.0, 8848.0);
	
	auto LatentHeatOfVaporization       = RegisterEquation(Model, "Latent heat of vaporization", MJPerKg);
	auto PsychrometricConstant          = RegisterEquation(Model, "Psycrhometric constant", kPaPerDegreesC);
	auto SaturationVaporPressure        = RegisterEquation(Model, "Saturation vapor pressure", kPa);
	auto ActualVaporPressure            = RegisterEquation(Model, "Actual vapor pressure", kPa);
	auto SlopeOfSaturationPressureCurve = RegisterEquation(Model, "Slope of saturation pressure curve", kPaPerDegreesC);
	auto NetShortwaveRadiation          = RegisterEquation(Model, "Net short-wave radiation", MJPerM2);
	auto NetEmissivity                  = RegisterEquation(Model, "Net emissivity", Dimensionless);
	auto CloudCoverFactor               = RegisterEquation(Model, "Cloud cover factor", Dimensionless);
	auto NetLongWaveRadiation           = RegisterEquation(Model, "Net long-wave radiation", MJPerM2);
	auto NetRadiation                   = RegisterEquation(Model, "Net radiation", MJPerM2);
	auto PotentialEvapotranspiration    = RegisterEquation(Model, "Potential evapotranspiration", Mm);
	
	EQUATION(Model, LatentHeatOfVaporization,
		return 2.501 - 2.361e-3 * INPUT(AirTemperature);
	)
	
	EQUATION(Model, PsychrometricConstant,
		double meanBarometricPressure = 101.3 - PARAMETER(Elevation)*(0.01152 - 0.544e-6*PARAMETER(Elevation));
		return 1.013e-3 * meanBarometricPressure / (0.622 * RESULT(LatentHeatOfVaporization));
	)
	
	EQUATION(Model, SaturationVaporPressure,
		//Tetens(1930), Murray(1967)
		return std::exp( (16.78*INPUT(AirTemperature) - 116.9) / (INPUT(AirTemperature) + 237.3) );
	)
	
	EQUATION(Model, ActualVaporPressure,
		return INPUT(RelativeHumidity) * RESULT(SaturationVaporPressure);
	)
	
	EQUATION(Model, SlopeOfSaturationPressureCurve,
		double temp = INPUT(AirTemperature) + 237.3;
		return 4098.0 * RESULT(SaturationVaporPressure) / (temp*temp);
	)
	
	EQUATION(Model, NetShortwaveRadiation,
		double sd   = RESULT(SnowDepthAsWaterEquivalent);
		double srad = RESULT(SolarRadiation);
		
		if(sd < 0.5) return srad * (1.0 - 0.23);
		
		return srad * (1.0 - 0.8);
	)
	
	EQUATION(Model, NetEmissivity,
		return - (0.34 - 0.139 * std::sqrt(RESULT(ActualVaporPressure)));
	)
	
	EQUATION(Model, CloudCoverFactor,
		return 0.9 * SafeDivide(INPUT(SolarRadiation), INPUT(MaxPossibleRadiation)) + 0.1;
	)
	
	EQUATION(Model, NetLongWaveRadiation,
		double tempKelvin = (INPUT(AirTemperature) + 273.15);
		return RESULT(NetEmissivity) * RESULT(CloudCoverFactor) * 4.9e-9 * tempKelvin * tempKelvin * tempKelvin * tempKelvin; //i.e. pow(tempKelvin, 4.0), but this is probably faster.
	)
	
	EQUATION(Model, NetRadiation,
		return RESULT(NetShortwaveRadiation) + RESULT(NetLongWaveRadiation);
	)
	
	EQUATION(Model, PotentialEvapotranspiration,
		double alphapet = 1.28;
		double petday = 
			  alphapet
			* (RESULT(SlopeOfSaturationPressureCurve) / (RESULT(SlopeOfSaturationPressureCurve) + RESULT(PsychrometricConstant))
			* RESULT(NetRadiation) / RESULT(LatentHeatOfVaporization);
		return Max(0.0, petday);
	)
}