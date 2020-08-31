


// In this file we will put some alternative PET modules that could be used by other models.

#include "Preprocessing/ThornthwaitePET.h"



static void
AddThornthwaitePETModule(mobius_model *Model)
{
	BeginModule(Model, "Thornthwaite PET", "1.0");
	
	SetModuleDescription(Model, R""""(
This is an implementation of

[^https://doi.org/10.2307%2F210739^ Thornthwaite, C. W. (1948). An approach toward a rational classification of climate. Geographical Review. 38 (1): 55–94]

Linear interpolation is used to convert monthly to daily values.	
)"""");
	
	auto Dimensionless = RegisterUnit(Model);
	auto MmPerDay = RegisterUnit(Model, "mm/day");
	auto Degrees  = RegisterUnit(Model, "°C");
	
	auto PETParams = RegisterParameterGroup(Model, "Potential evapotranspiration");
	RegisterParameterDouble(Model, PETParams, "Latitude", Degrees, 60.0, -90.0, 90.0, "Used in PET calculation if no PET timeseries was provided in the input data");
	auto PETMultiplicationFactor      = RegisterParameterDouble(Model, PETParams, "PET multiplication factor", Dimensionless, 1.0, 0.0, 2.0, "Parameter to scale potential evapotranspiration. Should be set to 1 in most cases.");
	
	
	auto PET = RegisterInput(Model, "Potential evapotranspiration", MmPerDay); //This input timeseries is filled in by the ComputeThornthwaitePET preprocessing step if the timeseries is not provided in the input file.
	
	AddPreprocessingStep(Model, ComputeThornthwaitePET); //NOTE: The preprocessing step is called at the start of each model run.
	
	auto PotentialEvapotranspiration = RegisterEquation(Model, "Potential evapotranspiration", MmPerDay);
	
	EQUATION(Model, PotentialEvapotranspiration,
		return PARAMETER(PETMultiplicationFactor) * INPUT(PET);
	)
	
	EndModule(Model);
}


static void
AddDegreeDayPETModule(mobius_model *Model)
{
	BeginModule(Model, "Degree-day PET", "0.1");
	
	SetModuleDescription(Model, R""""(
This is a very simple PET model using a linear relationship between PET and air temperature.
)"""");
	
	auto MmPerDay        = RegisterUnit(Model, "mm/day");
	auto MmPerDegCPerDay = RegisterUnit(Model, "mm/°C/day");
	auto Degrees  = RegisterUnit(Model, "°C");
	
	auto AirTemperature  = RegisterInput(Model, "Air temperature", Degrees);
	
	auto LandscapeUnits = RegisterIndexSet(Model, "Landscape units");
	auto PETParams      = RegisterParameterGroup(Model, "Potential evapotranspiration", LandscapeUnits);
	
	auto DegreeDayEvapotranspiration = RegisterParameterDouble(Model, PETParams, "Degree-day evapotranspiration", MmPerDegCPerDay, 0.12, 0.05, 0.2);
	auto EtpTemperatureMin           = RegisterParameterDouble(Model, PETParams, "Minimal temperature for evapotranspiration", Degrees, 0.0, -5.0, 5.0);
	
	auto PotentialEvapotranspiration = RegisterEquation(Model, "Potential evapotranspiration", MmPerDay);
	
	EQUATION(Model, PotentialEvapotranspiration,
		return PARAMETER(DegreeDayEvapotranspiration) * Max(0.0, INPUT(AirTemperature) - PARAMETER(EtpTemperatureMin));
	)
	
	EndModule(Model);
}


static void
AddPriestleyTaylorPETModule(mobius_model *Model)
{
	BeginModule(Model, "Priestley-Taylor PET", "0.1");
	
	SetModuleDescription(Model, R""""(
This is an adaptation of the Priestley-Taylor model as implemented by [^https://swat.tamu.edu/media/99192/swat2009-theory.pdf^ SWAT]

[^https://doi.org/10.1175/1520-0493(1972)100<0081:OTAOSH>2.3.CO;2^ C. H. B. Priestley; R. J. Taylor (1972). On the assessment of surface heat flux and evaporation using large-scale parameters. Monthly Weather Review. 100 (2): 81–82] 
)"""");
	
	auto SolarRadiationMax          = GetEquationHandle(Model, "Solar radiation on a clear sky day"); //From SolarRadiation.h : AddMaxSolarRadiationModule
	
	auto Dimensionless  = RegisterUnit(Model);
	auto M              = RegisterUnit(Model, "m");
	auto kPa            = RegisterUnit(Model, "kPa");
	auto InvRootkPa     = RegisterUnit(Model, "kPa^{-1/2}");
	auto kPaPerDegreesC = RegisterUnit(Model, "kPa/°C");
	auto MJPerM2        = RegisterUnit(Model, "MJ/m2/day");
	auto MJPerKg        = RegisterUnit(Model, "MJ/kg");
	auto Mm             = RegisterUnit(Model, "mm");
	
	
	auto PETParams = RegisterParameterGroup(Model, "Potential evapotranspiration");
	
	auto CloudCoverOffset            = RegisterParameterDouble(Model, PETParams, "Cloud cover offset", Dimensionless, 0.1, 0.0, 1.0, "The a in the equation  cc = a + b(R/Rmax), where R is solar radiation and Rmax is solar radiation on a clear day");
	auto CloudCoverScalingFactor     = RegisterParameterDouble(Model, PETParams, "Cloud cover scaling factor", Dimensionless, 0.9, 0.0, 1.0, "The b in the equation  cc = a + b(R/Rmax), where R is solar radiation and Rmax is solar radiation on a clear day");
	
	auto EmissivityAtZero            = RegisterParameterDouble(Model, PETParams, "Net emissivity at 0 vapor pressure", Dimensionless, 0.34, 0.0, 1.0);
	auto PressureEmissivityChange    = RegisterParameterDouble(Model, PETParams, "Change in emissivity caused by vapor pressure", InvRootkPa, 0.139, 0.0, 0.5);
	
	auto EarthAlbedo                 = RegisterParameterDouble(Model, PETParams, "Earth albedo", Dimensionless, 0.23, 0.0, 1.0);
	auto SnowAlbedo                  = RegisterParameterDouble(Model, PETParams, "Snow albedo", Dimensionless, 0.8, 0.0, 1.0);
	auto PETMultiplicationFactor     = RegisterParameterDouble(Model, PETParams, "PET multiplication factor", Dimensionless, 1.28, 1.0, 2.0);
	
	//NOTE: Since the hydrology module depends on this one being registered first, we have to register the snow depth equation we need from it. It does not matter, because when the hydrology module registers it later it will refer to the same entity.
	auto SnowDepthAsWaterEquivalent = RegisterEquation(Model, "Snow depth as water equivalent", Mm);

	auto AirTemperature             = RegisterInput(Model, "Air temperature");
	auto RelativeHumidity           = RegisterInput(Model, "Relative humidity", Dimensionless);
	auto SolarRadiation             = RegisterInput(Model, "Solar radiation", MJPerM2);
	
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
		return INPUT(RelativeHumidity)*0.01 * RESULT(SaturationVaporPressure);
	)
	
	EQUATION(Model, SlopeOfSaturationPressureCurve,
		double temp = INPUT(AirTemperature) + 237.3;
		return 4098.0 * RESULT(SaturationVaporPressure) / (temp*temp);
	)
	
	EQUATION(Model, NetShortwaveRadiation,
		//TODO: Should this be parametrized?
	
		double sd   = RESULT(SnowDepthAsWaterEquivalent);
		double srad = INPUT(SolarRadiation);
		
		double alb = PARAMETER(EarthAlbedo);
		double snowalb = PARAMETER(SnowAlbedo);
		if(sd < 0.5) return srad * (1.0 - alb);
		
		return srad * (1.0 - snowalb);
	)
	
	EQUATION(Model, NetEmissivity,
		//Brunt (1932)
		return -(PARAMETER(EmissivityAtZero) - PARAMETER(PressureEmissivityChange) * std::sqrt(RESULT(ActualVaporPressure)));
	)
	
	EQUATION(Model, CloudCoverFactor,
		//Wright, Jensen (1972)
		double cc = PARAMETER(CloudCoverScalingFactor) * SafeDivide(INPUT(SolarRadiation), RESULT(SolarRadiationMax)) + PARAMETER(CloudCoverOffset);
		return Clamp01(cc); //NOTE: Clamp to the interval [0, 1] in case there is something strange in the measured solar radiation data and it exceeds the theoretical max.
	)
	
	EQUATION(Model, NetLongWaveRadiation,
		double boltzmannConst = 4.903e-9;
		double tempKelvin = (INPUT(AirTemperature) + 273.15);
		return RESULT(NetEmissivity) * RESULT(CloudCoverFactor) * boltzmannConst * tempKelvin * tempKelvin * tempKelvin * tempKelvin; //i.e. pow(tempKelvin, 4.0), but this is probably faster.
	)
	
	EQUATION(Model, NetRadiation,
		return RESULT(NetShortwaveRadiation) + RESULT(NetLongWaveRadiation);
	)
	
	EQUATION(Model, PotentialEvapotranspiration,
		//TODO: We should look into subtracting energy going into heating the soil. SWAT sets this to 0, but may not be correct in winter in cold climates.
		double alphapet = PARAMETER(PETMultiplicationFactor);
		double petday = 
			  alphapet
			* (RESULT(SlopeOfSaturationPressureCurve) / (RESULT(SlopeOfSaturationPressureCurve) + RESULT(PsychrometricConstant)))
			* RESULT(NetRadiation) / RESULT(LatentHeatOfVaporization);
		return Max(0.0, petday);
	)
	
	EndModule(Model);
}


static void
AddPriestleyTaylorPETModule2(mobius_model *Model)
{
	//NOTE: This is the same as above, but with net radiation provided, and the scaling factor is per land use
	
	BeginModule(Model, "Priestley-Taylor PET", "0.1");
	
	SetModuleDescription(Model, R""""(
This is an adaptation of the Priestley-Taylor model as implemented by [^https://swat.tamu.edu/media/99192/swat2009-theory.pdf^ SWAT]

[^https://doi.org/10.1175/1520-0493(1972)100<0081:OTAOSH>2.3.CO;2^ C. H. B. Priestley; R. J. Taylor (1972). On the assessment of surface heat flux and evaporation using large-scale parameters. Monthly Weather Review. 100 (2): 81–82] 
)"""");
	
	auto SolarRadiationMax          = GetEquationHandle(Model, "Solar radiation on a clear sky day"); //From SolarRadiation.h : AddMaxSolarRadiationModule
	
	auto Dimensionless  = RegisterUnit(Model);
	auto M              = RegisterUnit(Model, "m");
	auto kPa            = RegisterUnit(Model, "kPa");
	auto InvRootkPa     = RegisterUnit(Model, "kPa^{-1/2}");
	auto kPaPerDegreesC = RegisterUnit(Model, "kPa/°C");
	auto MJPerM2        = RegisterUnit(Model, "MJ/m2/day");
	auto MJPerKg        = RegisterUnit(Model, "MJ/kg");
	auto Mm             = RegisterUnit(Model, "mm");
	
	
	auto LandscapeUnits = RegisterIndexSet(Model, "Landscape units");
	
	auto PETParams = RegisterParameterGroup(Model, "Potential evapotranspiration");
	auto PETScaling = RegisterParameterGroup(Model, "Potential evapotranspiration by land use", LandscapeUnits);
	
	auto PETMultiplicationFactor     = RegisterParameterDouble(Model, PETScaling, "PET multiplication factor", Dimensionless, 1.28, 0.3, 4.0, "", "alphaPET");
	auto EmissivityAtZero            = RegisterParameterDouble(Model, PETParams, "Net emissivity at 0 vapor pressure", Dimensionless, 0.34, 0.0, 1.0);
	auto PressureEmissivityChange    = RegisterParameterDouble(Model, PETParams, "Change in emissivity caused by vapor pressure", InvRootkPa, 0.139, 0.0, 0.5);
	
	//NOTE: Since the hydrology module depends on this one being registered first, we have to register the snow depth equation we need from it. It does not matter, because when the hydrology module registers it later it will refer to the same entity.
	auto SnowDepthAsWaterEquivalent = RegisterEquation(Model, "Snow depth as water equivalent", Mm);

	auto AirTemperature             = RegisterInput(Model, "Air temperature");
	auto RelativeHumidity           = RegisterInput(Model, "Relative humidity", Dimensionless);
	auto NetShortwaveRadiation      = RegisterInput(Model, "Net shortwave radiation", MJPerM2);
	auto CloudCover                 = RegisterInput(Model, "Cloud cover", Dimensionless);
	
	auto Elevation                      = GetParameterDoubleHandle(Model, "Elevation"); //From SolarRadiation.h : AddMaxSolarRadiationModule
	
	auto LatentHeatOfVaporization       = RegisterEquation(Model, "Latent heat of vaporization", MJPerKg);
	auto PsychrometricConstant          = RegisterEquation(Model, "Psycrhometric constant", kPaPerDegreesC);
	auto SaturationVaporPressure        = RegisterEquation(Model, "Saturation vapor pressure", kPa);
	auto SlopeOfSaturationPressureCurve = RegisterEquation(Model, "Slope of saturation pressure curve", kPaPerDegreesC);
	auto ActualVaporPressure            = RegisterEquation(Model, "Actual vapor pressure", kPa);
	auto NetEmissivity                  = RegisterEquation(Model, "Net emissivity", Dimensionless);
	auto NetLongWaveRadiation           = RegisterEquation(Model, "Net long-wave radiation", MJPerM2);
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
	
	EQUATION(Model, ActualVaporPressure,
		return INPUT(RelativeHumidity)*0.01 * RESULT(SaturationVaporPressure);
	)
	
	EQUATION(Model, NetEmissivity,
		//Brunt (1932)
		return -(PARAMETER(EmissivityAtZero) - PARAMETER(PressureEmissivityChange) * std::sqrt(RESULT(ActualVaporPressure)));
	)
	
	EQUATION(Model, NetLongWaveRadiation,
		double boltzmannConst = 4.903e-9;
		double tempKelvin = (INPUT(AirTemperature) + 273.15);
		return RESULT(NetEmissivity) * (1.0 - INPUT(CloudCover)) * boltzmannConst * tempKelvin * tempKelvin * tempKelvin * tempKelvin; //i.e. pow(tempKelvin, 4.0), but this is probably faster.
	)
	
	EQUATION(Model, PotentialEvapotranspiration,
		//TODO: We should look into subtracting energy going into heating the soil. SWAT sets this to 0, but may not be correct in winter in cold climates.
		double alphapet = PARAMETER(PETMultiplicationFactor);
		double petday = 
			  alphapet
			* (RESULT(SlopeOfSaturationPressureCurve) / (RESULT(SlopeOfSaturationPressureCurve) + RESULT(PsychrometricConstant)))
			* (INPUT(NetShortwaveRadiation)+RESULT(NetLongWaveRadiation)) / RESULT(LatentHeatOfVaporization);
		return Max(0.0, petday);
	)
	
	EndModule(Model);
}

/*
TODO: Finish the below...

static void
AddPenmanMonteithPETModule(mobius_model *Model)
{
	
	
	EQUATION(Model, AerodynamicResistance,
	
		double h    = PARAMETER(CanopyHeight);
		double z_0c = PARAMETER(RoughnessParameterClosedCanopy);   // m
		double L    = RESULT(LeafAreaIndex);
		double tt   = (-1.0 + exp(0.909 - 3.03*z_0c/h));
		double c_d  = tt*tt*tt*tt/4.0;                             // drag coefficient for individual canopy element 
	
		//Thom formulation (Thom 75):
		double z_w = PARAMETER(HeightOfWindInstrument);
		double z_om = //roughness length governing momentum transfer
		double z_h = PARAMETER(HeightOfHumidityMeasurement);
		double z_oh = 0.1*z_om; //roughness length governing transfer of heat and vapor
		double d = //zero plane displacement height .. computation
		double u_z = INPUT(WindSpeed);
		double k = 0.41; //Kármán's constant
		
		r_a = log((z_w - d)/z_om)*log((z_h-d)/z_oh)/(k*k*u_z);
		return r_a;
	)
	
	EQUATION(Model, PotentialEvapotranspiration,
		double lambda_v = RESULT(LatentHeatOfVaporization);        // J/kg
		double delta    = RESULT(SlopeOfSaturationPressureCurve);  // Pa/K
		double R_n      = RESULT(NetRadiation);                    // W/m2
		double G        = RESULT(SoilHeatFlux);                    // W/m2
		double v_s      = RESULT(SaturationVaporPressure);
		double v_a      = RESULT(ActualVaporPressure);
		double rho_a    = //air density at constant pressure
		double c_p      = //specific heat capacity of air
		double gamma    = RESULT(PsychrometricConstant);
		double r_s      = RESULT(SurfaceResistance);
		double r_a      = RESULT(AerodynamicResistance);
		
		return (1.0/lambda_v) * (delta*(R_n - G) + rho_a*c_p*(v_s - v_a)/r_a) / (delta + gamma*(1.0 + r_s/r_a));
	)

}
*/
