
//NOTE This is an extremely simple lake module that was made for just one specific project. It may not be widely applicable



inline double
SaturationVaporPressure(double Temperature)
{
	//NOTE: Saturation vapor pressure routine based on 
	// P. R. Lowe, 1977, "An approximating polynomial for the computation of saturation vapor pressure, J. Appl. Meteor., 16, 100-103.

	//TODO: Could some of this be unified with code in PET.h ?
	
	//TODO: There should be a separate formula for Temperature < 0.0
	
	// Takes temperature in celsius
	// Returns saturation vapor pressure in millibar=hectopascal.
	
	double a0 = 6.107799961;
	double a1 = 4.436518521e-1;
	double a2 = 1.428945805e-2;
	double a3 = 2.650648471e-4;
	double a4 = 3.031240396e-6;
	double a5 = 2.034080948e-8;
	double a6 = 6.136820929e-11;
	double t = Temperature;
	
	return (a0 + t*(a1 + t*(a2 + t*(a3 + t*(a4 + t*(a5 + t*a6))))));
}



void
AddIncaToxLakeModule(mobius_model *Model)
{
	BeginModule(Model, "INCA-Tox Lake", "_dev");
	
	auto Dimensionless  = RegisterUnit(Model);
	auto Metres         = RegisterUnit(Model, "m");
	auto M2             = RegisterUnit(Model, "m2");
	auto Ng             = RegisterUnit(Model, "ng");
	auto NgPerM3        = RegisterUnit(Model, "ng/m3");
	auto NgPerDay       = RegisterUnit(Model, "ng/day");
	auto Days           = RegisterUnit(Model, "day");
	auto DegreesCelsius = RegisterUnit(Model, "°C");
	auto WPerM2         = RegisterUnit(Model, "W/m^2");
	auto HPa            = RegisterUnit(Model, "HPa");
	auto Percent        = RegisterUnit(Model, "%");
	
	auto LakeParams = RegisterParameterGroup(Model, "Lake");
	
	auto LakeDepth            = RegisterParameterDouble(Model, LakeParams, "Depth", Metres, 0.0);
	auto EpiliminionThickness = RegisterParameterDouble(Model, LakeParams, "Epilimnion thickness", Metres, 0.0);
	auto LakeSurfaceArea      = RegisterParameterDouble(Model, LakeParams, "Surface area", M2, 0.0);
	
	auto PhotoDegradationRate = RegisterParameterDouble(Model, LakeParams, "Photo-degradation rate", Dimensionless, 0.0); //TODO: Other unit
	
	auto EpilimnionTemperatureLag = RegisterParameterDouble(Model, LakeParams, "Epilimnion temperature lag factor", Days, 1.0);
	
	auto SecchiDepth          = RegisterParameterDouble(Model, LakeParams, "Secchi depth", Metres, 2.0);
	
	auto LakeSolver = RegisterSolver(Model, "Lake solver", 0.01, IncaDascru);
	
	auto EpilimnionDegradationTemperatureModifier  = RegisterEquation(Model, "Temperature modifier for degradation in epilimnion", Dimensionless, LakeSolver);
	auto HypolimnionDegradationTemperatureModifier = RegisterEquation(Model, "Temperature modifier for degradation in hypolimnion", Dimensionless);
	
	auto EpilimnionContaminantMass = RegisterEquationODE(Model, "Epilimnion contaminant mass", Ng, LakeSolver);
	auto EpilimnionContaminantConc = RegisterEquation(Model, "Epilimnion contaminant concentration", NgPerM3, LakeSolver);
	
	auto EpilimnionContaminantDegradation  = RegisterEquation(Model, "Epilimnion contaminant degradation", NgPerDay, LakeSolver);
	auto HypolimnionContaminantDegradation = RegisterEquation(Model, "Hypolimnion contaminant degradation", NgPerDay, LakeSolver);
	
	auto HypolimnionContaminantMass = RegisterEquationODE(Model, "Hypolimnion contaminant mass", Ng, LakeSolver);
	auto HypolimnionContaminantConc = RegisterEquation(Model, "Hypolimnion contaminant concentration", NgPerM3, LakeSolver);
	
	auto LayerExchange              = RegisterEquation(Model, "Lake layer exchange", NgPerDay, LakeSolver);
	auto PhotoDegradation           = RegisterEquation(Model, "Photo-degradation", NgPerDay, LakeSolver);
	
	//auto TempRecord                 = RegisterEquation(Model, "Air temperature", DegreesCelsius);
	auto DaysSinceTempSwitch        = RegisterEquation(Model, "Days since temperature switch", Days);
	auto IsIce                      = RegisterEquation(Model, "Is ice", Dimensionless);
	
	auto DiffusiveAirLakeExchangeFlux = RegisterEquation(Model, "Diffusive air-lake exchange flux", NgPerDay, LakeSolver);
	
	
	auto CloudCover                           = RegisterEquation(Model, "Cloud cover", Dimensionless);
	auto Albedo                               = RegisterEquation(Model, "Albedo", Dimensionless, LakeSolver);
	auto ActualVaporPressure                  = RegisterEquation(Model, "Actual vapor pressure", HPa);
	auto EmittedLongwaveRadiation             = RegisterEquation(Model, "Emitted longwave radiation", WPerM2, LakeSolver);
	auto DownwellingLongwaveRadation          = RegisterEquation(Model, "Downwelling longwave radiation", WPerM2);
	auto LongwaveRadiation                    = RegisterEquation(Model, "Net longwave radiation", WPerM2, LakeSolver);
	auto NetShortwaveRadiation                = RegisterEquation(Model, "Net shortwave radiation", WPerM2, LakeSolver);
	
	auto EpilimnionAttn                       = RegisterEquation(Model, "Epilimnion attenuation fraction", Dimensionless);
	
	auto EpilimnionTemperature      = RegisterEquationODE(Model, "Epilimnion temperature", DegreesCelsius, LakeSolver);
	auto HypolimnionTemperature     = RegisterEquation(Model, "Hypolimnion temperature", DegreesCelsius);
	
	auto AirTemperature                     = GetInputHandle(Model, "Air temperature");
	
	auto ShortwaveRadiation                 = RegisterInput(Model, "Shortwave radiation", WPerM2);
	auto RelativeHumidity                   = RegisterInput(Model, "Relative humidity", Percent);
	
	
	auto SolarRadiationMax                  = GetEquationHandle(Model,  "Solar radiation on a clear sky day");
	
	auto ReachFlow                          = GetEquationHandle(Model, "Reach flow"); // m3/s
	auto ReachWaterContaminantConcentration = GetEquationHandle(Model, "Reach water contaminant concentration"); // ng/m3
	auto ReachContaminantFlux               = GetEquationHandle(Model, "Reach contaminant flux"); // ng/day
	
	//NOTE: We reuse these instead of computing them for the lake. This is ONLY OK because we have a slow-flowing river in our particular case!!
	auto AirWaterTransferVelocity           = GetEquationHandle(Model, "Reach overall air-water transfer velocity");
	auto AirWaterPartitioningCoefficient    = GetEquationHandle(Model, "Reach air-water partitioning coefficient");
	
	auto AtmosphericContaminantConcentration = GetParameterDoubleHandle(Model, "Atmospheric contaminant concentration");
	auto ReachContaminantDegradationRateConstant = GetParameterDoubleHandle(Model, "Contaminant degradation rate constant in the stream");
	
	auto DegradationResponseToTemperature = GetParameterDoubleHandle(Model, "Degradation rate response to one degree change in temperature");
	auto TemperatureAtWhichDegradationRatesAreMeasured = GetParameterDoubleHandle(Model, "Temperature at which degradation rates are measured");
	
	/*
	EQUATION(Model, TempRecord,
		return INPUT(AirTemperature);    //Need this as an equation since there is no LAST_INPUT lookup.
	)
	*/
	EQUATION(Model, DaysSinceTempSwitch,
		double temp = RESULT(EpilimnionTemperature);
		double lasttemp = LAST_RESULT(EpilimnionTemperature);
		double days = LAST_RESULT(DaysSinceTempSwitch);
		if((temp > 0.0 && lasttemp < 0.0) || (temp < 0.0 && lasttemp > 0.0)) return 0.0;
		return days + 1.0;
	)
	
	EQUATION(Model, IsIce,
		double is = LAST_RESULT(IsIce);
		double days = RESULT(DaysSinceTempSwitch);
		double temp = RESULT(EpilimnionTemperature);
		if(is > 0.0)
		{
			if(temp > 0.0) return 0.0;
			return 1.0;
		}
		else
		{
			if(temp < 0.0 && days >= 6.0) return 1.0;
			return 0.0;
		}
		return 0.0;
	)
	
	EQUATION(Model, HypolimnionDegradationTemperatureModifier,
		return pow(PARAMETER(DegradationResponseToTemperature), RESULT(HypolimnionTemperature) - PARAMETER(TemperatureAtWhichDegradationRatesAreMeasured));
	)
	
	EQUATION(Model, EpilimnionDegradationTemperatureModifier,
		return pow(PARAMETER(DegradationResponseToTemperature), RESULT(EpilimnionTemperature) - PARAMETER(TemperatureAtWhichDegradationRatesAreMeasured));
	)
	
	EQUATION(Model, EpilimnionContaminantDegradation,
		return PARAMETER(ReachContaminantDegradationRateConstant)*RESULT(EpilimnionContaminantMass)*RESULT(EpilimnionDegradationTemperatureModifier);
	)
	
	EQUATION(Model, HypolimnionContaminantDegradation,
		return PARAMETER(ReachContaminantDegradationRateConstant)*RESULT(HypolimnionContaminantMass)*RESULT(HypolimnionDegradationTemperatureModifier);
	)
	
	
	EQUATION(Model, EpilimnionContaminantMass,
		double in = RESULT(ReachContaminantFlux);
		double out = RESULT(ReachFlow)*RESULT(EpilimnionContaminantConc)*86400.0;     //NOTE: Assume lake outflow=inflow
		
		return in - out - RESULT(LayerExchange) - RESULT(PhotoDegradation) - RESULT(DiffusiveAirLakeExchangeFlux)
			- RESULT(EpilimnionContaminantDegradation);
	)
	
	EQUATION(Model, EpilimnionContaminantConc,
		//NOTE: Assumes conic lake shape
		double d = PARAMETER(LakeDepth);
		double d0 = PARAMETER(EpiliminionThickness);
		double A = PARAMETER(LakeSurfaceArea);
		double epilimnionvolume = A*(2.0*d-d0)*d0/(3.0*d);
		return RESULT(EpilimnionContaminantMass) / epilimnionvolume;
	)
	
	EQUATION(Model, HypolimnionContaminantMass,
		return RESULT(LayerExchange) - RESULT(HypolimnionContaminantDegradation);
	)
	
	EQUATION(Model, HypolimnionContaminantConc,
		//NOTE: Assumes conic lake shape
		double d = PARAMETER(LakeDepth);
		double d0 = PARAMETER(EpiliminionThickness);
		double A = PARAMETER(LakeSurfaceArea);
		double hypolimnionvolume = A*(d-d0)*(d-d0)/(3.0*d);
		return RESULT(HypolimnionContaminantMass) / hypolimnionvolume;
	)
	
	EQUATION(Model, LayerExchange,
		double rate = 4.0;  //TODO
		
		if(abs(RESULT(EpilimnionTemperature) - RESULT(HypolimnionTemperature)) > 1.0) rate = 0.0; //Stratification
	
		return rate*PARAMETER(LakeSurfaceArea)*(RESULT(EpilimnionContaminantConc) - RESULT(HypolimnionContaminantConc));
	)
	
	EQUATION(Model, PhotoDegradation,
		double icemodifier = (1.0 - 0.8*LAST_RESULT(IsIce)); //TODO: What is a good value instead of 0.8?
		//TODO: Shading from DOC conc.
		return PARAMETER(PhotoDegradationRate)*INPUT(ShortwaveRadiation)*RESULT(EpilimnionContaminantConc)*icemodifier*PARAMETER(LakeSurfaceArea);
	)
	
	
	EQUATION(Model, DiffusiveAirLakeExchangeFlux,
		return RESULT(AirWaterTransferVelocity) * (RESULT(EpilimnionContaminantConc) - PARAMETER(AtmosphericContaminantConcentration)/RESULT(AirWaterPartitioningCoefficient)) * PARAMETER(LakeSurfaceArea);
	)
	
	
	
	EQUATION(Model, HypolimnionTemperature,
		return 4.5;
	)
	
	
	
	
	EQUATION(Model, ActualVaporPressure,
		double relhum = 0.01 * INPUT(RelativeHumidity); // percent -> fraction
		
		double svap = SaturationVaporPressure(INPUT(AirTemperature));
		return relhum * svap;
	)
	
	EQUATION(Model, EmittedLongwaveRadiation,
		double watertkelv = RESULT(EpilimnionTemperature) + 273.15;
		double stefanBoltzmannConst = 5.670367e-8;
		double emissivity = 0.985;

		return emissivity * stefanBoltzmannConst * watertkelv * watertkelv * watertkelv * watertkelv;
	)
	
	EQUATION(Model, CloudCover,
		double cc = 0.9 * SafeDivide(INPUT(ShortwaveRadiation), RESULT(SolarRadiationMax)*11.5740741) + 0.1;
		return Clamp01(cc);
	)
	
	EQUATION(Model, DownwellingLongwaveRadation,
		//Downwelling long-wave radiation,  Josey (2003)
	
		//lwest =  -emiss_lw*sigmaSB.*(airt+273.15).^4.*(0.39 - 0.05*sqrt(ea)).*Fc - 4*emiss_lw*sigmaSB.*(airt+273.15).^3.*(watert - airt)
	
		double airtkelv = INPUT(AirTemperature) + 273.15;
		double stefanBoltzmannConst = 5.670367e-8;
		
		double cloudcover = RESULT(CloudCover);
		double vaporpressure = RESULT(ActualVaporPressure);
		
		double dew_point_temperature = 34.07 + 4157.0 / std::log(2.1718e8 / vaporpressure);    //(Henderson-Sellers 1984)
		double dew_point_depression = dew_point_temperature - airtkelv;
		
		double cloud_effect = 10.77 * cloudcover * cloudcover +   2.34 * cloudcover  - 18.44;
		double vapor_pressure_effect = 0.84 * (dew_point_depression + 4.01);
		
		double effectiveairtemp = airtkelv + cloud_effect + vapor_pressure_effect;
		
		return stefanBoltzmannConst * effectiveairtemp * effectiveairtemp * effectiveairtemp * effectiveairtemp;
	)
	
	EQUATION(Model, Albedo,
		double albedo = 0.04;
		if(LAST_RESULT(IsIce) > 0.0) albedo = 0.6;
		return albedo;
	)
	
	EQUATION(Model, LongwaveRadiation,
		/*
		double airtkelv = INPUT(AirTemperature) + 273.15;
		double watertkelv = RESULT(EpilimnionTemperature) + 273.15;
		double stefanBoltzmannConst = 5.670367e-8;
		double cloudcover = RESULT(CloudCover);
		double vaporpressure = RESULT(ActualVaporPressure);
		double emissivity = 0.985;
		double cloudcoverfactor = 1 - 0.76*RESULT(CloudCover);
		//lwest =  -emiss_lw*sigmaSB.*(airt+273.15).^4.*(0.39 - 0.05*sqrt(ea)).*Fc - 4*emiss_lw*sigmaSB.*(airt+273.15).^3.*(watert - airt)
		
		double longwave = -emissivity*stefanBoltzmannConst*airtkelv*airtkelv*airtkelv*airtkelv*(0.39 - 0.05*sqrt(vaporpressure))*cloudcoverfactor
			-4.0*emissivity*stefanBoltzmannConst*airtkelv*airtkelv*airtkelv*(watertkelv-airtkelv);
		*/
		double longwave = RESULT(DownwellingLongwaveRadation) - RESULT(EmittedLongwaveRadiation);
		if(LAST_RESULT(IsIce) > 0.0) longwave = 0.0;
		return longwave; 
	)

	EQUATION(Model, NetShortwaveRadiation,
		return (1.0 - RESULT(Albedo)) * INPUT(ShortwaveRadiation);
	)
	
	EQUATION(Model, EpilimnionAttn,
		double a = -2.7/PARAMETER(SecchiDepth);
		
		double top = (a*PARAMETER(LakeDepth) - 1.0)/(a*a);
		double bot = exp(a*PARAMETER(EpiliminionThickness))*(a*(PARAMETER(LakeDepth)-PARAMETER(EpiliminionThickness)) - 1.0)/(a*a);
		
		return (bot - top)/PARAMETER(LakeDepth);
	)
	
	EQUATION(Model, EpilimnionTemperature,
		//return LAST_RESULT(EpilimnionTemperature) + (INPUT(AirTemperature) - LAST_RESULT(EpilimnionTemperature))/PARAMETER(EpilimnionTemperatureLag);
		double Cw = 4.18e+6;             // Volumetric heat capacity of water (J K-1 m-3)
		double d = PARAMETER(LakeDepth);
		double d0 = PARAMETER(EpiliminionThickness);
		double epilimnionvolume = PARAMETER(LakeSurfaceArea)*(2.0*d-d0)*d0/(3.0*d);
		
		double longwave  = RESULT(LongwaveRadiation)*PARAMETER(LakeSurfaceArea);
		double shortwave = RESULT(NetShortwaveRadiation)*PARAMETER(LakeSurfaceArea)*RESULT(EpilimnionAttn);
		
		return (longwave + shortwave)*86400.0 / (Cw * epilimnionvolume);
	) 
	
	EndModule(Model);
}




