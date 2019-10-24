

// This will be a very simple lake model for use along with cathcment models.
// The goal is to simulate flows, temperature and carbon cycling, possibly along with N and P processes.


// The water balance part of the model is conceptually based off of VEMALA
// A National-Scale Nutrient Loading Model for Finnish Watersheds - VEMALA, Inse Huttunen et. al. 2016, Environ Model Assess 21, 83-109

// Air-lake fluxes are based off of
// Air-Sea bulk transfer coefficients in diabatic conditions, Junsei Kondo, 1975, Boundary-Layer Meteorology 9(1), 91-112


//NOTE: This model is IN DEVELOPMENT



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
	
	auto PhysParams = RegisterParameterGroup(Model, "Lake physical parameters");
	
	auto LakeSurfaceArea                = RegisterParameterDouble(Model, PhysParams, "Lake surface area", M2, 1e3, 0.0, 371e9);
	auto LakeLength                     = RegisterParameterDouble(Model, PhysParams, "Lake length", M, 300.0, 0.0, 1.03e6, "This parameter should be adjusted when calibrating lake outflow");
	auto LakeShoreSlope                 = RegisterParameterDouble(Model, PhysParams, "Lake shore slope", MPerM, 0.2, 0.0, 4.0, "This parameter should be adjusted when calibrating lake outflow. Slope is roughly 2*depth/width");
	auto WaterLevelAtWhichOutflowIsZero = RegisterParameterDouble(Model, PhysParams, "Water level at which outflow is 0", M, 10.0, 0.0, 1642.0);
	auto OutflowRatingCurveShape        = RegisterParameterDouble(Model, PhysParams, "Outflow rating curve shape", Dimensionless, 0.3, 0.0, 1.0, "0 if rating curve is linear, 1 if rating curve is a parabola. Values in between give linear interpolation between these types of curves.");
	auto OutflowRatingCurveMagnitude    = RegisterParameterDouble(Model, PhysParams, "Outflow rating curve magnitude", Dimensionless, 1.0, 0.01, 100.0, "Outflow is proportional to 10^(magnitude)");
	auto InitialWaterLevel              = RegisterParameterDouble(Model, PhysParams, "Initial water level", M, 10.0, 0.0, 1642.0);
	
	//TODO: We should make a flexible way for this to either be taken from (one or more) reach sections in a directly coupled model, OR be an input timeseries
	auto LakeInflow     = RegisterInput(Model, "Lake inflow", M3PerS);
	auto Precipitation  = RegisterInput(Model, "Precipitation", MmPerDay);
	auto AirTemperature = RegisterInput(Model, "Air temperature", DegreesCelsius);
	auto WindSpeed      = RegisterInput(Model, "Wind speed at 10m", MPerS);
	
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
	
	
	/*
	
	//auto LakeSurfaceTemperature = RegisterEquation(Model, "Lake surface temperature", DegreesCelsius);


	EQUATION(Model, Stability,
		double WW = (INPUT(WindSpeed) + 1e-10)
		double s0 = 0.25 * (RESULT(LakeSurfaceTemperature) - INPUT(AirTemperature)) / (WW * WW);
		return s0 * std::abs(s0) / (std::abs(s0 + 0.01);
	)
	
	EQUATION(Model, TransferCoefficientForLatentHeatFlux,
		double W = INPUT(WindSpeed);
		
		double ae_e, be_e, ce_e, pe_e;
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
	
	
	
	*/
	
	EQUATION(Model, Evaporation,
		//referencedensity is    rho_0, default 1025.0;   // [kg/m3]
	
		// return (RESULT(AirDensity) / PARAMETER(ReferenceDensity) * RESULT(TransferCoefficientForLatentHeatFlux) * INPUT(WindSpeed) * (PARAMETER(SpecificHumidity) - RESULT(SpecificSaturationHumidity));
		return 0.0; //TODO!!
	)
	
	
	
	
	
	EndModule(Model);
}