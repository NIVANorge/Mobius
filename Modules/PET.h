


// In this file we will put some alternative PET modules that could be used by other models.

#include "Preprocessing/ThornthwaitePET.h"



static void
AddThornthwaitePETModule(mobius_model *Model)
{
	BeginModule(Model, "Thornthwaite PET", "1.0");
	
	auto MmPerDay = RegisterUnit(Model, "mm/day");
	auto Degrees  = RegisterUnit(Model, "°C");
	
	auto System = GetParameterGroupHandle(Model, "System");
	RegisterParameterDouble(Model, System, "Latitude", Degrees, 60.0, -90.0, 90.0, "Used in PET calculation if no PET timeseries was provided in the input data");
	
	auto PET = RegisterInput(Model, "Potential evapotranspiration", MmPerDay); //This is overwritten by the ComputeThornthwaitePET step if needed.
	
	AddPreprocessingStep(Model, ComputeThornthwaitePET); //NOTE: The preprocessing step is called at the start of each model run.
	
	auto PotentialEvapotranspiration = RegisterEquation(Model, "Potential evapotranspiration", MmPerDay);
	
	EQUATION(Model, PotentialEvapotranspiration,
		return INPUT(PET);
	)
	
	EndModule(Model);
}


static void
AddDegreeDayPETModule(mobius_model *Model)
{
	BeginModule(Model, "Degree-day PET", "0.1");
	
	auto AirTemperature  = RegisterInput(Model, "Air temperature");
	
	auto MmPerDay        = RegisterUnit(Model, "mm/day");
	auto MmPerDegCPerDay = RegisterUnit(Model, "mm/°C/day");
	
	auto LandscapeUnits = RegisterIndexSet(Model, "Landscape units");
	auto PETParams      = RegisterParameterGroup(Model, "Potential evapotranspiration", LandscapeUnits);
	
	auto DegreeDayEvapotranspiration = RegisterParameterDouble(Model, PETParams, "Degree-day evapotranspiration", MmPerDegCPerDay, 0.12, 0.05, 0.2);
	
	auto PotentialEvapotranspiration = RegisterEquation(Model, "Potential evapotranspiration", MmPerDay);
	
	//TODO: Maybe add an offset temperature.
	EQUATION(Model, PotentialEvapotranspiration,
		return PARAMETER(DegreeDayEvapotranspiration) * Max(0.0, INPUT(AirTemperature));
	)
	
	EndModule(Model);
}


static void
AddPriestleyTaylorPETModule(mobius_model *Model)
{
	//NOTE: This is an attempt at adapting the Priestley-Taylor computations done by SWAT: https://swat.tamu.edu/media/99192/swat2009-theory.pdf
	//WARNING: This module has not yet been tested with real data.
	
	
	BeginModule(Model, "Priestley-Taylor PET", "0.1");
	
	auto AirTemperature             = GetInputHandle(Model, "Air temperature");
	auto SnowDepthAsWaterEquivalent = GetEquationHandle(Model, "Snow depth as water equivalent");
	
	auto SolarRadiationMax          = GetEquationHandle(Model, "Solar radiation on a clear sky day"); //From SolarRadiation.h : AddMaxSolarRadiationModule
	
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
	
	auto Elevation                      = GetParameterDoubleHandle(Model, "Elevation"); //From SolarRadiation.h : AddMaxSolarRadiationModule
	
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
		//Harrison (1963)
		return 2.501 - 2.361e-3 * INPUT(AirTemperature);
	)
	
	EQUATION(Model, PsychrometricConstant,
		//Doorenbos, Pruitt (1977)
		double meanBarometricPressure = 101.3 - PARAMETER(Elevation)*(0.01152 - 0.544e-6*PARAMETER(Elevation));
		//Brunt(1952)
		return 1.013e-3 * meanBarometricPressure / (0.622 * RESULT(LatentHeatOfVaporization));
	)
	
	EQUATION(Model, SaturationVaporPressure,
		//Tetens (1930), Murray (1967)
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
		double srad = INPUT(SolarRadiation);
		
		if(sd < 0.5) return srad * (1.0 - 0.23);
		
		return srad * (1.0 - 0.8);
	)
	
	EQUATION(Model, NetEmissivity,
		return - (0.34 - 0.139 * std::sqrt(RESULT(ActualVaporPressure)));
	)
	
	EQUATION(Model, CloudCoverFactor,
		return 0.9 * SafeDivide(INPUT(SolarRadiation), RESULT(SolarRadiationMax)) + 0.1;
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
			* (RESULT(SlopeOfSaturationPressureCurve) / (RESULT(SlopeOfSaturationPressureCurve) + RESULT(PsychrometricConstant)))
			* RESULT(NetRadiation) / RESULT(LatentHeatOfVaporization);
		return Max(0.0, petday);
	)
	
	EndModule(Model);
}

/*
static void
AddPriestleyTaylorPETModule2(mobius_model *Model)
{
	//NOTE: Same as above but with different input requirements. TODO: refactor this!!
	
	BeginModule(Model, "Priestley-Taylor PET", "0.1");
	
	auto AirTemperature             = RegisterInput(Model, "Air temperature");
	
	auto Dimensionless  = RegisterUnit(Model);
	auto M              = RegisterUnit(Model, "m");
	auto kPa            = RegisterUnit(Model, "kPa");
	auto kPaPerDegreesC = RegisterUnit(Model, "kPa/°C");
	auto MJPerM2        = RegisterUnit(Model, "MJ/m2");
	auto MJPerKg        = RegisterUnit(Model, "MJ/kg");
	auto Mm             = RegisterUnit(Model, "mm");
	
	//TODO: Provide ways to estimate these too?
	auto RelativeHumidity     = RegisterInput(Model, "Relative humidity", Dimensionless);
	auto LongwaveRadiation    = RegisterInput(Model, "Longwave radiation", MJPerM2);
	auto ShortwaveRadiation    = RegisterInput(Model, "Shortwave radiation", MJPerM2);
	
	auto System = GetParameterGroupHandle(Model, "System");
	auto Elevation                      = RegisterParameterDouble(Model, System, "Elevation", M, 0.0, 0.0, 8848.0); 
	
	auto LatentHeatOfVaporization       = RegisterEquation(Model, "Latent heat of vaporization", MJPerKg);
	auto PsychrometricConstant          = RegisterEquation(Model, "Psycrhometric constant", kPaPerDegreesC);
	auto SaturationVaporPressure        = RegisterEquation(Model, "Saturation vapor pressure", kPa);
	auto SlopeOfSaturationPressureCurve = RegisterEquation(Model, "Slope of saturation pressure curve", kPaPerDegreesC);
	auto NetRadiation                   = RegisterEquation(Model, "Net radiation", MJPerM2);
	auto PotentialEvapotranspiration    = RegisterEquation(Model, "Potential evapotranspiration", Mm);
	
	EQUATION(Model, LatentHeatOfVaporization,
		//Harrison (1963)
		return 2.501 - 2.361e-3 * INPUT(AirTemperature);
	)
	
	EQUATION(Model, PsychrometricConstant,
		//Doorenbos, Pruitt (1977)
		double meanBarometricPressure = 101.3 - PARAMETER(Elevation)*(0.01152 - 0.544e-6*PARAMETER(Elevation));
		//Brunt(1952)
		return 1.013e-3 * meanBarometricPressure / (0.622 * RESULT(LatentHeatOfVaporization));
	)
	
	EQUATION(Model, SaturationVaporPressure,
		//Tetens (1930), Murray (1967)
		return std::exp( (16.78*INPUT(AirTemperature) - 116.9) / (INPUT(AirTemperature) + 237.3) );
	)
	
	EQUATION(Model, SlopeOfSaturationPressureCurve,
		double temp = INPUT(AirTemperature) + 237.3;
		return 4098.0 * RESULT(SaturationVaporPressure) / (temp*temp);
	)
	
	EQUATION(Model, NetRadiation,
		return (INPUT(ShortwaveRadiation) + INPUT(LongwaveRadiation))*0.0864; //TODO: Just did this because inputs were in watt for my application. Fix!
	)
	
	EQUATION(Model, PotentialEvapotranspiration,
		double alphapet = 1.28;
		double petday = 
			  alphapet
			* (RESULT(SlopeOfSaturationPressureCurve) / (RESULT(SlopeOfSaturationPressureCurve) + RESULT(PsychrometricConstant)))
			* RESULT(NetRadiation) / RESULT(LatentHeatOfVaporization);
		return Max(0.0, petday);
	)
	
	EndModule(Model);
}
*/