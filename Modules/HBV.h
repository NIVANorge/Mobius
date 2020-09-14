

//NOTE: This module is based on the HBV (Hydrologiska Byråns Vattenbalansavdeling) model
// The HBV model was originally introduced in
// Bergström, S., 1979. Development and application of a conceptual runoff model for Scandinavian catchments, SHMI Report RHO 7, Norrköping, 134.


#if !defined(HBV_H)

static void AddSnowRoutine(mobius_model *Model)
{
	auto Mm                = RegisterUnit(Model, "mm");
	auto MmPerDegreePerDay = RegisterUnit(Model, "mm/°C/day");
	auto DegreesCelsius    = RegisterUnit(Model, "°C");
	auto Dimensionless     = RegisterUnit(Model);
	auto Cm                = RegisterUnit(Model, "cm");
	
	auto LandscapeUnits = RegisterIndexSet(Model, "Landscape units");
	
	auto Land = RegisterParameterGroup(Model, "Landscape units", LandscapeUnits);
	//auto System = GetParameterGroupHandle(Model, "System");
	
	auto DegreeDayFactor          = RegisterParameterDouble(Model, Land, "Degree day factor", MmPerDegreePerDay, 2.5, 1.5, 4.0, "Degree day factor, rate at which the snow will melt water in snow will freeze");
	auto RefreezingCoefficient    = RegisterParameterDouble(Model, Land, "Refreezing coefficient", Dimensionless, 0.05, 0.01, 0.1, "Proportion of meltwater that can refreeze");
	auto StorageFraction          = RegisterParameterDouble(Model, Land, "Storage fraction", Dimensionless, 0.1, 0.01, 0.2, "Proportion of meltwater that can be stored in the snowpack");
	auto SnowThresholdTemperature = RegisterParameterDouble(Model, Land, "Snow threshold temperature", DegreesCelsius, 0.0, -3.0, 5.0, "Threshold temperature above which precipitation falls as rain");
	auto InitialSnowDepth         = RegisterParameterDouble(Model, Land, "Initial snow depth", Mm, 0.0, 0.0, 9999.0, "The depth of snow, expressed as water equivalents, at the start of the simulation");
	
	auto AirTemperature = RegisterInput(Model, "Air temperature");
	auto Precipitation  = RegisterInput(Model, "Precipitation");
	
	
	auto SnowmeltTemperatureDifference = RegisterEquation(Model, "Snowmelt temperature difference", DegreesCelsius);
	auto Snowfall    = RegisterEquation(Model, "Snowfall", Mm);
	auto Rainfall    = RegisterEquation(Model, "Rainfall", Mm);
	auto Snowpack    = RegisterEquation(Model, "Snow depth as water equivalent", Mm);
	SetInitialValue(Model, Snowpack, InitialSnowDepth);
	auto MaxStorage  = RegisterEquation(Model, "Max storage", Mm);
	auto MeltWater   = RegisterEquation(Model, "Meltwater", Mm);
	auto WaterInSnow = RegisterEquation(Model, "Water in snow", Mm);
	auto ExcessMelt  = RegisterEquation(Model, "Excess melt", Mm);
	auto Refreeze    = RegisterEquation(Model, "Refreeze", Mm);
	auto WaterToSoil = RegisterEquation(Model, "Water to soil", Mm);
	
	EQUATION(Model, SnowmeltTemperatureDifference,
		return INPUT(AirTemperature) - PARAMETER(SnowThresholdTemperature);
	)
	
	EQUATION(Model, Snowfall,
		double precip = INPUT(Precipitation);
		return RESULT(SnowmeltTemperatureDifference) <= 0.0 ? precip : 0.0;
	)
	
	EQUATION(Model, Rainfall,
		double precip = INPUT(Precipitation);
		return RESULT(SnowmeltTemperatureDifference) > 0.0 ? precip : 0.0;
	)
	
	EQUATION(Model, Snowpack,
                double old_snow = LAST_RESULT(Snowpack);
		double snowpack1 = old_snow + RESULT(Snowfall) + RESULT(Refreeze);
		double snowpack2 = old_snow - RESULT(MeltWater);
		return RESULT(SnowmeltTemperatureDifference) <= 0.0 ? snowpack1 : snowpack2;
	)
	
	EQUATION(Model, MaxStorage,
		return PARAMETER(StorageFraction) * LAST_RESULT(Snowpack);
	)
	
	EQUATION(Model, MeltWater,
		double potentialmelt = PARAMETER(DegreeDayFactor) * RESULT(SnowmeltTemperatureDifference);
		double actualmelt = Min(potentialmelt, LAST_RESULT(Snowpack));
		if(LAST_RESULT(Snowpack) <= 0.0 || RESULT(SnowmeltTemperatureDifference) <= 0.0) actualmelt = 0.0;
		return actualmelt;
	)
	
	EQUATION(Model, WaterInSnow,
		double maxStorage = RESULT(MaxStorage);
		double meltWater  = RESULT(MeltWater);
		double refreeze   = RESULT(Refreeze);
		if(LAST_RESULT(Snowpack) == 0.0) return 0.0;
		if(RESULT(SnowmeltTemperatureDifference) > 0.0)
		{
			return Min(maxStorage, LAST_RESULT(WaterInSnow) + meltWater);
		}
		else
		{
			return Max(0.0, LAST_RESULT(WaterInSnow) - refreeze);
		}
	)
	
	EQUATION(Model, ExcessMelt,
		double availableWater = RESULT(MeltWater) + RESULT(WaterInSnow);
		double excess = 0.0;
		if(RESULT(SnowmeltTemperatureDifference) > 0.0 && availableWater > RESULT(MaxStorage))
		{
			excess = availableWater - RESULT(MaxStorage);
		}
		return excess;
	)
	
	EQUATION(Model, Refreeze,
		double refreeze = PARAMETER(RefreezingCoefficient) * PARAMETER(DegreeDayFactor) * -RESULT(SnowmeltTemperatureDifference);
		refreeze = Min(refreeze, LAST_RESULT(WaterInSnow));
		if(LAST_RESULT(Snowpack) == 0.0 || RESULT(SnowmeltTemperatureDifference) > 0.0 || LAST_RESULT(WaterInSnow) == 0.0) refreeze = 0.0;
		return refreeze;
	)
	
	EQUATION(Model, WaterToSoil,
		double rainfall = RESULT(Rainfall);
		double excess   = RESULT(ExcessMelt);
		if(RESULT(Snowpack) == 0.0) return rainfall;
		return excess;
	)
}


//TODO: These PET modules are a little bogus. Use better ones from other files instead?

static void
AddPotentialEvapotranspirationModuleV1(mobius_model *Model)
{
	//This one just loads potential evapotranspiration as an input.
	
	auto MmPerDay          = RegisterUnit(Model, "mm/day");
	
	auto PotentialEvapotranspirationIn = RegisterInput(Model, "Potential evapotranspiration");
	auto PotentialEvapotranspiration = RegisterEquation(Model, "Potential evapotranspiration", MmPerDay);
	
	EQUATION(Model, PotentialEvapotranspiration,
		return INPUT(PotentialEvapotranspirationIn);
	)
}

static void
AddPotentialEvapotranspirationModuleV2(mobius_model *Model)
{
	//NOTE: This is similar to what is used in PERSiST.
	
	auto DegreesCelsius    = RegisterUnit(Model, "°C");
	auto MmPerDegreePerDay = RegisterUnit(Model, "mm/°C/day");
	auto MmPerDay          = RegisterUnit(Model, "mm/day");
	auto Land = GetParameterGroupHandle(Model, "Landscape units");
	auto AirTemperature = RegisterInput(Model, "Air temperature");
	
	auto GrowingDegreeThreshold = RegisterParameterDouble(Model, Land, "Growing degree threshold",    DegreesCelsius,    0.0, -4.0,    4.0, "The temperature at or above which plant growth and hence evapotranspiration are assumed to occur");
	auto DegreeDayEvapotranspiration = RegisterParameterDouble(Model, Land, "Degree day evapotranspiration", MmPerDegreePerDay, 0.12, 0.05,   0.2, "Describes the baseline dependency of evapotranspiration on air temperature. The parameter represents the number of millimetres water evapotranspired per degree celcius above the growing degree threshold when evapotranspiration rates are not soil moisture limited");
	
	auto PotentialEvapotranspiration = RegisterEquation(Model, "Potential evapotranspiration", MmPerDay);
	
	EQUATION(Model, PotentialEvapotranspiration,
		return Max(0.0, INPUT(AirTemperature) - PARAMETER(GrowingDegreeThreshold)) * PARAMETER(DegreeDayEvapotranspiration);
	)
}

void
AddSoilMoistureRoutine(mobius_model *Model)
{
	auto Mm                = RegisterUnit(Model, "mm");
	auto MmPerDay          = RegisterUnit(Model, "mm/day");
	auto DegreesCelsius    = RegisterUnit(Model, "°C");
	auto Dimensionless     = RegisterUnit(Model);
	auto PerDay            = RegisterUnit(Model, "/day");
	auto Days              = RegisterUnit(Model, "day");
	
	auto LandscapeUnits    = RegisterIndexSet(Model, "Landscape units");
	
	auto Land              = GetParameterGroupHandle(Model, "Landscape units");
	
	auto Reaches                            = RegisterIndexSetBranched(Model, "Reaches");
	auto ReachParameters                    = RegisterParameterGroup(Model, "Reach parameters", Reaches);
	
	auto LandscapePercentages               = RegisterParameterGroup(Model, "Landscape percentages", LandscapeUnits);
	SetParentGroup(Model, LandscapePercentages, ReachParameters);
	auto Percent                            = RegisterParameterDouble(Model, LandscapePercentages, "%", Dimensionless, 20.0, 0.0, 100.0, "How much of the catchment area that is made up by this type of land cover.");
	
    //TODO: find good default (and min/max) values for these:
	auto SoilMoistureEvapotranspirationMax  = RegisterParameterDouble(Model, Land, "Fraction of field capacity where evapotranspiration reaches its maximal", Dimensionless, 0.7);
	auto FieldCapacity                      = RegisterParameterDouble(Model, Land, "Field capacity", Mm, 150.0, 10., 600., "Maximum soil moisture storage");
	auto Beta                               = RegisterParameterDouble(Model, Land, "Beta recharge exponent", Dimensionless, 1., 1.0, 10., "Power parameter that determines the relative contribution to groundwater recharge");
	auto InitialSoilMoisture                = RegisterParameterDouble(Model, Land, "Initial soil moisture", Mm, 100.0);
	
	
    auto AirTemperature = RegisterInput(Model, "Air temperature");

	
	auto WaterToSoil                 = GetEquationHandle(Model, "Water to soil"); //NOTE: from the snow model.
	auto PotentialEvapotranspiration = GetEquationHandle(Model, "Potential evapotranspiration"); //NOTE: from the potentialevapotranspiration module.

	auto SoilSolver = RegisterSolver(Model, "Soil solver", 0.1, IncaDascru);
	
	auto GroundwaterRechargeFraction = RegisterEquation(Model, "Groundwater recharge fraction", Dimensionless);
	SetSolver(Model, GroundwaterRechargeFraction, SoilSolver);
	auto Evapotranspiration = RegisterEquation(Model, "Evapotranspiration", MmPerDay);
	SetSolver(Model, Evapotranspiration, SoilSolver);
	auto GroundwaterRecharge = RegisterEquation(Model, "Groundwater recharge", MmPerDay);
	SetSolver(Model, GroundwaterRecharge, SoilSolver);
	auto SoilMoistureRecharge   = RegisterEquation(Model, "Soil moisture recharge", MmPerDay);
	SetSolver(Model, SoilMoistureRecharge, SoilSolver);
	auto SoilMoisture           = RegisterEquationODE(Model, "Soil moisture", Mm);
	SetSolver(Model, SoilMoisture, SoilSolver);
	SetInitialValue(Model, SoilMoisture, InitialSoilMoisture);
	
	auto TotalInputToGroundwater  = RegisterEquationCumulative(Model, "Total input to groundwater", GroundwaterRecharge, LandscapeUnits, Percent);           
	
	EQUATION(Model, GroundwaterRechargeFraction,
		double fraction = std::pow(RESULT(SoilMoisture) / PARAMETER(FieldCapacity), PARAMETER(Beta));  
		if (fraction > 1.0) fraction = 1.0;
		return fraction; 
	)      
	
	EQUATION(Model, Evapotranspiration,
		double smmax = PARAMETER(SoilMoistureEvapotranspirationMax) * PARAMETER(FieldCapacity);
		double potentialetpfraction = Min(RESULT(SoilMoisture) / smmax, 1.0);
		return RESULT(PotentialEvapotranspiration) * potentialetpfraction;
	)
	
	EQUATION(Model, GroundwaterRecharge,
		return RESULT(GroundwaterRechargeFraction) * RESULT(WaterToSoil);
	)
	
	EQUATION(Model, SoilMoistureRecharge,
		return (1.0 - RESULT(GroundwaterRechargeFraction)) * RESULT(WaterToSoil);
	)
	
	EQUATION(Model, SoilMoisture,
		return
			  RESULT(SoilMoistureRecharge)
			- RESULT(Evapotranspiration);
	)

}


static void
AddGroundwaterResponseRoutine(mobius_model *Model)
{
	auto Mm = RegisterUnit(Model, "mm");
	auto Days = RegisterUnit(Model, "day");
	auto PerDay = RegisterUnit(Model, "/day");
	auto MmPerDay = RegisterUnit(Model, "mm/day");
	
	auto Reaches = RegisterIndexSetBranched(Model, "Reaches");
	auto Groundwater = RegisterParameterGroup(Model, "Groundwater", Reaches);
	auto System = GetParameterGroupHandle(Model, "System");
	
	//TODO: Find good values for parameters.
	auto FirstRecessionCoefficient  = RegisterParameterDouble(Model, Groundwater, "Recession coefficient for groundwater slow flow (K1)", PerDay, 0.1);
	auto SecondRecessionCoefficient = RegisterParameterDouble(Model, Groundwater, "Recession coefficient for groundwater fast flow (K0)", PerDay, 0.1);
	auto SecondRunoffThreshold      = RegisterParameterDouble(Model, Groundwater, "Threshold for second runoff in groundwater storage (UZL)", Mm, 10.0);
	auto InitialGroundwaterStorage  = RegisterParameterDouble(Model, Groundwater, "Initial groundwater storage", Mm, 100.0);
	
	auto Recharge           = GetEquationHandle(Model, "Total input to groundwater"); //NOTE: From the soil moisture routine.

	auto FastFlow           = RegisterEquation(Model, "Fast groundwater flow", MmPerDay);
	auto SlowFlow           = RegisterEquation(Model, "Slow groundwater flow", MmPerDay);
	auto GroundwaterStorage = RegisterEquationODE(Model, "Groundwater storage", Mm);
	SetInitialValue(Model, GroundwaterStorage, InitialGroundwaterStorage);
			
	auto GroundwaterSolver = RegisterSolver(Model, "Groundwater solver", 0.1, IncaDascru);
	SetSolver(Model, FastFlow, GroundwaterSolver);
	SetSolver(Model, SlowFlow, GroundwaterSolver);
	SetSolver(Model, GroundwaterStorage, GroundwaterSolver);       
	
	EQUATION(Model, GroundwaterStorage,
		double sf = RESULT(SlowFlow);
		double ff = RESULT(FastFlow);
		double recharge = RESULT(Recharge);
		return recharge - ff - sf;
	)

	EQUATION(Model, FastFlow,
		double K0 = PARAMETER(FirstRecessionCoefficient);
		double UZL = PARAMETER(SecondRunoffThreshold);
		double storage = RESULT(GroundwaterStorage);
		double fastflow = storage > UZL ? (storage-UZL)*K0 : 0.;
		return fastflow;
	)         
	
	EQUATION(Model, SlowFlow,
		double K1 = PARAMETER(SecondRecessionCoefficient);
		double storage = RESULT(GroundwaterStorage);
		double slowflow = storage * K1;
		return slowflow;
	)    
}

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
	else
	{
		return a * (double)(2*M2 - I);
	}
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

#define HBV_H
#endif