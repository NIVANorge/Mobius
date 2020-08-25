#include "../UnitConversions.h"

static void
AddSimplyCModel(mobius_model *Model)
{
	BeginModule(Model, "SimplyC", "test_03");
	
	// Inputs
	auto SO4Deposition = RegisterInput(Model, "SO4 deposition");

	// Solvers already defined in hydrology module
	auto LandSolver = GetSolverHandle(Model, "SimplyQ land solver");
	auto ReachSolver = GetSolverHandle(Model, "SimplyQ reach solver");

	// Units
	auto Dimensionless  = RegisterUnit(Model);
	auto Kg			    = RegisterUnit(Model, "kg");
	auto KgPerKm2       = RegisterUnit(Model, "kg/km2");
	auto KgPerDay       = RegisterUnit(Model, "kg/day");
	auto KgPerKm2PerDay = RegisterUnit(Model, "kg/km2/day");
	auto Km2PerDay      = RegisterUnit(Model, "km2/day");
	auto MgPerL		    = RegisterUnit(Model, "mg/l");
	auto MgPerLPerDay   = RegisterUnit(Model, "mg/l/day");
	auto PerC           = RegisterUnit(Model, "1/°C");
	auto PerMgPerL      = RegisterUnit(Model, "1/(mg/l)");
	auto PerDay         = RegisterUnit(Model, "1/day"); 

	// Set up index sets
	auto Reach          = GetIndexSetHandle(Model, "Reaches"); //Defined in SimplyQ.h
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units"); //Defined in SimplyQ.h

	// PARAMS
	
	// Params defined in hydrol model
	auto CatchmentArea               = GetParameterDoubleHandle(Model, "Catchment area");
#ifdef SIMPLYQ_GROUNDWATER
	auto BaseflowIndex               = GetParameterDoubleHandle(Model, "Baseflow index");
#endif
	auto LandUseProportions			 = GetParameterDoubleHandle(Model, "Land use proportions");
	auto ProportionToQuickFlow		 = GetParameterDoubleHandle(Model, "Proportion of precipitation that contributes to quick flow");

	// Carbon params that don't vary with land class or sub-catchment/reach
	auto CarbonParamsGlobal = RegisterParameterGroup(Model, "Carbon global");
	
	auto SoilTemperatureDOCLinearCoefficient = RegisterParameterDouble(Model, CarbonParamsGlobal, "Soil temperature DOC creation linear coefficient", PerC, 0.0, 0.0, 20.0, "", "kT");

	auto SoilCSolubilityResponseToSO4deposition = RegisterParameterDouble(Model, CarbonParamsGlobal, "Soil carbon solubility response to SO4 deposition", PerMgPerL, 0.0, 0.0, 20.0, "", "kSO4");
	

#ifdef SIMPLYQ_GROUNDWATER
	auto DeepSoilDOCConcentration = RegisterParameterDouble(Model, CarbonParamsGlobal, "Mineral soil/groundwater DOC concentration", MgPerL, 0.0, 0.0, 70.0);
#endif

	// Carbon params that vary with land class
	auto CarbonParamsLand = RegisterParameterGroup(Model, "Carbon land", LandscapeUnits);
	
	auto BaselineSoilDOCDissolutionRate = RegisterParameterDouble(Model, CarbonParamsLand, "Baseline Soil DOC dissolution rate", MgPerLPerDay, 0.1, 0.0, 100.0, "", "cDOC");
	auto BaselineSoilDOCConcentration = RegisterParameterDouble(Model, CarbonParamsLand, "Baseline Soil DOC concentration", MgPerL, 10.0, 0.0, 100.0, "Equilibrium concentration under the following conditions: Soil water flow=0, Soil temperature = 0, SO4 deposition = 0", "baseDOC");
	auto SoilDOCMineralisationRate    = RegisterParameterDouble(Model, CarbonParamsLand, "DOC mineralisation+sorption rate", PerDay, 0.0, 0.0, 1.0, "", "dDOC");
	auto UseBaselineConc              = RegisterParameterBool(Model, CarbonParamsLand, "Compute mineralisation+sorption rate from baseline conc.", true, "If true, use the baseline concentration to determine mineralisation+sorption rate, otherwise use the mineralisation+sorption rate to determine baseline concentration");

	// EQUATIONS

	// Equations defined in hydrology module required here
	auto SoilWaterVolume 			 = GetEquationHandle(Model, "Soil water volume");
	auto QuickFlow                   = GetEquationHandle(Model, "Quick flow");
	auto SoilWaterFlow   		     = GetEquationHandle(Model, "Soil water flow");
#ifdef SIMPLYQ_GROUNDWATER
	auto GroundwaterFlow             = GetEquationHandle(Model, "Groundwater flow");
#endif
	auto ReachVolume                 = GetEquationHandle(Model, "Reach volume");
	auto ReachFlow                   = GetEquationHandle(Model, "Reach flow (end-of-day)");
	auto DailyMeanReachFlow          = GetEquationHandle(Model, "Reach flow (daily mean, cumecs)");
	auto SnowMelt					 = GetEquationHandle(Model, "Snow melt");
	auto PrecipitationFallingAsRain  = GetEquationHandle(Model, "Precipitation falling as rain");

	// Equation from soil temperature module
	auto SoilTemperature       = GetEquationHandle(Model, "Soil temperature corrected for insulating effect of snow");

	auto FieldCapacity = GetParameterDoubleHandle(Model, "Soil field capacity");


	auto SoilDOCDissolution = RegisterEquation(Model, "Soil organic carbon dissolution", KgPerKm2PerDay, LandSolver);
	auto SoilDOCMineralisation = RegisterEquation(Model, "Soil DOC mineralisation+sorption", KgPerKm2PerDay, LandSolver);
	// Carbon equations which vary by land class
	
	auto InitialSoilWaterDOCMass      = RegisterEquationInitialValue(Model, "Initial soil water DOC mass", KgPerKm2);
	auto SoilWaterDOCMass             = RegisterEquationODE(Model, "Soil water DOC mass", KgPerKm2, LandSolver);
	SetInitialValue(Model, SoilWaterDOCMass, InitialSoilWaterDOCMass);

	auto SoilWaterDOCConcentration = RegisterEquation(Model, "Soil water DOC concentration, mg/l", MgPerL, LandSolver);
	auto QuickFlowCarbonFluxToReach = RegisterEquation(Model, "Quick flow DOC flux scaled by land class area", KgPerKm2PerDay, LandSolver);
	
	auto SoilWaterCarbonFlux = RegisterEquation(Model, "Soil water carbon flux", KgPerKm2PerDay, LandSolver);
	
	auto DailyMeanSoilwaterCarbonFluxToReach = RegisterEquationODE(Model, "Soil water carbon flux to reach, daily mean", KgPerKm2PerDay, LandSolver);
	SetInitialValue(Model, DailyMeanSoilwaterCarbonFluxToReach, 0.0);	
	ResetEveryTimestep(Model, DailyMeanSoilwaterCarbonFluxToReach);
	
	
	auto SoilDOCMineralisationRateComputation = RegisterEquationInitialValue(Model, "Soil DOC mineralisation rate", PerDay);
	ParameterIsComputedBy(Model, SoilDOCMineralisationRate, SoilDOCMineralisationRateComputation, false); //false signifies parameter should be exposed in user interface.
	
	auto SoilBaselineDOCConcentrationComputation = RegisterEquationInitialValue(Model, "Soil baseline DOC concentration", MgPerL);
	ParameterIsComputedBy(Model, BaselineSoilDOCConcentration, SoilBaselineDOCConcentrationComputation, false);
	
	EQUATION(Model, SoilDOCMineralisationRateComputation,
		double usebaseline = PARAMETER(UseBaselineConc);
		double mineralisationrate = PARAMETER(SoilDOCMineralisationRate);
		double mineralisationrate0 = PARAMETER(BaselineSoilDOCDissolutionRate) / PARAMETER(BaselineSoilDOCConcentration);
		if(usebaseline) return mineralisationrate0;
		return mineralisationrate;
	)
	
	EQUATION(Model, SoilBaselineDOCConcentrationComputation,
		double usebaseline = PARAMETER(UseBaselineConc);
		double baseline = PARAMETER(BaselineSoilDOCConcentration);
		double baseline0 = PARAMETER(BaselineSoilDOCDissolutionRate) / PARAMETER(SoilDOCMineralisationRate);
		if(usebaseline) return baseline;
		return baseline0;
	)
	
	EQUATION(Model, InitialSoilWaterDOCMass,
		return PARAMETER(BaselineSoilDOCConcentration) * PARAMETER(FieldCapacity);
	)
	
	
	EQUATION(Model, SoilDOCDissolution,
		// mg/(l day) * mm = kg/(km2 day)
		return RESULT(SoilWaterVolume)*PARAMETER(BaselineSoilDOCDissolutionRate)*(1.0 + PARAMETER(SoilTemperatureDOCLinearCoefficient)*RESULT(SoilTemperature) - PARAMETER(SoilCSolubilityResponseToSO4deposition)*INPUT(SO4Deposition));
	)
	
	EQUATION(Model, SoilDOCMineralisation,
		return RESULT(SoilWaterDOCMass)*PARAMETER(SoilDOCMineralisationRate);
	)
	
	EQUATION(Model, SoilWaterDOCMass,
		return
			  RESULT(SoilDOCDissolution)
			- RESULT(SoilDOCMineralisation)
			- RESULT(QuickFlowCarbonFluxToReach)
			- RESULT(SoilWaterCarbonFlux);
	)
	
	EQUATION(Model, SoilWaterDOCConcentration,
		return SafeDivide(RESULT(SoilWaterDOCMass), RESULT(SoilWaterVolume));    // kg / (mm * km2) -> mg/l has conversion factor of 1 
	)
	
	EQUATION(Model, QuickFlowCarbonFluxToReach,
		double dissolution_volume = RESULT(SoilWaterVolume) + RESULT(QuickFlow);
		double conc = SafeDivide(RESULT(SoilWaterDOCMass), dissolution_volume);
		return RESULT(QuickFlow) * conc;
	)

	EQUATION(Model, SoilWaterCarbonFlux,
		return RESULT(SoilWaterDOCConcentration) * RESULT(SoilWaterFlow);
	)
	
	EQUATION(Model, DailyMeanSoilwaterCarbonFluxToReach,
		return RESULT(SoilWaterCarbonFlux)
#ifdef SIMPLYQ_GROUNDWATER
			* (1.0-PARAMETER(BaseflowIndex))
#endif
		;
	)
	
	// Instream equations

	auto TotalSoilwaterCarbonFluxToReach = RegisterEquationCumulative(Model, "Soilwater carbon flux to reach summed over landscape units", DailyMeanSoilwaterCarbonFluxToReach, LandscapeUnits, LandUseProportions);
	
	auto TotalQuickFlowCarbonFlux = RegisterEquationCumulative(Model, "Quick flow DOC flux to reach summed over landscape units", QuickFlowCarbonFluxToReach, LandscapeUnits, LandUseProportions);

	auto StreamDOCInputFromUpstream = RegisterEquation(Model, "Reach DOC input from upstream", KgPerDay);

#ifdef SIMPLYQ_GROUNDWATER	
	auto GroundwaterFluxToReach = RegisterEquation(Model, "Groundwater carbon flux to reach", KgPerKm2PerDay, ReachSolver);
#endif
	
	//To do: work initial condition out from baseline DOC parameter
	auto StreamDOCMass = RegisterEquationODE(Model, "Reach DOC mass", Kg, ReachSolver);
	SetInitialValue(Model, StreamDOCMass, 0.02); 

	auto StreamDOCFluxOut = RegisterEquation(Model, "DOC flux from reach, end-of-day", KgPerDay, ReachSolver);

	auto DailyMeanStreamDOCFlux = RegisterEquationODE(Model, "DOC flux from reach, daily mean", KgPerDay, ReachSolver);
	SetInitialValue(Model, DailyMeanStreamDOCFlux, 0.0);
	ResetEveryTimestep(Model, DailyMeanStreamDOCFlux);

	auto ReachDOCConcentration = RegisterEquation(Model, "Reach DOC concentration (volume weighted daily mean)", MgPerL);

#ifdef SIMPLYQ_GROUNDWATER
 	EQUATION(Model, GroundwaterFluxToReach,
		return RESULT(GroundwaterFlow)*PARAMETER(DeepSoilDOCConcentration);
	)
#endif
	
	EQUATION(Model, StreamDOCInputFromUpstream,
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(DailyMeanStreamDOCFlux, *Input);
		)
		return upstreamflux;
	)
	
	EQUATION(Model, StreamDOCMass,
		return
			  RESULT(StreamDOCInputFromUpstream)
			+ (RESULT(TotalQuickFlowCarbonFlux) + RESULT(TotalSoilwaterCarbonFluxToReach)) * PARAMETER(CatchmentArea)
#ifdef SIMPLYQ_GROUNDWATER
			+ RESULT(GroundwaterFluxToReach) * PARAMETER(CatchmentArea)
#endif
			- RESULT(StreamDOCFluxOut);		
	)
		
	EQUATION(Model, StreamDOCFluxOut,
		return 86400.0 * RESULT(ReachFlow) * SafeDivide(RESULT(StreamDOCMass), RESULT(ReachVolume));
	)

	EQUATION(Model, DailyMeanStreamDOCFlux,
		return RESULT(StreamDOCFluxOut);
	)

	EQUATION(Model, ReachDOCConcentration,
		return 1e3 * SafeDivide(RESULT(DailyMeanStreamDOCFlux), 86400.0 * RESULT(DailyMeanReachFlow));
	)
	
	EndModule(Model);
}

