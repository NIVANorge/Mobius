

//NOTE this module is in development!



static void
AddSimplyNModel(mobius_model *Model)
{
	BeginModule(Model, "SimplyN", "_dev");
	
	auto Dimensionless  = RegisterUnit(Model);
	auto Kg             = RegisterUnit(Model, "kg");
	auto MgPerL         = RegisterUnit(Model, "mg/l");
	auto KgPerKm2       = RegisterUnit(Model, "kg/km2");
	auto KgPerDay       = RegisterUnit(Model, "kg/day");
	auto KgPerKm2PerDay = RegisterUnit(Model, "kg/km2/day");
	auto KgPerHaPerYear = RegisterUnit(Model, "kg/Ha/year");
	auto KgPerHaPerDay  = RegisterUnit(Model, "kg/Ha/day");
	auto MPerDay        = RegisterUnit(Model, "m/day");
	auto Days           = RegisterUnit(Model, "day");
	auto PerDay         = RegisterUnit(Model, "1/day");
	
	//From SimplyQ:
	auto Reach          = GetIndexSetHandle(Model, "Reaches"); 
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	
	
	auto NitrogenGlobal = RegisterParameterGroup(Model, "Nitrogen global");
	
	auto SoilWaterDINImmobilisationRate   = RegisterParameterDouble(Model, NitrogenGlobal, "Soil water DIN immobilisation rate at 20°C", PerDay, 0.0, 0.0, 1.0, "", "imms");
	auto SoilWaterDINImmobilisationQ10    = RegisterParameterDouble(Model, NitrogenGlobal, "Soil water DIN immobilisation response to 10°C change in temperature (Q10)", Dimensionless, 1.0, 1.0, 2.0, "", "Q10imms");
	auto GroundwaterDINConcentration      = RegisterParameterDouble(Model, NitrogenGlobal, "Groundwater DIN concentration", MgPerL, 0.0, 0.0, 5.0, "", "DINgw");
	
	auto NitrogenLand   = RegisterParameterGroup(Model, "Nitrogen land", LandscapeUnits);
	
	auto InitialSoilWaterDINConcentration = RegisterParameterDouble(Model, NitrogenLand, "Initial soil water DIN concentration", MgPerL, 0.0, 0.0, 10.0);
	auto NetAnnualNInputToSoil            = RegisterParameterDouble(Model, NitrogenLand, "Net annual DIN input to soil", KgPerHaPerYear, 0.0, 0.0, 1000.0, "Inputs from deposition and fertilizer", "DINin");
	
	auto NetAnnualNUptake                 = RegisterParameterDouble(Model, NitrogenLand, "Net annual DIN uptake", KgPerHaPerYear, 0.0, 0.0, 1000.0, "Annual uptake of DIN by plants when there is sufficient availability.", "DINup");
	auto UptakeMaxDay                     = RegisterParameterDouble(Model, NitrogenLand, "Day of year when DIN uptake is maximal", Days, 180.0, 1.0, 355.0, "", "upmaxday");
	auto UptakeSeasonLength               = RegisterParameterDouble(Model, NitrogenLand, "Length of period where 95% of uptake happens", Days, 200.0, 1.0, 200.0, "day_of_max_uptake +- length_of_period/2 should be within the same calendar year, otherwise the math is bad.", "uplen");
	
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
	
	//From SimplySoilTemperature
	auto SoilTemperature    = GetEquationHandle(Model, "Soil temperature corrected for insulating effect of snow");  // [°C]
	
	
	
	auto IdealDINUptake            = RegisterEquation(Model, "Ideal DIN uptake", KgPerHaPerDay);
	auto DINUptake                 = RegisterEquation(Model, "DIN uptake limited by availability", KgPerHaPerDay, LandSolver);
	auto SoilWaterDINImmobilisation= RegisterEquation(Model, "Soil water DIN immobilisation", KgPerKm2PerDay, LandSolver);
	
	auto InitialSoilWaterDINMass   = RegisterEquationInitialValue(Model, "Initial soil water DIN mass", KgPerKm2);
	auto SoilWaterDINMass          = RegisterEquationODE(Model, "Soil water DIN mass", KgPerKm2, LandSolver);
	SetInitialValue(Model, SoilWaterDINMass, InitialSoilWaterDINMass);
	
	auto SoilWaterDINConcentration = RegisterEquation(Model, "Soil water DIN concentration", MgPerL, LandSolver);
	auto SoilWaterDINFlux          = RegisterEquation(Model, "Soil water DIN flux", KgPerKm2PerDay, LandSolver);
	
	auto DailyMeanSoilWaterDINFluxToReach = RegisterEquationODE(Model, "Soil water DIN flux to reach (daily mean)", KgPerKm2PerDay, LandSolver);
	ResetEveryTimestep(Model, DailyMeanSoilWaterDINFluxToReach);
	
	auto TotalSoilWaterDINFluxToReach = RegisterEquationCumulative(Model, "Soil water DIN flux to reach summed over landscape units", DailyMeanSoilWaterDINFluxToReach, LandscapeUnits, LandUseProportions);
	
	auto GroundwaterDINFluxToReach  = RegisterEquation(Model, "Groundwater DIN flux to reach", KgPerKm2PerDay, ReachSolver);
	
	auto InitialReachDINMass        = RegisterEquationInitialValue(Model, "Initial reach DIN mass", Kg);
	
	auto ReachDINInputFromUpstream  = RegisterEquation(Model, "Reach DIN inputs from upstream", KgPerDay);
	auto ReachDINMass               = RegisterEquationODE(Model, "Reach DIN mass", Kg, ReachSolver);
	SetInitialValue(Model, ReachDINMass, InitialReachDINMass);
	auto ReachDINFlux               = RegisterEquation(Model, "Reach DIN flux", KgPerDay, ReachSolver);
	auto DailyMeanReachDINFlux      = RegisterEquationODE(Model, "Daily mean reach DIN flux", KgPerDay, ReachSolver);
	ResetEveryTimestep(Model, DailyMeanReachDINFlux);
	
	auto ReachDINConcentration      = RegisterEquation(Model, "Reach DIN concentration (volume weighted daily mean)", MgPerL);
	
	
	EQUATION(Model, IdealDINUptake,
		double sigma = 0.25 * PARAMETER(UptakeSeasonLength);
		double mu    = (double)PARAMETER(UptakeMaxDay);
		double alpha = PARAMETER(NetAnnualNUptake);
		double x     = (double)CURRENT_TIME().DayOfYear;
		double xpon  = (x-mu)/sigma;
		
		// Normalized normal distribution multiplied by alpha, which then has integral equal to alpha. Note that since the x axis is not -inf to inf, but 1 to 365, the actual integral will not be exactly alpha, but the error should be small. There is also a small error in that we are only doing daily updates to the value, but that should also insignificant.
		return alpha * (0.3989422804 / sigma) * std::exp(-0.5*xpon*xpon);
	)
	
	EQUATION(Model, DINUptake,
		double threshold = 0.01; //TODO: make parameter?
		double conc = RESULT(SoilWaterDINConcentration);
		double ideal = RESULT(IdealDINUptake);
		return SCurveResponse(conc, 0.0, threshold, 0.0, ideal);
	)
	
	EQUATION(Model, SoilWaterDINImmobilisation,
		return RESULT(SoilWaterDINMass) * PARAMETER(SoilWaterDINImmobilisationRate)*std::pow(PARAMETER(SoilWaterDINImmobilisationQ10), (RESULT(SoilTemperature) - 20.0)/10.0);
	)
	
	
	EQUATION(Model, InitialSoilWaterDINMass,
		return PARAMETER(InitialSoilWaterDINConcentration)*RESULT(SoilWaterVolume);   // (mg/l)*mm == kg/km2
	)
	
	EQUATION(Model, SoilWaterDINMass,
		//TODO
		return
			  PARAMETER(NetAnnualNInputToSoil) * 100.0/356.0   // 1/Ha/year -> 1/km2/day
			- RESULT(SoilWaterDINImmobilisation)
			- RESULT(DINUptake) * 100.0                        // 1/Ha      -> 1/km2
			- RESULT(SoilWaterDINFlux);
	)
	
	EQUATION(Model, SoilWaterDINConcentration,
		return SafeDivide(RESULT(SoilWaterDINMass), RESULT(SoilWaterVolume)); // (kg/km2)/mm == mg/l
	)
	
	EQUATION(Model, SoilWaterDINFlux,
		return RESULT(SoilWaterDINConcentration) * RESULT(SoilWaterFlow);
	)
	
	EQUATION(Model, DailyMeanSoilWaterDINFluxToReach,
		return (1.0 - PARAMETER(BaseflowIndex))*RESULT(SoilWaterDINFlux);
	)
	
	EQUATION(Model, GroundwaterDINFluxToReach,
		return RESULT(GroundwaterFlow)*PARAMETER(GroundwaterDINConcentration); // (mg/l)*mm == kg/km2
	)
	
	
	EQUATION(Model, InitialReachDINMass,
		return 1e-3 * PARAMETER(GroundwaterDINConcentration) * RESULT(ReachVolume);
	)
	
	EQUATION(Model, ReachDINInputFromUpstream,
		double inputs = 0.0;
		FOREACH_INPUT(Reach,
			inputs += RESULT(DailyMeanReachDINFlux, *Input);
		)
		return inputs;
	)
	
	EQUATION(Model, ReachDINFlux,
		return 86400.0 * RESULT(ReachFlow) * SafeDivide(RESULT(ReachDINMass), RESULT(ReachVolume));
	)
	
	EQUATION(Model, ReachDINMass,
		return
		  RESULT(ReachDINInputFromUpstream)
		+ (RESULT(TotalSoilWaterDINFluxToReach) + RESULT(GroundwaterDINFluxToReach))*PARAMETER(CatchmentArea)
		- RESULT(ReachDINFlux);
	)
	
	EQUATION(Model, ReachDINConcentration,
		return 1e3 * SafeDivide(RESULT(DailyMeanReachDINFlux), 86400.0 * RESULT(DailyMeanReachFlow));
	)
	
	EQUATION(Model, DailyMeanReachDINFlux,
		return RESULT(ReachDINFlux);
	)
	
	EndModule(Model);
}