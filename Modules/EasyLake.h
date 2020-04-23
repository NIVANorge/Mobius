

// This will be a very simple lake model for use along with cathcment models.
// The goal is to simulate flows, temperature and carbon cycling, possibly along with N and P processes.


// The water balance part of the model is conceptually similar to VEMALA
// A National-Scale Nutrient Loading Model for Finnish Watersheds - VEMALA, Inse Huttunen et. al. 2016, Environ Model Assess 21, 83-109

// Air-lake fluxes are based off of
// Air-Sea bulk transfer coefficients in diabatic conditions, Junsei Kondo, 1975, Boundary-Layer Meteorology 9(1), 91-112
// Implementation is informed by the one in GOTM ( https://github.com/gotm-model/code/blob/master/src/airsea/kondo.F90 )

//NOTE: This model is IN DEVELOPMENT





	
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

static void
AddEasyLakePhysicalModule(mobius_model *Model)
{
	BeginModule(Model, "Easy-Lake physical", "_dev");
	
	auto Dimensionless  = RegisterUnit(Model);
	auto M              = RegisterUnit(Model, "m");
	auto M2             = RegisterUnit(Model, "m2");
	auto M3             = RegisterUnit(Model, "m3");
	auto MPerS          = RegisterUnit(Model, "m/s");
	auto M3PerS         = RegisterUnit(Model, "m3/s");
	auto M3PerDay       = RegisterUnit(Model, "m3/day");
	auto MmPerDay       = RegisterUnit(Model, "mm/day");
	auto PerM           = RegisterUnit(Model, "1/m");
	auto MPerM          = RegisterUnit(Model, "m/m");
	auto Degrees        = RegisterUnit(Model, "°");
	auto DegreesCelsius = RegisterUnit(Model, "°C");
	auto KgPerKg        = RegisterUnit(Model, "kg/kg");
	auto KgPerM3        = RegisterUnit(Model, "kg/m3");
	auto Percent        = RegisterUnit(Model, "%");
	auto Pascal         = RegisterUnit(Model, "Pa");
	auto HPa            = RegisterUnit(Model, "HPa");
	auto WPerM2         = RegisterUnit(Model, "W/m2");
	auto NPerM2         = RegisterUnit(Model, "N/m2");
	
	auto PhysParams = RegisterParameterGroup(Model, "Lake physical parameters");
	
	auto LakeSurfaceArea                = RegisterParameterDouble(Model, PhysParams, "Lake surface area", M2, 1e3, 0.0, 371e9);
	auto LakeLength                     = RegisterParameterDouble(Model, PhysParams, "Lake length", M, 300.0, 0.0, 1.03e6, "This parameter should be adjusted when calibrating lake outflow");
	auto LakeShoreSlope                 = RegisterParameterDouble(Model, PhysParams, "Lake shore slope", MPerM, 0.2, 0.0, 4.0, "This parameter should be adjusted when calibrating lake outflow. Slope is roughly 2*depth/width");
	auto WaterLevelAtWhichOutflowIsZero = RegisterParameterDouble(Model, PhysParams, "Water level at which outflow is 0", M, 10.0, 0.0, 1642.0);
	auto OutflowRatingCurveShape        = RegisterParameterDouble(Model, PhysParams, "Outflow rating curve shape", Dimensionless, 0.3, 0.0, 1.0, "0 if rating curve is linear, 1 if rating curve is a parabola. Values in between give linear interpolation between these types of curves.");
	auto OutflowRatingCurveMagnitude    = RegisterParameterDouble(Model, PhysParams, "Outflow rating curve magnitude", Dimensionless, 1.0, 0.01, 100.0, "Outflow is proportional to 10^(magnitude)");
	auto InitialWaterLevel              = RegisterParameterDouble(Model, PhysParams, "Initial water level", M, 10.0, 0.0, 1642.0);
	
	//TODO: We should make a flexible way for this to either be taken from (one or more) reach sections in a directly coupled model, OR be an input timeseries
	auto LakeInflow       = RegisterInput(Model, "Lake inflow", M3PerS);
	auto Precipitation    = RegisterInput(Model, "Precipitation", MmPerDay);
	auto AirTemperature   = RegisterInput(Model, "Air temperature", DegreesCelsius);
	
	
	//NOTE: Some of these may be hard to come by in some instances. We should provide ways to estimate them such as in PET.h
	auto WindSpeed        = RegisterInput(Model, "Wind speed at 10m", MPerS);
	auto RelativeHumidity = RegisterInput(Model, "Relative humidity", Percent);
	auto AirPressure      = RegisterInput(Model, "Air pressure", HPa);
	auto CloudCover       = RegisterInput(Model, "Cloud cover", Dimensionless);
	auto GlobalRadiation  = RegisterInput(Model, "Global radiation", WPerM2); // Net shortwave radiation falling at earth surface.
	
	auto LakeSolver = RegisterSolver(Model, "Lake solver", 0.1, IncaDascru);
	
	auto LakeVolume        = RegisterEquationODE(Model, "Lake volume", M3);
	auto InitialLakeVolume = RegisterEquationInitialValue(Model, "Initial lake volume", M3);
	SetSolver(Model, LakeVolume, LakeSolver);
	SetInitialValue(Model, LakeVolume, InitialLakeVolume);
	
	auto WaterLevel = RegisterEquationODE(Model, "Water level", M);
	SetSolver(Model, WaterLevel, LakeSolver);
	SetInitialValue(Model, WaterLevel, InitialWaterLevel);
	
	auto DVDT        = RegisterEquation(Model, "Change in lake volume", M3PerDay);
	SetSolver(Model, DVDT, LakeSolver);
	auto OutletWaterLevel = RegisterEquation(Model, "Outlet water level", M);
	SetSolver(Model, OutletWaterLevel, LakeSolver);
	auto LakeOutflow = RegisterEquation(Model, "Lake outflow", M3PerS);
	SetSolver(Model, LakeOutflow, LakeSolver);
	auto Evaporation = RegisterEquation(Model, "Evaporation", MmPerDay);
	SetSolver(Model, Evaporation, LakeSolver);
	
	/*
        Conceptual model for water balance:
		
                              |   ^
                              |   |
                              P   E
                              |   |
                              v   | 
---Qin-->------ w ------*------------ L ------------* -- Qout -->
         \      |      /                           /  ----------
          \     |     /                           /   |
           \    |    /                           /    |
            \   h   /      V = 0.5 * w * L * h  /     h0
             \  |  /                           /      |
              \ | /                           /       |
               \|/___________________________/        |
	
		w  - lake width (m)
		h  - lake depth (m)
		h0 - depth at which outflow is 0 (m)
		L  - lake length (m)
		S  - surface area              = w * h (m^2)
		Qin - inflow (m^3/s)
		Qout - outflow (m^3/s)         = rating_curve_formula(h - h0)
		P  - precipitation (mm/day)
		E  - evaporation (mm/day)
		t  - lake slope                = 2 * h / w                    (1)
		V  - volume                    = w * L * h                    (2)
		dV/dt = (Qin - Qout)*86400 + 1e-3*(P - E)*S                   (3)
		=>
		w = 2 * h / t                                                from (1)
		V = 0.5 * L * h * (2 * h / t) = L * h^2 / t                  from (2)
		dV/dt = (L / t) d(h^2)/dt = (L / t) 2*h * dh/dt
		=>
		dh/dt = (0.5 t / (h * L)) * dV/dt  --- solve this as an ODE equation along with (3)
	*/
	
	EQUATION(Model, InitialLakeVolume,
		return 0.5 * PARAMETER(InitialWaterLevel) * PARAMETER(LakeSurfaceArea);
	)
	
	EQUATION(Model, DVDT,
		//NOTE: We don't care about ice when it comes to the water balance. We may figure out later if this matters for the computation of outflow.
		//NOTE: In the conceptualisation, the surface area is actually not constant but varies with the water level. However, that is probably not important for precip & evaporation.
		return (INPUT(LakeInflow) - RESULT(LakeOutflow)) * 86400.0 + 1e-3 * (INPUT(Precipitation) - RESULT(Evaporation)) * PARAMETER(LakeSurfaceArea);
	)
	
	EQUATION(Model, LakeVolume,
		return RESULT(DVDT);
	)
	
	EQUATION(Model, WaterLevel,
		return 0.5 * (PARAMETER(LakeShoreSlope) / (PARAMETER(LakeLength) * RESULT(WaterLevel))) * RESULT(DVDT);
	)
	
	EQUATION(Model, OutletWaterLevel,
		return Max(0.0, RESULT(WaterLevel) - PARAMETER(WaterLevelAtWhichOutflowIsZero));
	)
	
	EQUATION(Model, LakeOutflow,
		double outletlevel = RESULT(OutletWaterLevel);
		double C3 = PARAMETER(OutflowRatingCurveShape);
		return std::pow(10.0, PARAMETER(OutflowRatingCurveMagnitude)) * (C3*outletlevel + (1.0 - C3)*outletlevel*outletlevel);
	)
	
	
	//TODO: Does reference density have to be a parameter, or could it be constant?
	auto ReferenceDensity = RegisterParameterDouble(Model, PhysParams, "Reference air density", KgPerM3, 1025.0, 1000.0, 1100.0); //TODO: What is this actually? Density of water at a specific temperature??
	auto Emissivity       = RegisterParameterDouble(Model, PhysParams, "Emissivity", Dimensionless, 0.97, 0.0, 1.0);
	auto Latitude         = RegisterParameterDouble(Model, PhysParams, "Latitude", Degrees, 60.0, -90.0, 90.0);
	
	
	
	auto SaturationSpecificHumidity           = RegisterEquation(Model, "Saturation specific humidity", KgPerKg);
	SetSolver(Model, SaturationSpecificHumidity, LakeSolver);
	auto ActualVaporPressure                  = RegisterEquation(Model, "Actual vapor pressure", HPa);
	auto ActualSpecificHumidity               = RegisterEquation(Model, "Actual specific humidity", KgPerKg);
	auto AirDensity                           = RegisterEquation(Model, "Air density", KgPerM3);
	auto Stability                            = RegisterEquation(Model, "Stability", Dimensionless);               //TODO: this probably has another unit
	SetSolver(Model, Stability, LakeSolver);
	auto TransferCoefficientForLatentHeatFlux = RegisterEquation(Model, "Transfer coefficient for latent heat flux", Dimensionless); //Correct unit?
	SetSolver(Model, TransferCoefficientForLatentHeatFlux, LakeSolver);
	auto TransferCoefficientForSensibleHeatFlux = RegisterEquation(Model, "Transfer coefficient for sensible heat flux", Dimensionless); // unit?
	SetSolver(Model, TransferCoefficientForSensibleHeatFlux, LakeSolver);
	auto SurfaceStressCoefficient = RegisterEquation(Model, "Surface stress coefficient", Dimensionless);
	SetSolver(Model, SurfaceStressCoefficient, LakeSolver);
	auto LatentHeatOfVaporization             = RegisterEquation(Model, "Latent heat of vaporization", Dimensionless); //TODO: Unit!
	SetSolver(Model, LatentHeatOfVaporization, LakeSolver);
	//auto RainfallHeatfluxCorrection           = RegisterEquation(Model, "Rainfall heat flux correction", Dimensionless); //TODO: Unit!
	auto LatentHeatFlux                       = RegisterEquation(Model, "Latent heat flux", WPerM2);
	SetSolver(Model, LatentHeatFlux, LakeSolver);
	auto SensibleHeatFlux                     = RegisterEquation(Model, "Sensible heat flux", WPerM2);
	SetSolver(Model, SensibleHeatFlux, LakeSolver);
	auto SurfaceStress                        = RegisterEquation(Model, "Surface stress", NPerM2);
	SetSolver(Model, SurfaceStress, LakeSolver);
	
	auto EmittedLongwaveRadiation             = RegisterEquation(Model, "Emitted longwave radiation", WPerM2);
	SetSolver(Model, EmittedLongwaveRadiation, LakeSolver);
	auto DownwellingLongwaveRadation          = RegisterEquation(Model, "Downwelling longwave radiation", WPerM2);
	auto LongwaveRadiation                    = RegisterEquation(Model, "Net longwave radiation", WPerM2);
	SetSolver(Model, LongwaveRadiation, LakeSolver);
	auto ShortwaveRadiation                   = RegisterEquation(Model, "Net shortwave radiation", WPerM2);
	
	auto LakeSurfaceTemperature     = RegisterEquation(Model, "Lake surface temperature", DegreesCelsius);
	SetSolver(Model, LakeSurfaceTemperature, LakeSolver);
	//SetInitialValue;
	
	/*
		Specific humidity is mass_vapor / mass_air (kg/kg)         (mass_air = mass_vapor + mass_dry_air)
		Saturation specific humidity is the specific humidity when the air is fully saturated with water vapor (function of temperature)
		
		Mixing ratio is  mass_vapor / mass_dry_air
		Saturation mixing ration is mixing ratio when air is saturated
		
		Vapor pressure is the partial pressure of water vapor. 
		Saturation vapor pressure is the partial pressure of water vapor when the air is fully saturated (function of temperature)
		
		pressure_air = pressure_dry_air + pressure_vapor   (Dalton's law)
		
		pressure * volume = moles * boltzmann_constant * temperature_kelvin    (Ideal gas law)
		
		=> pressure_vapor / pressure_dry_air = moles_vapor / moles_dry_air = (1/0.62198) * mass_vapor / mass_dry_air = (1/0.62198) * mixing_ratio
		
		=> mixing_ratio = 0.62198 * pressure_vapor / (pressure_air - pressure_vapor)
		
		specific_humidity = mixing_ratio / (1 + mixing_ratio)   ~= mixing_ratio      (approximately correct if vapor mass is significantly smaller than total air mass, i.e. mixing_ratio is much smaller than 1, which is usually is)
		
		Relative humidity is   vapor_pressure / saturation_vapor_pressure   = mixing_ratio / saturation_mixing_ratio


		NOTE: Thes saturation specific humidity computed here is that of the air touching the lake surface, while the actual specific humidity is computed for air a little above that.
	*/
	
	EQUATION(Model, SaturationSpecificHumidity,
		//NOTE: This assumes 'mixing ratio' ~= 'specific humidity', which is ok if vapor mass is significantly smaller than total air mass.
	
		double ratioconvertionfactor = 0.62198; //Converting molar ratio to mass ratio
		
		double svap = SaturationVaporPressure(RESULT(LakeSurfaceTemperature));
		
		//return ratioconvertionfactor * svap / (INPUT(AirPressure) - 0.377 * svap); //TODO: Find out what 0.377 is for. Shouldn't that just be 1?
		return ratioconvertionfactor * svap / (INPUT(AirPressure) - svap);
	)
	
	EQUATION(Model, ActualVaporPressure,
		double relhum = 0.01 * INPUT(RelativeHumidity); // percent -> fraction
		
		double svap = SaturationVaporPressure(INPUT(AirTemperature));
		return relhum * svap;
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
		double s0 = 0.25 * (RESULT(LakeSurfaceTemperature) - INPUT(AirTemperature)) / (WW * WW);
		return s0 * std::abs(s0) / (std::abs(s0) + 0.01);
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
	
	EQUATION(Model, SurfaceStressCoefficient,
		double W = INPUT(WindSpeed);
		
		double ae_d; double be_d; double ce_d; double pe_d;
		if(W < 2.2)        { ae_d = 0.0;   be_d = 1.08;   pe_d = -0.15;}
		else if (W < 5.0)  { ae_d = 0.771; be_d = 0.0858; pe_d = 1.0;  }
		else if (W < 8.0)  { ae_d = 0.867; be_d = 0.0667; pe_d = 1.0;  }
		else if (W < 25.0) { ae_d = 1.2;   be_d = 0.0025; pe_d = 1.0;  }
		else               { ae_d = 0.0;   be_d = 0.07;   pe_d = 1.0;  }
	
		double cdd = (ae_d + be_d*std::exp(pe_d * std::log(W + 1e-12)))*1e-3;
		
		double s = RESULT(Stability);
		if(s < 0.0)
		{
			double x;
			if(s > -3.3) 	x = 0.1 + 0.03*s + 0.9*std::exp(4.8 * s);
			else            x = 0.0;
			
			cdd *= x;
		}
		else
			cdd *= (1.0 + 0.47 * std::sqrt(s));
		
		return cdd;
	)
	
	EQUATION(Model, LatentHeatOfVaporization,
		//TODO: Should be different for snow..
		return (2.5 - 0.00234*RESULT(LakeSurfaceTemperature))*1e6;  //TODO: Figure out unit of this!
	)
	/*
	
	EQUATION(Model, RainfallHeatfluxCorrection,
		double airt       = INPUT(AirTemperature);
		double airtkelvin = airt + 273.15;
		double cpa        = 1008.0;        //Specific heat capacity of air at a given pressure.
		double cpw        = 3985.0;        //Specific heat capacity of water. But is that the correct number though?? Shouldn't it be 4186.0?
		double gasconstair = 287.058;
		double ratioconvertionfactor = 0.62198;
		
		double lheat = RESULT(LatentHeatOfVaporization);
		
		double x1 = 2.11e-5 * std::pow(airtkelvin/273.15, 1.94);
		double x2 = 0.02411 * (1.0 + airt*(3.309e-3 - 1.44e-6*airt)) / (RESULT(AirDensity) * cpa);
		double x3 = RESULT(ActualSpecificHumidity) * lheat / (gasconstair * airtkelvin * airtkelvin);
		
		double cd_rain = 1.0 / (1.0 + ratioconvertionfactor*x3*lheat*x1 / (cpa*x2));
		cd_rain *= cpw * ((RESULT(LakeSurfaceTemperature) - airt) + (RESULT(SaturationSpecificHumidity) - RESULT(ActualSpecificHumidity))*lheat/cpa); 
		return cd_rain / 86400.0; // 1/(kg/m2/s) -> 1/(mm/day)
	)
	*/
	
	EQUATION(Model, LatentHeatFlux,
		double latentheat = RESULT(TransferCoefficientForLatentHeatFlux) * RESULT(LatentHeatOfVaporization) * RESULT(AirDensity) * INPUT(WindSpeed) * (RESULT(ActualSpecificHumidity) - RESULT(SaturationSpecificHumidity));
		
		//NOTE: In GOTM code they say they correct the sensible heat flux for rainfall, but actually only correct the latent heat flux! (Is this important?)
	
		//NOTE: Again, this is probably not correct for snow and ice.
		return latentheat;// - INPUT(Precipitation) * RESULT(RainfallHeatfluxCorrection);
	)
	
	EQUATION(Model, SensibleHeatFlux,
		double cpa        = 1008.0;
		
		return RESULT(TransferCoefficientForSensibleHeatFlux) * cpa * RESULT(AirDensity) * INPUT(WindSpeed) * (INPUT(AirTemperature) - RESULT(LakeSurfaceTemperature));
	)
	
	EQUATION(Model, Evaporation,
		double evap = (RESULT(AirDensity) / PARAMETER(ReferenceDensity)) * RESULT(TransferCoefficientForLatentHeatFlux) * INPUT(WindSpeed) * (RESULT(ActualSpecificHumidity) - RESULT(SaturationSpecificHumidity));
		
		return -86400000.0*evap; //NOTE: Convert m/s to mm/day. Also, we want positive sign for value.
	)
	
	EQUATION(Model, SurfaceStress,
	
		//TODO: Also possible to correct this for rainfall.
		double Wind = INPUT(WindSpeed);
		return RESULT(SurfaceStressCoefficient) * RESULT(AirDensity) * Wind * Wind;
	)
	
	
	
	
	EQUATION(Model, EmittedLongwaveRadiation,
		double watertkelv = RESULT(LakeSurfaceTemperature) + 273.15;
		double stefanBoltzmannConst = 5.670367e-8;
		
		//TODO: emissivity could be different for snow/ice
		return PARAMETER(Emissivity) * stefanBoltzmannConst * watertkelv * watertkelv * watertkelv * watertkelv;
	)
	
	EQUATION(Model, DownwellingLongwaveRadation,
		//Downwelling long-wave radiation,  Josey (2003)
	
		double airtkelv = INPUT(AirTemperature) + 273.15;
		double stefanBoltzmannConst = 5.670367e-8;
		
		double cloudcover = INPUT(CloudCover);
		double vaporpressure = RESULT(ActualVaporPressure);
		
		double dew_point_temperature = 34.07 + 4157.0 / std::log(2.1718e8 / vaporpressure);    //(Henderson-Sellers 1984)
		double dew_point_depression = dew_point_temperature - airtkelv;
		
		double cloud_effect = 10.77 * cloudcover * cloudcover +   2.34 * cloudcover  - 18.44;
		double vapor_pressure_effect = 0.84 * (dew_point_depression + 4.01);
		
		double effectiveairtemp = airtkelv + cloud_effect + vapor_pressure_effect;
		
		return stefanBoltzmannConst * effectiveairtemp * effectiveairtemp * effectiveairtemp * effectiveairtemp;
	)
	
	EQUATION(Model, LongwaveRadiation,
		//NOTE: have different albedo for longwave and shortwave?
		double albedo = 0.045; //TODO: Should be different when ice/snow?
		return (1.0 - albedo) * RESULT(DownwellingLongwaveRadation) - RESULT(EmittedLongwaveRadiation);
	)
	

	EQUATION(Model, ShortwaveRadiation,
		//Net shortwave radiation (not albedo-corrected), partially based on Rosati & Miyaconda (1988), as implemented by GOTM
		
		//TODO: This is bugged, it returns way too low numbers.
		/*
		double SolarConstant = 1350.0;
		double tau           = 0.7;
		double eclipse       = 23.434 * Pi / 180.0;
		double ozoneppm      = 0.09;
		
		double doy = (double)CURRENT_TIME().DayOfYear;
		double days = (double)CURRENT_TIME().DaysThisYear;
		
		double LatitudeRad = PARAMETER(Latitude) * Pi / 180.0;
		
		
		double SolarDeclination = 0.409*sin(2.0*Pi*doy/days - 1.39);
	
		double SunsetHourAngle = acos(-tan(LatitudeRad)*tan(SolarDeclination));
		
		double cosZenitAngle = sin(LatitudeRad) * sin(SolarDeclination) + cos(LatitudeRad) * cos(SolarDeclination) * cos(SunsetHourAngle);
		
		double qatten = 0.0;
		
		if(cosZenitAngle <= 0.0)
			cosZenitAngle = 0.0;
		else
			qatten = pow(tau, 1.0/cosZenitAngle);
		
		double qzer = cosZenitAngle * SolarConstant;
		double qdir = qzer * qatten;
		double qdiff = ((1.0 - ozoneppm)*qzer - qdir) * 0.5;
		double qtot = qdir + qdiff;
		
		
		double equinox = 2.0 * Pi * (doy - 81.0) / days;
		double SinNoonAngle = sin(LatitudeRad)*sin(eclipse*sin(equinox)) + cos(LatitudeRad)*cos(eclipse*sin(equinox));
		double NoonAngleDeg = asin(SinNoonAngle) * 180.0 / Pi;
		
		double qshort = qtot * (1.0 - 0.62*INPUT(CloudCover) + 0.0019*NoonAngleDeg);
		if(qshort > qtot) qshort = qtot;
		return qshort;
		*/
		
		double albedo = 0.045; //TODO: Needs snow/ice-correction. Maybe also correction for solar angle!
		return (1.0 - albedo) * INPUT(GlobalRadiation);
	)
	

	//TODO: Visibility should eventually depend on DOC concentration
	//TODO: Absorbance may not be a technically correct name? Though is related..
	auto ShortwaveAbsorbance = RegisterParameterDouble(Model, PhysParams, "Shortwave absorbance", PerM, 1.5);

	
	//Stuff below here is loosely based on FLake (Mironov 05)
	auto ConvectiveHeatFluxScale = RegisterEquation(Model, "Convective heat flux scale", WPerM2);
	SetSolver(Model, ConvectiveHeatFluxScale, LakeSolver);
	
	//auto SurfaceShearVelocity = RegisterEquation(Model, "Surface shear velocity", MPerS);
	//SetSolver(Model, SurfaceShearVelocity, LakeSolver);
	
	auto MeanLakeTemperature = RegisterEquationODE(Model, "Mean lake temperature", DegreesCelsius);
	SetSolver(Model, MeanLakeTemperature, LakeSolver);
	SetInitialValue(Model, MeanLakeTemperature, 20.0); //TODO!
	
	auto EpilimnionTemperature = RegisterEquation(Model, "Epilimnion temperature", DegreesCelsius);
	SetSolver(Model, EpilimnionTemperature, LakeSolver);
	//SetInitialValue(Model, EpilimnionTemperature, 20.0);   //TODO!
	
	auto EpilimnionThickness = RegisterEquation(Model, "Epilimnion thickness", M);
	auto BottomTemperature   = RegisterEquation(Model, "Bottom temperature", DegreesCelsius);
	
	
	EQUATION(Model, ConvectiveHeatFluxScale,
		double surfaceheatflux = RESULT(LatentHeatFlux) + RESULT(SensibleHeatFlux) + RESULT(LongwaveRadiation);
		double surfaceshortwave = RESULT(ShortwaveRadiation);
		
		//Assuming shortwave radiation at depth h is given by
		// I(h) = s_0*e^-ah,     a = PARAMETER(ShortwaveAbsorbance), s_0=surfaceshortwave
		
		double absorb = PARAMETER(ShortwaveAbsorbance);
		double thickness = RESULT(EpilimnionThickness);
		
		double expah = std::exp(-absorb*thickness);
		
		return surfaceheatflux + surfaceshortwave * (1.0 + expah + (2.0 / (absorb*thickness))*(1.0 - expah)); 
	)
	
	/*
	EQUATION(Model, EpilimnionWaterDensity,
		double dtemp = (RESULT(EpilimnionTemperature) + 273.15 - 277.13); // Difference between temperature and reference temperature
		return 999.98*(1.0 - 0.5*1.6509e-5*dtemp*dtemp);   //(Farmer, Carmack 1981)
	)
	
	EQUATION(Model, SurfaceShearVelocity,
		return std::sqrt(RESULT(SurfaceStress) / RESULT(EpilimnionWaterDensity));    //TODO: Why do FLake doc say to involve specific heat capacity here?
	)
	
	EQUATION(Model, ObukhovLength,
		double shearvel = RESULT(SurfaceShearVelocity);
		double specificHeatCapacityOfWater = 4186.0;
		double buoy = 9.81 * 1.6509e-5 * (RESULT(EpilimnionTemperature) + 273.15 - 277.13);
		
		return shearvel * shearvel * shearvel / (buoy * RESULT(ConvectiveHeatFluxScale)) * RESULT(EpilimnionWaterDensity) * specificHeatCapacityOfWater;
	)
	
	EQUATION(Model, EquilibriumEpilimnionThickess,
		
	)
	*/
	
	EQUATION(Model, LakeSurfaceTemperature,
		//NOTE: In winter, this will be the temperature of the top of the ice/snow layer when that gets implemented.
		return RESULT(EpilimnionTemperature);
	)
	
	EQUATION(Model, MeanLakeTemperature,
		double dtemp = (RESULT(MeanLakeTemperature) + 273.15 - 277.13); // Difference between temperature and reference temperature
		double waterDensity = 999.98*(1.0 - 0.5*1.6509e-5*dtemp*dtemp);   //(Farmer, Carmack 1981)
		double specificHeatCapacityOfWater = 4186.0;
		
		//return RESULT(EpilimnionTemperature) - c_theta * (1.0 - RESULT(EpilimnionThickness)/RESULT(WaterLevel))*(RESULT(EpilimnionTemperature)-RESULT(BottomTemperature));
		double surfaceheatflux = RESULT(LatentHeatFlux) + RESULT(SensibleHeatFlux) + RESULT(LongwaveRadiation);
		double surfaceshortwave = RESULT(ShortwaveRadiation);
		
		return (1.0 / (RESULT(WaterLevel)  * waterDensity * specificHeatCapacityOfWater)) * (surfaceheatflux + surfaceshortwave) * 86400.0;
	)
	
	EQUATION(Model, EpilimnionTemperature,
		double c_theta = 0.5;  //TODO!
		
		double a = c_theta*(1.0 - RESULT(EpilimnionThickness)/RESULT(WaterLevel));
		
		return (RESULT(MeanLakeTemperature) - a*RESULT(BottomTemperature)) / (1.0 - a);
		/*
		double dtemp = (RESULT(EpilimnionTemperature) + 273.15 - 277.13); // Difference between temperature and reference temperature
		double waterDensity = 999.98*(1.0 - 0.5*1.6509e-5*dtemp*dtemp);   //(Farmer, Carmack 1981)
		double specificHeatCapacityOfWater = 4186.0;      //TODO: Check. Note, should also be modified for salinity?
		
		double surfaceheatflux = RESULT(LatentHeatFlux) + RESULT(SensibleHeatFlux) + RESULT(LongwaveRadiation); //TODO: Check this!
		double surfaceshortwave = RESULT(ShortwaveRadiation);
		
		double shortwave_absorbance = 2.0; //TODO: This should depend on water color when the DOC module is in. Also, should be parametrized
		
		double shortwave_passing_through = surfaceshortwave * std::exp(-shortwave_absorbance * RESULT(EpilimnionThickness));
		
		double heatfluxtometalimnion = 0.0; //TODO!!!
		
		return (1.0 / (RESULT(EpilimnionThickness) * waterDensity * specificHeatCapacityOfWater)) * (surfaceheatflux + surfaceshortwave - heatfluxtometalimnion - shortwave_passing_through) * 86400.0;
		*/
	)
	
	EQUATION(Model, BottomTemperature,
		return 4.0;      //TODO!
	)
	
	EQUATION(Model, EpilimnionThickness,
		return 2.0;   //TODO!
	)
	
	EndModule(Model);
}













	/*
	
	
	auto IceFormationTemperature = RegisterParameterDouble(Model, PhysParams, "Ice formation temperature", DegreesCelsius, 0.0, -2.0, 2.0);
	auto IceThicknessThreshold   = RegisterParameterDouble(Model, PhysParams, "Ice thickness threshold", M, 0.1, 0.0, 1.0, "Thickness at which frazil ice solidifies");
	
	
	
	// ice energy = (PARAMETER(IceFormationTemperature) - RESULT(LakeSurfaceTemperature)) * cw * surfaceLayerThickness; // = energy available for ice formation or melting (depending on sign)
	
	// melt energy -> 0 if surfacetemp <= formationtemp or Hice = 0, otherwise -ice energy
	
	// excess melt energy : if melt energy is more than needed to melt ice.
	
	// ice thickness, frazil ice thickness, and temperature of ice (difficult to find good solution order)
	
	// -> inputs to d(lake surface temperature)/dt
	
	
	EQUATION(Model, FrazilIceThickness,
		if(LAST_RESULT(IceThickness) > 0.0)
		{
			
		}
	)
	
	EQUATION(Model, IceThickness,
		double cw = 4.18e+6;   //volumetric heat capacity of water (J K-1 m-3)
		double iceDensity = 910;  // kg m-3
		double latentHeatOfFreezing = 333500; //(J kg-1)
	
		double frazil = LAST_RESULT(FrazilIceThickness);
		double Hice = LAST_RESULT(IceThickness);
		
		double precip = INPUT(Precipitation);
	
		if(Hice > 0.0)
		{
			if(RESULT(LakeSurfaceTemperature) <= PARAMETER(IceFormationTemperature))
			{
				double initialIceEnergy = (PARAMETER(IceFormationTemperature) - RESULT(LakeSurfaceTemperature)) * cw * surfaceLayerThickness;
				frazil += ( initialIceEnergy/ (iceDensity*latentHeatOfFreezing) );
				Hice += frazil + precip*0.001;
			}
			else
			{
				//TODO
			}
		}
		else
		{
			//TODO
		}
		
		return Hice;
	)
	*/
	