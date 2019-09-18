//SimplyP version 0.4



//  THIS VERSION IS IN DEVELOPMENT AND HAS BUGS!!!!
// If you want to use the model, use SimplyP.h instead for now





//NOTE: This is an adaption of
// https://github.com/LeahJB/SimplyP

//NOTE: This version is ahead of the Python version


//NOTE: Only include these if you are going to use them (they cause long compile times):
//#include "../boost_solvers.h"
//#include "../mtl_solvers.h"


static void
AddSimplyPModel(mobius_model *Model)
{
	BeginModule(Model, "SimplyP", "0.4");
	
	// UNITS
	auto Dimensionless  = RegisterUnit(Model);
	auto Kg             = RegisterUnit(Model, "kg");
	auto Mm             = RegisterUnit(Model, "mm");
	auto MmPerKg        = RegisterUnit(Model, "mm/kg");
	auto KgPerM2        = RegisterUnit(Model, "kg/m^2");
	auto MgPerL         = RegisterUnit(Model, "mg/l");
	auto MgPerKg        = RegisterUnit(Model, "mg/kg");
	auto KgPerHaPerYear = RegisterUnit(Model, "kg/ha/year");
	auto KgPerDay       = RegisterUnit(Model, "kg/day");
	auto KgPerMm        = RegisterUnit(Model, "kg/mm");
	
	// INDEX SETS
	auto Reach          = GetIndexSetHandle(Model, "Reaches");
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	//auto Arable             = RequireIndex(Model, LandscapeUnits, "Arable");
	//auto ImprovedGrassland  = RequireIndex(Model, LandscapeUnits, "Improved grassland");
	//auto Seminatural        = RequireIndex(Model, LandscapeUnits, "Semi-natural");
	
	// PARAMETERS
	
	// Phosphorus params that don't vary by sub-catchment/reach or land class
	auto Phosphorous = RegisterParameterGroup(Model, "Phosphorous");
	
	auto MSoilPerM2                     = RegisterParameterDouble(Model, Phosphorous, "Soil mass per m2", KgPerM2, 95.0, 0.0, 200.0);
	auto PhosphorousSorptionCoefficient = RegisterParameterDouble(Model, Phosphorous, "Phosphorous sorption coefficient", MmPerKg, 1.13e-4, 0.0, 0.1, "Gradient of linear relationship between labile P and TDP concentration. Value only read in from file if calibration run mode is set to false, otherwise it is estimated by the model");
	
	auto GroundwaterTDPConcentration    = RegisterParameterDouble(Model, Phosphorous, "Groundwater TDP concentration", MgPerL, 0.02, 0.0, 10.0);
	auto PPEnrichmentFactor             = RegisterParameterDouble(Model, Phosphorous, "Particulate P enrichment factor", Dimensionless, 1.6, 1.0, 5.0, "P content of eroded material compared to P content of bulk soils");
	auto SRPFraction                    = RegisterParameterDouble(Model, Phosphorous, "SRP fraction", Dimensionless, 0.7, 0.0, 1.0, "Factor to multiply TDP by to estimate instream SRP concentration");

	// Phosphorus parameters that vary by sub-catchment/reach
	auto PhosphorousReach = RegisterParameterGroup(Model, "Phosphorous reach", Reach);
	auto EffluentTDP                    = RegisterParameterDouble(Model, PhosphorousReach, "Reach effluent TDP inputs", KgPerDay, 0.1, 0.0, 10.0);
	
	// Phorphorus parameters that vary by land class
	auto PhosphorousLand = RegisterParameterGroup(Model, "Phosphorous land", LandscapeUnits);
	auto NetAnnualPInput                = RegisterParameterDouble(Model, PhosphorousLand, "Net annual P input to soil", KgPerHaPerYear, 10.0, -100.0, 100.0);
	auto InitialEPC0                    = RegisterParameterDouble(Model, PhosphorousLand, "Initial soil water TDP concentration and EPC0",      MgPerL, 0.1, 0.0, 10.0, "If the dynamic soil P option is set to false, this value is the soil water TDP concentration throughout the model run.");
	auto InitialSoilPConcentration      = RegisterParameterDouble(Model, PhosphorousLand, "Initial total soil P content", MgPerKg, 1458, 0.0, 10000.0);
	auto SoilInactivePConcentration     = RegisterParameterDouble(Model, Phosphorous, "Inactive soil P content", MgPerKg, 873, 0.0, 10000.0, "Is recommended to be about the same as the Initial total soil P content of seminatural land, but a little lower.");
	
	// Params that vary by reach and land class (add to existing group)
	auto SubcatchmentGeneral = GetParameterGroupHandle(Model, "Subcatchment characteristics by land class");
	//auto LandUseProportionsNC = RegisterParameterDouble(Model, SubcatchmentGeneral, "Land use proportions from newly-converted", Dimensionless, 0.0, 0.0, 1.0, "Proportion of each land use class that has been newly-converted from another type. Only one of these should be >0");

	// Add to global system parameter group
	auto System = GetParameterGroupHandle(Model, "System");
	auto DynamicEPC0                    = RegisterParameterBool(Model, System, "Dynamic soil water EPC0, TDP and soil labile P", true, "Calculate a dynamic soil water EPC0 (the equilibrium P concentration of zero sorption), and therefore soilwater TDP concentration, so that it varies with labile P content? The labile P will also therefore vary");
	auto CalibrationMode                = RegisterParameterBool(Model, System, "Run in calibration mode", true, "Run model in calibration mode? If true, the initial agricultural soil water TDP concentration (and therefore EPC0) is calibrated and used to estimate the phosphorus sorption coefficient. If false, the sorption coefficient is read in from the parameter file");
	
	// Optional timeseries that the user can use instead of the corresponding parameters.
	auto NetAnnualPInputTimeseries = RegisterInput(Model, "Net annual P input to soil", KgPerHaPerYear);
	auto EffluentTDPTimeseries = RegisterInput(Model, "Effluent TDP inputs", KgPerDay);
	
	
	// Params defined in hydrol or sed modules
	auto CatchmentArea               = GetParameterDoubleHandle(Model, "Catchment area");
	auto SedimentInputNonlinearCoefficient = GetParameterDoubleHandle(Model, "Sediment input non-linear coefficient");
	auto LandUseProportions          = GetParameterDoubleHandle(Model, "Land use proportions");
	auto BaseflowIndex               = GetParameterDoubleHandle(Model, "Baseflow index");
	
	
	// START EQUATIONS
	
	// Equations defined in other modules
	auto SoilWaterVolume                = GetEquationHandle(Model, "Soil water volume");
	auto InfiltrationExcess             = GetEquationHandle(Model, "Infiltration excess");
	auto SoilWaterFlow                  = GetEquationHandle(Model, "Soil water flow");	
	auto ReachVolume                    = GetEquationHandle(Model, "Reach volume");
	auto ReachFlow                      = GetEquationHandle(Model, "Reach flow (end-of-day)");
	auto DailyMeanReachFlow             = GetEquationHandle(Model, "Reach flow (daily mean, cumecs)");
	auto GroundwaterFlow                = GetEquationHandle(Model, "Groundwater flow");
	auto ReachSedimentInputCoefficient  = GetEquationHandle(Model, "Sediment input coefficient");
	auto ErosionFactor                  = GetEquationHandle(Model, "Erosion factor");
	
	
/* 	// P sorption coefficient calculation
	//Method 1: This method calculates the parameter or reads it from file, depending on calibration mode. The result is saved to the model dataset, so its value can be extracted e.g. via the python wrapper, but not output in MobiView
	
	auto ComputedPhosphorousSorptionCoefficient = RegisterEquationInitialValue(Model, "Computed phosphorous sorption coefficient", MmPerKg);
	ParameterIsComputedBy(Model, PhosphorousSorptionCoefficient, ComputedPhosphorousSorptionCoefficient, false);  //NOTE: The 'false' is there to say that this parameter SHOULD still be exposed in parameter files.
	
	EQUATION(Model, ComputedPhosphorousSorptionCoefficient,
		double providedvalue = PARAMETER(PhosphorousSorptionCoefficient);
		double computedvalue = 
				1e-6 * (PARAMETER(InitialSoilPConcentration, Arable) -
				  PARAMETER(InitialSoilPConcentration, Seminatural))
				  /ConvertMgPerLToKgPerMm(PARAMETER(InitialEPC0, Arable), PARAMETER(CatchmentArea));
		
		//NOTE: If a value of 0 is possible, that has to be accommodated for in the equations below or otherwise it will crash.
		
		if(PARAMETER(CalibrationMode)) return computedvalue;
		return providedvalue;
	) */
	
	// P equations
	
	// Method 2: regular equation to define the sorption coefficient. Returns a constant value, which can be seen in e.g. MobiView.
	auto SoilPSorptionCoefficient = RegisterEquation(Model, "Soil phosphorous sorption coefficient", MmPerKg);
	auto InitialSoilWaterEPC0 = RegisterEquationInitialValue(Model, "Initial soil water EPC0", KgPerMm);
	auto SoilWaterEPC0   = RegisterEquation(Model, "Soil water EPC0", KgPerMm);
	SetInitialValue(Model, SoilWaterEPC0, InitialSoilWaterEPC0);
	
	EQUATION(Model, SoilPSorptionCoefficient,
		/* Units: (kg/mg)(mg/kgSoil)(mm/kg)*/
		double initialEPC0 = ConvertMgPerLToKgPerMm(PARAMETER(InitialEPC0), PARAMETER(CatchmentArea));

		double Kf = 1e-6 * SafeDivide((PARAMETER(InitialSoilPConcentration)-PARAMETER(SoilInactivePConcentration)), initialEPC0);
				  
		double KfPar = PARAMETER(PhosphorousSorptionCoefficient);
				  
		if(PARAMETER(CalibrationMode)) return Kf;
		
		return KfPar;
	)			

	auto InitialSoilTDPMass     = RegisterEquationInitialValue(Model, "Initial soil TDP mass", Kg);
	auto SoilTDPMass            = RegisterEquation(Model, "Soil TDP mass", Kg);
	SetInitialValue(Model, SoilTDPMass, InitialSoilTDPMass);
	
	auto InitialSoilLabilePMass = RegisterEquationInitialValue(Model, "Initial soil labile P mass", Kg);
	auto SoilLabilePMass = RegisterEquation(Model, "Soil labile P mass", Kg);
	SetInitialValue(Model, SoilLabilePMass, InitialSoilLabilePMass);
	
	EQUATION(Model, InitialSoilTDPMass,
		return RESULT(SoilWaterEPC0) * RESULT(SoilWaterVolume);
	)
	
	EQUATION(Model, SoilTDPMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double b = (RESULT(SoilPSorptionCoefficient) * Msoil + LAST_RESULT(SoilWaterFlow) + RESULT(InfiltrationExcess)) / LAST_RESULT(SoilWaterVolume);
		double a = IF_INPUT_ELSE_PARAMETER(NetAnnualPInputTimeseries, NetAnnualPInput) * 100.0 * PARAMETER(CatchmentArea) / 365.0 + RESULT(SoilPSorptionCoefficient) * Msoil * RESULT(SoilWaterEPC0);
		double value = a / b + (LAST_RESULT(SoilTDPMass) - a / b) * exp(-b);
		
		if(!PARAMETER(DynamicEPC0)) return LAST_RESULT(SoilTDPMass);
		
		return value;
	)
	
	EQUATION(Model, InitialSoilLabilePMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		return 1e-6 * (PARAMETER(InitialSoilPConcentration)-PARAMETER(SoilInactivePConcentration)) * Msoil;
	)
	
	EQUATION(Model, SoilLabilePMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double b0 = RESULT(SoilPSorptionCoefficient) * Msoil + LAST_RESULT(SoilWaterFlow) + RESULT(InfiltrationExcess);
		double b = b0 / LAST_RESULT(SoilWaterVolume);
		double a = IF_INPUT_ELSE_PARAMETER(NetAnnualPInputTimeseries, NetAnnualPInput) * 100.0 * PARAMETER(CatchmentArea) / 365.0 + RESULT(SoilPSorptionCoefficient) * Msoil * RESULT(SoilWaterEPC0);
		//TODO: factor out calculations of b0, a? Would probably not matter that much to speed though.
	
		double sorp = RESULT(SoilPSorptionCoefficient) * Msoil * (a / b0 - RESULT(SoilWaterEPC0) + (LAST_RESULT(SoilTDPMass)/LAST_RESULT(SoilWaterVolume) - a/b0)*(1.0 - exp(-b))/b);
		if(!std::isfinite(sorp)) sorp = 0.0; //NOTE: Otherwise calculation can break down in some rare cases.
		
		if(!PARAMETER(DynamicEPC0)) sorp = 0.0;
	
		return LAST_RESULT(SoilLabilePMass) + sorp;
	)
	
	EQUATION(Model, InitialSoilWaterEPC0,
		 return ConvertMgPerLToKgPerMm(PARAMETER(InitialEPC0), PARAMETER(CatchmentArea));
	)
	
	EQUATION(Model, SoilWaterEPC0,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double Kf = RESULT(SoilPSorptionCoefficient);
		double Plab_A = LAST_RESULT(SoilLabilePMass);
		
		if(PARAMETER(DynamicEPC0)) return SafeDivide(Plab_A, (Kf * Msoil));
		
		return LAST_RESULT(SoilWaterEPC0);
	)
	
	
	// Post-processing soil P equations (convert units)
	auto SoilWaterTDPConcentration = RegisterEquation(Model, "Soil water TDP concentration", MgPerL);
	auto EPC0MgL                   = RegisterEquation(Model, "Soil water EPC0 in mg/l", MgPerL);
	auto SoilLabilePConcentration  = RegisterEquation(Model, "Soil labile P concentration", MgPerKg);
	
	EQUATION(Model, SoilWaterTDPConcentration,
		double dynamicTDPConc = ConvertKgPerMmToMgPerL(SafeDivide(RESULT(SoilTDPMass),RESULT(SoilWaterVolume)), PARAMETER(CatchmentArea));
		double constantTDPConc = PARAMETER(InitialEPC0);
		
		if(!PARAMETER(DynamicEPC0)) return constantTDPConc;
		return dynamicTDPConc;		
	)
	
	EQUATION(Model, EPC0MgL,
		double dynamic_EPC0 = ConvertKgPerMmToMgPerL(RESULT(SoilWaterEPC0), PARAMETER(CatchmentArea));
		double constantEPC0 = PARAMETER(InitialEPC0);
		
		if(!PARAMETER(DynamicEPC0)) return constantEPC0; //This is not necessary since the SoilWaterEPC0 equation does this check anyway..
		return dynamic_EPC0;
	)
	
	EQUATION(Model, SoilLabilePConcentration,
		double labilePMassMg = 1e6 * RESULT(SoilLabilePMass);
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double constantLabilePConc = PARAMETER(InitialSoilPConcentration);
		
		if(!PARAMETER(DynamicEPC0)) return constantLabilePConc;
		return labilePMassMg/Msoil;
	)
	
	auto SoilTDPOutput = RegisterEquation(Model, "Soil TDP output", KgPerDay);
	auto TotalSoilTDPOutput = RegisterEquationCumulative(Model, "Total soil TDP output", SoilTDPOutput, LandscapeUnits);
	
	EQUATION(Model, SoilTDPOutput,
		//TODO: Should consider using "integrated" values here similar to in the equation for the TDP mass value..
		return SafeDivide(RESULT(SoilTDPMass), RESULT(SoilWaterVolume)) * (1.0-PARAMETER(BaseflowIndex)) * RESULT(SoilWaterFlow) * PARAMETER(LandUseProportions); 
	)
	
	auto ReachSolver = GetSolverHandle(Model, "SimplyQ reach solver");
	
	// Reach equations	
	
	auto StreamTDPFlux = RegisterEquation(Model, "Reach TDP flux", KgPerDay);
	SetSolver(Model, StreamTDPFlux, ReachSolver);
	
	auto StreamPPFlux  = RegisterEquation(Model, "Reach PP flux", KgPerDay);
	SetSolver(Model, StreamPPFlux, ReachSolver);
	
	auto StreamTDPMass = RegisterEquationODE(Model, "Reach TDP mass", Kg);
	SetInitialValue(Model, StreamTDPMass, 0.0);
	SetSolver(Model, StreamTDPMass, ReachSolver);
	
	auto StreamPPMass  = RegisterEquationODE(Model, "Reach PP mass", Kg);
	SetInitialValue(Model, StreamPPMass, 0.0);
	SetSolver(Model, StreamPPMass, ReachSolver);
	
	auto DailyMeanStreamTDPFlux = RegisterEquationODE(Model, "Reach daily mean TDP flux", KgPerDay);
	SetInitialValue(Model, DailyMeanStreamTDPFlux, 0.0);
	SetSolver(Model, DailyMeanStreamTDPFlux, ReachSolver);
	ResetEveryTimestep(Model, DailyMeanStreamTDPFlux);
	
	auto DailyMeanStreamPPFlux = RegisterEquationODE(Model, "Reach daily mean PP flux", KgPerDay);
	SetInitialValue(Model, DailyMeanStreamPPFlux, 0.0);
	SetSolver(Model, DailyMeanStreamPPFlux, ReachSolver);
	ResetEveryTimestep(Model, DailyMeanStreamPPFlux);
	
	auto ReachTDPInputFromUpstream = RegisterEquation(Model, "Reach TDP input from upstream", KgPerDay);
	auto ReachPPInputFromErosion   = RegisterEquation(Model, "Reach PP input from erosion and entrainment", KgPerDay);
	SetSolver(Model, ReachPPInputFromErosion, ReachSolver);
	//auto TotalReachPPInputFromErosion = RegisterEquationCumulative(Model, "Total reach PP input from erosion", ReachPPInputFromErosion, LandscapeUnits);
	auto ReachPPInputFromUpstream  = RegisterEquation(Model, "Reach PP input from upstream", KgPerDay);
	
	EQUATION(Model, StreamTDPFlux,
		return 86400.0 * RESULT(ReachFlow) * SafeDivide(RESULT(StreamTDPMass), RESULT(ReachVolume));
	)
	
	EQUATION(Model, DailyMeanStreamTDPFlux,
		return RESULT(StreamTDPFlux);
	)
	
	EQUATION(Model, StreamPPFlux,
		return 86400.0 * RESULT(ReachFlow) * SafeDivide(RESULT(StreamPPMass), RESULT(ReachVolume));
	)
	
	EQUATION(Model, DailyMeanStreamPPFlux,
		return RESULT(StreamPPFlux);
	)
	
	EQUATION(Model, ReachTDPInputFromUpstream,
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(DailyMeanStreamTDPFlux, *Input);
		)
		return upstreamflux;
	)
	
	EQUATION(Model, StreamTDPMass,
		return
			  RESULT(TotalSoilTDPOutput)
			+ RESULT(GroundwaterFlow) * ConvertMgPerLToKgPerMm(PARAMETER(GroundwaterTDPConcentration), PARAMETER(CatchmentArea))
			+ IF_INPUT_ELSE_PARAMETER(EffluentTDPTimeseries, EffluentTDP)
			+ RESULT(ReachTDPInputFromUpstream)
			- RESULT(StreamTDPFlux);
	)
	
	EQUATION(Model, ReachPPInputFromErosion,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double PP = 0.0;
		double P_inactive = 1e-6*PARAMETER(SoilInactivePConcentration)*Msoil;
		for(index_t Land = FIRST_INDEX(LandscapeUnits); Land < INDEX_COUNT(LandscapeUnits); ++Land)
		{
			//NOTE: This could be factored out as its own equation and summed up using an EquationCumulative:
			PP += (RESULT(SoilLabilePMass, Land) + P_inactive) * RESULT(ReachSedimentInputCoefficient, Land);
		}
		return RESULT(ErosionFactor) * PARAMETER(PPEnrichmentFactor) * PP / Msoil;
	)
	
	EQUATION(Model, ReachPPInputFromUpstream,
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(DailyMeanStreamPPFlux, *Input);
		)
		return upstreamflux;
	)
	
	EQUATION(Model, StreamPPMass,
		return
			  RESULT(ReachPPInputFromErosion)
			+ RESULT(ReachPPInputFromUpstream)
			- RESULT(StreamPPFlux);
	)
	
	
	// Post-processing equations (mostly unit conversions)

	auto DailyMeanStreamTPFlux = RegisterEquation(Model, "Reach daily mean TP flux", KgPerDay);
	auto DailyMeanStreamSRPFlux = RegisterEquation(Model, "Reach daily mean SRP flux", KgPerDay);
	auto TDPConcentration = RegisterEquation(Model, "Reach TDP concentration", MgPerL); //Volume-weighted daily mean
	auto PPConcentration  = RegisterEquation(Model, "Reach PP concentration", MgPerL); //Volume-weighted daily mean
	auto TPConcentration  = RegisterEquation(Model, "Reach TP concentration", MgPerL); //Volume-weighted daily mean
	auto SRPConcentration = RegisterEquation(Model, "Reach SRP concentration", MgPerL); //Volume-weighted daily mean

	EQUATION(Model, DailyMeanStreamTPFlux,
		return RESULT(DailyMeanStreamTDPFlux) + RESULT(DailyMeanStreamPPFlux);
	)
	
	EQUATION(Model, DailyMeanStreamSRPFlux,
		return RESULT(DailyMeanStreamTDPFlux) * PARAMETER(SRPFraction);
	)
	
	EQUATION(Model, TDPConcentration,
		//Converting flow m3/s->m3/day, and converting kg/m3 to mg/l. TODO: make conversion functions
		return 1e3 * SafeDivide(RESULT(DailyMeanStreamTDPFlux), 86400.0 * RESULT(DailyMeanReachFlow));
	)
	
	EQUATION(Model, PPConcentration,
		//Converting flow m3/s->m3/day, and converting kg/m3 to mg/l. TODO: make conversion functions
		return 1e3 * SafeDivide(RESULT(DailyMeanStreamPPFlux), 86400.0 * RESULT(DailyMeanReachFlow));
	)
	
	EQUATION(Model, TPConcentration,
		return RESULT(TDPConcentration) + RESULT(PPConcentration);
	)
	
	EQUATION(Model, SRPConcentration,
		return RESULT(TDPConcentration) * PARAMETER(SRPFraction);
	)
	
	
	EndModule(Model);
}
/*
static void
AddSimplyPInputToWaterBodyModule(mobius_model *Model)
{
	auto M3PerSecond = RegisterUnit(Model, "m^3/s");
	auto KgPerDay    = RegisterUnit(Model, "kg/day");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	
	auto WaterBody = RegisterParameterGroup(Model, "Input to water body", Reach);
	
	auto IsInputToWaterBody = RegisterParameterBool(Model, WaterBody, "Is input to water body", false, "Whether or not the flow and various fluxes from this reach should be summed up in the calculation of inputs to a water body or lake");
	
	auto DailyMeanReachFlow = GetEquationHandle(Model, "Reach flow (daily mean, cumecs)");
	auto DailyMeanSuspendedSedimentFlux = GetEquationHandle(Model, "Reach daily mean suspended sediment flux");
	auto DailyMeanTDPFlux = GetEquationHandle(Model, "Reach daily mean TDP flux");
	auto DailyMeanPPFlux  = GetEquationHandle(Model, "Reach daily mean PP flux");
	auto DailyMeanTPFlux  = GetEquationHandle(Model, "Reach daily mean TP flux");
	auto DailyMeanSRPFlux = GetEquationHandle(Model, "Reach daily mean SRP flux");
	
	auto CatchmentArea = GetParameterDoubleHandle(Model, "Catchment area");
	
	auto FlowToWaterBody     = RegisterEquation(Model, "Flow to water body", M3PerSecond);
	auto SSFluxToWaterBody   = RegisterEquation(Model, "SS flux to water body", KgPerDay);
	auto TDPFluxToWaterBody  = RegisterEquation(Model, "TDP flux to water body", KgPerDay);
	auto PPFluxToWaterBody   = RegisterEquation(Model, "PP flux to water body", KgPerDay);
	auto TPFluxToWaterBody   = RegisterEquation(Model, "TP flux to water body", KgPerDay);
	auto SRPFluxToWaterBody  = RegisterEquation(Model, "SRP flux to water body", KgPerDay);
	
	//TODO: We should maybe have a shorthand for this kind of cumulative equation?
	
	EQUATION(Model, FlowToWaterBody,
		double sum = 0.0;
		
		for(index_t ReachIndex = FIRST_INDEX(Reach); ReachIndex < INDEX_COUNT(Reach); ++ReachIndex)
		{
			double q = RESULT(DailyMeanReachFlow, ReachIndex);
			if(PARAMETER(IsInputToWaterBody, ReachIndex))
			{
				sum += q;
			}
		}
		return sum;
	)
	
	EQUATION(Model, SSFluxToWaterBody,
		double sum = 0.0;
		
		for(index_t ReachIndex = FIRST_INDEX(Reach); ReachIndex < INDEX_COUNT(Reach); ++ReachIndex)
		{
			double ss = RESULT(DailyMeanSuspendedSedimentFlux, ReachIndex);
			if(PARAMETER(IsInputToWaterBody, ReachIndex))
			{
				sum += ss;
			}
		}
		return sum;
	)
	
	EQUATION(Model, TDPFluxToWaterBody,
		double sum = 0.0;
		
		for(index_t ReachIndex = FIRST_INDEX(Reach); ReachIndex < INDEX_COUNT(Reach); ++ReachIndex)
		{
			double tdp = RESULT(DailyMeanTDPFlux, ReachIndex);
			if(PARAMETER(IsInputToWaterBody, ReachIndex))
			{
				sum += tdp;
			}
		}
		return sum;
	)
	
	EQUATION(Model, PPFluxToWaterBody,
		double sum = 0.0;
		
		for(index_t ReachIndex = FIRST_INDEX(Reach); ReachIndex < INDEX_COUNT(Reach); ++ReachIndex)
		{
			double pp = RESULT(DailyMeanPPFlux, ReachIndex);
			if(PARAMETER(IsInputToWaterBody, ReachIndex))
			{
				sum += pp;
			}
		}
		return sum;
	)
	
	EQUATION(Model, TPFluxToWaterBody,
		double sum = 0.0;
		
		for(index_t ReachIndex = FIRST_INDEX(Reach); ReachIndex < INDEX_COUNT(Reach); ++ReachIndex)
		{
			double tp = RESULT(DailyMeanTPFlux, ReachIndex);
			if(PARAMETER(IsInputToWaterBody, ReachIndex))
			{
				sum += tp;
			}
		}
		return sum;
	)
	
	EQUATION(Model, SRPFluxToWaterBody,
		double sum = 0.0;
		
		for(index_t ReachIndex = FIRST_INDEX(Reach); ReachIndex < INDEX_COUNT(Reach); ++ReachIndex)
		{
			double srp = RESULT(DailyMeanSRPFlux, ReachIndex);
			if(PARAMETER(IsInputToWaterBody, ReachIndex))
			{
				sum += srp;
			}
		}
		return sum;
	)
	
}

*/

