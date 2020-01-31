



/*
	This is an addon to SimplyQ that allows a lake to take the place of a reach. It is a very simple water balance model.

	NOTE: This is in development!
*/


static void
AddSimplyQLakeAddon(mobius_model *Model)
{
	//NOTE: This is not registered as a separate module because we just put things into the SimplyQ module
	
	auto M2          = RegisterUnit(Model, "m2");
	auto M           = RegisterUnit(Model, "m");
	auto M2PerS      = RegisterUnit(Model, "m2/s");
	auto MmPerDegreePerDay = RegisterUnit(Model, "mm/°C/day");
	
	auto ReachParams = GetParameterGroupHandle(Model, "General subcatchment and reach parameters");
	
	auto IsLake           = RegisterParameterBool(Model, ReachParams, "This is a lake", false);
	auto LakeSurfaceArea  = RegisterParameterDouble(Model, ReachParams, "Lake surface area", M2, 1e3, 0.0, 371e9, "This parameter is only used if the reach is a lake");
	auto LakeDepth        = RegisterParameterDouble(Model, ReachParams, "Lake depth at 0 outflow", M, 10.0, 0.0, 1642.0, "This parameter is only used if the reach is a lake");
	auto LakeRatingCurveConst = RegisterParameterDouble(Model, ReachParams, "Lake rating curve constant", M2PerS, 0.1, 1e-6, 1e6, "How much outflow is generated per meter the water level is above the level of the outlet. This parameter is only used if the reach is a lake");
	auto LakeDegreeDayEvaporation = RegisterParameterDouble(Model, ReachParams, "Lake degree-day evaporation", MmPerDegreePerDay, 0.1, 0.0, 1.0, "This parameter is only used if the reach is a lake");
	
	auto InitialReachVolume      = GetEquationHandle(Model, "Initial reach volume");
	auto ReachVolume             = GetEquationHandle(Model, "Reach volume");
	auto ReachFlow               = GetEquationHandle(Model, "Reach flow (end-of-day)");
	auto ReachFlowInputFromLand  = GetEquationHandle(Model, "Reach flow input from land");
	auto ReachFlowInputFromUpstream = GetEquationHandle(Model, "Reach flow input from upstream");
	
	auto ReachLength             = GetParameterDoubleHandle(Model, "Reach length");
	auto EffectiveReachLength    = GetParameterDoubleHandle(Model, "Effective reach length");
	auto ManningsCoefficient     = GetParameterDoubleHandle(Model, "Manning's coefficient");
	auto ReachSlope              = GetParameterDoubleHandle(Model, "Reach slope");
	
	auto Precipitation           = GetInputHandle(Model, "Precipitation");
	auto AirTemperature          = GetInputHandle(Model, "Air temperature");
	
	EQUATION_OVERRIDE(Model, InitialReachVolume,
		double flow   = RESULT(ReachFlow);
		double length = PARAMETER(ReachLength);
		double depth  = PARAMETER(LakeDepth);
		double surface = PARAMETER(LakeSurfaceArea);
		if(PARAMETER(IsLake))
		{
			//NOTE: Assumes a wedge-like shape. If pyramidic or conical, the multiplier should be 0.33 instead of 0.5. Could be a parameter?
			return 0.5 * surface * depth;
		}
		else
		{
			// If it is not a lake, use the same volume equation as in SimplyQ
			double reachdepth = 0.349 * pow(flow, 0.34);
			double reachwidth = 2.71 * pow(length, 0.557);
			return reachdepth * reachwidth * length;
		}
	)
	
	EQUATION_OVERRIDE(Model, ReachVolume,
		// If it is not a lake, the computation should be the same as in SimplyQ
		double dVdt = 86400.0 * (RESULT(ReachFlowInputFromLand) + RESULT(ReachFlowInputFromUpstream) - RESULT(ReachFlow));
		
		double ddevap     = PARAMETER(LakeDegreeDayEvaporation);
		double precip     = INPUT(Precipitation);
		double airt       = INPUT(AirTemperature);
		double surface    = PARAMETER(LakeSurfaceArea);
		
		if(PARAMETER(IsLake))
		{
			dVdt += (precip - Max(0.0, airt*ddevap))*surface*1e-3;   // Convert m^2*mm->m^3 
		}
		
		return dVdt;
	)
	
	EQUATION_OVERRIDE(Model, ReachFlow,
		double volume = RESULT(ReachVolume);
		double slope  = PARAMETER(ReachSlope);
		double effectivelenght = PARAMETER(EffectiveReachLength);
		double manning = PARAMETER(ManningsCoefficient);
		double depth  = PARAMETER(LakeDepth);
		double surface = PARAMETER(LakeSurfaceArea);
		double lakeratingcurveconst = PARAMETER(LakeRatingCurveConst);
	
		if(PARAMETER(IsLake))
		{
			// Volume at 0 outflow. Should be the same as in the computation above!
			double volume0 = 0.5 * surface * depth;
			double excessvol = Max(0.0, volume - volume0);
			double excessheight = excessvol / surface;   //Assumes that the banks are steep, but it should not matter that much??

			return lakeratingcurveconst * excessheight;
		}
		else
		{
			// If it is not a lake, use the same flow equation as in SimplyQ
			double val = volume * sqrt(slope)
								  / (effectivelenght * manning);
			
			return 0.28 * val * sqrt(val);
		}
	)
}