

//NOTE this module is in development!



static void
AddSimplyNModel(mobius_model *Model)
{
	BeginModule(Model, "SimplyN", "_dev");
	
	auto Kg             = RegisterUnit(Model, "kg");
	auto MgPerL         = RegisterUnit(Model, "mg/l");
	auto KgPerKm2       = RegisterUnit(Model, "kg/km2");
	auto KgPerDay       = RegisterUnit(Model, "kg/day");
	auto KgPerKm2PerDay = RegisterUnit(Model, "kg/km2/day");
	auto KgPerHaPerYear = RegisterUnit(Model, "kg/Ha/year");
	auto MPerDay        = RegisterUnit(Model, "m/day");
	
	//From SimplyQ:
	auto Reach          = GetIndexSetHandle(Model, "Reaches"); 
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	
	
	auto NitrogenGlobal = RegisterParameterGroup(Model, "Nitrogen global");
	
	auto BaseDenitrificationRate          = RegisterParameterDouble(Model, NitrogenGlobal, "Base denitrification rate", MPerDay, 0.0, 0.0, 1.0);
	auto GroundwaterNO3Concentration      = RegisterParameterDouble(Model, NitrogenGlobal, "Groundwater NO3 concentration", MgPerL, 0.0, 0.0, 5.0);
	
	auto NitrogenLand   = RegisterParameterGroup(Model, "Nitrogen land");
	
	auto InitialSoilWaterNO3Concentration = RegisterParameterDouble(Model, NitrogenLand, "Initial soil water NO3 concentration", MgPerL, 0.0, 0.0, 10.0);
	auto NetAnnualNInputToSoil            = RegisterParameterDouble(Model, NitrogenLand, "Net annual N input to soil", KgPerHaPerYear, 0.0, 0.0, 1000.0);
	
	
	//From SimplyQ:
	auto LandSolver  = GetSolverHandle(Model, "SimplyQ land solver");
	auto ReachSolver = GetSolverHandle(Model, "SimplyQ reach solver");

	auto SoilWaterVolume    = GetEquationHandle(Model, "Soil water volume"); // [mm]
	auto SoilWaterFlow      = GetEquationHandle(Model, "Soil water flow");   // [mm/day]
	auto GroundwaterFlow    = GetEquationHandle(Model, "Groundwater flow");  // [mm/day]
	auto DailyMeanReachFlow = GetEquationHandle(Model, "Reach flow (daily mean, cumecs)"); // [m3/day]
	auto ReachFlow          = GetEquationHandle(Model, "Reach flow (end-of-day)");        // [m3/day]
	auto ReachVolume        = GetEquationHandle(Model, "Reach volume");      // [m3]
	
	auto BaseflowIndex      = GetParameterDoubleHandle(Model, "Baseflow index");
	auto LandUseProportions = GetParameterDoubleHandle(Model, "Land use proportions");
	auto CatchmentArea      = GetParameterDoubleHandle(Model, "Catchment area");
	
	
	
	
	auto InitialSoilWaterNO3Mass   = RegisterEquationInitialValue(Model, "Initial soil water NO3 mass", KgPerKm2);
	auto SoilWaterNO3Mass          = RegisterEquationODE(Model, "Soil water NO3 mass", KgPerKm2, LandSolver);
	SetInitialValue(Model, SoilWaterNO3Mass, InitialSoilWaterNO3Mass);
	
	auto SoilWaterNO3Concentration = RegisterEquation(Model, "Soil water NO3 concentration", MgPerL, LandSolver);
	auto SoilWaterNO3Flux          = RegisterEquation(Model, "Soil water NO3 flux", KgPerKm2PerDay, LandSolver);
	
	auto DailyMeanSoilWaterNO3FluxToReach = RegisterEquationODE(Model, "Soil water NO3 flux to reach (daily mean)", KgPerKm2PerDay, LandSolver);
	ResetEveryTimestep(Model, DailyMeanSoilWaterNO3FluxToReach);
	
	auto TotalSoilWaterNO3FluxToReach = RegisterEquationCumulative(Model, "Soil water NO3 flux to reach summed over landscape units", DailyMeanSoilWaterNO3FluxToReach, LandscapeUnits, LandUseProportions);
	
	auto GroundwaterNO3FluxToReach  = RegisterEquation(Model, "Groundwater NO3 flux to reach", KgPerKm2PerDay, ReachSolver);
	
	
	auto ReachNO3InputFromUpstream  = RegisterEquation(Model, "Reach NO3 inputs from upstream", KgPerDay);
	auto ReachNO3Mass               = RegisterEquationODE(Model, "Reach NO3 mass", Kg, ReachSolver);
	auto ReachNO3Flux               = RegisterEquation(Model, "Reach NO3 flux", KgPerDay, ReachSolver);
	auto DailyMeanReachNO3Flux      = RegisterEquationODE(Model, "Daily mean reach NO3 flux", KgPerDay, ReachSolver);
	ResetEveryTimestep(Model, DailyMeanReachNO3Flux);
	
	auto ReachNO3Concentration      = RegisterEquation(Model, "Reach NO3 concentration (volume weighted daily mean)", MgPerL);
	
	
	EQUATION(Model, InitialSoilWaterNO3Mass,
		return PARAMETER(InitialSoilWaterNO3Concentration)*RESULT(SoilWaterVolume);   // (mg/l)*mm == kg/km2
	)
	
	EQUATION(Model, SoilWaterNO3Mass,
		//TODO
		return
			  PARAMETER(NetAnnualNInputToSoil) * 100.0/356.0
			- RESULT(SoilWaterNO3Flux);
	)
	
	EQUATION(Model, SoilWaterNO3Concentration,
		return SafeDivide(RESULT(SoilWaterNO3Mass), RESULT(SoilWaterVolume)); // (kg/km2)/mm == mg/l
	)
	
	EQUATION(Model, SoilWaterNO3Flux,
		return RESULT(SoilWaterNO3Concentration) * RESULT(SoilWaterFlow);
	)
	
	EQUATION(Model, DailyMeanSoilWaterNO3FluxToReach,
		return (1.0 - PARAMETER(BaseflowIndex))*RESULT(SoilWaterNO3Flux);
	)
	
	EQUATION(Model, GroundwaterNO3FluxToReach,
		return RESULT(GroundwaterFlow)*PARAMETER(GroundwaterNO3Concentration); // (mg/l)*mm == kg/km2
	)
	
	
	
	EQUATION(Model, ReachNO3InputFromUpstream,
		double inputs = 0.0;
		FOREACH_INPUT(Reach,
			inputs += RESULT(DailyMeanReachNO3Flux, *Input);
		)
		return inputs;
	)
	
	EQUATION(Model, ReachNO3Flux,
		return 86400.0 * RESULT(ReachFlow) * SafeDivide(RESULT(ReachNO3Mass), RESULT(ReachVolume));
	)
	
	EQUATION(Model, ReachNO3Mass,
		return
		  RESULT(ReachNO3InputFromUpstream)
		+ (RESULT(TotalSoilWaterNO3FluxToReach) + RESULT(GroundwaterNO3FluxToReach))*PARAMETER(CatchmentArea)
		- RESULT(ReachNO3Flux);
	)
	
	EQUATION(Model, ReachNO3Concentration,
		return 1e3 * SafeDivide(RESULT(DailyMeanReachNO3Flux), 86400.0 * RESULT(DailyMeanReachFlow));
	)
	
	EQUATION(Model, DailyMeanReachNO3Flux,
		return RESULT(ReachNO3Flux);
	)
	
	EndModule(Model);
}