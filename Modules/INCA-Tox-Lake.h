
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
	auto KgPerM3        = RegisterUnit(Model, "kg/m3");
	
	auto LakeParams = RegisterParameterGroup(Model, "Lake");
	
	auto LakeDepth            = RegisterParameterDouble(Model, LakeParams, "Depth", Metres, 0.0);
	auto EpiliminionThickness = RegisterParameterDouble(Model, LakeParams, "Epilimnion thickness", Metres, 0.0);
	auto LakeSurfaceArea      = RegisterParameterDouble(Model, LakeParams, "Surface area", M2, 0.0);
	
	auto OpticalCrossection = RegisterParameterDouble(Model, LakeParams, "Optical crossection", Dimensionless, 255.0, 0.0, 1000.0, "For photodegradation"); //TODO: Other unit
	
	auto SecchiDepth          = RegisterParameterDouble(Model, LakeParams, "Secchi depth", Metres, 2.0);
	
	auto DiffusiveExchangeScale = RegisterParameterDouble(Model, LakeParams, "Diffusive exchange scaling factor", Dimensionless, 1.0);
	
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
	auto AirPressure                        = RegisterInput(Model, "Air pressure", HPa);
	auto WindSpeed                          = GetInputHandle(Model, "Wind speed");
	
	auto SolarRadiationMax                  = GetEquationHandle(Model,  "Solar radiation on a clear sky day");
	
	auto ReachFlow                          = GetEquationHandle(Model, "Reach flow"); // m3/s
	auto ReachWaterContaminantConcentration = GetEquationHandle(Model, "Reach water contaminant concentration"); // ng/m3
	auto ReachContaminantFlux               = GetEquationHandle(Model, "Reach contaminant flux"); // ng/day
	
	//NOTE: We reuse these instead of computing them for the lake. This is ONLY OK because we have a slow-flowing river in our particular case!!
	auto AirWaterTransferVelocity           = GetEquationHandle(Model, "Reach overall air-water transfer velocity");
	auto AirWaterPartitioningCoefficient    = GetEquationHandle(Model, "Reach air-water partitioning coefficient");
	
	auto AtmosphericContaminantConcentration = GetParameterDoubleHandle(Model, "Atmospheric contaminant concentration");
	auto ReachContaminantDegradationRateConstant = GetParameterDoubleHandle(Model, "Contaminant degradation rate constant in the stream");
	
	auto AtmosphericContaminantConcentrationIn = GetInputHandle(Model, "Atmospheric contaminant concentration");
	auto DepositionToLand                      = GetInputHandle(Model, "Contaminant deposition to land");
	
	
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
		//TODO: Using the reach value here makes it compute this for each reach!
		return PARAMETER(ReachContaminantDegradationRateConstant)*RESULT(EpilimnionContaminantMass)*RESULT(EpilimnionDegradationTemperatureModifier);
	)
	
	EQUATION(Model, HypolimnionContaminantDegradation,
		return PARAMETER(ReachContaminantDegradationRateConstant)*RESULT(HypolimnionContaminantMass)*RESULT(HypolimnionDegradationTemperatureModifier);
	)
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	
	EQUATION(Model, EpilimnionContaminantMass,
		auto LastReach = INDEX_NUMBER(Reach, INDEX_COUNT(Reach)-1);
		double in = RESULT(ReachContaminantFlux, LastReach) + INPUT(DepositionToLand)*PARAMETER(LakeSurfaceArea);
		double out = RESULT(ReachFlow, LastReach)*RESULT(EpilimnionContaminantConc)*86400.0;     //NOTE: Assume lake outflow=inflow
		
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
		double oc_Nitro = PARAMETER(OpticalCrossection); // m2/mol Nitro            Optical cross-section of DOM
		double qy_Nitro = 0.45;  // mol Nitro /mol quanta   Quantum yield
		
		double f_uv = 0.06; //Fract.                of PAR in incoming solar radiation
		double e_uv = 351843.0; // J/mol              Average energy of Par photons
		
		//‒oc_Nitro * qy_Nitro f_par(1/e_par)*(86400)*Qsw*Attn_epilimnion * [Nitrosamines]" in mg N m-3 d-1
		double shortwave = RESULT(NetShortwaveRadiation)*RESULT(EpilimnionAttn)*PARAMETER(LakeSurfaceArea);
		return oc_Nitro * qy_Nitro * (f_uv / e_uv) * 86400.0 * shortwave * RESULT(EpilimnionContaminantMass)*1e-6;
	)
	
	
	EQUATION(Model, DiffusiveAirLakeExchangeFlux,
		double atmospheric = IF_INPUT_ELSE_PARAMETER(AtmosphericContaminantConcentrationIn, AtmosphericContaminantConcentration);
		double flux = RESULT(AirWaterTransferVelocity) * (RESULT(EpilimnionContaminantConc) - atmospheric/RESULT(AirWaterPartitioningCoefficient)) * PARAMETER(LakeSurfaceArea);
		if(LAST_RESULT(IsIce) > 0.0) flux = 0.0;
		return PARAMETER(DiffusiveExchangeScale)*flux;
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
	
	auto AirDensity                           = RegisterEquation(Model, "Air density", KgPerM3, LakeSolver);
	auto Stability                            = RegisterEquation(Model, "Stability", Dimensionless, LakeSolver);
	auto ActualSpecificHumidity               = RegisterEquation(Model, "Actual specific humidity", Dimensionless);
	auto SaturationSpecificHumidity           = RegisterEquation(Model, "Saturation specific humidity", Dimensionless, LakeSolver);
	auto TransferCoefficientForLatentHeatFlux = RegisterEquation(Model, "Transfer coefficient for latent heat flux", Dimensionless, LakeSolver);
	auto TransferCoefficientForSensibleHeatFlux = RegisterEquation(Model, "Transfer coefficient for sensible heat flux", Dimensionless, LakeSolver); // unit?
	auto SensibleHeatFlux                     = RegisterEquation(Model, "Sensible heat flux", WPerM2, LakeSolver);
	auto LatentHeatOfVaporization             = RegisterEquation(Model, "Latent heat of vaporization", Dimensionless, LakeSolver);
	auto LatentHeatFlux                       = RegisterEquation(Model, "Latent heat flux", WPerM2, LakeSolver);
	
	
	EQUATION(Model, SaturationSpecificHumidity,
		//NOTE: This assumes 'mixing ratio' ~= 'specific humidity', which is ok if vapor mass is significantly smaller than total air mass.
	
		double ratioconvertionfactor = 0.62198; //Converting molar ratio to mass ratio
		
		double svap = SaturationVaporPressure(RESULT(EpilimnionTemperature));
		
		//return ratioconvertionfactor * svap / (INPUT(AirPressure) - 0.377 * svap); //TODO: Find out what 0.377 is for. Shouldn't that just be 1?
		return ratioconvertionfactor * svap / (INPUT(AirPressure) - svap);
	)
	
	EQUATION(Model, ActualSpecificHumidity,
		double ratioconvertionfactor = 0.62198;
		double actualvaporpressure = RESULT(ActualVaporPressure);
		
		//return ratioconvertionfactor * actualvaporpressure / (INPUT(AirPressure) - 0.377*actualvaporpressure);
		return ratioconvertionfactor * actualvaporpressure / (INPUT(AirPressure) - actualvaporpressure);
	)
	
	EQUATION(Model, AirDensity,
		double tempkelvin = INPUT(AirTemperature) + 273.15;
		double specificgasconstdryair = 287.058; // J/(kg K)
		double specificgasconstvapor  = 461.495; // (J/(kg K))
		return ((INPUT(AirPressure) - RESULT(ActualVaporPressure)) / (tempkelvin*specificgasconstdryair)  +  RESULT(ActualVaporPressure) / (tempkelvin * specificgasconstvapor))* 100.0;   //Multiply by 100 for HPa -> Pa
	)
	
	EQUATION(Model, Stability,
		double WW = (INPUT(WindSpeed) + 1e-10);
		double s0 = 0.25 * (RESULT(EpilimnionTemperature) - INPUT(AirTemperature)) / (WW * WW);
		return s0 * std::abs(s0) / (std::abs(s0) + 0.01);
	)
	
	EQUATION(Model, TransferCoefficientForSensibleHeatFlux,
		double W = INPUT(WindSpeed);
		
		double ae_h; double be_h; double ce_h; double pe_h;
		if(W < 2.2)        { ae_h = 0.0;   be_h = 1.185;   ce_h = 0.0;      pe_h = -0.157;}
		else if (W < 5.0)  { ae_h = 0.927; be_h = 0.0546;  ce_h = 0.0;      pe_h = 1.0;  }
		else if (W < 8.0)  { ae_h = 1.15;  be_h = 0.01;    ce_h = 0.0;      pe_h = 1.0;  }
		else if (W < 25.0) { ae_h = 1.17;  be_h = 0.0075;  ce_h = -0.00045; pe_h = 1.0;  }
		else               { ae_h = 1.652; be_h = -0.017;  ce_h = 0.0;      pe_h = 1.0;  }
	
		double WM8 = (W - 8.0);
		double chd = (ae_h + be_h*std::exp(pe_h * std::log(W + 1e-12)) + ce_h*WM8*WM8)*1e-3;
		
		double s = RESULT(Stability);
		if(s < 0.0)
		{
			double x;
			if(s > -3.3) 	x = 0.1 + 0.03*s + 0.9*std::exp(4.8 * s);
			else            x = 0.0;
			
			chd *= x;
		}
		else
			chd *= (1.0 + 0.63 * std::sqrt(s));
		
		return chd;
	)
	
	EQUATION(Model, TransferCoefficientForLatentHeatFlux,
		double W = INPUT(WindSpeed);
		
		double ae_e; double be_e; double ce_e; double pe_e;   //NOTE: we can't use commas inside the EQUATION macro, or it screws up the comma counting of the preprocessor.
		if(W < 2.2)        { ae_e = 0.0;   be_e = 1.23;    ce_e = 0.0;     pe_e = -0.16;}
		else if (W < 5.0)  { ae_e = 0.969; be_e = 0.0521;  ce_e = 0.0;     pe_e = 1.0;  }
		else if (W < 8.0)  { ae_e = 1.18;  be_e = 0.01;    ce_e = 0.0;     pe_e = 1.0;  }
		else if (W < 25.0) { ae_e = 1.196; be_e = 0.008;   ce_e = -0.0004; pe_e = 1.0;  }
		else               { ae_e = 1.68;  be_e = -0.016;  ce_e = 0.0;     pe_e = 1.0;  }
	
		double WM8 = (W - 8.0);
		double ced = (ae_e + be_e*std::exp(pe_e * std::log(W + 1e-12)) + ce_e*WM8*WM8)*1e-3;
		
		double s = RESULT(Stability);
		if(s < 0.0)
		{
			double x;
			if(s > -3.3) 	x = 0.1 + 0.03*s + 0.9*std::exp(4.8 * s);
			else            x = 0.0;
			
			ced *= x;
		}
		else
			ced *= (1.0 + 0.63 * std::sqrt(s));
		
		if(W==0.0) ced =0.0;
		
		return ced;
	)
	
	EQUATION(Model, SensibleHeatFlux,
		double cpa        = 1008.0;
		
		return RESULT(TransferCoefficientForSensibleHeatFlux) * cpa * RESULT(AirDensity) * INPUT(WindSpeed) * (INPUT(AirTemperature) - RESULT(EpilimnionTemperature));
	)
	
	EQUATION(Model, LatentHeatOfVaporization,
		//TODO: Should be different for snow..
		return (2.5 - 0.00234*RESULT(EpilimnionTemperature))*1e6;  //TODO: Figure out unit of this!
	)
	
	EQUATION(Model, LatentHeatFlux,
		double latentheat = RESULT(TransferCoefficientForLatentHeatFlux) * RESULT(LatentHeatOfVaporization) * RESULT(AirDensity) * INPUT(WindSpeed) * (RESULT(ActualSpecificHumidity) - RESULT(SaturationSpecificHumidity));
		
		//NOTE: In GOTM code they say they correct the sensible heat flux for rainfall, but actually only correct the latent heat flux! (Is this important?)
	
		//NOTE: Again, this is probably not correct for snow and ice.
		return latentheat;// - INPUT(Precipitation) * RESULT(RainfallHeatfluxCorrection);
	)
	
	EQUATION(Model, EpilimnionTemperature,
		//return LAST_RESULT(EpilimnionTemperature) + (INPUT(AirTemperature) - LAST_RESULT(EpilimnionTemperature))/PARAMETER(EpilimnionTemperatureLag);
		double Cw = 4.18e+6;             // Volumetric heat capacity of water (J K-1 m-3)
		double d = PARAMETER(LakeDepth);
		double d0 = PARAMETER(EpiliminionThickness);
		double epilimnionvolume = PARAMETER(LakeSurfaceArea)*(2.0*d-d0)*d0/(3.0*d);
		
		double longwave  = RESULT(LongwaveRadiation)*PARAMETER(LakeSurfaceArea);
		double shortwave = RESULT(NetShortwaveRadiation)*PARAMETER(LakeSurfaceArea)*RESULT(EpilimnionAttn);
		double sensible  = RESULT(SensibleHeatFlux)*PARAMETER(LakeSurfaceArea);
		double latent    = RESULT(LatentHeatFlux)*PARAMETER(LakeSurfaceArea);
		
		if(LAST_RESULT(IsIce) > 0.0)
		{
			sensible = 0.0;
			latent = 0.0;
		}
		
		return (longwave + sensible + latent + shortwave)*86400.0 / (Cw * epilimnionvolume);
	)
	
	EndModule(Model);
}




