#include "UnitConversions.h"

static void
AddSimplyCModel(mobius_model *Model)
{
	
	// Inputs
	auto AirTemperature = GetInputHandle(Model, "Air temperature");

	// Solvers already defined in hydrology module
	auto SimplySolverLand = GetSolverHandle(Model, "Simply solver, land");
	auto SimplySolverReach = GetSolverHandle(Model, "Simply solver, reach");

	// Units
	auto Kg			= RegisterUnit(Model, "kg");
	auto KgPerDay	= RegisterUnit(Model, "kg/day");
	auto Dimensionless = RegisterUnit(Model);
	auto MgPerL		 = RegisterUnit(Model, "mg/l");
	auto PerDegreesC = RegisterUnit(Model, "1/degreesC");

	// Set up indexers
	auto Reach          = GetIndexSetHandle(Model, "Reaches"); //Defined in SimplyHydrol.h
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units"); //Defined in SimplyHydrol.h
//	auto LowCarbon   = RequireIndex(Model, LandscapeUnits, "Low soil carbon");
//	auto HighCarbon  = RequireIndex(Model, LandscapeUnits, "High soil carbon");

	// PARAMS
	
	// Params defined in hydrol model
	auto CatchmentArea               = GetParameterDoubleHandle(Model, "Catchment area");
//	auto BaseflowIndex               = GetParameterDoubleHandle(Model, "Baseflow index");
	auto LandUseProportions			 = GetParameterDoubleHandle(Model, "Land use proportions");

	// Carbon params that don't vary with land class or sub-catchment/reach
	auto CarbonParamsGlobal = RegisterParameterGroup(Model, "Carbon global");
	auto SoilTemperatureCoefficient = RegisterParameterDouble(Model, CarbonParamsGlobal, "Gradient of the soil water [DOC] response to changing soil temperature", PerDegreesC, 0.1, 0.001, 1.0);
	auto SoilDOCInterceptCoefficient = RegisterParameterDouble(Model, CarbonParamsGlobal, "Coefficient describing intercept in [DOC]= m * soilT + c equation, as a proportion of the baseline DOC concentration", Dimensionless, 0.5);
//	auto DeepSoilDOCConcentration = RegisterParameterDouble(Model, CarbonParamsGlobal, "Mineral soil/groundwater DOC concentration", MgPerL, 0.0, 0.0, 30.0);

	// Carbon params that vary with land class
	auto CarbonParamsLand = RegisterParameterGroup(Model, "Carbon land", LandscapeUnits);
	auto BaselineSoilDOCConcentration = RegisterParameterDouble(Model, CarbonParamsLand, "Baseline soil water DOC concentration", MgPerL, 10.0, 0.0, 70.0);

	// EQUATIONS

	// Equations defined in hydrology module required here
	auto SoilWaterVolume 			 = GetEquationHandle(Model, "Soil water volume");
	auto InfiltrationExcess          = GetEquationHandle(Model, "Infiltration excess");
	auto SoilWaterFlow   		     = GetEquationHandle(Model, "Soil water flow");
//	auto TotalGroundwaterFlowToReach = GetEquationHandle(Model, "Total groundwater flow to reach from all land classes");
	auto ReachVolume                 = GetEquationHandle(Model, "Reach volume");
	auto ReachFlow                   = GetEquationHandle(Model, "Reach flow (end-of-day)");
	auto DailyMeanReachFlow          = GetEquationHandle(Model, "Reach flow (daily mean, mm/day)");

	// Equation from soil temperature module
	auto SoilTemperature       = GetEquationHandle(Model, "Soil temperature corrected for insulating effect of snow");

	// Carbon equations which vary by land class

	auto SoilWaterCarbonConcentration = RegisterEquation(Model, "Soil water DOC concentration, mg/l", MgPerL);
	
	auto InfiltrationExcessCarbonFluxToReach = RegisterEquation(Model, "Quick flow DOC flux scaled by land class area", KgPerDay);
	
	auto SoilwaterCarbonFlux = RegisterEquation(Model, "Soil water carbon flux", KgPerDay);
	SetSolver(Model, SoilwaterCarbonFlux, SimplySolverLand);
	
	auto DailyMeanSoilwaterCarbonFluxToReach = RegisterEquationODE(Model, "Soil water carbon flux to reach, daily mean", KgPerDay);
	SetInitialValue(Model, DailyMeanSoilwaterCarbonFluxToReach, 0.0);	
	SetSolver(Model, DailyMeanSoilwaterCarbonFluxToReach, SimplySolverLand);
	ResetEveryTimestep(Model, DailyMeanSoilwaterCarbonFluxToReach);

	EQUATION(Model, SoilWaterCarbonConcentration,
		double minSoilTemp = -PARAMETER(SoilDOCInterceptCoefficient)/PARAMETER(SoilTemperatureCoefficient);
		double SoilTempAboveZero = Max(minSoilTemp, RESULT(SoilTemperature)); //Quick fix to prevent negative concentrations. Needs revisiting
		return PARAMETER(SoilTemperatureCoefficient) * PARAMETER(BaselineSoilDOCConcentration) * SoilTempAboveZero
			   + PARAMETER(SoilDOCInterceptCoefficient) * PARAMETER(BaselineSoilDOCConcentration);
	)
	
	EQUATION(Model, InfiltrationExcessCarbonFluxToReach,
		return PARAMETER(LandUseProportions) * RESULT(InfiltrationExcess)
			   * ConvertMgPerLToKgPerMm(RESULT(SoilWaterCarbonConcentration), PARAMETER(CatchmentArea));
	)

	EQUATION(Model, SoilwaterCarbonFlux,
		return
			ConvertMgPerLToKgPerMm(RESULT(SoilWaterCarbonConcentration), PARAMETER(CatchmentArea))
			//* ((1.0-PARAMETER(BaseflowIndex)) * RESULT(SoilWaterFlow)) ; // With groundwater
			* RESULT(SoilWaterFlow); // No groundwater
	)
	
	EQUATION(Model, DailyMeanSoilwaterCarbonFluxToReach,
		return PARAMETER(LandUseProportions) * RESULT(SoilwaterCarbonFlux);
	)
	
	// Instream equations

	auto TotalSoilwaterCarbonFluxToReach = RegisterEquationCumulative(Model, "Soilwater carbon flux to reach summed over landscape units", DailyMeanSoilwaterCarbonFluxToReach, LandscapeUnits);
	
	auto TotalInfiltrationExcessCarbonFlux = RegisterEquationCumulative(Model, "Quick flow DOC flux to reach summed over landscape units", InfiltrationExcessCarbonFluxToReach, LandscapeUnits);
	
//	auto GroundwaterFluxToReach = RegisterEquation(Model, "Groundwater carbon flux to reach", KgPerDay);
	
	auto StreamDOCMass = RegisterEquationODE(Model, "Reach DOC mass", Kg);
	SetInitialValue(Model, StreamDOCMass, 0.0);
	SetSolver(Model, StreamDOCMass, SimplySolverReach);

	auto StreamDOCFluxOut = RegisterEquation(Model, "DOC flux from reach, end-of-day", KgPerDay);
	SetSolver(Model, StreamDOCFluxOut, SimplySolverReach);

	auto DailyMeanStreamDOCFlux = RegisterEquationODE(Model, "DOC flux from reach, daily mean", KgPerDay);
	SetInitialValue(Model, DailyMeanStreamDOCFlux, 0.0);
	SetSolver(Model, DailyMeanStreamDOCFlux, SimplySolverReach);
	ResetEveryTimestep(Model, DailyMeanStreamDOCFlux);

	auto ReachDOCConcentration = RegisterEquation(Model, "Reach DOC concentration (volume weighted daily mean)", MgPerL);

/* 	EQUATION(Model, GroundwaterFluxToReach,
		return RESULT(TotalGroundwaterFlowToReach)*ConvertMgPerLToKgPerMm(PARAMETER(DeepSoilDOCConcentration), PARAMETER(CatchmentArea));
	) */
	
	EQUATION(Model, StreamDOCMass,
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(DailyMeanStreamDOCFlux, *Input);
		)
		return
			RESULT(TotalInfiltrationExcessCarbonFlux)
			+ RESULT(TotalSoilwaterCarbonFluxToReach)
//			+ RESULT(GroundwaterFluxToReach)
			+ upstreamflux
			- RESULT(StreamDOCFluxOut);		
	)
		
	EQUATION(Model, StreamDOCFluxOut,
		return RESULT(StreamDOCMass) * RESULT(ReachFlow) / RESULT(ReachVolume);
	)

	EQUATION(Model, DailyMeanStreamDOCFlux,
		return RESULT(StreamDOCFluxOut);
	)

	EQUATION(Model, ReachDOCConcentration,
		return ConvertKgPerMmToMgPerL(RESULT(DailyMeanStreamDOCFlux) / RESULT(DailyMeanReachFlow), PARAMETER(CatchmentArea));
	)
	
	
}

