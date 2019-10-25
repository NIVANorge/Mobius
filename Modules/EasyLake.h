

// This will be a very simple lake model for use along with cathcment models.
// The goal is to simulate flows, temperature and carbon cycling, possibly along with N and P processes.


// The water balance part of the model is conceptually based off of VEMALA
// A National-Scale Nutrient Loading Model for Finnish Watersheds - VEMALA, Inse Huttunen et. al. 2016, Environ Model Assess 21, 83-109

// Air-lake fluxes are based off of
// Air-Sea bulk transfer coefficients in diabatic conditions, Junsei Kondo, 1975, Boundary-Layer Meteorology 9(1), 91-112


//NOTE: This model is IN DEVELOPMENT




//NOTE: Humidity routine based on 
// P. R. Lowe, 1977, "An approximating polynomial for the computation of saturation vapor pressure, J. Appl. Meteor., 16, 100-103.

//TODO: Could some of this be unified with code in PET.h ?
	
inline double
SaturationVaporPressure(double Temperature)
{
	// Returns saturation vapor pressure in millibar=hectopascal.
	
	double a1 = 6.107799961;
	double a2 = 4.436518521e-1;
	double a3 = 1.428945805e-2;
	double a4 = 2.650648471e-4;
	double a5 = 3.031240396e-6;
	double a6 = 2.034080948e-8;
	double a7 = 6.136820929e-11;
	double tw = Temperature;
	
	return (a1 + tw*(a2 + tw*(a3 + tw*(a4 + tw*(a5 + tw*(a6 + tw*a7))))));
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
	auto MmPerDay       = RegisterUnit(Model, "mm/day");
	auto MPerM          = RegisterUnit(Model, "m/m");
	auto DegreesCelsius = RegisterUnit(Model, "°C");
	auto KgPerKg        = RegisterUnit(Model, "kg/kg");
	auto KgPerM3        = RegisterUnit(Model, "kg/m3");
	auto Percent        = RegisterUnit(Model, "%");
	auto Pascal         = RegisterUnit(Model, "Pa");
	auto HPa            = RegisterUnit(Model, "HPa");
	
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
	auto WindSpeed        = RegisterInput(Model, "Wind speed at 10m", MPerS);
	auto RelativeHumidity = RegisterInput(Model, "Relative humidity", Percent);
	auto AirPressure      = RegisterInput(Model, "Air pressure", HPa);
	
	auto LakeSolver = RegisterSolver(Model, "Lake solver", 0.1, IncaDascru);
	
	auto LakeVolume        = RegisterEquationODE(Model, "Lake volume", M3);
	auto InitialLakeVolume = RegisterEquationInitialValue(Model, "Initial lake volume", M3);
	SetSolver(Model, LakeVolume, LakeSolver);
	SetInitialValue(Model, LakeVolume, InitialLakeVolume);
	
	auto WaterLevel = RegisterEquationODE(Model, "Water level", M);
	SetSolver(Model, WaterLevel, LakeSolver);
	SetInitialValue(Model, WaterLevel, InitialWaterLevel);
	
	auto DVDT        = RegisterEquation(Model, "Change in lake volume", M3PerS);
	SetSolver(Model, DVDT, LakeSolver);
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
		//NOTE: We don't care about ice when it comes to the water balance. This is a simplification that should not matter too much.
		//NOTE: In the conceptualisation, the surface area is actually not constant but varies with the water level. However, that is probably not important for precip & evaporation.
		return (INPUT(LakeInflow) - RESULT(LakeOutflow)) * 86400.0 + 1e-3 * (INPUT(Precipitation) - RESULT(Evaporation)) * PARAMETER(LakeSurfaceArea);
	)
	
	EQUATION(Model, LakeVolume,
		return RESULT(DVDT);
	)
	
	EQUATION(Model, WaterLevel,
		return 0.5 * (PARAMETER(LakeShoreSlope) / (PARAMETER(LakeLength) * RESULT(WaterLevel))) * RESULT(DVDT);
	)
	
	EQUATION(Model, LakeOutflow,
		double excess = Max(0.0, RESULT(WaterLevel) - PARAMETER(WaterLevelAtWhichOutflowIsZero));
		double C3 = PARAMETER(OutflowRatingCurveShape);
		return std::pow(10.0, PARAMETER(OutflowRatingCurveMagnitude)) * (C3*excess + (1.0 - C3)*excess*excess);
	)
	
	
	//TODO: Does this have to be a parameter, or could it be constant?
	auto ReferenceDensity = RegisterParameterDouble(Model, PhysParams, "Reference air density", KgPerM3, 1025.0, 1000.0, 1100.0);
	
	
	
	
	
	auto SaturationSpecificHumidity           = RegisterEquation(Model, "Saturation specific humidity", KgPerKg);
	auto ActualSpecificHumidity               = RegisterEquation(Model, "Actual specific humidity", KgPerKg);
	auto AirDensity                           = RegisterEquation(Model, "Air density", KgPerM3);
	auto Stability                            = RegisterEquation(Model, "Stability", Dimensionless);               //TODO: this probably has another unit
	auto TransferCoefficientForLatentHeatFlux = RegisterEquation(Model, "Transfer coefficient for latent heat flux", Dimensionless); //Correct unit?
	
	
	auto LakeSurfaceTemperature     = RegisterEquation(Model, "Lake surface temperature", DegreesCelsius);
	//SetSolver(Model, LakeSurfaceTemperature, LakeSolver);
	//SetInitialValue;
	
	
	
	EQUATION(Model, SaturationSpecificHumidity,
		//NOTE: I think this assumes mixing ratio ~= specific humidity, which is ok if vapor mass is significantly smaller than total air mass.
	
		double ratioconvertionfactor = 0.62198; //Converting molar ratio to mass ratio
		
		double svap = SaturationVaporPressure(RESULT(LakeSurfaceTemperature));
		
		return ratioconvertionfactor * svap / (INPUT(AirPressure) - 0.377 * svap); //TODO: Find out what 0.377 is for. Shouldn't that just be 1?
	)
	
	EQUATION(Model, ActualSpecificHumidity,
		double ratioconvertionfactor = 0.62198;
		double relhum = 0.01 * INPUT(RelativeHumidity); // percent -> fraction
		
		double svap = SaturationVaporPressure(INPUT(AirTemperature));
		double actualvaporpressure =  relhum * svap;
		
		return ratioconvertionfactor * actualvaporpressure / (INPUT(AirPressure) - 0.377*actualvaporpressure);
	)


	//TODO: Figure out why this eq is correct:
	EQUATION(Model, AirDensity,
		double gasconstair = 287.058;
		double ratioconvertionfactor = 0.62198;
		
		return 100.0 * INPUT(AirPressure) / (gasconstair * (INPUT(AirTemperature) + 273.15) * (1.0 + ratioconvertionfactor*RESULT(ActualSpecificHumidity)));
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
		
		return ced;
	)
	
	/*
	EQUATION(Model, TransferCoefficientForSensibleHeatFlux,
		double W = INPUT(WindSpeed);
		
		double ae_h; double be_h; double ce_h; double pe_h;
		if(W < 2.2)        { ae_h = 0.0;   be_h = 1.185;   ce_h = 0.0;      pe_h = -0.157;}
		else if (W < 5.0)  { ae_h = 0.927; be_h = 0.0546;  ce_h = 0.0;      pe_h = 1.0;  }
		else if (W < 8.0)  { ae_h = 1.15;  be_h = 0.01;    ce_h = 0.0;      pe_h = 1.0;  }
		else if (W < 25.0) { ae_h = 1.17;  be_h = 0.0075;  ce_h = -0.00045; pe_h = 1.0;  }
		else               { ae_h = 1.652; be_h = -0.017;  ce_h = 0.0;      pe_h = 1.0;  }
	
		double WM8 = (W - 8.0);
		double ced = (ae_h + be_h*std::exp(pe_h * std::log(W + 1e-12)) + ce_h*WM8*WM8)*1e-3;
		
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
		
		return ced;
	)
	
	EQUATION(Model, LatentHeatOfVaporization,
		return (2.5 - 0.00234*RESULT(LakeSurfaceTemperature))*1e6;  //TODO: Figure out unit of this!
	)
	
	EQUATION(Model, RainfallHeatfluxCorrection,
		double airt       = INPUT(AirTemperature);
		double airtkelvin = airt + 273.15;
		double cpa        = 1008.0;        //Specific heat capacity of air at constant pressure. Parameter??
		double cpw        = 3985.0;        //Specific heat capacity of water??
		double gasconstair = 287.058;
		double ratioconvertionfactor = 0.62198;
		
		double lheat = RESULT(LatentHeatOfVaporization);
		
		double x1 = 2.11e-5 * std::pow(airtkelvin/273.15, 1.94);
		double x2 = 0.02411 * (1.0 + airt*(3.309e-3 - 1.44e-6*airt)) / (RESULT(AirDensity) * cpa);
		double x3 = RESULT(ActualSpecificHumidity) * lheat / (gasconstair * airtkelvin * airtkelvin);
		
		cd_rain = 1.0 / (1.0 + ratioconvertionfactor*x3*lheat*x1 / (cpa*x2));
		return cd_rain * cpw * ((RESULT(LakeSurfaceTemperature) - airt) + (RESULT(SaturationSpecificHumidity) - RESULT(ActualSpecificHumidity))*lheat/cpa); 
	)
	
	EQUATION(Model, LatentHeatFlux,
		double latentheat = -RESULT(TransferCoefficientForLatentHeatFlux) * RESULT(LatentHeatOfVaporization) * RESULT(AirDensity) * INPUT(WindSpeed) * (RESULT(ActualSpecificHumidity) - RESULT(SaturationSpecificHumidity));
		
		double rainfall = INPUT(Precipitation) / 86400.0; // mm/day -> kg/m2/s
	
		//NOTE: In GOTM code they say the correct the sensible heat flux for rainfall, but actually only correct the latent heat flux! (Is this important?)
	
		return latentheat - rainfall * RESULT(RainfallHeatfluxCorrection);
	)
	
	EQUATION(Model, SensibleHeatFlux,
		double cpa        = 1008.0;
	
		return - RESULT(TransferCoefficientForSensibleHeatFlux) * cpa * RESULT(AirDensity) * INPUT(WindSpeed) * (RESULT(LakeSurfaceTemperature) - INPUT(AirTemperature));
	)*/
	
	EQUATION(Model, Evaporation,
		double evap = (RESULT(AirDensity) / PARAMETER(ReferenceDensity)) * RESULT(TransferCoefficientForLatentHeatFlux) * INPUT(WindSpeed) * (RESULT(ActualSpecificHumidity) - RESULT(SaturationSpecificHumidity));
		
		return -86400000.0*evap; //NOTE: Convert m/s to mm/day. Also, we want positive sign for value.
	)
	
	EQUATION(Model, LakeSurfaceTemperature,
		return INPUT(AirTemperature);       //TODO!!!!
	)
	
	
	
	EndModule(Model);
}