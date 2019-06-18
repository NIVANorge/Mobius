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
	auto M3                = RegisterUnit(Model, "m^3");
	auto M3PerSecond       = RegisterUnit(Model, "m^3/s");
	auto Km2               = RegisterUnit(Model, "km^2");
	auto M                 = RegisterUnit(Model, "m");
	auto DegreesCelsius    = RegisterUnit(Model, "°C");
	auto MPerM			   = RegisterUnit(Model, "m/m");
	auto SecondsPerCubeRouteM	= RegisterUnit(Model, "s/(m^1/3)");
	
	// Set up index sets
	auto Reach = RegisterIndexSetBranched(Model, "Reaches");
	auto LandscapeUnits = RegisterIndexSet(Model, "Landscape units");
	
	// Global snow parameters
	auto Snow = RegisterParameterGroup(Model, "Snow");
	
	auto InitialSnowDepth        = RegisterParameterDouble(Model, Snow, "Initial snow depth as water equivalent", Mm, 0.0, 0.0, 50000.0);
	auto DegreeDayFactorSnowmelt = RegisterParameterDouble(Model, Snow, "Degree-day factor for snowmelt", MmPerDegreePerDay, 2.74, 0.0, 5.0);
	
	// Hydrology parameters that don't currently vary by sub-catchment or reach
	auto Hydrology = RegisterParameterGroup(Model, "Hydrology");
	
	auto ProportionToQuickFlow   = RegisterParameterDouble(Model, Hydrology, "Proportion of precipitation that contributes to quick flow", Dimensionless, 0.020, 0.0, 1.0); //Max ok, or breaks model?
	auto PETMultiplicationFactor      = RegisterParameterDouble(Model, Hydrology, "PET multiplication factor", Dimensionless, 1.0, 0.0, 2.0);
	auto SoilFieldCapacity       = RegisterParameterDouble(Model, Hydrology, "Soil field capacity", Mm, 290.0, 0.0, 1000.0);
#ifdef SIMPLYQ_GROUNDWATER
	auto BaseflowIndex           = RegisterParameterDouble(Model, Hydrology, "Baseflow index", Dimensionless, 0.70, 0.0, 1.0);
	auto GroundwaterTimeConstant = RegisterParameterDouble(Model, Hydrology, "Groundwater time constant", Days, 65.0, 0.5, 400.0);
	auto MinimumGroundwaterFlow  = RegisterParameterDouble(Model, Hydrology, "Minimum groundwater flow", MmPerDay, 0.40, 0.0, 10.0);
#endif
	auto ManningsCoefficient	 = RegisterParameterDouble(Model, Hydrology, "Manning's coefficient", SecondsPerCubeRouteM, 0.04, 0.012, 0.1, "Default of 0.04 is for clean winding natural channels. See e.g. Chow 1959 for a table of values for other channel types") ;
	
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
	
	auto SoilWaterTimeConstant   = RegisterParameterDouble(Model, HydrologyLand, "Soil water time constant", Days, 2.0, 0.01, 40.0);
	
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
	
	// Make solvers
	
	auto LandSolver = RegisterSolver(Model, "SimplyQ land solver", 0.01 /* 1.0/20000.0 */, IncaDascru);
	auto ReachSolver = RegisterSolver(Model, "SimplyQ reach solver", 0.1 /* 1.0/20000.0 */, IncaDascru);

	// Soil water equations
	
	auto SoilWaterFlow = RegisterEquation(Model, "Soil water flow", MmPerDay); // Total flow out of soil box (including that which then goes to GW)
	SetSolver(Model, SoilWaterFlow, LandSolver);
	
	auto SoilWaterVolume = RegisterEquationODE(Model, "Soil water volume", Mm);
	SetInitialValue(Model, SoilWaterVolume, SoilFieldCapacity);
	SetSolver(Model, SoilWaterVolume, LandSolver);
	
	auto DailyMeanSoilWaterFlow = RegisterEquationODE(Model, "Daily mean soil water flow", MmPerDay);
	SetSolver(Model, DailyMeanSoilWaterFlow, LandSolver);
	ResetEveryTimestep(Model, DailyMeanSoilWaterFlow);
	
	EQUATION(Model, SoilWaterFlow,
		double smd = PARAMETER(SoilFieldCapacity) - RESULT(SoilWaterVolume);
		return -smd * ActivationControl(RESULT(SoilWaterVolume), PARAMETER(SoilFieldCapacity), 0.01) / PARAMETER(SoilWaterTimeConstant);
	)
	
	EQUATION(Model, SoilWaterVolume,
		return
			  RESULT(Infiltration)
			- PARAMETER(PETMultiplicationFactor) * INPUT(PotentialEvapoTranspiration) * (1.0 - exp(log(0.01) * RESULT(SoilWaterVolume) / PARAMETER(SoilFieldCapacity))) //NOTE: Should 0.01 be a parameter?
			- RESULT(SoilWaterFlow);	
	)
	
	EQUATION(Model, DailyMeanSoilWaterFlow,
		return RESULT(SoilWaterFlow);
	)
	
	auto TotalSoilWaterFlow       = RegisterEquationCumulative(Model, "Landuse weighted soil water flow", DailyMeanSoilWaterFlow, LandscapeUnits, LandUseProportions);
	
	
	// Groundwater and In-stream hydrology equations (group together, as one reach equation is used in a groundwater equation)
	
	// Derived parameters used in instream hydrol equations
	auto UpstreamArea 	= RegisterParameterDouble(Model, HydrologyReach, "Upstream land area", Km2, 0.0); //Derived param
	
	auto ComputedUpstreamArea = RegisterEquationInitialValue(Model, "Upstream area", Km2); //Excluding the current reach
	ParameterIsComputedBy(Model, UpstreamArea, ComputedUpstreamArea, true);  //'true' says this parameter shouldn't be exposed in parameter files and UI

	EQUATION(Model, ComputedUpstreamArea,
		double upstreamarea = 0.0;
		FOREACH_INPUT(Reach,
			upstreamarea += PARAMETER(CatchmentArea, *Input);
			upstreamarea += PARAMETER(UpstreamArea, *Input);
		)
		
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

#ifdef SIMPLYQ_GROUNDWATER
	auto InitialGroundwaterVolume = RegisterEquationInitialValue(Model, "Initial groundwater volume", Mm);
	auto GroundwaterVolume        = RegisterEquationODE(Model, "Groundwater volume", Mm);
	SetInitialValue(Model, GroundwaterVolume, InitialGroundwaterVolume);
	SetSolver(Model, GroundwaterVolume, ReachSolver);
	
	auto GroundwaterFlow          = RegisterEquation(Model, "Groundwater flow", MmPerDay);
	SetSolver(Model, GroundwaterFlow, ReachSolver);
#endif

	auto ReachFlowInputFromUpstream    = RegisterEquation(Model, "Reach flow input from upstream", M3PerSecond);
	SetSolver(Model, ReachFlowInputFromUpstream, ReachSolver);
	
	auto ReachFlowInputFromLand        = RegisterEquation(Model, "Reach flow input from land", M3PerSecond);
	SetSolver(Model, ReachFlowInputFromLand, ReachSolver);

	auto ReachVolume        = RegisterEquationODE(Model, "Reach volume", M3);
	auto InitialReachVolume = RegisterEquationInitialValue(Model, "Initial reach volume", M3); 
	SetInitialValue(Model, ReachVolume, InitialReachVolume);
	SetSolver(Model, ReachVolume, ReachSolver);
	
	auto ReachFlow          = RegisterEquation(Model, "Reach flow (end-of-day)", M3PerSecond);
	auto InitialReachFlow   = RegisterEquationInitialValue(Model, "Initial reach flow", M3PerSecond);
	SetInitialValue(Model, ReachFlow, InitialReachFlow);
	SetSolver(Model, ReachFlow, ReachSolver);
	
	auto DailyMeanReachFlow = RegisterEquationODE(Model, "Reach flow (daily mean, cumecs)", M3PerSecond);
	SetInitialValue(Model, DailyMeanReachFlow, 0.0);
	SetSolver(Model, DailyMeanReachFlow, ReachSolver);
	ResetEveryTimestep(Model, DailyMeanReachFlow);
	
	auto DailyMeanReachFlowMm = RegisterEquation(Model, "Reach flow (daily mean, mm/day)", MmPerDay);
	
	// Groundwater equations
	
#ifdef SIMPLYQ_GROUNDWATER
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
	
	EQUATION(Model, InitialGroundwaterVolume,
		
		double tc = PARAMETER(GroundwaterTimeConstant);
		double upstreamvol = 0.0;
		FOREACH_INPUT(Reach,
			upstreamvol += RESULT(GroundwaterVolume, *Input);
		)
		u64 upstreamcount = INPUT_COUNT(Reach);
		
		if(upstreamcount == 0)
		{
			//If we are a headwater, assume that the initial groundwater flow is the initial reach flow times the baseflow index
			double initflow = ConvertM3PerSecondToMmPerDay(RESULT(ReachFlow), PARAMETER(CatchmentArea)) * PARAMETER(BaseflowIndex);
			return initflow * tc; //So the initial volume is the initial flow times the time constant.
		}
		
		// If we are not a headwater, we want the initial groundwater flow to be the same (per unit area) as in our upstream reach (if there is only one upstream reach). Assuming the groundwater time constant and baseflow index is the same across reaches, this is achieved by setting the initial volume to be the same as the upstream one.
		// If there are multiple upstream reaches, we average over the upstream volumes instead.
		// TODO: Time will tell if this is good practice. Future model versions will likely allow BFI and groundwater time constant to vary across sub-catchments, and then need to be able to take this into account.
		double avgupstreamvol = upstreamvol / (double)upstreamcount;
		
		return avgupstreamvol;
	)
	
	auto Control = RegisterEquation(Model, "Control", Dimensionless);
	
	EQUATION(Model, Control,
		//NOTE: We create this equation to put in the code that allow us to "hack" certain values.
		// The return value of this equation does not mean anything.
		
		double volume = RESULT(GroundwaterFlow)*PARAMETER(GroundwaterTimeConstant);  //Wow, somehow this does not register index sets correctly if it is passed directly inside the macro below! May want to debug that.
		SET_RESULT(GroundwaterVolume, volume);
		
		return 0.0;
	)
#endif
	
	// In-stream equations
	
	EQUATION(Model, ReachFlowInputFromLand,
		//Flow from land in mm/day, converted to m3/s
#ifdef SIMPLYQ_GROUNDWATER
		double fromland = RESULT(InfiltrationExcess) + (1.0 - PARAMETER(BaseflowIndex)) * RESULT(TotalSoilWaterFlow) + RESULT(GroundwaterFlow);
#else
		double fromland = RESULT(InfiltrationExcess) + RESULT(TotalSoilWaterFlow);
#endif
		return ConvertMmPerDayToM3PerSecond(fromland, PARAMETER(CatchmentArea));
	)
	
	EQUATION(Model, ReachFlowInputFromUpstream,
		double upstreamflow = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflow += RESULT(DailyMeanReachFlow, *Input);
		)
		return upstreamflow;
	)
	
	EQUATION(Model, InitialReachFlow,
		// TO DO: ability to add initial reach flow for any reach, and estimate it for others by e.g. area-scaling
		double upstreamflow = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflow += RESULT(ReachFlow, *Input);
		)
		double initflow = PARAMETER(InitialInStreamFlow);

		if(INPUT_COUNT(Reach) == 0) return initflow;
		else return upstreamflow;
	)
	
	EQUATION(Model, ReachVolume,
		return 86400.0 * (RESULT(ReachFlowInputFromLand) + RESULT(ReachFlowInputFromUpstream) - RESULT(ReachFlow));
	)
	
	EQUATION(Model, ReachFlow,
		/* Derived from: Q=V/T, where T=L/u, so Q = Vu/L (where V is reach volume, u is reach velocity, L is reach length)
		Then get an expression for u using the Manning equation, assuming a rectangular cross section and empirical power law relationships between stream depth and Q and stream width and Q. Then rearrange so all Qs are on the left hand side with exponent of 1. See https://hal.archives-ouvertes.fr/hal-00296854/document */
		
		double val = RESULT(ReachVolume) * sqrt(PARAMETER(ReachSlope))
							  / (PARAMETER(EffectiveReachLength) * PARAMETER(ManningsCoefficient));
		
		return 0.28 * val * sqrt(val); //NOTE: This is just an optimization; equiv to pow(val, 1.5)
	)
	
	EQUATION(Model, InitialReachVolume,
	//Assumes rectangular cross section. See comment in ReachFlow equation for source
		double reachdepth = 0.349 * pow(RESULT(ReachFlow), 0.34);
		double reachwidth = 2.71 * pow(RESULT(ReachFlow), 0.557);
		return reachdepth * reachwidth * PARAMETER(ReachLength);
	)
	
	EQUATION(Model, DailyMeanReachFlow,
		//NOTE: Since DailyMeanReachFlow is reset to start at 0 every timestep and since its derivative is the reach flow, its value becomes the integral of the reach flow over the timestep, i.e. the daily mean value.
		return RESULT(ReachFlow);
	)
	
	EQUATION(Model, DailyMeanReachFlowMm,
		return ConvertM3PerSecondToMmPerDay(RESULT(DailyMeanReachFlow), PARAMETER(CatchmentArea));
	)
}
