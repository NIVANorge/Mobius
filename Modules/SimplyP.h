//SimplyP version 0.3

//NOTE: This is an adaption of
// https://github.com/LeahJB/SimplyP

//NOTE: This version is ahead of the Python version


//NOTE: Only include these if you are going to use them (they cause long compile times):
//#include "../boost_solvers.h"
//#include "../mtl_solvers.h"

#include "Preprocessing/ThornthwaitePET.h"

inline double
ConvertMmPerDayToM3PerDay(double MmPerDay, double CatchmentArea)
{
	return MmPerDay * 1000.0 * CatchmentArea;
}

inline double
ConvertM3PerSecondToMmPerDay(double M3PerSecond, double CatchmentArea)
{
	return M3PerSecond * 86400.0 / (1000.0 * CatchmentArea);
}

inline double
ConvertMmPerDayToM3PerSecond(double MmPerDay, double CatchmentArea)
{
	return MmPerDay * CatchmentArea / 86.4;
}

inline double
ConvertKgPerMmToMgPerL(double KgPerMm, double CatchmentArea)
{
	return KgPerMm / CatchmentArea;
}

inline double
ConvertMgPerLToKgPerMm(double MgPerL, double CatchmentArea)
{
	return MgPerL * CatchmentArea;
}

inline double
ConvertMmToM3(double Mm, double CatchmentArea)
{
	return Mm * 1000.0 * CatchmentArea;
}

inline double
ConvertM3ToMm(double M3, double CatchmentArea)
{
	return M3 / (1000 * CatchmentArea);
}

inline double
ConvertMmToLitres(double Mm, double CatchmentArea)
{
	return Mm * 1e6 * CatchmentArea;
}

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
AddSimplyPHydrologyModule(mobius_model *Model)
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
	auto DegreesCelsius    = RegisterUnit(Model, "°C");
	auto MPerM			   = RegisterUnit(Model, "m/m");
	
	// Set up index sets
	auto Reach = RegisterIndexSetBranched(Model, "Reaches");
	
	auto LandscapeUnits = RegisterIndexSet(Model, "Landscape units");
	
	auto Arable             = RequireIndex(Model, LandscapeUnits, "Arable");
	auto ImprovedGrassland  = RequireIndex(Model, LandscapeUnits, "Improved grassland");
	auto Seminatural        = RequireIndex(Model, LandscapeUnits, "Semi-natural");	
	
	// Global snow parameters
	auto Snow = RegisterParameterGroup(Model, "Snow");
	
	auto InitialSnowDepth        = RegisterParameterDouble(Model, Snow, "Initial snow depth as water equivalent", Mm, 0.0, 0.0, 50000.0);
	auto DegreeDayFactorSnowmelt = RegisterParameterDouble(Model, Snow, "Degree-day factor for snowmelt", MmPerDegreePerDay, 2.74, 0.0, 5.0);
	
	// Hydrology parameters that hopefully don't vary by sub-catchment or reach
	auto Hydrology = RegisterParameterGroup(Model, "Hydrology");
	
	auto ProportionToQuickFlow   = RegisterParameterDouble(Model, Hydrology, "Proportion of precipitation that contributes to quick flow", Dimensionless, 0.020, 0.0, 1.0);
	auto PETReductionFactor      = RegisterParameterDouble(Model, Hydrology, "PET multiplication factor", Dimensionless, 1.0, 0.0, 1.0);
	auto SoilFieldCapacity       = RegisterParameterDouble(Model, Hydrology, "Soil field capacity", Mm, 290.0, 0.0, 5000.0);
	auto BaseflowIndex           = RegisterParameterDouble(Model, Hydrology, "Baseflow index", Dimensionless, 0.70, 0.0, 1.0);
	auto GroundwaterTimeConstant = RegisterParameterDouble(Model, Hydrology, "Groundwater time constant", Days, 65.0, 0.5, 400.0);
	auto MinimumGroundwaterFlow  = RegisterParameterDouble(Model, Hydrology, "Minimum groundwater flow", MmPerDay, 0.40, 0.0, 10.0);
	auto ManningsCoefficient	 = RegisterParameterDouble(Model, Hydrology, "Manning's coefficient", Dimensionless, 0.04, 0.012, 0.1, "Default of 0.04 is for clean winding natural channels. See e.g. Chow 1959 for a table of values for other channel types") ;
	
	// General parameters that vary by reach or sub-catchment
	auto ReachParams = RegisterParameterGroup(Model, "General subcatchment and reach parameters", Reach);
	
	auto CatchmentArea           = RegisterParameterDouble(Model, ReachParams, "Catchment area", Km2, 51.7, 0.0, 10000.0);
	auto ReachLength             = RegisterParameterDouble(Model, ReachParams, "Reach length", M, 10000.0, 0.0, 10000000.0, "This is divided by two for headwater reaches in the model to calculate the average reach length travelled by water. If this is inappropriate for your headwater reach, adjust this parameter accordingly");
	auto ReachSlope    = RegisterParameterDouble(Model, ReachParams, "Reach slope", MPerM, 0.02, 0.00001, 3.0);	
	
	// Hydrology parameters that may vary by sub-catchment/reach
	auto HydrologyReach = RegisterParameterGroup(Model, "Hydrology subcatchment/reach", Reach);
	
	auto InitialInStreamFlow     = RegisterParameterDouble(Model, HydrologyReach, "Initial in-stream flow", M3PerSecond, 1.0, 0.0, 1000000.0, "This parameter is only used by reaches that don't have other reaches as inputs.");

	
	// Terrestrial hydrology parameters that vary by land class
	auto HydrologyLand = RegisterParameterGroup(Model, "Hydrology land", LandscapeUnits);
	
	auto SoilWaterTimeConstant   = RegisterParameterDouble(Model, HydrologyLand, "Soil water time constant", Days, 2.0, 0.05, 40.0, "Note: arable and improved grassland are grouped as 'agricultural' land, and only the arable soil hydrology parameters are used");
	
	// General parameters that vary by land class and reach
	auto SubcatchmentGeneral = RegisterParameterGroup(Model, "Subcatchment characteristics by land class", LandscapeUnits);
	SetParentGroup(Model, SubcatchmentGeneral, ReachParams);
	
	auto LandUseProportions   = RegisterParameterDouble(Model, SubcatchmentGeneral, "Land use proportions", Dimensionless, 0.5, 0.0, 1.0);
	
	// Inputs
	auto Precipitation  = RegisterInput(Model, "Precipitation", MmPerDay);
	auto AirTemperature = RegisterInput(Model, "Air temperature", Degrees);
	
	// Start equations
	
	// Non-ODE equations
	auto PrecipitationFallingAsSnow = RegisterEquation(Model, "Precipitation falling as snow", MmPerDay);
	auto PrecipitationFallingAsRain = RegisterEquation(Model, "Precipitation falling as rain", MmPerDay);
	auto PotentialDailySnowmelt     = RegisterEquation(Model, "Potential daily snowmelt", MmPerDay);
	auto SnowMelt                   = RegisterEquation(Model, "Snow melt", MmPerDay);
	auto SnowDepth                  = RegisterEquation(Model, "Snow depth", Mm);
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
	
	auto PotentialEvapoTranspiration = RegisterInput(Model, "Potential evapotranspiration", MmPerDay);
	
	auto InfiltrationExcess = RegisterEquation(Model, "Infiltration excess", MmPerDay);
	auto Infiltration       = RegisterEquation(Model, "Infiltration", MmPerDay);
	
	EQUATION(Model, InfiltrationExcess,
		return PARAMETER(ProportionToQuickFlow) * RESULT(HydrologicalInputToSoilBox);
	)
	
	EQUATION(Model, Infiltration,
		return (1.0 - PARAMETER(ProportionToQuickFlow)) * RESULT(HydrologicalInputToSoilBox);
	)
	
	// ODE equations
	
	auto SimplyPSolver = RegisterSolver(Model, "SimplyP solver", 0.01 /* 1.0/20000.0 */, IncaDascru);
	//auto SimplyPSolver = RegisterSolver(Model, "SimplyP solver", 0.001, BoostRK4);
	//auto SimplyPSolver = RegisterSolver(Model, "SimplyP solver", 0.001, BoostCashCarp54, 1e-6, 1e-6);
	//auto SimplyPSolver = RegisterSolver(Model, "SimplyP solver", 0.001, BoostRosenbrock4, 1e-3, 1e-3);
	//auto SimplyPSolver = RegisterSolver(Model, "SimplyP solver", 0.0025, Mtl4ImplicitEuler);   //NOTE: Being a first order method, this one is not that good..
	
	//NOTE: Ideally we would want the soil water volume equations to just be one equation that is autoindexed over landscape units, but that would create a difficulty when merging outflow from the landscape units to the reach as we could not do that inside the same solver (currently). Also, we could not let one instance of the calculation span both Arable and Improved-grassland as is done with Agricultural here.
	

	auto AgriculturalSoilWaterFlow = RegisterEquation(Model, "Agricultural soil water flow", MmPerDay);
	SetSolver(Model, AgriculturalSoilWaterFlow, SimplyPSolver);
	
	auto AgriculturalSoilWaterVolume = RegisterEquationODE(Model, "Agricultural soil water volume", Mm);
	SetInitialValue(Model, AgriculturalSoilWaterVolume, SoilFieldCapacity);
	SetSolver(Model, AgriculturalSoilWaterVolume, SimplyPSolver);
	
	EQUATION(Model, AgriculturalSoilWaterFlow,
		double smd = PARAMETER(SoilFieldCapacity) - RESULT(AgriculturalSoilWaterVolume);
		//return - smd / (PARAMETER(SoilWaterTimeConstant, Arable) * (1.0 + exp(smd)));
		return -smd * ActivationControl(RESULT(AgriculturalSoilWaterVolume), PARAMETER(SoilFieldCapacity), 0.01) / PARAMETER(SoilWaterTimeConstant, Arable);
	)
	
	EQUATION(Model, AgriculturalSoilWaterVolume,
		return
			  RESULT(Infiltration)
			- PARAMETER(PETReductionFactor) * INPUT(PotentialEvapoTranspiration) * (1.0 - exp(log(0.01) * RESULT(AgriculturalSoilWaterVolume) / PARAMETER(SoilFieldCapacity))) //NOTE: Should 0.01 be a parameter?
			- RESULT(AgriculturalSoilWaterFlow);	
	)
	
	auto SeminaturalSoilWaterFlow = RegisterEquation(Model, "Semi-natural soil water flow", MmPerDay);
	SetSolver(Model, SeminaturalSoilWaterFlow, SimplyPSolver);
	
	auto SeminaturalSoilWaterVolume = RegisterEquationODE(Model, "Semi-natural soil water volume", Mm);
	SetInitialValue(Model, SeminaturalSoilWaterVolume, SoilFieldCapacity);
	SetSolver(Model, SeminaturalSoilWaterVolume, SimplyPSolver);
	
	EQUATION(Model, SeminaturalSoilWaterFlow,
		double smd = PARAMETER(SoilFieldCapacity) - RESULT(SeminaturalSoilWaterVolume);
		//return - smd / (PARAMETER(SoilWaterTimeConstant, Seminatural) * (1.0 + exp(smd)));
		return - smd * ActivationControl(RESULT(SeminaturalSoilWaterVolume), PARAMETER(SoilFieldCapacity), 0.01) / PARAMETER(SoilWaterTimeConstant, Seminatural);
	)
	
	EQUATION(Model, SeminaturalSoilWaterVolume,
		return
			  RESULT(Infiltration)
			- PARAMETER(PETReductionFactor) * INPUT(PotentialEvapoTranspiration) * (1.0 - exp(log(0.01) * RESULT(AgriculturalSoilWaterVolume) / PARAMETER(SoilFieldCapacity))) //NOTE: Should 0.01 be a parameter?
			- RESULT(SeminaturalSoilWaterFlow);	
	)
	
	auto TotalSoilWaterFlow       = RegisterEquation(Model, "Landuse weighted soil water flow", MmPerDay);
	SetSolver(Model, TotalSoilWaterFlow, SimplyPSolver);
	
	EQUATION(Model, TotalSoilWaterFlow,
		double f_A = PARAMETER(LandUseProportions, Arable) + PARAMETER(LandUseProportions, ImprovedGrassland);
		double f_S = PARAMETER(LandUseProportions, Seminatural);
		return f_A * RESULT(AgriculturalSoilWaterFlow) + f_S * RESULT(SeminaturalSoilWaterFlow);
	)
	
	
	// Groundwater and In-stream hydrology equations (group together as one reach equation is used in a groundwater equation)
	
	// Derived parameters used in instream hydrol equations
	auto UpstreamArea 	= RegisterParameterDouble(Model, HydrologyReach, "Upstream land area", Km2, 0.0); //Derived param
	
	auto ComputedUpstreamArea = RegisterEquationInitialValue(Model, "Upstream area", Km2); //Excluding the current reach
	ParameterIsComputedBy(Model, UpstreamArea, ComputedUpstreamArea, true);  //'true' says this parameter shouldn't be exposed in parameter files

	EQUATION(Model, ComputedUpstreamArea,
		double upstreamarea = 0.0;
		FOREACH_INPUT(Reach,
			upstreamarea += PARAMETER(CatchmentArea, *Input);
			upstreamarea += PARAMETER(UpstreamArea, *Input);
			);
		
		return upstreamarea;
	)
	
	auto EffectiveReachLength  	 = RegisterParameterDouble(Model, ReachParams, "Effective reach length", M, 4000.0); //Derived param	
	auto ComputeEffectiveReachLength = RegisterEquationInitialValue(Model, "Effective reach length", M);
	ParameterIsComputedBy(Model, EffectiveReachLength, ComputeEffectiveReachLength, true);
	
	EQUATION(Model, ComputeEffectiveReachLength,
		// The reach length which an average water particle experiences in the sub-catchment. Assumes
		// constant sinuosity and geometry throughout the sub-catchment, and that the reach extends most
		// of the sub-catchment length (for top reaches)
		double f_US = PARAMETER(UpstreamArea)/(PARAMETER(UpstreamArea)+PARAMETER(CatchmentArea));
		double f_R = PARAMETER(CatchmentArea)/(PARAMETER(UpstreamArea)+PARAMETER(CatchmentArea));
		double effective_length = f_US*PARAMETER(ReachLength) + f_R*PARAMETER(ReachLength)/2.0;		
		
		return effective_length;
	)

	auto InitialGroundwaterVolume = RegisterEquationInitialValue(Model, "Initial groundwater volume", Mm);
	auto GroundwaterVolume        = RegisterEquationODE(Model, "Groundwater volume", Mm);
	SetInitialValue(Model, GroundwaterVolume, InitialGroundwaterVolume);
	SetSolver(Model, GroundwaterVolume, SimplyPSolver);
	
	auto GroundwaterFlow          = RegisterEquation(Model, "Groundwater flow", MmPerDay);
	SetSolver(Model, GroundwaterFlow, SimplyPSolver);
	
	auto ReachFlowInput    = RegisterEquation(Model, "Reach flow input", MmPerDay);
	SetSolver(Model, ReachFlowInput, SimplyPSolver);

	auto ReachVolume        = RegisterEquationODE(Model, "Reach volume", Mm);	
	auto InitialReachVolume = RegisterEquationInitialValue(Model, "Initial reach volume", Mm); 
	SetInitialValue(Model, ReachVolume, InitialReachVolume);
	SetSolver(Model, ReachVolume, SimplyPSolver);
	
	auto ReachFlow          = RegisterEquation(Model, "Reach flow (end-of-day)", MmPerDay);
	auto InitialReachFlow   = RegisterEquationInitialValue(Model, "Initial reach flow", MmPerDay);
	SetInitialValue(Model, ReachFlow, InitialReachFlow);
	SetSolver(Model, ReachFlow, SimplyPSolver);
	
	auto DailyMeanReachFlow = RegisterEquationODE(Model, "Reach flow (daily mean, mm/day)", MmPerDay);
	SetInitialValue(Model, DailyMeanReachFlow, 0.0);
	SetSolver(Model, DailyMeanReachFlow, SimplyPSolver);
	ResetEveryTimestep(Model, DailyMeanReachFlow);
	
	auto DailyMeanReachFlowCumecs = RegisterEquation(Model, "Reach flow (daily mean, cumecs)", M3PerSecond);
	
	// Groundwater equations
	
	EQUATION(Model, GroundwaterFlow,
		double flow0   = RESULT(GroundwaterVolume) / PARAMETER(GroundwaterTimeConstant);
		double flowmin = PARAMETER(MinimumGroundwaterFlow);
		double t = ActivationControl(flow0, flowmin, 0.01);
		return (1.0 - t)*flowmin + t*flow0;
	)
	
	EQUATION(Model, GroundwaterVolume,		
		return PARAMETER(BaseflowIndex) * RESULT(TotalSoilWaterFlow)
			- RESULT(GroundwaterFlow);
	)
	
/* 	EQUATION(Model, InitialGroundwaterVolume,
		double initialflow = PARAMETER(BaseflowIndex) * RESULT(ReachFlow);
		return initialflow * PARAMETER(GroundwaterTimeConstant);
	) */
	
	EQUATION(Model, InitialGroundwaterVolume,
		double upstreamflow = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflow += RESULT(ReachFlow, *Input); //Should this be scaled by ratio of areas?
			)
		double terrestrial_flowinput = RESULT(ReachFlow) - upstreamflow;
		double initialgroundwaterflow = PARAMETER(BaseflowIndex) * terrestrial_flowinput;
		return initialgroundwaterflow * PARAMETER(GroundwaterTimeConstant);
	)
	
	auto Control = RegisterEquation(Model, "Control", Dimensionless);
	
	EQUATION(Model, Control,
		//NOTE: We create this equation to put in the code that allow us to "hack" certain values.
		// The return value of this equation does not mean anything.
		
		double volume = RESULT(GroundwaterFlow)*PARAMETER(GroundwaterTimeConstant);  //Wow, somehow this does not register index sets correctly if it is passed directly inside the macro below! May want to debug that.
		SET_RESULT(GroundwaterVolume, volume);
		
		return 0.0;
	)
	
	// In-stream equations
	
	EQUATION(Model, ReachFlowInput,
		double upstreamflow = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflow += RESULT(DailyMeanReachFlow, *Input) * PARAMETER(CatchmentArea, *Input) / PARAMETER(CatchmentArea);
		)
		
		return upstreamflow + RESULT(InfiltrationExcess) + (1.0 - PARAMETER(BaseflowIndex)) * RESULT(TotalSoilWaterFlow) + RESULT(GroundwaterFlow);
	)
	
	EQUATION(Model, InitialReachFlow,
		double upstreamflow = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflow += RESULT(ReachFlow, *Input) * PARAMETER(CatchmentArea, *Input) / PARAMETER(CatchmentArea);
		)
		double initflow = ConvertM3PerSecondToMmPerDay(PARAMETER(InitialInStreamFlow), PARAMETER(CatchmentArea));

		if(INPUT_COUNT(Reach) == 0) return initflow;
		else return upstreamflow;
	)
	
	EQUATION(Model, ReachVolume,
		return RESULT(ReachFlowInput) - RESULT(ReachFlow);
	)
	
	EQUATION(Model, ReachFlow,
		/* Derived from: Q=V/T, where T=L/u, so Q = Vu/L (where V is reach volume, u is reach velocity, L is reach length)
		Then get an expression for u using the Manning equation, assuming a rectangular cross section and empirical power law relationships between stream depth and Q and stream width and Q. Then rearrange so all Qs are on the left hand side with exponent of 1. Some unit conversions as Mannings and empirical equations assume Q in m3/s. See https://hal.archives-ouvertes.fr/hal-00296854/document */
		
		double reachvolume_m3 = ConvertMmToM3(RESULT(ReachVolume), PARAMETER(CatchmentArea));
		double reachflow_m3perS = 0.28 * pow(
								  reachvolume_m3 * pow(PARAMETER(ReachSlope), 0.5)
								  / (PARAMETER(EffectiveReachLength) * PARAMETER(ManningsCoefficient)),
								  (1.5));
			
		return ConvertM3PerSecondToMmPerDay(reachflow_m3perS, PARAMETER(CatchmentArea));
	)
	
	EQUATION(Model, InitialReachVolume,
	//Assumes rectangular cross section
		double reachdepth = 0.349 * pow(PARAMETER(InitialInStreamFlow), 0.34);
		double reachwidth = 2.71 * pow(PARAMETER(InitialInStreamFlow), 0.557);
		double reachvolume_m3 = reachdepth * reachwidth * PARAMETER(ReachLength);
		
		return ConvertM3ToMm(reachvolume_m3, PARAMETER(CatchmentArea));
	)
	
	EQUATION(Model, DailyMeanReachFlow,
		//NOTE: Since DailyMeanReachFlow is reset to start at 0 every timestep and since its derivative is the reach flow, its value becomes the integral of the reach flow over the timestep, i.e. the daily mean value.
		return RESULT(ReachFlow);
	)
	
	EQUATION(Model, DailyMeanReachFlowCumecs,
		return ConvertMmPerDayToM3PerSecond(RESULT(DailyMeanReachFlow), PARAMETER(CatchmentArea));
	)
}


static void
AddSimplyPSedimentModule(mobius_model *Model)
{
	auto Dimensionless = RegisterUnit(Model);
	auto Kg            = RegisterUnit(Model, "kg");
	auto KgPerDay      = RegisterUnit(Model, "kg/day");
	auto KgPerMm       = RegisterUnit(Model, "kg/mm");
	auto JulianDay     = RegisterUnit(Model, "Julian day");
	auto Degrees       = RegisterUnit(Model, "°");
	auto MgPerL        = RegisterUnit(Model, "mg/l");
	
	// Set up index sets
	auto Reach          = GetIndexSetHandle(Model, "Reaches");
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	
	auto Arable             = RequireIndex(Model, LandscapeUnits, "Arable");
	auto ImprovedGrassland  = RequireIndex(Model, LandscapeUnits, "Improved grassland");
	auto Seminatural        = RequireIndex(Model, LandscapeUnits, "Semi-natural");	
	
	// Params already defined
	auto CatchmentArea = GetParameterDoubleHandle(Model, "Catchment area");
	auto ReachSlope    = GetParameterDoubleHandle(Model, "Reach slope");	
	
	// Add to global system param group
	auto System = GetParameterGroupHandle(Model, "System");
	auto DynamicErodibility                      = RegisterParameterBool(Model, System, "Dynamic erodibility", true, "If true, simulate the change in erodibility on arable land through the year due to cropping and harvesting practices");
	
	// Global sediment parameters (don't vary by land use/sub-catchment/reach
	auto Sediment = RegisterParameterGroup(Model, "Sediment");
	
	auto ReachSedimentInputScalingFactor         = RegisterParameterDouble(Model, Sediment, "Reach sediment input scaling factor", KgPerMm, 1500.0, 0.0, 100000.0, "Calibrated parameter linking simulated sediment to simulated discharge");
	auto SedimentInputNonlinearCoefficient = RegisterParameterDouble(Model, Sediment, "Sediment input non-linear coefficient", Dimensionless, 2.0, 0.1, 5.0); 
	auto DayOfYearWhenSoilErodibilityIsMaxSpring = RegisterParameterUInt(Model, Sediment, "Day of year when soil erodibility is at its max for spring-grown crops", JulianDay, 60, 30, 335, "Parameter only used if Dynamic erodibility is set to true and spring-sown crops are present in the catchment");
	auto DayOfYearWhenSoilErodibilityIsMaxAutumn = RegisterParameterUInt(Model, Sediment, "Day of year when soil erodibility is at its max for autumn-grown crops", JulianDay, 304, 30, 335, "Parameter only used if Dynamic erodibility is set to true and autumn-sown crops are present in the catchment");
	
	// Add more params to the general reach parameter group created in hydrol module
	auto ReachParams = GetParameterGroupHandle(Model, "General subcatchment and reach parameters");
	
	auto ProportionOfSpringGrownCrops            = RegisterParameterDouble(Model, ReachParams, "Proportion of spring grown crops", Dimensionless, 0.65, 0.0, 1.0, "Proportion spring-sown crops to make total arable land area (assume rest is autumn-sown). Only needed if Dynamic erodibility is true");
	
	// Params that vary by land class and reach
	auto SubcatchmentGeneral = GetParameterGroupHandle(Model, "Subcatchment characteristics by land class");
	auto MeanSlopeOfLand                         = RegisterParameterDouble(Model, SubcatchmentGeneral, "Mean slope of land in the subcatchment", Degrees, 4.0, 0.0, 90.0);
	
	auto LandUseProportions = GetParameterDoubleHandle(Model, "Land use proportions");
	
	// Sediment params that vary by land class
	auto SedimentLand = RegisterParameterGroup(Model, "Sediment land", LandscapeUnits);
	auto VegetationCoverFactor                   = RegisterParameterDouble(Model, SedimentLand, "Vegetation cover factor", Dimensionless, 0.2, 0.0, 1.0, "Vegetation cover factor, describing ratio between long-term erosion under the land use class, compared to under bare soil of the same soil type, slope, etc. Source from (R)USLE literature and area-weight as necessary to obtain a single value for the land class.");
	auto ReductionOfLoadInSediment               = RegisterParameterDouble(Model, SedimentLand, "Reduction of load in sediment", Dimensionless, 0.0, 0.0, 1.0, "Proportional reduction in load of sediment delivered to the reach due to management measures, e.g. buffer strips, filter fences, conservation tillage, etc."); //Note: may be better indexing this by reach? TO DO
	
	// Start equations
	
	auto SimplyPSolver = GetSolverHandle(Model, "SimplyP solver");
	
	// Equations already defined
	auto ReachFlow          = GetEquationHandle(Model, "Reach flow (end-of-day)");
	auto DailyMeanReachFlow = GetEquationHandle(Model, "Reach flow (daily mean, mm/day)");
	auto ReachVolume        = GetEquationHandle(Model, "Reach volume");
	
	// New equations
	auto TimeDependentVegetationCoverFactor = RegisterEquation(Model, "Time dependent vegetation cover factor", Dimensionless);
	auto ReachSedimentInputCoefficient         = RegisterEquation(Model, "Sediment input coefficient", KgPerMm);
	auto TotalReachSedimentInputCoefficient    = RegisterEquationCumulative(Model, "Sediment input coefficient summed over land classes", ReachSedimentInputCoefficient, LandscapeUnits);
	
	auto SuspendedSedimentFlux = RegisterEquation(Model, "Reach suspended sediment flux", KgPerDay);
	SetSolver(Model, SuspendedSedimentFlux, SimplyPSolver);
	
	auto ReachSedimentInput = RegisterEquation(Model, "Reach sediment input (erosion and entrainment)", KgPerDay);
	SetSolver(Model, ReachSedimentInput, SimplyPSolver);
	
	auto SuspendedSedimentMass = RegisterEquationODE(Model, "Reach suspended sediment mass", Kg);
	SetInitialValue(Model, SuspendedSedimentMass, 0.0);
	SetSolver(Model, SuspendedSedimentMass, SimplyPSolver);
	
	auto DailyMeanSuspendedSedimentFlux = RegisterEquationODE(Model, "Reach daily mean suspended sediment flux", KgPerDay);
	SetInitialValue(Model, DailyMeanSuspendedSedimentFlux, 0.0);
	SetSolver(Model, DailyMeanSuspendedSedimentFlux, SimplyPSolver);
	ResetEveryTimestep(Model, DailyMeanSuspendedSedimentFlux);
	
	auto SuspendedSedimentConcentration = RegisterEquation(Model, "Reach suspended sediment concentration", MgPerL); //Volume-weighted daily mean
	
	EQUATION(Model, TimeDependentVegetationCoverFactor,
		
		/*If arable land, work out a dynamic crop cover factor, to account for the variation
		in erodibility through the year due to harvesting and planting practices.
		*/
			
		double C_cover = PARAMETER(VegetationCoverFactor);
		
		double d_maxE[2];
		d_maxE[0] = (double)PARAMETER(DayOfYearWhenSoilErodibilityIsMaxSpring);
		d_maxE[1] = (double)PARAMETER(DayOfYearWhenSoilErodibilityIsMaxAutumn);
		
		double coverproportion[2];
		coverproportion[0] = PARAMETER(ProportionOfSpringGrownCrops);
		coverproportion[1] = 1.0 - coverproportion[0];
		
		//NOTE: Sine wave not implemented, only triangular.
		if(PARAMETER(DynamicErodibility) && (CURRENT_INDEX(LandscapeUnits) == Arable))
		{
			double C_cov = 0.0;
			
			double E_risk_period = 60.0;
			for(size_t Season = 0; Season < 2; ++Season)
			{
				double d_start = d_maxE[Season] - E_risk_period / 2.0;
				double d_end   = d_maxE[Season] + E_risk_period / 2.0;
				double d_mid   = d_maxE[Season];
				
				double dayNo = (double)CURRENT_DAY_OF_YEAR();
				
				double C_season;
				if(dayNo >= d_start && dayNo <= d_end)
				{
					if(dayNo < d_mid) C_season = LinearInterpolate(dayNo, d_start, d_mid, C_cover, 1.0);
					else              C_season = LinearInterpolate(dayNo, d_mid,   d_end, 1.0, C_cover);
				}
				else C_season = C_cover - E_risk_period*(1.0 - C_cover)/(2.0*(DAYS_THIS_YEAR()-E_risk_period));
				
				C_cov += C_season * coverproportion[Season];
			}
			
			return C_cov;
		}

		return C_cover;
	)
	
	EQUATION(Model, ReachSedimentInputCoefficient,
		//# Reach sed input coefficient per land use class (kg/mm). See documentation for rationale/source
		double Esus_i =
			  PARAMETER(ReachSedimentInputScalingFactor) * 1000.0 //1000 just for convenient range in input parameter
			* PARAMETER(ReachSlope)
			* PARAMETER(MeanSlopeOfLand)
			* RESULT(TimeDependentVegetationCoverFactor)
			* (1.0 - PARAMETER(ReductionOfLoadInSediment));
		
		return Esus_i * PARAMETER(LandUseProportions); //NOTE: This is to make them sum up correctly for use later
	)	
	
	EQUATION(Model, ReachSedimentInput,
		// Note need to remove upstream flow as equation applies to transport from the land to the reach. Otherwise get
		// gradual increase downstream in sed input just because reach flow accumulates
		double upstreamflow = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflow += RESULT(ReachFlow, *Input); //Should this be scaled by ratio of areas?
			)
		double reachflow_excludingupstream = RESULT(ReachFlow) - upstreamflow;		
		return RESULT(TotalReachSedimentInputCoefficient) * pow(reachflow_excludingupstream, PARAMETER(SedimentInputNonlinearCoefficient));
	)
	
	EQUATION(Model, SuspendedSedimentMass,	
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(DailyMeanSuspendedSedimentFlux, *Input);
		)
		
		return RESULT(ReachSedimentInput) + upstreamflux - RESULT(SuspendedSedimentFlux);
	)
	
	EQUATION(Model, SuspendedSedimentFlux,
		return SafeDivide(RESULT(SuspendedSedimentMass) * RESULT(ReachFlow), RESULT(ReachVolume));
	)
	
	EQUATION(Model, DailyMeanSuspendedSedimentFlux,
		return RESULT(SuspendedSedimentFlux);
	)
	
	EQUATION(Model, SuspendedSedimentConcentration,
		return ConvertKgPerMmToMgPerL(SafeDivide(RESULT(DailyMeanSuspendedSedimentFlux), RESULT(DailyMeanReachFlow)), PARAMETER(CatchmentArea));
	)
}

static void
AddSimplyPPhosphorusModule(mobius_model *Model)
{
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
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto Arable             = RequireIndex(Model, LandscapeUnits, "Arable");
	auto ImprovedGrassland  = RequireIndex(Model, LandscapeUnits, "Improved grassland");
	auto Seminatural        = RequireIndex(Model, LandscapeUnits, "Semi-natural");
	
	// PARAMETERS
	
	// Phosphorus params that don't vary by sub-catchment/reach or land class
	auto Phosphorous = RegisterParameterGroup(Model, "Phosphorous");
	
	auto MSoilPerM2                     = RegisterParameterDouble(Model, Phosphorous, "Soil mass per m2", KgPerM2, 95.0, 0.0, 200.0);
	auto PhosphorousSorptionCoefficient = RegisterParameterDouble(Model, Phosphorous, "Phosphorous sorption coefficient", MmPerKg, 1.13e-4, 0.0, 0.1, "Gradient of linear relationship between labile P and TDP concentration. Value only read in from file if calibration run mode is set to false, otherwise it is estimated by the model");
	auto NetAnnualPInputAgricultural    = RegisterParameterDouble(Model, Phosphorous, "Net annual P input to agricultural soil", KgPerHaPerYear, 10.0, -100.0, 100.0);
	auto NetAnnualPInputNewlyConverted  = RegisterParameterDouble(Model, Phosphorous, "Net annual P input to newly-converted soil", KgPerHaPerYear, -5.0, -100.0, 100.0);
	auto GroundwaterTDPConcentration    = RegisterParameterDouble(Model, Phosphorous, "Groundwater TDP concentration", MgPerL, 0.02, 0.0, 10.0);
	auto PPEnrichmentFactor             = RegisterParameterDouble(Model, Phosphorous, "Particulate P enrichment factor", Dimensionless, 1.6, 1.0, 5.0, "P content of eroded material compared to P content of bulk soils");
	auto SRPFraction                    = RegisterParameterDouble(Model, Phosphorous, "SRP fraction", Dimensionless, 0.7, 0.0, 1.0, "Factor to multiply TDP by to estimate instream SRP concentration");
	
	// Phosphorus parameters that vary by sub-catchment/reach
	auto PhosphorousReach = RegisterParameterGroup(Model, "Phosphorous reach", Reach);
	auto EffluentTDP                    = RegisterParameterDouble(Model, PhosphorousReach, "Reach effluent TDP inputs", KgPerDay, 0.1, 0.0, 10.0);
	auto NCType                         = RegisterParameterUInt(Model, Phosphorous, "Newly-converted type", Dimensionless, 2, 0, 2, "0=Agricultural (from semi-natural), 2=Semi-natural (from agricultural), anything else=None");
	
	// Phorphorus parameters that vary by land class
	auto PhosphorousLand = RegisterParameterGroup(Model, "Phosphorous land", LandscapeUnits);
	auto InitialEPC0                    = RegisterParameterDouble(Model, PhosphorousLand, "Initial soil water TDP concentration and EPC0",      MgPerL, 0.1, 0.0, 10.0, "Note: arable and improved grassland are grouped as 'agricultural' land and semi-natural initial EPC0 is assumed to be zero; so only the arable value is used");
	auto InitialSoilPConcentration      = RegisterParameterDouble(Model, PhosphorousLand, "Initial total soil P content", MgPerKg, 1458, 0.0, 10000.0, "Note: arable and improved grassland are grouped as 'agricultural' land, so only the arable and semi-natural values are read in");
	
	// Params that vary by reach and land class (add to existing group)
	auto SubcatchmentGeneral = GetParameterGroupHandle(Model, "Subcatchment characteristics by land class");
	auto LandUseProportionsNC = RegisterParameterDouble(Model, SubcatchmentGeneral, "Land use proportions from newly-converted", Dimensionless, 0.0, 0.0, 1.0);

	// Add to global system parameter group
	auto System = GetParameterGroupHandle(Model, "System");
	auto DynamicEPC0                    = RegisterParameterBool(Model, System, "Dynamic soil water EPC0, TDP and soil labile P", true, "Calculate a dynamic soil water EPC0 (the equilibrium P concentration of zero sorption), and therefore soilwater TDP concentration, so that it varies with labile P content? The labile P will also therefore vary");
	auto CalibrationMode                = RegisterParameterBool(Model, System, "Run in calibration mode", true, "Run model in calibration mode? If true, the initial agricultural soil water TDP concentration (and therefore EPC0) is calibrated and used to estimate the phosphorus sorption coefficient. If false, the sorption coefficient is read in from the parameter file");
	
	// Params defined in hydrol or sed modules
	auto CatchmentArea               = GetParameterDoubleHandle(Model, "Catchment area");
	auto SedimentInputNonlinearCoefficient = GetParameterDoubleHandle(Model, "Sediment input non-linear coefficient");
	auto LandUseProportions          = GetParameterDoubleHandle(Model, "Land use proportions");
	auto BaseflowIndex               = GetParameterDoubleHandle(Model, "Baseflow index");
	
	
	// START EQUATIONS
	auto SimplyPSolver               = GetSolverHandle(Model, "SimplyP solver");
	
	// Equations defined in other modules
	auto AgriculturalSoilWaterVolume = GetEquationHandle(Model, "Agricultural soil water volume");
	auto SeminaturalSoilWaterVolume  = GetEquationHandle(Model, "Semi-natural soil water volume");
	auto InfiltrationExcess          = GetEquationHandle(Model, "Infiltration excess");
	auto AgriculturalSoilWaterFlow   = GetEquationHandle(Model, "Agricultural soil water flow");
	auto SeminaturalSoilWaterFlow    = GetEquationHandle(Model, "Semi-natural soil water flow");
	auto ReachVolume                 = GetEquationHandle(Model, "Reach volume");
	auto ReachFlow                   = GetEquationHandle(Model, "Reach flow (end-of-day)");
	auto DailyMeanReachFlow          = GetEquationHandle(Model, "Reach flow (daily mean, mm/day)");
	auto GroundwaterFlow             = GetEquationHandle(Model, "Groundwater flow");
	auto ReachSedimentInputCoefficient  = GetEquationHandle(Model, "Sediment input coefficient");
	
	
/* 	// P sorption coefficient calculation
	//Method 1: This method calculates the parameter or reads it from file, depending on calibration mode. The result is saved to the model dataset, so its value can be extracted e.g. via the python wrapper, but not output in INCAViewer
	
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
	
	// Method 2: regular equation to define the sorption coefficient. Returns a constant value, which can be seen in INCAViewer.
	auto SoilPSorptionCoefficient = RegisterEquation(Model, "Soil phosphorous sorption coefficient", MmPerKg);
	auto InitialAgriculturalSoilWaterEPC0 = RegisterEquationInitialValue(Model, "Initial agricultural soil water EPC0", KgPerMm);
	auto AgriculturalSoilWaterEPC0   = RegisterEquation(Model, "Agricultural soil water EPC0", KgPerMm);
	SetInitialValue(Model, AgriculturalSoilWaterEPC0, InitialAgriculturalSoilWaterEPC0);
	
	EQUATION(Model, SoilPSorptionCoefficient,
		/* # Assume SN has EPC0=0, PlabConc =0. Units: (kg/mg)(mg/kgSoil)(mm/kg)
		Kf = 10**-6*(p_LU['A']['SoilPconc']-p_LU['S']['SoilPconc'])/p_LU['A']['EPC0_0'] */
		auto InitialAgriculturalEPC0 = ConvertMgPerLToKgPerMm(PARAMETER(InitialEPC0, Arable), PARAMETER(CatchmentArea));
		auto Kf = 1e-6 * (PARAMETER(InitialSoilPConcentration, Arable) -
				  PARAMETER(InitialSoilPConcentration, Seminatural))
				  /InitialAgriculturalEPC0;
				  
		double KfPar = PARAMETER(PhosphorousSorptionCoefficient);
				  
		if(PARAMETER(CalibrationMode)) return Kf;
		
		return KfPar;
	)
				
	
#define DISCRETISE_SOIL_P 1
	
#if DISCRETISE_SOIL_P
	auto InitialAgriculturalSoilTDPMass     = RegisterEquationInitialValue(Model, "Initial agricultural soil TDP mass", Kg);
	auto AgriculturalSoilTDPMass            = RegisterEquation(Model, "Agricultural soil TDP mass", Kg);
	SetInitialValue(Model, AgriculturalSoilTDPMass, InitialAgriculturalSoilTDPMass);
	
	auto InitialAgriculturalSoilLabilePMass = RegisterEquationInitialValue(Model, "Initial agricultural soil labile P mass", Kg);
	auto AgriculturalSoilLabilePMass = RegisterEquation(Model, "Agricultural soil labile P mass", Kg);
	SetInitialValue(Model, AgriculturalSoilLabilePMass, InitialAgriculturalSoilLabilePMass);
	
	EQUATION(Model, InitialAgriculturalSoilTDPMass,
		return RESULT(AgriculturalSoilWaterEPC0) * RESULT(AgriculturalSoilWaterVolume);
	)
	
	EQUATION(Model, AgriculturalSoilTDPMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double b = (RESULT(SoilPSorptionCoefficient) * Msoil + LAST_RESULT(AgriculturalSoilWaterFlow) + RESULT(InfiltrationExcess)) / LAST_RESULT(AgriculturalSoilWaterVolume);
		double a = PARAMETER(NetAnnualPInputAgricultural) * 100.0 * PARAMETER(CatchmentArea) / 365.0 + RESULT(SoilPSorptionCoefficient) * Msoil * RESULT(AgriculturalSoilWaterEPC0);
		double value = a / b + (LAST_RESULT(AgriculturalSoilTDPMass) - a / b) * exp(-b);
		
		if(!PARAMETER(DynamicEPC0)) return LAST_RESULT(AgriculturalSoilTDPMass);
		
		return value;
	)
	
	EQUATION(Model, InitialAgriculturalSoilLabilePMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		return 1e-6 * (PARAMETER(InitialSoilPConcentration, Arable) - PARAMETER(InitialSoilPConcentration, Seminatural)) * Msoil;
	)
	
	EQUATION(Model, AgriculturalSoilLabilePMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double b0 = RESULT(SoilPSorptionCoefficient) * Msoil + LAST_RESULT(AgriculturalSoilWaterFlow) + RESULT(InfiltrationExcess);
		double b = b0 / LAST_RESULT(AgriculturalSoilWaterVolume);
		double a = PARAMETER(NetAnnualPInputAgricultural) * 100.0 * PARAMETER(CatchmentArea) / 365.0 + RESULT(SoilPSorptionCoefficient) * Msoil * RESULT(AgriculturalSoilWaterEPC0);
		//TODO: factor out calculations of b0, a? Would probably not matter that much to speed though.
	
		double sorp = RESULT(SoilPSorptionCoefficient) * Msoil * (a / b0 - RESULT(AgriculturalSoilWaterEPC0) + (LAST_RESULT(AgriculturalSoilTDPMass)/LAST_RESULT(AgriculturalSoilWaterVolume) - a/b0)*(1.0 - exp(-b))/b);
		
		if(!PARAMETER(DynamicEPC0)) sorp = 0.0;
	
		return LAST_RESULT(AgriculturalSoilLabilePMass) + sorp;
	)
	
#else
	auto AgriculturalSoilNetPSorption = RegisterEquation(Model, "Agricultural soil net P sorption", KgPerDay);
	SetSolver(Model, AgriculturalSoilNetPSorption, SimplyPSolver);
	
	auto AgriculturalSoilTDPFlux = RegisterEquation(Model, "Agricultural soil TDP flux", KgPerDay);
	SetSolver(Model, AgriculturalSoilTDPFlux, SimplyPSolver);
	
	auto InitialAgriculturalSoilLabilePMass = RegisterEquationInitialValue(Model, "Initial agricultural soil labile P mass", Kg);
	auto AgriculturalSoilLabilePMass = RegisterEquationODE(Model, "Agricultural soil labile P mass", Kg);
	SetInitialValue(Model, AgriculturalSoilLabilePMass, InitialAgriculturalSoilLabilePMass);
	SetSolver(Model, AgriculturalSoilLabilePMass, SimplyPSolver);
	
	auto InitialAgriculturalSoilTDPMass = RegisterEquationInitialValue(Model, "Initial agricultural soil TDP mass", Kg);
	auto AgriculturalSoilTDPMass     = RegisterEquationODE(Model, "Agricultural soil TDP mass", Kg);
	SetInitialValue(Model, AgriculturalSoilTDPMass, InitialAgriculturalSoilTDPMass);
	SetSolver(Model, AgriculturalSoilTDPMass, SimplyPSolver);
	
	
	EQUATION(Model, AgriculturalSoilNetPSorption,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		//dPlabA_dt = Kf*Msoil*((TDPsA_i/VsA_i)-EPC0_A)  # Net sorption
		double sorption = RESULT(SoilPSorptionCoefficient) * Msoil * (RESULT(AgriculturalSoilTDPMass) / RESULT(AgriculturalSoilWaterVolume) - RESULT(AgriculturalSoilWaterEPC0) );
		
		return sorption;
	)
	
	EQUATION(Model, InitialAgriculturalSoilLabilePMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		return 1e-6 * (PARAMETER(InitialSoilPConcentration, Arable) - PARAMETER(InitialSoilPConcentration, Seminatural)) * Msoil;
	)
	
	EQUATION(Model, AgriculturalSoilLabilePMass,
		return RESULT(AgriculturalSoilNetPSorption);
		//return 0;
	)
	
	EQUATION(Model, InitialAgriculturalSoilTDPMass,
		return RESULT(AgriculturalSoilWaterEPC0) * RESULT(AgriculturalSoilWaterVolume);
	)
	
	EQUATION(Model, AgriculturalSoilTDPFlux,
		return
			  RESULT(AgriculturalSoilTDPMass) 
			  * (RESULT(AgriculturalSoilWaterFlow) + RESULT(InfiltrationExcess)) / RESULT(AgriculturalSoilWaterVolume);
	)
	
	EQUATION(Model, AgriculturalSoilTDPMass,
		//dTDPsA_dt = ((P_netInput['A']*100*A_catch/365)    # Net inputs (fert+manure-uptake) (kg/ha/yr)
                 //- Kf*Msoil*((TDPsA_i/VsA_i)-EPC0_A)  # Net sorpn (kg/day) (could be alt above)
                 //- (QsA_i*TDPsA_i/VsA_i)              # Outflow via soil water flow (kg/day)
                 //- (Qq_i*TDPsA_i/VsA_i))              # Outflow via quick flow (kg/day)
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		return
			  PARAMETER(NetAnnualPInputAgricultural) * 100.0 * PARAMETER(CatchmentArea) / 365.0
			- RESULT(AgriculturalSoilNetPSorption)
			- RESULT(AgriculturalSoilTDPFlux);
	)
#endif

	EQUATION(Model, InitialAgriculturalSoilWaterEPC0,
		 //p_LU.ix['EPC0_0',LU] = UC_Cinv(p_LU[LU]['EPC0_init_mgl'], p_SC.ix['A_catch',SC])
		 return ConvertMgPerLToKgPerMm(PARAMETER(InitialEPC0, Arable), PARAMETER(CatchmentArea));
	)
	
	EQUATION(Model, AgriculturalSoilWaterEPC0,
		/*
		if dynamic_options['Dynamic_EPC0'] == 'y':
			EPC0_A_i = Plab0_A/(Kf*Msoil) # Agricultural EPC0; equals EPC0_0 on the 1st timestep
		else:
			EPC0_A_i = p_LU['A']['EPC0_0']
		*/
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double Kf = RESULT(SoilPSorptionCoefficient);
		double Plab_A = LAST_RESULT(AgriculturalSoilLabilePMass);
		
		if(PARAMETER(DynamicEPC0)) return Plab_A / (Kf * Msoil);
		
		return LAST_RESULT(AgriculturalSoilWaterEPC0);
	)
	
	
	
	auto SoilFieldCapacity = GetParameterDoubleHandle(Model, "Soil field capacity");

	auto NewlyConvertedSoilWaterVolume = RegisterEquation(Model, "Newly-converted soil water volume", Mm);
	SetInitialValue(Model, NewlyConvertedSoilWaterVolume, SoilFieldCapacity); //NOTE: This is needed for the Labile P + TDP computations in the first timestep!
	SetSolver(Model, NewlyConvertedSoilWaterVolume, SimplyPSolver);
	
	auto NewlyConvertedSoilWaterFlow   = RegisterEquation(Model, "Newly-converted soil water flow", Mm);
	SetSolver(Model, NewlyConvertedSoilWaterFlow, SimplyPSolver);
	
	auto InitialNewlyConvertedSoilWaterEPC0 = RegisterEquationInitialValue(Model, "Initial newly-converted soil water EPC0", KgPerMm);
	auto NewlyConvertedSoilWaterEPC0   = RegisterEquation(Model, "Newly-converted soil water EPC0", KgPerMm);
	SetInitialValue(Model, NewlyConvertedSoilWaterEPC0, InitialNewlyConvertedSoilWaterEPC0);
	
#if DISCRETISE_SOIL_P
	auto InitialNewlyConvertedSoilTDPMass     = RegisterEquationInitialValue(Model, "Initial newly-converted soil TDP mass", Kg);
	auto NewlyConvertedSoilTDPMass            = RegisterEquation(Model, "Newly-converted soil TDP mass", Kg);
	SetInitialValue(Model, NewlyConvertedSoilTDPMass, InitialNewlyConvertedSoilTDPMass);
	
	auto InitialNewlyConvertedSoilLabilePMass = RegisterEquationInitialValue(Model, "Initial newly-converted soil labile P mass", Kg);
	auto NewlyConvertedSoilLabilePMass = RegisterEquation(Model, "Newly-converted soil labile P mass", Kg);
	SetInitialValue(Model, NewlyConvertedSoilLabilePMass, InitialNewlyConvertedSoilLabilePMass);
	
	EQUATION(Model, InitialNewlyConvertedSoilTDPMass,
		return RESULT(AgriculturalSoilWaterEPC0) * RESULT(AgriculturalSoilWaterVolume);
	)
	
	EQUATION(Model, NewlyConvertedSoilTDPMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double b = (RESULT(SoilPSorptionCoefficient) * Msoil + LAST_RESULT(NewlyConvertedSoilWaterFlow) + RESULT(InfiltrationExcess)) / LAST_RESULT(NewlyConvertedSoilWaterVolume);
		double a = PARAMETER(NetAnnualPInputNewlyConverted) * 100.0 * PARAMETER(CatchmentArea) / 365.0 + RESULT(SoilPSorptionCoefficient) * Msoil * RESULT(NewlyConvertedSoilWaterEPC0);
		
		if(!PARAMETER(DynamicEPC0)) return LAST_RESULT(NewlyConvertedSoilTDPMass);
		
		return a / b + (LAST_RESULT(NewlyConvertedSoilTDPMass) - a / b) * exp(-b);
	)
	
	EQUATION(Model, InitialNewlyConvertedSoilLabilePMass,
		double ag = RESULT(AgriculturalSoilLabilePMass);
		index_t nctype = INDEX_NUMBER(LandscapeUnits, PARAMETER(NCType));
		if(nctype == Seminatural) return ag;
		return 0.0;
	)
	
	EQUATION(Model, NewlyConvertedSoilLabilePMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double b0 = RESULT(SoilPSorptionCoefficient) * Msoil + LAST_RESULT(NewlyConvertedSoilWaterFlow) + RESULT(InfiltrationExcess);
		double b = b0 / LAST_RESULT(NewlyConvertedSoilWaterVolume);
		double a = PARAMETER(NetAnnualPInputNewlyConverted) * 100.0 * PARAMETER(CatchmentArea) / 365.0 + RESULT(SoilPSorptionCoefficient) * Msoil * RESULT(NewlyConvertedSoilWaterEPC0);
		//TODO: factor out calculations of b0, a? Would probably not matter that much to speed though.
	
		double sorp = RESULT(SoilPSorptionCoefficient) * Msoil * (a / b0 - RESULT(NewlyConvertedSoilWaterEPC0) + (LAST_RESULT(NewlyConvertedSoilTDPMass)/LAST_RESULT(NewlyConvertedSoilWaterVolume) - a/b0)*(1.0 - exp(-b))/b);
		
		if(!PARAMETER(DynamicEPC0)) sorp = 0.0;
	
		return LAST_RESULT(NewlyConvertedSoilLabilePMass) + sorp;
	)

#else
	
	auto NewlyConvertedSoilNetPSorption = RegisterEquation(Model, "Newly-converted soil net P sorption", KgPerDay);
	SetSolver(Model, NewlyConvertedSoilNetPSorption, SimplyPSolver);
	
	auto NewlyConvertedSoilTDPFlux = RegisterEquation(Model, "Newly-converted soil TDP flux", KgPerDay);
	SetSolver(Model, NewlyConvertedSoilTDPFlux, SimplyPSolver);
	
	auto InitialNewlyConvertedSoilLabilePMass = RegisterEquationInitialValue(Model, "Initial newly-converted soil labile P mass", Kg);
	auto NewlyConvertedSoilLabilePMass = RegisterEquationODE(Model, "Newly-converted soil labile P mass", Kg);
	SetInitialValue(Model, NewlyConvertedSoilLabilePMass, InitialNewlyConvertedSoilLabilePMass);
	SetSolver(Model, NewlyConvertedSoilLabilePMass, SimplyPSolver);
	
	auto InitialNewlyConvertedSoilTDPMass = RegisterEquationInitialValue(Model, "Initial newly-converted soil TDP mass", Kg);
	auto NewlyConvertedSoilTDPMass     = RegisterEquationODE(Model, "Newly-converted soil TDP mass", Kg);
	SetInitialValue(Model, NewlyConvertedSoilTDPMass, InitialNewlyConvertedSoilTDPMass);
	SetSolver(Model, NewlyConvertedSoilTDPMass, SimplyPSolver);
	
	EQUATION(Model, NewlyConvertedSoilNetPSorption,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		//dPlabA_dt = Kf*Msoil*((TDPsA_i/VsA_i)-EPC0_A)  # Net sorption
		double sorption = RESULT(SoilPSorptionCoefficient) * Msoil * (RESULT(NewlyConvertedSoilTDPMass) / RESULT(NewlyConvertedSoilWaterVolume) - RESULT(NewlyConvertedSoilWaterEPC0) );
		
		return sorption;
	)
	
	EQUATION(Model, InitialNewlyConvertedSoilLabilePMass,
		double ag = RESULT(AgriculturalSoilLabilePMass);
		index_t nctype = INDEX_NUMBER(LandscapeUnits, PARAMETER(NCType));
		if(nctype == Seminatural) return ag;
		return 0.0;
	)
	
	EQUATION(Model, NewlyConvertedSoilLabilePMass,
		return RESULT(NewlyConvertedSoilNetPSorption);
	)
	
	EQUATION(Model, InitialNewlyConvertedSoilTDPMass,
		return RESULT(NewlyConvertedSoilWaterEPC0) * RESULT(NewlyConvertedSoilWaterVolume);
	)
	
	EQUATION(Model, NewlyConvertedSoilTDPFlux,
		return
			  RESULT(NewlyConvertedSoilTDPMass) 
			  * (RESULT(NewlyConvertedSoilWaterFlow) + RESULT(InfiltrationExcess)) / RESULT(NewlyConvertedSoilWaterVolume);
	)
	
	EQUATION(Model, NewlyConvertedSoilTDPMass,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		return
			  PARAMETER(NetAnnualPInputNewlyConverted) * 100.0 * PARAMETER(CatchmentArea) / 365.0
			- RESULT(NewlyConvertedSoilNetPSorption)
			- RESULT(NewlyConvertedSoilTDPFlux);
	)
#endif
	
	EQUATION(Model, NewlyConvertedSoilWaterVolume,
		index_t nctype = INDEX_NUMBER(LandscapeUnits, PARAMETER(NCType));
		double ag = RESULT(AgriculturalSoilWaterVolume);
		double sn = RESULT(SeminaturalSoilWaterVolume);
		if(nctype == Arable) return ag;
		return sn;
	)
	
	EQUATION(Model, NewlyConvertedSoilWaterFlow,
		index_t nctype = INDEX_NUMBER(LandscapeUnits, PARAMETER(NCType));
		double ag = RESULT(AgriculturalSoilWaterFlow);
		double sn = RESULT(SeminaturalSoilWaterFlow);
		if(nctype == Arable) return ag;
		return sn;
	)
	
	
	EQUATION(Model, InitialNewlyConvertedSoilWaterEPC0,
		index_t nctype = INDEX_NUMBER(LandscapeUnits, PARAMETER(NCType));
		return ConvertMgPerLToKgPerMm(PARAMETER(InitialEPC0, nctype), PARAMETER(CatchmentArea));
	)
	
	EQUATION(Model, NewlyConvertedSoilWaterEPC0,
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double Kf = RESULT(SoilPSorptionCoefficient);
		double Plab_A = LAST_RESULT(NewlyConvertedSoilLabilePMass);
		
		if(PARAMETER(DynamicEPC0)) return Plab_A / (Kf * Msoil);
		
		return LAST_RESULT(NewlyConvertedSoilWaterEPC0);
	)
	
	// Post-processing soil P equations (convert units)

	// To do: add these in for newly-converted land too
	
	auto AgriculturalSoilwaterTDPConcentration = RegisterEquation(Model, "Agricultural soil water TDP concentration", MgPerL);
	auto AgriculturalEPC0MgL = RegisterEquation(Model, "Agricultural soil water EPC0 in mg/l", MgPerL);
	auto AgriculturalSoilLabilePConcentration = RegisterEquation(Model, "Agricultural soil labile P concentration", MgPerKg);
	
	EQUATION(Model, AgriculturalSoilwaterTDPConcentration,
		double dynamicTDPConc = ConvertKgPerMmToMgPerL(RESULT(AgriculturalSoilTDPMass)/RESULT(AgriculturalSoilWaterVolume), PARAMETER(CatchmentArea));
		double constantTDPConc = PARAMETER(InitialEPC0);
		
		if(!PARAMETER(DynamicEPC0)) return constantTDPConc;
		return dynamicTDPConc;		
	)
	
	EQUATION(Model, AgriculturalEPC0MgL,
		double dynamic_EPC0 = ConvertKgPerMmToMgPerL(RESULT(AgriculturalSoilWaterEPC0), PARAMETER(CatchmentArea));
		double constantEPC0 = PARAMETER(InitialEPC0);
		
		if(!PARAMETER(DynamicEPC0)) return constantEPC0;
		return dynamic_EPC0;
	)
	
	EQUATION(Model, AgriculturalSoilLabilePConcentration,
/* 	df_TC['Plabile_A_mgkg'] = (10**6*df_TC['P_labile_A_kg']/
								(p['Msoil_m2'] * 10**6 * p_SC.loc['A_catch',SC])) */
		double labilePMassMg = 1e6 * RESULT(AgriculturalSoilLabilePMass);
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double constantLabilePConc = PARAMETER(InitialSoilPConcentration);
		
		if(!PARAMETER(DynamicEPC0)) return constantLabilePConc;
		return labilePMassMg/Msoil;
	)
	
	
	// Reach equations	
	
	auto StreamTDPFlux = RegisterEquation(Model, "Reach TDP flux", KgPerDay);
	SetSolver(Model, StreamTDPFlux, SimplyPSolver);
	
	auto StreamPPFlux  = RegisterEquation(Model, "Reach PP flux", KgPerDay);
	SetSolver(Model, StreamPPFlux, SimplyPSolver);
	
	auto StreamTDPMass = RegisterEquationODE(Model, "Reach TDP mass", Kg);
	SetInitialValue(Model, StreamTDPMass, 0.0);
	SetSolver(Model, StreamTDPMass, SimplyPSolver);
	
	auto StreamPPMass  = RegisterEquationODE(Model, "Reach PP mass", Kg);
	SetInitialValue(Model, StreamPPMass, 0.0);
	SetSolver(Model, StreamPPMass, SimplyPSolver);
	
	auto DailyMeanStreamTDPFlux = RegisterEquationODE(Model, "Reach daily mean TDP flux", KgPerDay);
	SetInitialValue(Model, DailyMeanStreamTDPFlux, 0.0);
	SetSolver(Model, DailyMeanStreamTDPFlux, SimplyPSolver);
	ResetEveryTimestep(Model, DailyMeanStreamTDPFlux);
	
	auto DailyMeanStreamPPFlux = RegisterEquationODE(Model, "Reach daily mean PP flux", KgPerDay);
	SetInitialValue(Model, DailyMeanStreamPPFlux, 0.0);
	SetSolver(Model, DailyMeanStreamPPFlux, SimplyPSolver);
	ResetEveryTimestep(Model, DailyMeanStreamPPFlux);
	
	EQUATION(Model, StreamTDPFlux,
		//dTDPr_out_dt = Qr_i*TDPr_i/Vr_i
		return RESULT(StreamTDPMass) * SafeDivide(RESULT(ReachFlow), RESULT(ReachVolume));
	)
	
	EQUATION(Model, DailyMeanStreamTDPFlux,
		return RESULT(StreamTDPFlux);
	)
	
	EQUATION(Model, StreamPPFlux,
		return RESULT(StreamPPMass) * SafeDivide(RESULT(ReachFlow),RESULT(ReachVolume));
	)
	
	EQUATION(Model, DailyMeanStreamPPFlux,
		return RESULT(StreamPPFlux);
	)
	
	EQUATION(Model, StreamTDPMass,
		/*
		dTDPr_dt = ((1-beta)*f_A*QsA_i*(TDPsA_i/VsA_i)          # Soil input, old agri. Units:(mm/d)(kg/mm)
					+ (1-beta)*f_NC_A*QsNC_i*(TDPsNC_i/VsNC_i)  # Soil input, new agri land
					+ (1-beta)*f_NC_S*QsNC_i*(TDPsNC_i/VsNC_i)  # Soil input, new SN land
					+ f_A*Qq_i*(TDPsA_i/VsA_i)                  # Quick input, old agri. Units:(mm/d)(kg/mm)
					+ f_NC_A*Qq_i*(TDPsNC_i/VsNC_i)             # Quick input, newly-converted agri
					+ f_NC_S*Qq_i*(TDPsNC_i/VsNC_i)             # Quick inputs, newly-converted SN
					+ Qg_i*UC_Cinv(TDPg,A_catch)                # Groundwater input. Units: (mm/d)(kg/mm)
					+ TDPeff                                    # Effluent input (kg/day)
					+ TDPr_US_i                                 # Inputs from upstream 
					- Qr_i*(TDPr_i/Vr_i))                       # Reach outflow. Units: (mm/d)(kg/mm)
		*/
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(DailyMeanStreamTDPFlux, *Input);
		)
		
		double f_A = PARAMETER(LandUseProportions, Arable) + PARAMETER(LandUseProportions, ImprovedGrassland);
		double f_S = PARAMETER(LandUseProportions, Seminatural);
		double f_NC_A = PARAMETER(LandUseProportionsNC, Arable) + PARAMETER(LandUseProportionsNC, ImprovedGrassland);
		double f_NC_S = PARAMETER(LandUseProportionsNC, Seminatural);
		
		double fromAgriculturalSoil =
			    f_A * (1.0 - f_NC_A)
			  * (RESULT(AgriculturalSoilTDPMass)  / RESULT(AgriculturalSoilWaterVolume))
			  * ((1.0-PARAMETER(BaseflowIndex)) * RESULT(AgriculturalSoilWaterFlow) + RESULT(InfiltrationExcess));
		
		double fromNewlyConvertedSoil =
				(f_A * f_NC_A + f_S * f_NC_S)      //NOTE: at least one of f_NC_A or f_NC_S will always be 0.
			  * (RESULT(NewlyConvertedSoilTDPMass) / RESULT(NewlyConvertedSoilWaterVolume))
			  * ((1.0-PARAMETER(BaseflowIndex)) * RESULT(NewlyConvertedSoilWaterFlow) + RESULT(InfiltrationExcess));
		
		return
			  fromAgriculturalSoil
			+ fromNewlyConvertedSoil
			+ RESULT(GroundwaterFlow) * ConvertMgPerLToKgPerMm(PARAMETER(GroundwaterTDPConcentration), PARAMETER(CatchmentArea))
			+ PARAMETER(EffluentTDP)
			+ upstreamflux
			- RESULT(StreamTDPFlux);
	)
	
	EQUATION(Model, StreamPPMass,
		/*
		dPPr_dt = (E_PP *
               (f_Ar*Msus_in_i['A']*(PlabA_i+P_inactive)/Msoil       # Old arable land
                + f_IG*Msus_in_i['IG']*(PlabA_i+P_inactive)/Msoil    # Old improved grassland
                + f_S*Msus_in_i['S']*P_inactive/Msoil)               # Semi-natural land
               + f_NC_Ar*Msus_in_i['A']*(PlabNC_i+P_inactive)/Msoil  # Newly-converted arable
               + f_NC_IG*Msus_in_i['IG']*(PlabNC_i+P_inactive)/Msoil # Newly-converted IG
               + f_NC_S*Msus_in_i['S']*(PlabNC_i+P_inactive)/Msoil   # New semi-natural
               + PPr_US_i                                            # Inputs from upstream 
               - Qr_i*(PPr_i/Vr_i))                                  # Reach outflow (mm/d)(kg/mm)
		*/
		//P_inactive = 10**-6*p_LU['S']['SoilPconc']*Msoil
		double Msoil = PARAMETER(MSoilPerM2) * 1e6 * PARAMETER(CatchmentArea);
		double P_inactive = 1e-6*PARAMETER(InitialSoilPConcentration, Seminatural)*Msoil;
		double E_PP  = PARAMETER(PPEnrichmentFactor);
		double Ag = (RESULT(AgriculturalSoilLabilePMass)+P_inactive) / Msoil;
		double Sn = P_inactive / Msoil;
		
		double f_NC_Ar = PARAMETER(LandUseProportionsNC, Arable);
		double f_NC_IG = PARAMETER(LandUseProportionsNC, ImprovedGrassland);
		double f_NC_S  = PARAMETER(LandUseProportionsNC, Seminatural);
		
		double Nc = (RESULT(NewlyConvertedSoilLabilePMass) + P_inactive) / Msoil;
		
		//NOTE: These are already multiplied with f_X. I.e. Esus_in_Ar is actually Esus_in_i['Ar']*f_Ar
		double Esus_in_Ar = RESULT(ReachSedimentInputCoefficient, Arable);
		double Esus_in_IG = RESULT(ReachSedimentInputCoefficient, ImprovedGrassland);
		double Esus_in_S  = RESULT(ReachSedimentInputCoefficient, Seminatural);
		
		//NOTE: RESULT(ReachSedimentInputCoefficient, Arable) = Esus_i['A']*f_A   etc.
		// Msus_in_i = Esus_i * Qr_i**k_M
		double coeff = pow(RESULT(ReachFlow), PARAMETER(SedimentInputNonlinearCoefficient));
		double sedimentinput =
			E_PP * (
			  (Esus_in_Ar*(1.0 - f_NC_Ar) + Esus_in_IG*(1.0 - f_NC_IG))*Ag
		     + Esus_in_S * (1.0 - f_NC_S) * Sn
			 + (Esus_in_Ar * f_NC_Ar + Esus_in_IG * f_NC_IG + Esus_in_S * f_NC_S) * Nc
			) * coeff;
		
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(DailyMeanStreamPPFlux, *Input);
		)
		return
			  sedimentinput
			+ upstreamflux
			- RESULT(StreamPPFlux);
	)
	
	
	// Post-processing equations (mostly unit conversions)
	
	auto TDPConcentration = RegisterEquation(Model, "Reach TDP concentration", MgPerL); //Volume-weighted daily mean
	auto PPConcentration  = RegisterEquation(Model, "Reach PP concentration", MgPerL); //Volume-weighted daily mean
	auto DailyMeanStreamTPFlux = RegisterEquation(Model, "Reach daily mean TP flux", KgPerDay);
	auto TPConcentration  = RegisterEquation(Model, "Reach TP concentration", MgPerL); //Volume-weighted daily mean
	auto DailyMeanStreamSRPFlux = RegisterEquation(Model, "Reach daily mean SRP flux", KgPerDay);
	auto SRPConcentration = RegisterEquation(Model, "Reach SRP concentration", MgPerL); //Volume-weighted daily mean
	
	EQUATION(Model, TDPConcentration,
		return ConvertKgPerMmToMgPerL(SafeDivide(RESULT(DailyMeanStreamTDPFlux), RESULT(DailyMeanReachFlow)), PARAMETER(CatchmentArea));
	)
	
	EQUATION(Model, PPConcentration,
		return ConvertKgPerMmToMgPerL(SafeDivide(RESULT(DailyMeanStreamPPFlux), RESULT(DailyMeanReachFlow)), PARAMETER(CatchmentArea));
	)
	
	EQUATION(Model, DailyMeanStreamTPFlux,
		return RESULT(DailyMeanStreamTDPFlux) + RESULT(DailyMeanStreamPPFlux);
	)
	
	EQUATION(Model, TPConcentration,
		return RESULT(TDPConcentration) + RESULT(PPConcentration);
	)
	
	EQUATION(Model, DailyMeanStreamSRPFlux,
		return RESULT(DailyMeanStreamTDPFlux) * PARAMETER(SRPFraction);
	)
	
	EQUATION(Model, SRPConcentration,
		return RESULT(TDPConcentration) * PARAMETER(SRPFraction);
	)
}

static void
AddSimplyPInputToWaterBodyModule(mobius_model *Model)
{
	auto M3PerSecond = RegisterUnit(Model, "m^3/s");
	auto KgPerDay    = RegisterUnit(Model, "kg/day");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	
	auto WaterBody = RegisterParameterGroup(Model, "Input to water body", Reach);
	
	auto IsInputToWaterBody = RegisterParameterBool(Model, WaterBody, "Is input to water body", false, "Whether or not the flow and various fluxes from this reach should be summed up in the calculation of inputs to a water body or lake");
	
	auto DailyMeanReachFlow = GetEquationHandle(Model, "Reach flow (daily mean, mm/day)");
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
			double ca = PARAMETER(CatchmentArea, ReachIndex);
			double q  = RESULT(DailyMeanReachFlow, ReachIndex); //NOTE: This is in mm/day
			if(PARAMETER(IsInputToWaterBody, ReachIndex))
			{
				sum += ConvertMmPerDayToM3PerSecond(q, ca);
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
			double srp = RESULT(DailyMeanSRPFlux, ReachIndex); //NOTE: This is in mm/day
			if(PARAMETER(IsInputToWaterBody, ReachIndex))
			{
				sum += srp;
			}
		}
		return sum;
	)
	
}



