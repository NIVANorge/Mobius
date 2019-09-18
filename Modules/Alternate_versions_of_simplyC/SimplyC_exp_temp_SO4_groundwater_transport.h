#include "UnitConversions.h"

static void
AddSimplyCModel(mobius_model *Model)
{
	BeginModule(Model, "SimplyC", "test_01")
	// Inputs
	auto AirTemperature = GetInputHandle(Model, "Air temperature");

	auto SO4Deposition = RegisterInput(Model, "SO4 deposition");

	// Solvers already defined in hydrology module
	auto LandSolver = GetSolverHandle(Model, "SimplyQ land solver");
	auto ReachSolver = GetSolverHandle(Model, "SimplyQ reach solver");

	// Units
	auto Kg			= RegisterUnit(Model, "kg");
	auto KgPerDay	= RegisterUnit(Model, "kg/day");
	auto Dimensionless = RegisterUnit(Model);
	auto MgPerL		 = RegisterUnit(Model, "mg/l");
	auto PerDegreesC = RegisterUnit(Model, "1/degreesC");

	// Set up index sets
	auto Reach          = GetIndexSetHandle(Model, "Reaches"); //Defined in SimplyQ.h
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units"); //Defined in SimplyQ.h
//	auto LowCarbon   = RequireIndex(Model, LandscapeUnits, "Low soil carbon"); //Not sure whether to do this or just recommend people follow this
//	auto HighCarbon  = RequireIndex(Model, LandscapeUnits, "High soil carbon"); //Not sure whether to do this

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
	auto DOCSoilTemperatureResponse = RegisterParameterDouble(Model, CarbonParamsGlobal, "Response of soil water [DOC] to changing soil temperature", PerDegreesC, 0.1, 0.01, 4.0);
	auto SoilCarbonSolubilityResponseToSO4 = RegisterParameterDouble(Model, CarbonParamsGlobal, "Soil carbon solubility response to SO4 deposition", Dimensionless, 0.0, 0.0, 20.0);


	// Carbon params that vary with land class
	auto CarbonParamsLand = RegisterParameterGroup(Model, "Carbon land", LandscapeUnits);
	auto BaselineSoilDOCConcentration = RegisterParameterDouble(Model, CarbonParamsLand, "Baseline soil water DOC concentration", MgPerL, 10.0, 0.0, 70.0);

	// EQUATIONS

	// Equations defined in hydrology module required here
	auto SoilWaterVolume 			 = GetEquationHandle(Model, "Soil water volume");
	auto InfiltrationExcess          = GetEquationHandle(Model, "Infiltration excess");
	auto SoilWaterFlow   		     = GetEquationHandle(Model, "Soil water flow");
#ifdef SIMPLYQ_GROUNDWATER
	auto GroundwaterFlow             = GetEquationHandle(Model, "Groundwater flow");
	auto GroundwaterVolume           = GetEquationHandle(Model, "Groundwater volume");
#endif
	auto ReachVolume                 = GetEquationHandle(Model, "Reach volume");
	auto ReachFlow                   = GetEquationHandle(Model, "Reach flow (end-of-day)");
	auto SnowMelt					 = GetEquationHandle(Model, "Snow melt");
	auto SnowDepth					 = GetEquationHandle(Model, "Snow depth as water equivalent");
	auto PrecipitationFallingAsRain  = GetEquationHandle(Model, "Precipitation falling as rain");

	// Equation from soil temperature module
	auto SoilTemperature       = GetEquationHandle(Model, "Soil temperature corrected for insulating effect of snow");

	// Carbon equations which vary by land class

	auto SoilWaterCarbonConcentration = RegisterEquation(Model, "Soil water DOC concentration, mg/l", MgPerL);
	
	auto InfiltrationExcessCarbonFluxToReach = RegisterEquation(Model, "Quick flow DOC flux scaled by land class area", KgPerDay);
	
	auto SoilwaterCarbonFlux = RegisterEquation(Model, "Soil water carbon flux", KgPerDay);
	SetSolver(Model, SoilwaterCarbonFlux, LandSolver);
	
	auto DailyMeanSoilwaterCarbonFlux = RegisterEquationODE(Model, "Soil water carbon flux, daily mean", KgPerDay);
	SetInitialValue(Model, DailyMeanSoilwaterCarbonFlux, 0.0);	
	SetSolver(Model, DailyMeanSoilwaterCarbonFlux, LandSolver);
	ResetEveryTimestep(Model, DailyMeanSoilwaterCarbonFlux);

	EQUATION(Model, SoilWaterCarbonConcentration,
		return (PARAMETER(BaselineSoilDOCConcentration) - PARAMETER(SoilCarbonSolubilityResponseToSO4)*INPUT(SO4Deposition))*pow(PARAMETER(DOCSoilTemperatureResponse), RESULT(SoilTemperature));
	)
	
/* 	EQUATION(Model, InfiltrationExcessCarbonFluxToReach,
		return PARAMETER(LandUseProportions) * RESULT(InfiltrationExcess)
			   * ConvertMgPerLToKgPerMm(RESULT(SoilWaterCarbonConcentration), PARAMETER(CatchmentArea));
	) */
	
	EQUATION(Model, InfiltrationExcessCarbonFluxToReach,
		double quickDOCconcentration;
		double f_melt = PARAMETER(ProportionToQuickFlow)*RESULT(SnowMelt)/RESULT(InfiltrationExcess);
		double f_rain = 1.0-f_melt;
		double soilwaterDOCconc = RESULT(SoilWaterCarbonConcentration);
		if (RESULT(InfiltrationExcess)>0.) quickDOCconcentration = f_melt*6.0 + f_rain*soilwaterDOCconc;
		else quickDOCconcentration = soilwaterDOCconc;
			
		return RESULT(InfiltrationExcess)
			   * ConvertMgPerLToKgPerMm(quickDOCconcentration, PARAMETER(CatchmentArea));
	)

	EQUATION(Model, SoilwaterCarbonFlux,
		return
			ConvertMgPerLToKgPerMm(RESULT(SoilWaterCarbonConcentration), PARAMETER(CatchmentArea))
			* RESULT(SoilWaterFlow);
	)
	
	EQUATION(Model, DailyMeanSoilwaterCarbonFlux,
		return RESULT(SoilwaterCarbonFlux);
	)
	
	// Instream equations

	auto TotalSoilwaterCarbonFlux = RegisterEquationCumulative(Model, "Soilwater carbon flux summed over landscape units", DailyMeanSoilwaterCarbonFlux, LandscapeUnits, LandUseProportions);
	
	auto TotalInfiltrationExcessCarbonFlux = RegisterEquationCumulative(Model, "Quick flow DOC flux to reach summed over landscape units", InfiltrationExcessCarbonFluxToReach, LandscapeUnits, LandUseProportions);

#ifdef SIMPLYQ_GROUNDWATER
	auto GroundwaterDOCMass = RegisterEquationODE(Model, "Groundwater DOC mass", Kg);
	SetSolver(Model, GroundwaterDOCMass, ReachSolver);

	auto GroundwaterFluxToReach = RegisterEquation(Model, "Groundwater carbon flux to reach", KgPerDay);
	SetSolver(Model, GroundwaterFluxToReach, ReachSolver);
	
	auto GroundwaterDOCConcentration = RegisterEquation(Model, "Groundwater DOC concentration", MgPerL);
#endif
	
	//To do: work initial condition out from baseline DOC parameter
	auto StreamDOCMass = RegisterEquationODE(Model, "Reach DOC mass", Kg);
	SetInitialValue(Model, StreamDOCMass, 0.02); 
	SetSolver(Model, StreamDOCMass, ReachSolver);

	auto StreamDOCFluxOut = RegisterEquation(Model, "DOC flux from reach, end-of-day", KgPerDay);
	SetSolver(Model, StreamDOCFluxOut, ReachSolver);

	auto DailyMeanStreamDOCFlux = RegisterEquationODE(Model, "DOC flux from reach, daily mean", KgPerDay);
	SetInitialValue(Model, DailyMeanStreamDOCFlux, 0.0);
	SetSolver(Model, DailyMeanStreamDOCFlux, ReachSolver);
	ResetEveryTimestep(Model, DailyMeanStreamDOCFlux);

	auto ReachDOCConcentration = RegisterEquation(Model, "Reach DOC concentration (volume weighted daily mean)", MgPerL);

#ifdef SIMPLYQ_GROUNDWATER
	EQUATION(Model, GroundwaterDOCMass,
		return PARAMETER(BaseflowIndex) * RESULT(TotalSoilwaterCarbonFlux) - RESULT(GroundwaterFluxToReach);
	)
	
	EQUATION(Model, GroundwaterDOCConcentration,
		return ConvertKgPerMmToMgPerL(SafeDivide(RESULT(GroundwaterDOCMass), RESULT(GroundwaterVolume)), PARAMETER(CatchmentArea));
	)

 	EQUATION(Model, GroundwaterFluxToReach,
		//return RESULT(GroundwaterFlow)*ConvertMgPerLToKgPerMm(PARAMETER(DeepSoilDOCConcentration), PARAMETER(CatchmentArea));
		return RESULT(GroundwaterFlow) * SafeDivide(RESULT(GroundwaterDOCMass), RESULT(GroundwaterVolume));
	)
#endif
	
	EQUATION(Model, StreamDOCMass,
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(DailyMeanStreamDOCFlux, *Input);
		)
		return
			RESULT(TotalInfiltrationExcessCarbonFlux)
			+ RESULT(TotalSoilwaterCarbonFlux)
#ifdef SIMPLYQ_GROUNDWATER
			* (1.0 - PARAMETER(BaseflowIndex))
			+ RESULT(GroundwaterFluxToReach)
#endif
			+ upstreamflux
			- RESULT(StreamDOCFluxOut);		
	)
		
	EQUATION(Model, StreamDOCFluxOut,
		return 86400.0 * RESULT(ReachFlow) * SafeDivide(RESULT(StreamDOCMass), RESULT(ReachVolume));
	)

	EQUATION(Model, DailyMeanStreamDOCFlux,
		return RESULT(StreamDOCFluxOut);
	)

	EQUATION(Model, ReachDOCConcentration,
		return 1000.0 * SafeDivide(RESULT(StreamDOCMass), RESULT(ReachVolume));
	)
	
	EndModule(Model);
}

