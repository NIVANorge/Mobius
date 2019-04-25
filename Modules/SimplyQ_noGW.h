// Simply Hydrology module

#include "Preprocessing/ThornthwaitePET.h"
#include "UnitConversions.h"

inline double
ActivationControl0(double X)
{
	return (3.0 - 2.0*X)*X*X;
}

inline double
ActivationControl(double X, double Threshold, double RelativeActivationDistance)
{
	if(X < Threshold) return 0.0;
	double Dist = Threshold * RelativeActivationDistance;
	if(X > Threshold + Dist) return 1.0;
	return ActivationControl0( (X - Threshold) / Dist );
}

static void
AddSimplyHydrologyModule(mobius_model *Model)
{
	auto Degrees = RegisterUnit(Model, "°");
	auto System = GetParameterGroupHandle(Model, "System");
	RegisterParameterDouble(Model, System, "Latitude", Degrees, 60.0, -90.0, 90.0, "Used in PET calculation if no PET timeseries was provided in the input data");
	
	AddPreprocessingStep(Model, ComputeThornthwaitePET); //NOTE: The preprocessing step is called at the start of each model run.
	
	auto Dimensionless     = RegisterUnit(Model);
	auto Mm                = RegisterUnit(Model, "mm");
	auto MmPerDegreePerDay = RegisterUnit(Model, "mm/°C/day");
	auto Days              = RegisterUnit(Model, "days");
	auto MmPerDay          = RegisterUnit(Model, "mm/day");
	auto PerM3             = RegisterUnit(Model, "m^{-3}");
	auto M3PerSecond       = RegisterUnit(Model, "m^3/s");
	auto Km2               = RegisterUnit(Model, "km^2");
	auto M                 = RegisterUnit(Model, "m");
	
	// Set up indexers
	auto Reach = RegisterIndexSetBranched(Model, "Reaches");
	auto LandscapeUnits = RegisterIndexSet(Model, "Landscape units");
	
	// Global snow parameters
	auto Snow = RegisterParameterGroup(Model, "Snow");
	
	auto InitialSnowDepth        = RegisterParameterDouble(Model, Snow, "Initial snow depth as water equivalent", Mm, 0.0, 0.0, 50000.0);
	auto DegreeDayFactorSnowmelt = RegisterParameterDouble(Model, Snow, "Degree-day factor for snowmelt", MmPerDegreePerDay, 2.74, 0.0, 5.0);
	
	// Hydrology parameters that don't vary by sub-catchment or reach
	auto Hydrology = RegisterParameterGroup(Model, "Hydrology");
	
	auto ProportionToQuickFlow   = RegisterParameterDouble(Model, Hydrology, "Proportion of precipitation that contributes to quick flow", Dimensionless, 0.020, 0.0, 0.99);
	auto PETReductionFactor      = RegisterParameterDouble(Model, Hydrology, "PET multiplication factor", Dimensionless, 1.0, 0.0, 2.0);
	auto SoilFieldCapacity       = RegisterParameterDouble(Model, Hydrology, "Soil field capacity", Mm, 290.0, 0.1, 1000.0);
	auto A                       = RegisterParameterDouble(Model, Hydrology, "Gradient of stream velocity-discharge relationship", PerM3, 0.5, 0.01, 0.99, "The a in V = aQ^b");
	auto B                       = RegisterParameterDouble(Model, Hydrology, "Exponent of stream velocity-discharge relationship", Dimensionless, 0.42, 0.1, 0.99, "The b in V = aQ^b");
	
	// General parameters that vary by reach or sub-catchment
	auto ReachParams = RegisterParameterGroup(Model, "General subcatchment and reach parameters", Reach);
	
	auto CatchmentArea           = RegisterParameterDouble(Model, ReachParams, "Catchment area", Km2, 51.7, 0.0, 10000.0);
	auto ReachLength             = RegisterParameterDouble(Model, ReachParams, "Reach length", M, 10000.0, 0.0, 10000000.0);
	
	// Instream hydrology parameters that vary by reach
	auto HydrologyReach = RegisterParameterGroup(Model, "Hydrology reach", Reach);
	
	auto InitialInStreamFlow     = RegisterParameterDouble(Model, HydrologyReach, "Initial in-stream flow", M3PerSecond, 1.0, 0.0, 1000000.0, "This parameter is only used by reaches that don't have other reaches as inputs.");
	
	// Terrestrial hydrology parameters that vary by land class
	auto HydrologyLand = RegisterParameterGroup(Model, "Hydrology land", LandscapeUnits);
	
	auto SoilWaterTimeConstant   = RegisterParameterDouble(Model, HydrologyLand, "Soil water time constant", Days, 2.0, 0.01, 30.0);
	
	// General parameters that vary by land class and reach
	auto SubcatchmentGeneral = RegisterParameterGroup(Model, "Subcatchment characteristics by land class", LandscapeUnits);
	SetParentGroup(Model, SubcatchmentGeneral, ReachParams);
	
	auto LandUseProportions   = RegisterParameterDouble(Model, SubcatchmentGeneral, "Land use proportions", Dimensionless, 0.5, 0.0, 1.0);
	
	// Inputs
	auto Precipitation  = RegisterInput(Model, "Precipitation");
	auto AirTemperature = RegisterInput(Model, "Air temperature");
	
	// Start equations
	
	// Non-ODE equations
	auto PrecipitationFallingAsSnow = RegisterEquation(Model, "Precipitation falling as snow", MmPerDay);
	auto PrecipitationFallingAsRain = RegisterEquation(Model, "Precipitation falling as rain", MmPerDay);
	auto PotentialDailySnowmelt     = RegisterEquation(Model, "Potential daily snowmelt", MmPerDay);
	auto SnowMelt                   = RegisterEquation(Model, "Snow melt", MmPerDay);
	auto SnowDepth                  = RegisterEquation(Model, "Snow depth as water equivalent", Mm);
	SetInitialValue(Model, SnowDepth, InitialSnowDepth);
	auto HydrologicalInputToSoilBox = RegisterEquation(Model, "Hydrological input to soil box", MmPerDay);
	
	EQUATION(Model, PrecipitationFallingAsSnow,
		double precip = INPUT(Precipitation);
		return (INPUT(AirTemperature) < 0) ? precip : 0.0;
	)
	
	EQUATION(Model, PrecipitationFallingAsRain,
		double precip = INPUT(Precipitation);
		return (INPUT(AirTemperature) > 0) ? precip : 0.0;
	)
	
	EQUATION(Model, PotentialDailySnowmelt,
		return Max(0.0, PARAMETER(DegreeDayFactorSnowmelt) * INPUT(AirTemperature));
	)
	
	EQUATION(Model, SnowMelt,
		return Min(LAST_RESULT(SnowDepth), RESULT(PotentialDailySnowmelt));
	)
	
	EQUATION(Model, SnowDepth,
		return LAST_RESULT(SnowDepth) + RESULT(PrecipitationFallingAsSnow) - RESULT(SnowMelt);
	)
	
	EQUATION(Model, HydrologicalInputToSoilBox,
		return RESULT(SnowMelt) + RESULT(PrecipitationFallingAsRain);
	)
	
	auto PotentialEvapoTranspiration = RegisterInput(Model, "Potential evapotranspiration");
	
	auto InfiltrationExcess = RegisterEquation(Model, "Infiltration excess", MmPerDay);
	auto Infiltration       = RegisterEquation(Model, "Infiltration", MmPerDay);
	
	EQUATION(Model, InfiltrationExcess,
		return PARAMETER(ProportionToQuickFlow) * RESULT(HydrologicalInputToSoilBox);
	)
	
	EQUATION(Model, Infiltration,
		return (1.0 - PARAMETER(ProportionToQuickFlow)) * RESULT(HydrologicalInputToSoilBox);
	)
	
	// ODE equations
	
	// Land phase
	
	auto SimplySolverLand = RegisterSolver(Model, "Simply solver, land", 0.01 /* 1.0/20000.0 */, IncaDascru);
	//auto SimplySolverLand = RegisterSolver(Model, "SimplyP solver", 0.001, BoostRK4);
	//auto SimplySolverLand = RegisterSolver(Model, "SimplyP solver", 0.001, BoostCashCarp54, 1e-6, 1e-6);
	//auto SimplySolverLand = RegisterSolver(Model, "SimplyP solver", 0.001, BoostRosenbrock4, 1e-3, 1e-3);
	//auto SimplySolverLand = RegisterSolver(Model, "SimplyP solver", 0.0025, Mtl4ImplicitEuler);   //NOTE: Being a first order method, this one is not that good..
	
	auto SoilWaterVolume = RegisterEquationODE(Model, "Soil water volume", Mm);
	SetInitialValue(Model, SoilWaterVolume, SoilFieldCapacity);
	SetSolver(Model, SoilWaterVolume, SimplySolverLand);
	
	auto SoilWaterFlow = RegisterEquation(Model, "Soil water flow", MmPerDay); // Total flow out of soil box
	SetSolver(Model, SoilWaterFlow, SimplySolverLand);

//	auto DailyMeanSoilWaterFlow = RegisterEquationODE(Model, "Soil water flow, daily mean", MmPerDay);
//	SetSolver(Model, DailyMeanSoilWaterFlow, SimplySolverLand);
//	ResetEveryTimestep(Model, DailyMeanSoilWaterFlow);
	
	auto SoilWaterFlowToReach = RegisterEquationODE(Model, "Soil water flow to reach, daily mean", MmPerDay); // This is used as basis of a cumulative equation below
	SetInitialValue(Model, SoilWaterFlowToReach, 0.0);		
	SetSolver(Model, SoilWaterFlowToReach, SimplySolverLand);
	ResetEveryTimestep(Model, SoilWaterFlowToReach);
	
	EQUATION(Model, SoilWaterFlow,
		double smd = PARAMETER(SoilFieldCapacity) - RESULT(SoilWaterVolume);
		//return - smd / (PARAMETER(SoilWaterTimeConstant, Arable) * (1.0 + exp(smd)));
		return -smd * ActivationControl(RESULT(SoilWaterVolume), PARAMETER(SoilFieldCapacity), 0.01) / PARAMETER(SoilWaterTimeConstant);
	)
	
	EQUATION(Model, SoilWaterFlowToReach,
		return RESULT(SoilWaterFlow) * PARAMETER(LandUseProportions);
	)
	
	EQUATION(Model, SoilWaterVolume,
		return
			  RESULT(Infiltration)
			- PARAMETER(PETReductionFactor) * INPUT(PotentialEvapoTranspiration) * (1.0 - exp(log(0.01) * RESULT(SoilWaterVolume) / PARAMETER(SoilFieldCapacity))) //NOTE: Should 0.01 be a parameter?
			- RESULT(SoilWaterFlow);	
	)
	
	// Sub-catchment/Reach equations
	
	auto SimplySolverReach = RegisterSolver(Model, "Simply solver, reach", 0.01 /* 1.0/20000.0 */, IncaDascru);
	
	auto TotalSoilwaterFlowToReach = RegisterEquationCumulative(Model, "Total soilwater flow to reach from all land classes", SoilWaterFlowToReach, LandscapeUnits); //Sum over LU
		
//	auto ReachFlowInput    = RegisterEquation(Model, "Reach flow input", MmPerDay);
//	SetSolver(Model, ReachFlowInput, SimplySolverReach);
	
	auto InitialReachVolume = RegisterEquationInitialValue(Model, "Initial reach volume", Mm); 
	auto ReachVolume        = RegisterEquationODE(Model, "Reach volume", Mm);
	SetInitialValue(Model, ReachVolume, InitialReachVolume);
	SetSolver(Model, ReachVolume, SimplySolverReach);
	
	auto InitialReachFlow   = RegisterEquationInitialValue(Model, "Initial reach flow", MmPerDay);
	auto ReachFlow          = RegisterEquationODE(Model, "Reach flow (end-of-day)", MmPerDay);
	SetInitialValue(Model, ReachFlow, InitialReachFlow);
	SetSolver(Model, ReachFlow, SimplySolverReach);
	
	auto DailyMeanReachFlow = RegisterEquationODE(Model, "Reach flow (daily mean, mm/day)", MmPerDay);
	SetInitialValue(Model, DailyMeanReachFlow, 0.0);
	SetSolver(Model, DailyMeanReachFlow, SimplySolverReach);
	ResetEveryTimestep(Model, DailyMeanReachFlow);

	
	EQUATION(Model, InitialReachFlow,
		double upstreamflow = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflow += RESULT(ReachFlow, *Input) * PARAMETER(CatchmentArea, *Input) / PARAMETER(CatchmentArea);
		)
		double initflow = ConvertM3PerSecondToMmPerDay(PARAMETER(InitialInStreamFlow), PARAMETER(CatchmentArea));

		if(INPUT_COUNT(Reach) == 0) return initflow;
		else return upstreamflow;
	)
	
	EQUATION(Model, ReachFlow,
		double upstreamflow = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflow += RESULT(DailyMeanReachFlow, *Input) * PARAMETER(CatchmentArea, *Input) / PARAMETER(CatchmentArea);
					 )
		double ReachFlowInput = upstreamflow + RESULT(InfiltrationExcess) + RESULT(TotalSoilwaterFlowToReach);
		return
			(ReachFlowInput - RESULT(ReachFlow))
			* PARAMETER(A) * pow(RESULT(ReachFlow), PARAMETER(B))*86400.0 / ((1.0-PARAMETER(B))*PARAMETER(ReachLength));
	)
	
	EQUATION(Model, InitialReachVolume,
		double initialreachtimeconstant = PARAMETER(ReachLength) / (PARAMETER(A) * pow(RESULT(ReachFlow), PARAMETER(B)) * 86400.0);
		
		return RESULT(ReachFlow) * initialreachtimeconstant;
	)
	
	EQUATION(Model, ReachVolume,
		double upstreamflow = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflow += RESULT(DailyMeanReachFlow, *Input) * PARAMETER(CatchmentArea, *Input) / PARAMETER(CatchmentArea);
					 )
		double ReachFlowInput = upstreamflow + RESULT(InfiltrationExcess) + RESULT(TotalSoilwaterFlowToReach);
		return ReachFlowInput - RESULT(ReachFlow);
	)
	
	EQUATION(Model, DailyMeanReachFlow,
		//NOTE: Since DailyMeanReachFlow is reset to start at 0 every timestep and since its derivative is the reach flow, its value becomes the integral of the reach flow over the timestep, i.e. the daily mean value.
		return RESULT(ReachFlow);
	)

	// Post-processing
	auto DailyMeanReachFlowCumecs = RegisterEquation(Model, "Reach flow (daily mean, cumecs)", M3PerSecond);
	
	EQUATION(Model, DailyMeanReachFlowCumecs,
		return ConvertMmPerDayToM3PerSecond(RESULT(DailyMeanReachFlow), PARAMETER(CatchmentArea));
	)
}
