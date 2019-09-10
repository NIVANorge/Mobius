#include "../UnitConversions.h"

static void
AddSimplyCModel(mobius_model *Model)
{
	
	// Inputs
	auto SO4Deposition = RegisterInput(Model, "SO4 deposition");

	// Solvers already defined in hydrology module
	auto LandSolver = GetSolverHandle(Model, "SimplyQ land solver");
	auto ReachSolver = GetSolverHandle(Model, "SimplyQ reach solver");

	// Units
	auto Dimensionless = RegisterUnit(Model);
	auto Kg			= RegisterUnit(Model, "kg");
	auto KgPerKm2   = RegisterUnit(Model, "kg/km2");
	auto KgPerDay   = RegisterUnit(Model, "kg/day");
	auto KgPerKm2PerDay = RegisterUnit(Model, "kg/km2/day");
	auto Km2PerDay  = RegisterUnit(Model, "km2/day");
	auto MgPerL		 = RegisterUnit(Model, "mg/l");
	auto MgPerLPerC  = RegisterUnit(Model, "mg/l/°C");
	auto MgPerLPerC2 = RegisterUnit(Model, "mg/l/(°C)^2");

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
	
	auto SoilTemperatureDOCLinearCoefficient = RegisterParameterDouble(Model, CarbonParamsGlobal, "Soil temperature DOC concentration linear coefficient", MgPerLPerC, 0.0);
	auto SoilTemperatureDOCSquareCoefficient = RegisterParameterDouble(Model, CarbonParamsGlobal, "Soil temperature DOC concentration square coefficient", MgPerLPerC2, 0.0);

	auto SoilCSolubilityResponseToSO4deposition = RegisterParameterDouble(Model, CarbonParamsGlobal, "Soil carbon solubility response to SO4 deposition", Dimensionless, 0.0, 0.0, 20.0);
	
	auto DiluteSnow = RegisterParameterBool(Model, CarbonParamsGlobal, "Lower carbon concentration in meltwater", true);
	auto SnowMeltDOCConcentration = RegisterParameterDouble(Model, CarbonParamsGlobal, "Snow melt DOC concentration", MgPerL, 0.0, 0.0, 70.0);

	auto EquilibrationFactor = RegisterParameterDouble(Model, CarbonParamsGlobal, "Soil DOC equilibration factor", Km2PerDay, 0.1, 0.0, 10.0);

#ifdef SIMPLYQ_GROUNDWATER
	auto DeepSoilDOCConcentration = RegisterParameterDouble(Model, CarbonParamsGlobal, "Mineral soil/groundwater DOC concentration", MgPerL, 0.0, 0.0, 70.0);
#endif

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
#endif
	auto ReachVolume                 = GetEquationHandle(Model, "Reach volume");
	auto ReachFlow                   = GetEquationHandle(Model, "Reach flow (end-of-day)");
	auto DailyMeanReachFlow          = GetEquationHandle(Model, "Reach flow (daily mean, cumecs)");
	auto SnowMelt					 = GetEquationHandle(Model, "Snow melt");
	auto PrecipitationFallingAsRain  = GetEquationHandle(Model, "Precipitation falling as rain");

	// Equation from soil temperature module
	auto SoilTemperature       = GetEquationHandle(Model, "Soil temperature corrected for insulating effect of snow");

	// Carbon equations which vary by land class
	auto SoilWaterDOCMass             = RegisterEquationODE(Model, "Soil water DOC mass", KgPerKm2);
	SetSolver(Model, SoilWaterDOCMass, LandSolver);
	//SetInitialValue
	auto SoilWaterEquilibriumDOCConcentration = RegisterEquation(Model, "Soil water equilibrium DOC concentration", MgPerL);
	auto SoilWaterDOCConcentration = RegisterEquation(Model, "Soil water DOC concentration, mg/l", MgPerL);
	SetSolver(Model, SoilWaterDOCConcentration, LandSolver);
	
	auto InfiltrationExcessCarbonFluxToReach = RegisterEquation(Model, "Quick flow DOC flux scaled by land class area", KgPerKm2PerDay);
	SetSolver(Model, InfiltrationExcessCarbonFluxToReach, LandSolver);
	
	auto SoilWaterCarbonFlux = RegisterEquation(Model, "Soil water carbon flux", KgPerKm2PerDay);
	SetSolver(Model, SoilWaterCarbonFlux, LandSolver);
	
	auto DailyMeanSoilwaterCarbonFluxToReach = RegisterEquationODE(Model, "Soil water carbon flux to reach, daily mean", KgPerKm2PerDay);
	SetInitialValue(Model, DailyMeanSoilwaterCarbonFluxToReach, 0.0);	
	SetSolver(Model, DailyMeanSoilwaterCarbonFluxToReach, LandSolver);
	ResetEveryTimestep(Model, DailyMeanSoilwaterCarbonFluxToReach);
	
	
	EQUATION(Model, SoilWaterDOCMass,
		double kgperkm2diff = (RESULT(SoilWaterEquilibriumDOCConcentration) - RESULT(SoilWaterDOCConcentration));
		return
			  kgperkm2diff * PARAMETER(EquilibrationFactor)
			- RESULT(InfiltrationExcessCarbonFluxToReach)
			- RESULT(SoilWaterCarbonFlux);
			//TODO: if there is groundwater, we have to subtract the extra bit of DOC that goes to the deep soil to die.
	)
	
	EQUATION(Model, SoilWaterDOCConcentration,
		return SafeDivide(RESULT(SoilWaterDOCMass), RESULT(SoilWaterVolume));    // kg / (mm * km2) -> mg/l has conversion factor of 1 
	)

	EQUATION(Model, SoilWaterEquilibriumDOCConcentration,
		return PARAMETER(BaselineSoilDOCConcentration) + (PARAMETER(SoilTemperatureDOCLinearCoefficient) + PARAMETER(SoilTemperatureDOCSquareCoefficient) * RESULT(SoilTemperature)) * RESULT(SoilTemperature) - PARAMETER(SoilCSolubilityResponseToSO4deposition)*INPUT(SO4Deposition);
	)
	
	EQUATION(Model, InfiltrationExcessCarbonFluxToReach,
		double quickDOCconcentration;
		
		double f_melt = PARAMETER(ProportionToQuickFlow)*RESULT(SnowMelt)/RESULT(InfiltrationExcess);
		double f_rain = 1.0-f_melt;
		double soilwaterDOCconc = RESULT(SoilWaterDOCConcentration);
		double meltDOCconc      = PARAMETER(SnowMeltDOCConcentration);
		if (RESULT(InfiltrationExcess)>0.) quickDOCconcentration = f_melt*meltDOCconc + f_rain*soilwaterDOCconc;
		else quickDOCconcentration = soilwaterDOCconc;

		if(!PARAMETER(DiluteSnow)) quickDOCconcentration = soilwaterDOCconc;
		
		return RESULT(InfiltrationExcess) * quickDOCconcentration;
	)

	EQUATION(Model, SoilWaterCarbonFlux,
		return
			RESULT(SoilWaterDOCConcentration)
#ifdef SIMPLYQ_GROUNDWATER
			* (1.0-PARAMETER(BaseflowIndex))
#endif
			* RESULT(SoilWaterFlow);
	)
	
	EQUATION(Model, DailyMeanSoilwaterCarbonFluxToReach,
		return RESULT(SoilWaterCarbonFlux);
	)
	
	// Instream equations

	auto TotalSoilwaterCarbonFluxToReach = RegisterEquationCumulative(Model, "Soilwater carbon flux to reach summed over landscape units", DailyMeanSoilwaterCarbonFluxToReach, LandscapeUnits, LandUseProportions);
	
	auto TotalInfiltrationExcessCarbonFlux = RegisterEquationCumulative(Model, "Quick flow DOC flux to reach summed over landscape units", InfiltrationExcessCarbonFluxToReach, LandscapeUnits, LandUseProportions);

#ifdef SIMPLYQ_GROUNDWATER	
	auto GroundwaterFluxToReach = RegisterEquation(Model, "Groundwater carbon flux to reach", KgPerDay);
	SetSolver(Model, GroundwaterFluxToReach, ReachSolver);
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
 	EQUATION(Model, GroundwaterFluxToReach,
		return RESULT(GroundwaterFlow)*ConvertMgPerLToKgPerMm(PARAMETER(DeepSoilDOCConcentration), PARAMETER(CatchmentArea));
	)
#endif
	
	EQUATION(Model, StreamDOCMass,
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(DailyMeanStreamDOCFlux, *Input);
		)
		return
			(RESULT(TotalInfiltrationExcessCarbonFlux)
			+ RESULT(TotalSoilwaterCarbonFluxToReach)) * PARAMETER(CatchmentArea)
#ifdef SIMPLYQ_GROUNDWATER
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
	
	
}

