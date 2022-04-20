//#include "UnitConversions.h"

static void
AddSimplyCModel(mobius_model *Model)
{
	BeginModule(Model, "SimplyC", "0.0.2");
	
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
	auto Days           = RegisterUnit(Model, "day");

	// Set up index sets
	auto Reach          = GetIndexSetHandle(Model, "Reaches"); //Defined in SimplyQ.h
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units"); //Defined in SimplyQ.h

	// PARAMS

	// Carbon params that don't vary with land class or sub-catchment/reach
	auto CarbonParamsGlobal = RegisterParameterGroup(Model, "Carbon global");
	
	auto SoilTemperatureDOCLinearCoefficient    = RegisterParameterDouble(Model, CarbonParamsGlobal, "Soil temperature DOC creation linear coefficient", PerC, 0.0, 0.0, 20.0, "", "kT");
	auto SoilCSolubilityResponseToSO4deposition = RegisterParameterDouble(Model, CarbonParamsGlobal, "Soil carbon solubility response to SO4 deposition", PerMgPerL, 0.0, 0.0, 20.0, "", "kSO4");
	
	auto DeepSoilDOCType                        = RegisterParameterEnum(Model, CarbonParamsGlobal, "Deep soil/groundwater DOC computation", {"soil_avg", "constant", "mass_balance"}, "soil_avg", "soil_avg: conc in deep soil is avg. of soil runoff, const: constant conc. mass_balance: DOC mass balance is computed, with a decay half life.");
	auto SoilAvg = EnumValue(Model, DeepSoilDOCType, "soil_avg");
	auto Const   = EnumValue(Model, DeepSoilDOCType, "constant");
	auto MassBal = EnumValue(Model, DeepSoilDOCType, "mass_balance");
	
	auto DeepSoilDOCConcentration               = RegisterParameterDouble(Model, CarbonParamsGlobal, "Deep soil/groundwater DOC concentration", MgPerL, 5.0, 0.0, 70.0, "Only used if deep soil conc. is constant", "DOCgw");
	auto DeepSoilDOCHalfLife                    = RegisterParameterDouble(Model, CarbonParamsGlobal, "Deep soil/groundwater DOC half life", Days, 70.0, 0.5, 1000.0, "Only used if deep soil conc. is mass_balance", "DOChlgw");

	// Carbon params that vary with land class
	auto CarbonParamsLand = RegisterParameterGroup(Model, "Carbon land", LandscapeUnits);
	
	auto BaselineSoilDOCDissolutionRate         = RegisterParameterDouble(Model, CarbonParamsLand, "Baseline Soil DOC dissolution rate", MgPerLPerDay, 0.1, 0.0, 100.0, "", "cDOC");
	auto BaselineSoilDOCConcentration           = RegisterParameterDouble(Model, CarbonParamsLand, "Baseline Soil DOC concentration", MgPerL, 10.0, 0.0, 100.0, "Equilibrium concentration under the following conditions: Soil water flow=0, Soil temperature = 0, SO4 deposition = 0", "baseDOC");
	auto SoilDOCMineralisationRate              = RegisterParameterDouble(Model, CarbonParamsLand, "DOC mineralisation+sorption rate", PerDay, 0.0, 0.0, 1.0, "", "dDOC");
	auto UseBaselineConc                        = RegisterParameterBool(Model, CarbonParamsLand, "Compute mineralisation+sorption rate from baseline conc.", true, "If true, use the baseline concentration to determine mineralisation+sorption rate, otherwise use the mineralisation+sorption rate to determine baseline concentration");


	
	
	
	// Equations defined in SimplyQ
	auto SoilWaterVolume 			 = GetEquationHandle(Model, "Soil water volume");
	auto QuickFlow                   = GetEquationHandle(Model, "Quick flow");
	auto SoilWaterFlow   		     = GetEquationHandle(Model, "Soil water flow");
	auto TotalSoilWaterFlow          = GetEquationHandle(Model, "Landuse weighted soil water flow");
	auto GroundwaterFlow             = GetEquationHandle(Model, "Groundwater flow");
	auto GroundwaterVolume           = GetEquationHandle(Model, "Groundwater volume");
	auto ReachVolume                 = GetEquationHandle(Model, "Reach volume");
	auto ReachFlow                   = GetEquationHandle(Model, "Reach flow (end-of-day)");
	auto DailyMeanReachFlow          = GetEquationHandle(Model, "Reach flow (daily mean, cumecs)");
	auto SnowMelt					 = GetEquationHandle(Model, "Snow melt");
	auto PrecipitationFallingAsRain  = GetEquationHandle(Model, "Precipitation falling as rain");

	// Params defined in SimplyQ
	auto CatchmentArea               = GetParameterDoubleHandle(Model, "Catchment area");
	auto BaseflowIndex               = GetParameterDoubleHandle(Model, "Baseflow index");
	auto LandUseProportions			 = GetParameterDoubleHandle(Model, "Land use proportions");
	auto FieldCapacity               = GetParameterDoubleHandle(Model, "Soil field capacity");
	auto GroundwaterTimeConstant     = GetParameterDoubleHandle(Model, "Groundwater time constant");

	// Equation from soil temperature module
	auto SoilTemperature             = GetEquationHandle(Model, "Soil temperature corrected for insulating effect of snow");
	
	
	
	
	
	
	// On-land equations
	
	auto SoilDOCDissolution                     = RegisterEquation(Model, "Soil DOC dissolution", KgPerKm2PerDay, LandSolver);
	auto SoilDOCMineralisation                  = RegisterEquation(Model, "Soil DOC mineralisation+sorption", KgPerKm2PerDay, LandSolver);
	
	auto InitialSoilWaterDOCMass                = RegisterEquationInitialValue(Model, "Initial soil water DOC mass", KgPerKm2);
	auto SoilWaterDOCMass                       = RegisterEquationODE(Model, "Soil water DOC mass", KgPerKm2, LandSolver);
	SetInitialValue(Model, SoilWaterDOCMass, InitialSoilWaterDOCMass);

	auto SoilWaterDOCConcentration              = RegisterEquation(Model, "Soil water DOC concentration, mg/l", MgPerL, LandSolver);
	SetInitialValue(Model, SoilWaterDOCConcentration, SoilWaterDOCConcentration);  //NOTE: To force it to evaluate so that the result can be used by initial groundwater conc.
	auto QuickFlowDOCFluxToReach                = RegisterEquation(Model, "Quick flow DOC flux", KgPerKm2PerDay, LandSolver); 
	
	auto SoilWaterDOCFlux = RegisterEquation(Model, "Soil water DOC flux", KgPerKm2PerDay, LandSolver);
	
	auto DailyMeanSoilwaterDOCFlux              = RegisterEquationODE(Model, "Soil water DOC flux, daily mean", KgPerKm2PerDay, LandSolver);
	SetInitialValue(Model, DailyMeanSoilwaterDOCFlux, 0.0);	
	ResetEveryTimestep(Model, DailyMeanSoilwaterDOCFlux);
	
	// Instream and deep soil equations

	auto TotalSoilwaterDOCFlux                  = RegisterEquationCumulative(Model, "Soilwater DOC flux summed over landscape units", DailyMeanSoilwaterDOCFlux, LandscapeUnits, LandUseProportions);
	auto TotalQuickFlowDOCFlux                  = RegisterEquationCumulative(Model, "Quick flow DOC flux to reach summed over landscape units", QuickFlowDOCFluxToReach, LandscapeUnits, LandUseProportions);
	auto AvgSoilWaterDOCConcentration           = RegisterEquationCumulative(Model, "Avg. soil water DOC concentration", SoilWaterDOCConcentration, LandscapeUnits, LandUseProportions);
	
	auto GroundwaterDOCLoss                     = RegisterEquation(Model, "Groundwater DOC loss", KgPerKm2PerDay, ReachSolver);
	auto GroundwaterDOCConcentration            = RegisterEquation(Model, "Groundwater DOC concentration", MgPerL, ReachSolver);
	auto GroundwaterDOCFlux                     = RegisterEquation(Model, "Groundwater DOC flux", KgPerKm2PerDay, ReachSolver);
	auto InitialGroundwaterDOCMass              = RegisterEquationInitialValue(Model, "Initial groundwater DOC mass", KgPerKm2);
	auto GroundwaterDOCMass                     = RegisterEquationODE(Model, "Groundwater DOC mass", KgPerKm2, ReachSolver);
	SetInitialValue(Model, GroundwaterDOCMass, InitialGroundwaterDOCMass); //TODO: work this out from other parameters

	auto StreamDOCInputFromUpstream             = RegisterEquation(Model, "DOC input from upstream", KgPerDay);
	auto DOCInputFromCatchment                  = RegisterEquation(Model, "DOC input from catchment", KgPerDay, ReachSolver);
	
	auto InitialStreamDOCMass                   = RegisterEquationInitialValue(Model, "Initial reach DOC mass", Kg);
	auto StreamDOCMass                          = RegisterEquationODE(Model, "Reach DOC mass", Kg, ReachSolver);
	SetInitialValue(Model, StreamDOCMass, 0.02); //To do: work initial condition out from baseline DOC parameter

	auto StreamDOCFluxOut                       = RegisterEquation(Model, "DOC flux from reach, end-of-day", KgPerDay, ReachSolver);

	auto DailyMeanStreamDOCFlux                 = RegisterEquationODE(Model, "DOC flux from reach, daily mean", KgPerDay, ReachSolver);
	SetInitialValue(Model, DailyMeanStreamDOCFlux, 0.0);
	ResetEveryTimestep(Model, DailyMeanStreamDOCFlux);

	auto ReachDOCConcentration                  = RegisterEquation(Model, "Reach DOC concentration (volume weighted daily mean)", MgPerL);
	



	
	
	
	
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
		//NOTE: Assume steady-state initial conc.
		return PARAMETER(FieldCapacity) * PARAMETER(BaselineSoilDOCConcentration) * (1.0 + PARAMETER(SoilTemperatureDOCLinearCoefficient)*RESULT(SoilTemperature) - PARAMETER(SoilCSolubilityResponseToSO4deposition)*INPUT(SO4Deposition));
	)
	
	
	EQUATION(Model, SoilDOCDissolution,
		// mg/(l day) * mm = kg/(km2 day)
		
		/*
		double volume = PARAMETER(FieldCapacity);
		double volume = RESULT(SoilWaterVolume);
		if(PARAMETER(ProductionProportionalToVolume)) volume = volume2;
		*/
		double volume = RESULT(SoilWaterVolume);
		
		double value = volume*PARAMETER(BaselineSoilDOCDissolutionRate)*(1.0 + PARAMETER(SoilTemperatureDOCLinearCoefficient)*RESULT(SoilTemperature) - PARAMETER(SoilCSolubilityResponseToSO4deposition)*INPUT(SO4Deposition));
		return Max(0.0, value);
	)
	
	EQUATION(Model, SoilDOCMineralisation,
		return RESULT(SoilWaterDOCMass)*PARAMETER(SoilDOCMineralisationRate);
	)
	
	EQUATION(Model, SoilWaterDOCMass,
		return
			  RESULT(SoilDOCDissolution)
			- RESULT(SoilDOCMineralisation)
			- RESULT(QuickFlowDOCFluxToReach)
			- RESULT(SoilWaterDOCFlux);
	)
	
	EQUATION(Model, SoilWaterDOCConcentration,
		return SafeDivide(RESULT(SoilWaterDOCMass), RESULT(SoilWaterVolume));    // kg / (mm * km2) -> mg/l has conversion factor of 1 
	)
	
	EQUATION(Model, QuickFlowDOCFluxToReach,
		double dissolution_volume = RESULT(SoilWaterVolume) + RESULT(QuickFlow);
		double conc = SafeDivide(RESULT(SoilWaterDOCMass), dissolution_volume);
		return RESULT(QuickFlow) * conc;
	)

	EQUATION(Model, SoilWaterDOCFlux,
		return RESULT(SoilWaterDOCConcentration) * RESULT(SoilWaterFlow);
	)
	
	EQUATION(Model, DailyMeanSoilwaterDOCFlux,
		return RESULT(SoilWaterDOCFlux);
	)
	
	constexpr double ln2 = 0.69314718056;
	
	EQUATION(Model, InitialGroundwaterDOCMass,
		//NOTE: Steady state-ish, assuming Qin = Qout.
		double r = SafeDivide(ln2, PARAMETER(DeepSoilDOCHalfLife));
		double V = RESULT(GroundwaterVolume);
		double T = PARAMETER(GroundwaterTimeConstant);
		double C = RESULT(AvgSoilWaterDOCConcentration);
		double val = SafeDivide(V*C, (T*r + 1.0));
		
		if(PARAMETER(DeepSoilDOCType) != MassBal) return 0.0;
		return val;
	)
	
	EQUATION(Model, GroundwaterDOCMass,
		double val = RESULT(TotalSoilwaterDOCFlux)*PARAMETER(BaseflowIndex) - RESULT(GroundwaterDOCLoss) - RESULT(GroundwaterDOCFlux);
		if(PARAMETER(DeepSoilDOCType) != MassBal) return 0.0;
		return val;
	)
	
	EQUATION(Model, GroundwaterDOCLoss,
		double decay_rate = SafeDivide(ln2, PARAMETER(DeepSoilDOCHalfLife));
		return RESULT(GroundwaterDOCMass) * decay_rate;
	)

 	EQUATION(Model, GroundwaterDOCFlux,
		return RESULT(GroundwaterDOCConcentration) * RESULT(GroundwaterFlow);     // kg / (mm * km2) -> mg/l has conversion factor of 1
	)
	
	EQUATION(Model, GroundwaterDOCConcentration,
		double massbal  = SafeDivide(RESULT(GroundwaterDOCMass), RESULT(GroundwaterVolume)); // kg / (mm * km2) -> mg/l has conversion factor of 1
		double constant = PARAMETER(DeepSoilDOCConcentration);
		double soilavg  = RESULT(AvgSoilWaterDOCConcentration);
		
		auto type = PARAMETER(DeepSoilDOCType);
		if(type == Const) return constant;
		else if(type == MassBal) return massbal;
		return soilavg;
	)
	
	EQUATION(Model, StreamDOCInputFromUpstream,
		double upstreamflux = 0.0;
		for(index_t Input : BRANCH_INPUTS(Reach))
			upstreamflux += RESULT(DailyMeanStreamDOCFlux, Input);
		
		return upstreamflux;
	)
	
	EQUATION(Model, DOCInputFromCatchment,
		return
			(
			  RESULT(TotalQuickFlowDOCFlux) 
			+ RESULT(TotalSoilwaterDOCFlux)*(1.0-PARAMETER(BaseflowIndex))
			+ RESULT(GroundwaterDOCFlux)
			)* PARAMETER(CatchmentArea);
	)
	
	EQUATION(Model, InitialStreamDOCMass,
		return RESULT(GroundwaterDOCConcentration) * RESULT(ReachVolume) * 1000.0; 
	)
	
	EQUATION(Model, StreamDOCMass,
		return
			  RESULT(StreamDOCInputFromUpstream)
			+ RESULT(DOCInputFromCatchment)
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


static void
AddSimplyTOCModule(mobius_model *Model)
{
	//NOTE: This one requires that you have added SimplyC AND SimplySed to the model beforehand
	
	BeginModule(Model, "SimplyC TOC", "0.0");
	
	auto Dimensionless = RegisterUnit(Model);
	auto MgPerL        = RegisterUnit(Model, "mg/l");
	
	auto ReachDOCConcentration = GetEquationHandle(Model, "Reach DOC concentration (volume weighted daily mean)");
	auto ReachSSConcentration  = GetEquationHandle(Model, "Reach suspended sediment concentration");
	
	auto TOCParams = RegisterParameterGroup(Model, "TOC");
	auto SedimentCarbonContent = RegisterParameterDouble(Model, TOCParams, "Suspended sediment carbon content", Dimensionless, 0.0, 0.0, 1.0, "Fraction of mass of suspended sediment that is organic carbon");
	
	auto ReachTOCConcentration = RegisterEquation(Model, "Reach TOC concentration", MgPerL);
	
	EQUATION(Model, ReachTOCConcentration,
		return RESULT(ReachDOCConcentration) + PARAMETER(SedimentCarbonContent)*RESULT(ReachSSConcentration);
	)
}
