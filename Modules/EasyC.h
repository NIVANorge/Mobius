

//The development of this model has been discontinued for now.



#if !defined(CARBON_MODEL_H)


//NOTE: This is in development and is far from finished



inline double
SCurve(double X, double Threshold1, double Threshold2)
{
	if(X < Threshold1) return 0.0;
	if(X > Threshold2) return 1.0;
	double Y = (X - Threshold1) / (Threshold2 - Threshold1);
	return (3.0 - 2.0*Y)*Y*Y;
}



static void
AddCarbonInSoilModule(mobius_model *Model)
{
	auto Km2     = RegisterUnit(Model, "km2");
	auto M3      = RegisterUnit(Model, "m3");
	auto KgPerM2 = RegisterUnit(Model, "kg/m2");
	auto KgPerM2PerDay = RegisterUnit(Model, "kg/m2/day");
	auto PerDay  = RegisterUnit(Model, "/day");
	auto Kg      = RegisterUnit(Model, "kg");
	auto MetresPerDay = RegisterUnit(Model, "m/day");
	auto KgPerM3 = RegisterUnit(Model, "kg/m3");
	auto Dimensionless = RegisterUnit(Model);
	auto M3PerDay = RegisterUnit(Model, "m3/day");
	auto KgPerDay = RegisterUnit(Model, "kg/day");
	auto Mm      = RegisterUnit(Model, "mm");
	
	
	auto Land   = GetParameterGroupHandle(Model, "Landscape units");

	auto InitialSOCInUpperSoilPerArea = RegisterParameterDouble(Model, Land, "Initial SOC in upper soil layer", KgPerM2, 2.0);
	auto InitialSOCInLowerSoilPerArea = RegisterParameterDouble(Model, Land, "Initial SOC in lower soil layer", KgPerM2, 2.0);
	auto LitterFall = RegisterParameterDouble(Model, Land, "Litter fall", KgPerM2PerDay, 0.02, 0.0, 10.0, "Litter fall from the canopy to the upper soil layer");
	
	
	auto SorptionRateUpperBox   = RegisterParameterDouble(Model, Land, "Sorption rate in upper soil", PerDay, 0.1, 0.0, 1.0, "Rate coefficient for DOC sorption (DOC to SOC)");
	auto SorptionRateLowerBox   = RegisterParameterDouble(Model, Land, "Sorption rate in lower soil", PerDay, 0.1, 0.0, 1.0, "Rate coefficient for DOC sorption (DOC to SOC)");
	
	auto DesorptionRateUpperBox = RegisterParameterDouble(Model, Land, "Desorption rate in upper soil", PerDay, 1e-4, 0.0, 1.0, "Rate coefficient for SOC desorption (SOC to DOC)");
	auto DesorptionRateLowerBox = RegisterParameterDouble(Model, Land, "Desorption rate in lower soil", PerDay, 1e-4, 0.0, 1.0, "Rate coefficient for SOC desorption (SOC to DOC)");
	
	auto BaseMineralisationRateUpperBox = RegisterParameterDouble(Model, Land, "Base mineralisation rate in upper soil", PerDay, 0.1);
	auto BaseMineralisationRateLowerBox = RegisterParameterDouble(Model, Land, "Base mineralisation rate in lower soil", PerDay, 0.1);
	
	auto System = GetParameterGroupHandle(Model, "System");
	auto DegasVelocity = RegisterParameterDouble(Model, System, "Degas velocity", MetresPerDay, 15.0, 0.0, 100.0, "DIC mass transfer velocity from upper soil layer/river to the atmosphere");
	auto DICConcentrationAtSaturation = RegisterParameterDouble(Model, System, "DIC concentration at saturation", KgPerM3, 0.018, 0.0, 0.1, "If DIC concentration is higher than this degas sets in");
	auto MineralisationResponseToTemperature = RegisterParameterDouble(Model, System, "Mineralisation response to temperature", Dimensionless, 1.3);
	
	auto ZeroRateDepth = RegisterParameterDouble(Model, Land, "Zero rate depth", Mm, 75.0);
	auto MaxRateDepth  = RegisterParameterDouble(Model, Land, "Max rate depth", Mm, 75.0);
	
	auto SoilSolver = GetSolverHandle(Model, "Soil solver");

	auto CatchmentArea = GetParameterDoubleHandle(Model, "Catchment area");
	auto Percent       = GetParameterDoubleHandle(Model, "%");
	//auto FieldCapacity = GetParameterDoubleHandle(Model, "Field capacity");
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	
	auto SoilTemperature = GetEquationHandle(Model, "Soil temperature");

	
	auto UpperSoilGroundwaterRecharge         = GetEquationHandle(Model, "Upper soil layer groundwater recharge");
	auto UpperSoilMoistureRecharge            = GetEquationHandle(Model, "Upper soil layer moisture recharge");
	auto UpperSoilMoisture                    = GetEquationHandle(Model, "Soil moisture in upper soil layer");
	auto LowerSoilGroundwaterRecharge         = GetEquationHandle(Model, "Lower soil layer groundwater recharge");
	auto LowerSoilMoistureRecharge            = GetEquationHandle(Model, "Lower soil layer moisture recharge");
	auto LowerSoilMoisture                    = GetEquationHandle(Model, "Soil moisture in lower soil layer");
	
	
	auto InitialSOCInUpperSoil = RegisterEquationInitialValue(Model, "Initial SOC in upper soil box", Kg);
	auto InitialSOCInLowerSoil = RegisterEquationInitialValue(Model, "Initial SOC in lower soil box", Kg);
	
	auto SoilMoistureFactor = RegisterEquation(Model, "Soil moisture factor", Dimensionless);
	SetSolver(Model, SoilMoistureFactor, SoilSolver);
	
	auto SoilTemperatureFactor = RegisterEquation(Model, "Soil temperature factor", Dimensionless);
	
	auto MineralisationRateInUpperSoilBox = RegisterEquation(Model, "Mineralisation rate in upper soil box", PerDay);
	SetSolver(Model, MineralisationRateInUpperSoilBox, SoilSolver);
	auto MineralisationRateInLowerSoilBox = RegisterEquation(Model, "Mineralisation rate in lower soil box", PerDay);
	SetSolver(Model, MineralisationRateInLowerSoilBox, SoilSolver);
	
	auto SOCInUpperSoilBox = RegisterEquationODE(Model, "SOC in upper soil box", Kg);
	SetSolver(Model, SOCInUpperSoilBox, SoilSolver);
	SetInitialValue(Model, SOCInUpperSoilBox, InitialSOCInUpperSoil);
	auto DOCInUpperSoilBox = RegisterEquationODE(Model, "DOC in upper soil box", Kg);
	SetSolver(Model, DOCInUpperSoilBox, SoilSolver);
	auto DICInUpperSoilBox = RegisterEquationODE(Model, "DIC in upper soil box", Kg);
	SetSolver(Model, DICInUpperSoilBox, SoilSolver);
	
	auto SOCInLowerSoilBox = RegisterEquationODE(Model, "SOC in lower soil box", Kg);
	SetSolver(Model, SOCInLowerSoilBox, SoilSolver);
	SetInitialValue(Model, SOCInLowerSoilBox, InitialSOCInLowerSoil);
	auto DOCInLowerSoilBox = RegisterEquationODE(Model, "DOC in lower soil box", Kg);
	SetSolver(Model, DOCInLowerSoilBox, SoilSolver);
	auto DICInLowerSoilBox = RegisterEquationODE(Model, "DIC in lower soil box", Kg);
	SetSolver(Model, DICInLowerSoilBox, SoilSolver);
	
	auto DOCFromLandscapeUnitToGroundwater = RegisterEquation(Model, "DOC from landscape unit to groundwater", KgPerDay);
	SetSolver(Model, DOCFromLandscapeUnitToGroundwater, SoilSolver);
	auto DICFromLandscapeUnitToGroundwater = RegisterEquation(Model, "DIC from landscape unit to groundwater", KgPerDay);
	SetSolver(Model, DICFromLandscapeUnitToGroundwater, SoilSolver);
	
	
	EQUATION(Model, InitialSOCInUpperSoil,
		return PARAMETER(InitialSOCInUpperSoilPerArea) * (PARAMETER(CatchmentArea) * 1e6) * (PARAMETER(Percent) / 100.0);
	)
	
	EQUATION(Model, InitialSOCInLowerSoil,
		return PARAMETER(InitialSOCInLowerSoilPerArea) * (PARAMETER(CatchmentArea) * 1e6) * (PARAMETER(Percent) / 100.0);
	)
	
	EQUATION(Model, SoilMoistureFactor,
		return SCurve(RESULT(UpperSoilMoisture), PARAMETER(ZeroRateDepth), PARAMETER(MaxRateDepth));
	)
	
	EQUATION(Model, SoilTemperatureFactor,
		return pow(PARAMETER(MineralisationResponseToTemperature), RESULT(SoilTemperature));
	)
	
	EQUATION(Model, MineralisationRateInUpperSoilBox,
		return 
			PARAMETER(BaseMineralisationRateUpperBox) 
		  * RESULT(SoilTemperatureFactor)
		  * RESULT(SoilMoistureFactor);
	)
	
	EQUATION(Model, SOCInUpperSoilBox,
		return 
			  PARAMETER(LitterFall) * (PARAMETER(CatchmentArea)*1e6) * (PARAMETER(Percent)/100.0)
			+ PARAMETER(SorptionRateUpperBox) * RESULT(DOCInUpperSoilBox) 
			- PARAMETER(DesorptionRateUpperBox) * RESULT(SOCInUpperSoilBox);
	)
	
	EQUATION(Model, DOCInUpperSoilBox,
		double relativedocconcentration = SafeDivide(RESULT(DOCInUpperSoilBox), RESULT(UpperSoilMoisture));
		
		return 
			PARAMETER(DesorptionRateUpperBox) * RESULT(SOCInUpperSoilBox)           //Desorption
		  - PARAMETER(SorptionRateUpperBox) * RESULT(DOCInUpperSoilBox)             //Sorption
		  - RESULT(DOCInUpperSoilBox) * RESULT(MineralisationRateInUpperSoilBox)    //Mineralisation 
		  
		  - (RESULT(LowerSoilMoistureRecharge) + RESULT(UpperSoilGroundwaterRecharge)) * relativedocconcentration; // Transport
	)
	
	EQUATION(Model, DICInUpperSoilBox,
		double degasvelocity = PARAMETER(DegasVelocity);
		double dicconcentrationatsaturation = PARAMETER(DICConcentrationAtSaturation);
		
		double relativedicconcentration = SafeDivide(RESULT(DICInUpperSoilBox), RESULT(UpperSoilMoisture));
		double dicconcentration = SafeDivide(relativedicconcentration / 1000.0,  (PARAMETER(CatchmentArea) * 1e6) * (PARAMETER(Percent) / 100.0));
		
		double degas = Max(0.0, PARAMETER(DegasVelocity) * (dicconcentration - PARAMETER(DICConcentrationAtSaturation)));
		
		return
			RESULT(MineralisationRateInUpperSoilBox) * RESULT(DOCInUpperSoilBox)
		  - degas
		  - (RESULT(LowerSoilMoistureRecharge) + RESULT(UpperSoilGroundwaterRecharge)) * relativedicconcentration; // Transport 
	)
	
	EQUATION(Model, MineralisationRateInLowerSoilBox,
		return 
			PARAMETER(BaseMineralisationRateUpperBox) 
		  * RESULT(SoilTemperatureFactor);
	)
	
	EQUATION(Model, SOCInLowerSoilBox,
		return
			  PARAMETER(SorptionRateLowerBox) * RESULT(DOCInLowerSoilBox) 
			- PARAMETER(DesorptionRateLowerBox) * RESULT(SOCInLowerSoilBox);
	)
	
	
	EQUATION(Model, DOCInLowerSoilBox,
		double relativedocconcentrationupp = SafeDivide(RESULT(DOCInUpperSoilBox), RESULT(UpperSoilMoisture));
		double relativedocconcentrationlow = SafeDivide(RESULT(DOCInLowerSoilBox), RESULT(LowerSoilMoisture));
	
		return
			- PARAMETER(SorptionRateLowerBox) * RESULT(DOCInLowerSoilBox) 
			+ PARAMETER(DesorptionRateLowerBox) * RESULT(SOCInLowerSoilBox)
			- RESULT(MineralisationRateInLowerSoilBox) * RESULT(DOCInLowerSoilBox)
			
			+ relativedocconcentrationupp * RESULT(LowerSoilMoistureRecharge)
			- relativedocconcentrationlow * RESULT(LowerSoilGroundwaterRecharge);
	)
	
	EQUATION(Model, DICInLowerSoilBox,
		double relativedicconcentrationupp = SafeDivide(RESULT(DICInUpperSoilBox), RESULT(UpperSoilMoisture));
		double relativedicconcentrationlow = SafeDivide(RESULT(DICInLowerSoilBox), RESULT(LowerSoilMoisture));
	
		return
			  RESULT(MineralisationRateInLowerSoilBox) * RESULT(DOCInLowerSoilBox)
			
			+ relativedicconcentrationupp * RESULT(LowerSoilMoistureRecharge)
			- relativedicconcentrationlow * RESULT(LowerSoilGroundwaterRecharge);
	)

	
	EQUATION(Model, DOCFromLandscapeUnitToGroundwater,
		double relativedocconcentrationupp = SafeDivide(RESULT(DOCInUpperSoilBox), RESULT(UpperSoilMoisture));
		double relativedocconcentrationlow = SafeDivide(RESULT(DOCInLowerSoilBox), RESULT(LowerSoilMoisture));
		
		return relativedocconcentrationupp * RESULT(UpperSoilGroundwaterRecharge) + relativedocconcentrationlow * RESULT(LowerSoilGroundwaterRecharge);
	)
	
	EQUATION(Model, DICFromLandscapeUnitToGroundwater,
		double relativedicconcentrationupp = SafeDivide(RESULT(DICInUpperSoilBox), RESULT(UpperSoilMoisture));
		double relativedicconcentrationlow = SafeDivide(RESULT(DICInLowerSoilBox), RESULT(LowerSoilMoisture));
		
		return relativedicconcentrationupp * RESULT(UpperSoilGroundwaterRecharge) + relativedicconcentrationlow * RESULT(LowerSoilGroundwaterRecharge);
	)
}

static void
AddCarbonInGroundwaterModule(mobius_model *Model)
{
	auto Kg       = RegisterUnit(Model, "kg");
	auto KgPerDay = RegisterUnit(Model, "kg/day");
	
	auto GroundwaterSolver = GetSolverHandle(Model, "Groundwater solver");
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	
	auto DOCFromLandscapeUnitToGroundwater = GetEquationHandle(Model, "DOC from landscape unit to groundwater");
	auto DICFromLandscapeUnitToGroundwater = GetEquationHandle(Model, "DIC from landscape unit to groundwater");
	auto FastFlow                          = GetEquationHandle(Model, "Fast groundwater flow");
	auto SlowFlow                          = GetEquationHandle(Model, "Slow groundwater flow");
	auto GroundwaterStorage                = GetEquationHandle(Model, "Groundwater storage");
	
	auto DOCToGroundwater = RegisterEquationCumulative(Model, "DOC to Groundwater", DOCFromLandscapeUnitToGroundwater, LandscapeUnits);
	auto DICToGroundwater = RegisterEquationCumulative(Model, "DIC to Groundwater", DICFromLandscapeUnitToGroundwater, LandscapeUnits);
	
	auto DOCInGroundwaterStorage = RegisterEquationODE(Model, "DOC in groundwater storage", Kg);
	SetSolver(Model, DOCInGroundwaterStorage, GroundwaterSolver);
	auto DICInGroundwaterStorage = RegisterEquationODE(Model, "DIC in groundwater storage", Kg);
	SetSolver(Model, DICInGroundwaterStorage, GroundwaterSolver);
	
	auto DOCFromGroundwaterToRouting = RegisterEquation(Model, "DOC from groundwater to routing routine", KgPerDay);
	SetSolver(Model, DOCFromGroundwaterToRouting, GroundwaterSolver);
	auto DICFromGroundwaterToRouting = RegisterEquation(Model, "DIC from groundwater to routing routine", KgPerDay);
	SetSolver(Model, DICFromGroundwaterToRouting, GroundwaterSolver);

	EQUATION(Model, DOCFromGroundwaterToRouting,
		return SafeDivide(RESULT(DOCInGroundwaterStorage) * (RESULT(FastFlow) + RESULT(SlowFlow)), RESULT(GroundwaterStorage));
	)
	
	EQUATION(Model, DICFromGroundwaterToRouting,
		return SafeDivide(RESULT(DICInGroundwaterStorage) * (RESULT(FastFlow) + RESULT(SlowFlow)), RESULT(GroundwaterStorage));
	)
	
	EQUATION(Model, DOCInGroundwaterStorage,
		return RESULT(DOCToGroundwater) - RESULT(DOCFromGroundwaterToRouting);
	)
	
	EQUATION(Model, DICInGroundwaterStorage,
		return RESULT(DICToGroundwater) - RESULT(DICFromGroundwaterToRouting);
	)	
}

static void
AddCarbonRoutingRoutine(mobius_model *Model)
{
	auto KgPerDay = RegisterUnit(Model, "kg/day");

	
	auto DOCFromRoutingToReach = RegisterEquation(Model, "DOC from routing to reach", KgPerDay);
	auto DICFromRoutingToReach = RegisterEquation(Model, "DIC from routing to reach", KgPerDay);
	
	auto DOCFromGroundwaterToRouting = GetEquationHandle(Model, "DOC from groundwater to routing routine");
	auto DICFromGroundwaterToRouting = GetEquationHandle(Model, "DIC from groundwater to routing routine");
	
	auto MaxBase = GetParameterUIntHandle(Model, "Flow routing max base");
	
	
	EQUATION(Model, DOCFromRoutingToReach,
		RESULT(DOCFromGroundwaterToRouting); //NOTE: To force a dependency since this is not automatic when we use EARLIER_RESULT;
	
		u64 M = PARAMETER(MaxBase);		
		double sum = 0.0;

		for(u64 I = 1; I <= M; ++I)
		{
			sum += RoutingCoefficient(M, I) * EARLIER_RESULT(DOCFromGroundwaterToRouting, I-1);
		}
			
		return sum;
	)
	
	EQUATION(Model, DICFromRoutingToReach,
		RESULT(DICFromGroundwaterToRouting); //NOTE: To force a dependency since this is not automatic when we use EARLIER_RESULT;
	
		u64 M = PARAMETER(MaxBase);		
		double sum = 0.0;

		for(u64 I = 1; I <= M; ++I)
		{
			sum += RoutingCoefficient(M, I) * EARLIER_RESULT(DICFromGroundwaterToRouting, I-1);
		}
			
		return sum;
	)
}

static void
AddCarbonInReachModule(mobius_model *Model)
{
	auto PerDay   = RegisterUnit(Model, "/day");
	auto KgPerDay = RegisterUnit(Model, "kg/day");
	auto Kg       = RegisterUnit(Model, "kg");
	auto Dimensionless = RegisterUnit(Model);
	auto KgPerM3  = RegisterUnit(Model, "kg/m3");
	
	auto WaterTemperature = GetEquationHandle(Model, "Water temperature");
	auto ReachVolume      = GetEquationHandle(Model, "Reach volume");
	auto ReachFlow        = GetEquationHandle(Model, "Reach flow");
	
	auto MineralisationResponseToTemperature = GetParameterDoubleHandle(Model, "Mineralisation response to temperature");
	auto DegasVelocity = GetParameterDoubleHandle(Model, "Degas velocity");
	auto DICConcentrationAtSaturation = GetParameterDoubleHandle(Model, "DIC concentration at saturation");
	
	auto DOCFromRoutingToReach = GetEquationHandle(Model, "DOC from routing to reach");
	auto DICFromRoutingToReach = GetEquationHandle(Model, "DIC from routing to reach");
	
	auto Reach           = GetIndexSetHandle(Model, "Reaches");
	auto ReachParameters = GetParameterGroupHandle(Model, "Reach parameters");
	
	auto ReachSolver     = GetSolverHandle(Model, "Reach solver");
	
	auto BaseMineralisationRateInReach = RegisterParameterDouble(Model, ReachParameters, "Base mineralisation rate in reach", PerDay, 1e-5);
	auto Sigma1                        = RegisterParameterDouble(Model, ReachParameters, "Sigma 1", Dimensionless, 1e-3, 1e-4, 1e-2, "Parameter used for determining the rate of photo-oxidation in reach");   //TODO: Figure out dimension later
	auto Sigma2                        = RegisterParameterDouble(Model, ReachParameters, "Sigma 2", Dimensionless, 1e-4, 1e-5, 1e-3, "Parameter used for determining the rate of photo-oxidation in reach");   //TODO: Figure out dimension later
	
	auto SolarRadiation = RegisterInput(Model, "Solar radiation");
	
	auto MineralisationRateInReach = RegisterEquation(Model, "Mineralisation rate in reach", PerDay);
	auto PhotoOxidationRateInReach = RegisterEquation(Model, "Photo-oxidation rate in reach", PerDay);
	SetSolver(Model, PhotoOxidationRateInReach, ReachSolver);
	auto DOCInputToReach           = RegisterEquation(Model, "DOC input to reach", KgPerDay);
	auto DICInputToReach           = RegisterEquation(Model, "DIC input to reach", KgPerDay);
	
	auto DOCOutputFromReach        = RegisterEquation(Model, "DOC output from reach", KgPerDay);
	SetSolver(Model, DOCOutputFromReach, ReachSolver);
	auto DICOutputFromReach        = RegisterEquation(Model, "DIC output from reach", KgPerDay);
	SetSolver(Model, DICOutputFromReach, ReachSolver);
	
	auto DOCInReach                = RegisterEquationODE(Model, "DOC in reach", Kg);
	SetSolver(Model, DOCInReach, ReachSolver);
	//TODO: Initial value?
	auto DICInReach                = RegisterEquationODE(Model, "DIC in reach", Kg);
	SetSolver(Model, DICInReach, ReachSolver);
	//TODO: Initial value?
	
	auto DOCConcentrationInReach = RegisterEquation(Model, "DOC concentration in reach", KgPerM3);
	SetSolver(Model, DOCConcentrationInReach, ReachSolver);
	auto DICConcentrationInReach = RegisterEquation(Model, "DIC concentration in reach", KgPerM3);
	SetSolver(Model, DICConcentrationInReach, ReachSolver);
	
	EQUATION(Model, MineralisationRateInReach,
		return 
			  PARAMETER(BaseMineralisationRateInReach)
			* pow(RESULT(WaterTemperature), PARAMETER(MineralisationResponseToTemperature));
	)
	
	EQUATION(Model, PhotoOxidationRateInReach,
		return (PARAMETER(Sigma1) * INPUT(SolarRadiation)) / (PARAMETER(Sigma2) + RESULT(DOCConcentrationInReach));
	)
	
	EQUATION(Model, DOCInputToReach,
		double docinput = RESULT(DOCFromRoutingToReach);
		// TODO: Effluent input?
		FOREACH_INPUT(Reach,
			docinput += RESULT(DOCOutputFromReach, *Input);
		)
		
		return docinput;
	)
	
	EQUATION(Model, DICInputToReach,
		double dicinput = RESULT(DICFromRoutingToReach);
		// TODO: Effluent input?
		FOREACH_INPUT(Reach,
			dicinput += RESULT(DICOutputFromReach, *Input);
		)
		
		return dicinput;
	)
	
	EQUATION(Model, DOCOutputFromReach,
		return RESULT(DOCInReach) * SafeDivide(RESULT(ReachFlow), RESULT(ReachVolume));
	)
	
	EQUATION(Model, DICOutputFromReach,
		return RESULT(DICInReach) * SafeDivide(RESULT(ReachFlow), RESULT(ReachVolume));
	)
	
	EQUATION(Model, DOCInReach,
		return
			  RESULT(DOCInputToReach)
			- RESULT(DOCOutputFromReach)
			- RESULT(DOCInReach) * (RESULT(PhotoOxidationRateInReach) + RESULT(MineralisationRateInReach));
	)
	
	EQUATION(Model, DOCConcentrationInReach,
		return SafeDivide(RESULT(DOCInReach), RESULT(ReachVolume));
	)
	
	EQUATION(Model, DICInReach,
		return
			  RESULT(DICInputToReach)
			- RESULT(DICOutputFromReach)
			+ RESULT(DOCInReach) * (RESULT(PhotoOxidationRateInReach) + RESULT(MineralisationRateInReach));
			- PARAMETER(DegasVelocity) * (RESULT(DICInReach) / RESULT(ReachVolume) - PARAMETER(DICConcentrationAtSaturation));
	)
	
	EQUATION(Model, DICConcentrationInReach,
		return SafeDivide(RESULT(DICInReach), RESULT(ReachVolume));
	)
}

static void
AddEasyCModel(mobius_model *Model)
{
	AddCarbonInSoilModule(Model);
	AddCarbonInGroundwaterModule(Model);
	AddCarbonRoutingRoutine(Model);
	AddCarbonInReachModule(Model);
}

#define CARBON_MODEL_H
#endif
