

//NOTE: This module is based on the HBV (Hydrologiska Byråns Vattenbalansavdeling) model
// The HBV model was originally introduced in
// Bergström, S., 1979. Development and application of a conceptual runoff model for Scandinavian catchments, SHMI Report RHO 7, Norrköping, 134.

// Present implementation based on
// https://www.uio.no/studier/emner/matnat/geofag/nedlagte-emner/GEO4430/v06/undervisningsmateriale/HBVMOD.PDF


#if !defined(HBV_H)


static void
AddHBVModel(mobius_model *Model)
{
	BeginModule(Model, "HBV", "0.2");
	
	
	auto Dimensionless = RegisterUnit(Model);
	auto Mm            = RegisterUnit(Model);
	auto PerDay        = RegisterUnit(Model);
	auto MmPerDay      = RegisterUnit(Model, "mm/day");
	
	
	
	auto LandscapeUnits = RegisterIndexSet(Model, "Landscape units");
	
	auto LandParams = RegisterParameterGroup(Model, "Land params");
	auto FieldCapacity        = RegisterParameterDouble(Model, LandParams, "Field capacity", Mm, 50.0, 0.0, 1000.0);
	auto EvapDryLimit         = RegisterParameterDouble(Model, LandParams, "Level where ETP is potential", Dimensionless, 0.9, 0.0, 1.0, "Soil moisture fraction of field capacity where evapotranspiration is starting to be reduced");
	auto InfiltrationMax      = RegisterParameterDouble(Model, LandParams, "Max. infiltration", MmPerDay, 100.0, 1.0, 10000.0);
	auto DrawMax              = RegisterParameterDouble(Model, LandParams, "Draw-up scaling factor", MmPerDay, 2.0, 0.0, 100.0);
	auto PercolationMaxFrac   = RegisterParameterDouble(Model, LandParams, "Max. percolation fraction", Dimensionless, 0.2, 0.0, 1.0);
	auto PercolationNonlinear = RegisterParameterDouble(Model, LandParams, "Percolation nonlinear coefficient", Dimensionless, 0.0, 0.0, 5.0, "If 0, percolation is not limited by soil dryness. Otherwise, more water is retained in the soil when it is dry to begin with");
	auto DeepPercolation      = RegisterParameterDouble(Model, LandParams, "Deep percolation", MmPerDay, 2.0, 0.0, 1000.0);
	auto UpperZoneRate1       = RegisterParameterDouble(Model, LandParams, "Upper zone rate 1", PerDay, 0.2, 0.0, 1.0);
	auto UpperZoneRate2       = RegisterParameterDouble(Model, LandParams, "Upper zone rate 2", PerDay, 0.2, 0.0, 1.0);
	auto UpperZoneThreshold   = RegisterParameterDouble(Model, LandParams, "Upper zone threshold", Mm, 10.0, 0.0, 1000.0);	
	auto LowerZoneRate        = RegisterParameterDouble(Model, LandParams, "Lower zone rate", PerDay, 0.2, 0.0, 1.0);        // TODO: Should this be per subcatchment instead of per land unit?
	
	auto Infiltration       = RegisterEquation(Model, "Infiltration", MmPerDay);
	auto InfiltrationExcess = RegisterEquation(Model, "Infiltration excess", MmPerDay);
	auto Percolation        = RegisterEquation(Model, "Percolation", MmPerDay);
	auto DrawUp             = RegisterEquation(Model, "Draw-up", MmPerDay);
	auto DeepPerc           = RegisterEquation(Model, "Deep percolation", MmPerDay);
	auto SoilMoisture       = RegisterEquation(Model, "Soil mosture", Mm);
	auto Evapotranspiration = RegisterEquation(Model, "Evapotranspiration", Mm);
	auto UpperFlow          = RegisterEquation(Model, "Upper zone flow", MmPerDay);
	auto LowerFlow          = RegisterEquation(Model, "Lower zone flow", MmPerDay);
	auto UpperZoneWater     = RegisterEquation(Model, "Upper zone water", MmPerDay);
	auto LowerZoneWater     = RegisterEquation(Model, "Lower zone water", MmPerDay);
	auto UnroutedFlow       = RegisterEquation(Model, "Unrouted flow", MmPerDay);
	
	SetInitialValue(Model, SoilMoisture, FieldCapacity);
	
	// Declared in the HBVSnow module
	auto HydrolInputToSoil           = GetEquationHandle(Model, "Hydrological input to soil box");
	
	// Declared in some PET module or other
	auto PotentialEvapotranspiration = GetEquationHandle(Model, "Potential evapotranspiration");
	
	
	EQUATION(Model, Infiltration,
		return std::min(RESULT(HydrolInputToSoil), PARAMETER(InfiltrationMax));
	)
	
	EQUATION(Model, InfiltrationExcess,
		return RESULT(HydrolInputToSoil) - RESULT(Infiltration);
	)
	
	EQUATION(Model, Percolation,
		double insoil = PARAMETER(PercolationMaxFrac);
		double beta   = PARAMETER(PercolationNonlinear);
		double fc     = PARAMETER(FieldCapacity);
		double sm = LAST_RESULT(SoilMoisture);
		
		double cuz;
		if(sm < fc)
			cuz = insoil * std::pow(sm/fc, beta);
		else
			cuz = insoil;
		return RESULT(Infiltration)*cuz;
	)
	
	EQUATION(Model, Evapotranspiration,
		double sm = LAST_RESULT(SoilMoisture) + RESULT(Infiltration) - RESULT(Percolation);
		double pe = RESULT(PotentialEvapotranspiration);
		double fc = PARAMETER(FieldCapacity);
		double lpdel = PARAMETER(EvapDryLimit);
		
		// NOTE: in HBV Nordic they also multiply etp by (1 - snow_cover)
		
		double etp;
		if(sm < fc*lpdel)
			etp = pe*sm / (fc*lpdel);
		else
			etp = pe;
		return std::min(etp, sm);
	)
	
	EQUATION(Model, DrawUp,
		double lzmax = PARAMETER(DeepPercolation) / PARAMETER(LowerZoneRate);
		double drawup = 2.0 * PARAMETER(DrawMax) * (LAST_RESULT(LowerZoneWater) / lzmax)*(PARAMETER(FieldCapacity) - LAST_RESULT(SoilMoisture))/PARAMETER(FieldCapacity);
		drawup = std::min(drawup, LAST_RESULT(UpperZoneWater)); //TODO: limiting should maybe take into account inputs at that day?
		return std::max(0.0, drawup);
	)
	
	EQUATION(Model, SoilMoisture,
		return
			  LAST_RESULT(SoilMoisture)
			+ RESULT(Infiltration)
			+ RESULT(DrawUp)
			- RESULT(Percolation)
			- RESULT(Evapotranspiration);
	)
	
	EQUATION(Model, DeepPerc,
		return std::min(RESULT(Percolation), PARAMETER(DeepPercolation));
	)
	
	EQUATION(Model, UpperFlow,
		double up = LAST_RESULT(UpperZoneWater) + 0.5*(RESULT(Percolation) - RESULT(DeepPerc));
		double uz1 = PARAMETER(UpperZoneThreshold);
		double kuz1 = PARAMETER(UpperZoneRate1);
		double kuz2 = PARAMETER(UpperZoneRate2);
		double uqu = 0.0;
		double uql = 0.0;
		if(up < uz1) {
			uql = up * 2.0 * kuz1 / ( 2.0 + kuz1 );
		} else {
			uqu = (up - uz1) * 2.0 * kuz2 / ( 2.0 + kuz2 );
			uql = uz1 * kuz1;
		}
		return uqu + uql;
	)
	
	EQUATION(Model, UpperZoneWater,
		return
			  LAST_RESULT(UpperZoneWater)
			+ RESULT(Percolation)
			+ RESULT(InfiltrationExcess)    // TODO: why is this one not counted in the upperflow computation?
			- RESULT(UpperFlow)
			- RESULT(DrawUp);
	)
	
	EQUATION(Model, LowerFlow,
		return (LAST_RESULT(LowerZoneWater) + RESULT(DeepPerc)) * PARAMETER(LowerZoneRate);
	)
	
	EQUATION(Model, LowerZoneWater,
		return
			  LAST_RESULT(LowerZoneWater)
			+ RESULT(DeepPerc)
			- RESULT(LowerFlow);
	)
	
	EQUATION(Model, UnroutedFlow,
		return RESULT(UpperFlow) + RESULT(LowerFlow);
	)
	
	
	EndModule(Model);
}

/*
inline double
RoutingCoefficient(u64 M, u64 I)
{
	//TODO: Double check that the calculation of a is correct.
	
	double a;
	u64 M2;
	if((M % 2) == 0)
	{
		double Md = (double)M * 0.5;
		a = 1.0 / (Md*(Md + 1.0));
		M2 = M / 2;
	}
	else
	{
		double Md = floor((double)M * 0.5);
		a = 1.0 / (Square(Md + 1.0) + 0.5);
		M2 = M / 2 + 1;
	}
	
	if(I <= M2)
	{
		return a * (double)I;
	}
	return a * (double)(2*M2 - I);
}

static void
AddWaterRoutingRoutine(mobius_model *Model)
{
	auto M3PerDay = RegisterUnit(Model, "m3/day");
	auto Days     = RegisterUnit(Model, "day");
	auto Km2      = RegisterUnit(Model, "km2");
	
	auto ReachParameters = GetParameterGroupHandle(Model, "Reach parameters");
	auto MaxBase = RegisterParameterUInt(Model, ReachParameters, "Flow routing max base", Days, 5, 1, 10, "Width of the convolution filter that smooths out the flow from the groundwater to the river over time");
	auto CatchmentArea = RegisterParameterDouble(Model, ReachParameters, "Catchment area", Km2, 1.0); //Should it be called subcatchment area instead?
	
	auto FlowToRouting = RegisterEquation(Model, "Flow to routing routine", M3PerDay);
	auto FlowFromRoutingToReach = RegisterEquation(Model, "Flow from routing routine to reach", M3PerDay);
	
	auto FastFlow = GetEquationHandle(Model, "Fast groundwater flow");
	auto SlowFlow = GetEquationHandle(Model, "Slow groundwater flow");
	
        
	EQUATION(Model, FlowToRouting,
		double runoffdepth = 
			  RESULT(FastFlow)
			+ RESULT(SlowFlow);
		//NOTE: Convert mm/day to m3/day
		return
			  (runoffdepth / 1000.0)                // mm/day -> m/day
			* (PARAMETER(CatchmentArea) * 1e6);     // km2    -> m2
	)
	
	EQUATION(Model, FlowFromRoutingToReach,
		RESULT(FlowToRouting); //NOTE: To force a dependency since this is not automatic when we use EARLIER_RESULT;
	
		u64 M = PARAMETER(MaxBase);		
		double sum = 0.0;

		for(u64 I = 1; I <= M; ++I)
		{
			sum += RoutingCoefficient(M, I) * EARLIER_RESULT(FlowToRouting, I-1);
		}
			
		return sum;
	)
}

static void
AddReachFlowRoutine(mobius_model *Model)
{
	//NOTE: This almost just copied from PERSiST.
	auto M3PerDay = RegisterUnit(Model, "m3/day");
	auto M3       = RegisterUnit(Model, "m3");
	auto MPerDay  = RegisterUnit(Model, "m/day");
	auto M        = RegisterUnit(Model, "m");
	auto Days     = RegisterUnit(Model, "day");
	auto PerM2    = RegisterUnit(Model, "/m2");
	auto Dimensionless = RegisterUnit(Model);
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	auto ReachParameters = GetParameterGroupHandle(Model, "Reach parameters");
	
	auto ReachHasEffluentInput = RegisterParameterBool(Model, ReachParameters, "Reach has effluent input", false);
	auto EffluentFlow = RegisterParameterDouble(Model, ReachParameters, "Effluent flow", M3PerDay, 0.0, 0.0, 99999999.0, "The rate of liquid inputs to a reach from e.g. sewage treatment works");
	auto ReachLenght              = RegisterParameterDouble(Model, ReachParameters, "Reach length", M, 1000.0, 0.1, 999999.0, "The length of the main stem of the stream / reach in a subcatchment");
	auto ReachWidth               = RegisterParameterDouble(Model, ReachParameters, "Reach width", M, 10.0, 0.1, 9999.0, "The average width of the main stem of the stream / reach in a subcatchment");
	auto A                        = RegisterParameterDouble(Model, ReachParameters, "a", PerM2, 0.04, 0.00001, 0.99, "The flow velocity 'a' parameter V=aQ^b");
	auto B                        = RegisterParameterDouble(Model, ReachParameters, "b", Dimensionless, 0.67, 0.1, 0.99, "The flow velocity 'b' parameter V=aQ^b");
	auto InitialFlow = RegisterParameterDouble(Model, ReachParameters, "Initial reach flow", M3PerDay, 1000.0, 0.0001, 9999999.0, "The flow in the reach at the start of the simulation. This parameter is only used for reaches that don't have other reaches as inputs.");
	
	auto ReachSolver = RegisterSolver(Model, "Reach solver", 0.1, IncaDascru);
	
	auto FlowFromRoutingToReach = GetEquationHandle(Model, "Flow from routing routine to reach");
	
	auto ReachFlowInput    = RegisterEquation(Model, "Reach flow input", M3PerDay);
	
	auto InitialReachTimeConstant = RegisterEquationInitialValue(Model, "Initial reach time constant", Days);
	auto ReachTimeConstant        = RegisterEquation(Model, "Reach time constant", Days);
	SetSolver(Model, ReachTimeConstant, ReachSolver);
	SetInitialValue(Model, ReachTimeConstant, InitialReachTimeConstant);
	
	auto InitialReachFlow         = RegisterEquationInitialValue(Model, "Initial reach flow", M3PerDay);
	auto ReachFlow                = RegisterEquationODE(Model, "Reach flow", M3PerDay);
	SetSolver(Model, ReachFlow, ReachSolver);
	SetInitialValue(Model, ReachFlow, InitialReachFlow);
	
	auto InitialReachVolume       = RegisterEquationInitialValue(Model, "Initial reach volume", M3);
	auto ReachVolume              = RegisterEquationODE(Model, "Reach volume", M3);
	SetSolver(Model, ReachVolume, ReachSolver);
	SetInitialValue(Model, ReachVolume, InitialReachVolume);
	
	auto ReachVelocity = RegisterEquation(Model, "Reach velocity", MPerDay);
	auto ReachDepth    = RegisterEquation(Model, "Reach depth", M);
	

	EQUATION(Model, ReachFlowInput,
		double reachInput = RESULT(FlowFromRoutingToReach);
		double effluentInput = PARAMETER(EffluentFlow);
		if(PARAMETER(ReachHasEffluentInput)) reachInput += effluentInput;
		for(index_t Input : BRANCH_INPUTS(Reach))
			reachInput += RESULT(ReachFlow, Input);
		
		return reachInput;
	)
	
	EQUATION(Model, InitialReachTimeConstant,
		return PARAMETER(ReachLenght) / (PARAMETER(A) * pow(RESULT(ReachFlow), PARAMETER(B)));
	)
	
	EQUATION(Model, ReachTimeConstant,
		double tc = RESULT(ReachVolume) / RESULT(ReachFlow);
		if(RESULT(ReachFlow) > 0.0 && RESULT(ReachVolume) > 0.0) return tc;
		return 0.0;
	)
    
	EQUATION(Model, InitialReachFlow,
		double upstreamFlow = 0.0;
		for(index_t Input : BRANCH_INPUTS(Reach))
			upstreamFlow += RESULT(ReachFlow, Input);
		
		double initialFlow  = PARAMETER(InitialFlow);
		
		return (INPUT_COUNT(Reach) == 0) ? initialFlow : upstreamFlow;
	)

	EQUATION(Model, ReachFlow,
		double flow = ( RESULT(ReachFlowInput) - RESULT(ReachFlow) ) / ( RESULT(ReachTimeConstant) * (1.0 - PARAMETER(B)));
		if(RESULT(ReachTimeConstant) > 0.0) return flow;
		return 0.0;
	)

	EQUATION(Model, InitialReachVolume,
		return RESULT(ReachFlow) * RESULT(ReachTimeConstant);
	)
    
	EQUATION(Model, ReachVolume,
		return RESULT(ReachFlowInput) - RESULT(ReachFlow);
	)

	EQUATION(Model, ReachVelocity,
		return PARAMETER(A) * pow(RESULT(ReachFlow), PARAMETER(B));
	)
    
	EQUATION(Model, ReachDepth,
		return RESULT(ReachFlow) / (RESULT(ReachVelocity) * PARAMETER(ReachWidth));
	)
}


static void
AddHBVModel(mobius_model *Model)
{
	BeginModule(Model, "HBV", "0.2");
	
	AddSnowRoutine(Model);
	AddPotentialEvapotranspirationModuleV2(Model);
	AddSoilMoistureRoutine(Model);
	AddGroundwaterResponseRoutine(Model);
	AddWaterRoutingRoutine(Model);
	AddReachFlowRoutine(Model);
	
	EndModule(Model);
}
*/

#define HBV_H
#endif