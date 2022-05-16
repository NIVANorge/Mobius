

static void
AddSimplySedimentModule(mobius_model *Model)
{
	BeginModule(Model, "SimplySed", "0.5.0");
	
	SetModuleDescription(Model, R""""(
This is a simple sediment transport module created as a part of SimplyP.

New to version 0.5:
* Linear and power coefficents should have less covariance, making them easier to calibrate.
)"""");
	
	auto Dimensionless = RegisterUnit(Model);
	auto Kg            = RegisterUnit(Model, "kg");
	auto KgPerDay      = RegisterUnit(Model, "kg/day");
	auto KgPerM3       = RegisterUnit(Model, "kg/m^3");
	auto JulianDay     = RegisterUnit(Model, "Julian day");
	auto Degrees       = RegisterUnit(Model, "°");
	auto MgPerL        = RegisterUnit(Model, "mg/l");
	
	// Set up index sets
	auto Reach          = GetIndexSetHandle(Model, "Reaches");
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	
	// Params already defined
	auto CatchmentArea = GetParameterDoubleHandle(Model, "Catchment area");
	auto ReachSlope    = GetParameterDoubleHandle(Model, "Reach slope");
	auto ReachLength   = GetParameterDoubleHandle(Model, "Reach length");
	
	// Global sediment parameters (don't vary by land use/sub-catchment/reach
	auto Sediment = RegisterParameterGroup(Model, "Erodibility and sediments");
	
	//TODO: Fix units and document!
	auto ReachSedimentInputScalingFactor         = RegisterParameterDouble(Model, Sediment, "Reach sediment input scaling factor", KgPerM3, 15.0, 0.0, 100.0, "Calibrated parameter linking simulated sediment input from land to simulated flow from land", "Ksed");
	auto SedimentInputNonlinearCoefficient       = RegisterParameterDouble(Model, Sediment, "Sediment input non-linear coefficient", Dimensionless, 2.0, 0.1, 5.0, "", "psed");
	
	// Params that vary by land class and reach
	auto SubcatchmentGeneral = RegisterParameterGroup(Model, "Land slope", Reach, LandscapeUnits);
	auto MeanSlopeOfLand                         = RegisterParameterDouble(Model, SubcatchmentGeneral, "Mean slope of land in the subcatchment", Degrees, 4.0, 0.0, 90.0);
	
	auto LandUseProportions = GetParameterDoubleHandle(Model, "Land use proportions");
	
	// Sediment params that vary by land class
	auto SedimentLand = RegisterParameterGroup(Model, "Sediment land", LandscapeUnits);
	auto VegetationCoverFactor                   = RegisterParameterDouble(Model, SedimentLand, "Vegetation cover factor", Dimensionless, 0.2, 0.0, 1.0, "Vegetation cover factor, describing ratio between long-term erosion under the land use class, compared to under bare soil of the same soil type, slope, etc. Source from (R)USLE literature and area-weight as necessary to obtain a single value for the land class.", "kveg");
	auto ReductionOfLoadInSediment               = RegisterParameterDouble(Model, SedimentLand, "Reduction of load in sediment", Dimensionless, 0.0, 0.0, 1.0, "Proportional reduction in load of sediment delivered to the reach due to management measures, e.g. buffer strips, filter fences, conservation tillage, etc.", "kload"); //Note: may be better indexing this by reach? TO DO
	
	auto DynamicErodibility                      = RegisterParameterBool(Model, SedimentLand, "Dynamic erodibility", false, "Requires one of your land use classes to be 'Arable' (exact name match). If set to 'true', the model simulates the change in erodibility on arable land through the year due to cropping and harvesting");
	auto DayOfYearWhenSoilErodibilityIsMaxSpring = RegisterParameterUInt(Model, SedimentLand, "Day of year when soil erodibility is at its max for spring-grown crops", JulianDay, 60, 30, 335, "Parameter only used if Dynamic erodibility is set to true and spring-sown crops are present in the catchment");
	auto DayOfYearWhenSoilErodibilityIsMaxAutumn = RegisterParameterUInt(Model, SedimentLand, "Day of year when soil erodibility is at its max for autumn-grown crops", JulianDay, 304, 30, 335, "Parameter only used if Dynamic erodibility is set to true and autumn-sown crops are present in the catchment");
	auto ProportionOfSpringGrownCrops            = RegisterParameterDouble(Model, SedimentLand, "Proportion of spring grown crops", Dimensionless, 0.65, 0.0, 1.0, "Proportion of total arable land that is spring-sown crops. Only needed if Dynamic erodibility is set to true.");
	
	
	// Start equations
	
	auto ReachSolver = GetSolverHandle(Model, "SimplyQ reach solver");
	
	// Equations already defined
	auto ReachFlow          = GetEquationHandle(Model, "Reach flow (end-of-day)");
	auto ReachFlowInputFromLand = GetEquationHandle(Model, "Flow input from land");
	auto DailyMeanReachFlow = GetEquationHandle(Model, "Reach flow (daily mean, cumecs)");
	auto ReachVolume        = GetEquationHandle(Model, "Reach volume");
	
	// Sediment equations
	auto TimeDependentVegetationCoverFactor    = RegisterEquation(Model, "Time dependent vegetation cover factor", Dimensionless);
	auto ReachSedimentInputCoefficient         = RegisterEquation(Model, "Sediment input coefficient", KgPerM3);
	auto TotalReachSedimentInputCoefficient    = RegisterEquationCumulative(Model, "Sediment input coefficient summed over land classes", ReachSedimentInputCoefficient, LandscapeUnits, LandUseProportions);
	auto ErosionFactor                         = RegisterEquation(Model, "Erosion factor", Dimensionless, ReachSolver);
	
	auto SuspendedSedimentFlux = RegisterEquation(Model, "Reach suspended sediment flux", KgPerDay, ReachSolver);
	
	auto ReachSedimentInput = RegisterEquation(Model, "Reach sediment input (erosion and entrainment)", KgPerDay, ReachSolver);
	
	auto ReachSedimentInputUpstream = RegisterEquation(Model, "Reach sediment input (upstream flux)", KgPerDay);
	
	auto SuspendedSedimentMass = RegisterEquationODE(Model, "Reach suspended sediment mass", Kg, ReachSolver);
	SetInitialValue(Model, SuspendedSedimentMass, 0.0);
	
	auto DailyMeanSuspendedSedimentFlux = RegisterEquationODE(Model, "Reach daily mean suspended sediment flux", KgPerDay, ReachSolver);
	SetInitialValue(Model, DailyMeanSuspendedSedimentFlux, 0.0);
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
		if(PARAMETER(DynamicErodibility))
		{
			double C_cov = 0.0;
			double E_risk_period = 60.0;
			for(size_t Season = 0; Season < 2; ++Season)
			{
				double d_start = d_maxE[Season] - E_risk_period / 2.0;
				double d_end   = d_maxE[Season] + E_risk_period / 2.0;
				double d_mid   = d_maxE[Season];
				double dayNo = (double)CURRENT_TIME().DayOfYear;
				double C_season;
				if(dayNo >= d_start && dayNo <= d_end)
				{
					if(dayNo < d_mid) C_season = LinearInterpolate(dayNo, d_start, d_mid, C_cover, 1.0);
					else              C_season = LinearInterpolate(dayNo, d_mid,   d_end, 1.0, C_cover);
				}
				else C_season = C_cover - E_risk_period*(1.0 - C_cover)/(2.0*(CURRENT_TIME().DaysThisYear-E_risk_period));
				C_cov += C_season * coverproportion[Season];
			}
			return C_cov;
		}
		return C_cover;
	)
	
	EQUATION(Model, ReachSedimentInputCoefficient,
		// Reach sed input coefficient per land use class. 
		return
			PARAMETER(MeanSlopeOfLand)
			* RESULT(TimeDependentVegetationCoverFactor)
			* (1.0 - PARAMETER(ReductionOfLoadInSediment));
	
		//Note: if this changes, also needs to change in the particulate P equations
	)
	
	EQUATION(Model, ErosionFactor,
		double A_catch = PARAMETER(CatchmentArea);
		double a = PARAMETER(ReachSedimentInputScalingFactor) * 1e-3;
		return A_catch * pow(a * 86400.0*RESULT(ReachFlowInputFromLand) / A_catch, PARAMETER(SedimentInputNonlinearCoefficient));
	)
	
	EQUATION(Model, ReachSedimentInput,
		return RESULT(TotalReachSedimentInputCoefficient) * RESULT(ErosionFactor);
	)
	
	EQUATION(Model, ReachSedimentInputUpstream,
		double upstreamflux = 0.0;
		for(index_t Input : BRANCH_INPUTS(Reach))
			upstreamflux += RESULT(DailyMeanSuspendedSedimentFlux, Input);
		return upstreamflux;
	)
	
	EQUATION(Model, SuspendedSedimentMass,	
		return RESULT(ReachSedimentInput) + RESULT(ReachSedimentInputUpstream) - RESULT(SuspendedSedimentFlux);
	)
	
	EQUATION(Model, SuspendedSedimentFlux,
		return 86400.0 * RESULT(ReachFlow) * SafeDivide(RESULT(SuspendedSedimentMass), RESULT(ReachVolume));
	)
	
	EQUATION(Model, DailyMeanSuspendedSedimentFlux,
		return RESULT(SuspendedSedimentFlux);
	)
	
	EQUATION(Model, SuspendedSedimentConcentration,
		//Converting flow m3/s->m3/day, and converting kg/m3 to mg/l. TODO: make conversion functions
		return 1e3 * SafeDivide(RESULT(DailyMeanSuspendedSedimentFlux), 86400.0 * RESULT(DailyMeanReachFlow));
	)
	
	EndModule(Model);
}



static void
AddAwfullySimplySedimentModule(mobius_model *Model)
{
	BeginModule(Model, "AwfullySimplySed", "0.0.1");
	
	SetModuleDescription(Model, R""""(
This is an awfully simple sediment transport module, simplified from SimplySed for semi-natural areas (without agriculture).
)"""");
	
	auto Dimensionless = RegisterUnit(Model);
	auto Kg            = RegisterUnit(Model, "kg");
	auto KgPerDay      = RegisterUnit(Model, "kg/day");
	auto KgPerM3       = RegisterUnit(Model, "kg/m^3");
	auto JulianDay     = RegisterUnit(Model, "Julian day");
	auto Degrees       = RegisterUnit(Model, "°");
	auto MgPerL        = RegisterUnit(Model, "mg/l");
	
	auto Sediment = RegisterParameterGroup(Model, "Erodibility and sediments");
	// TODO: fix units!
	auto ReachSedimentInputScalingFactor         = RegisterParameterDouble(Model, Sediment, "Reach sediment input scaling factor", Dimensionless, 15.0, 0.0, 100.0, "Calibrated parameter linking simulated sediment input from land to simulated flow from land", "Ksed");
	auto SedimentInputNonlinearCoefficient       = RegisterParameterDouble(Model, Sediment, "Sediment input non-linear coefficient", Dimensionless, 2.0, 0.1, 5.0, "", "psed");
	
	
	auto ReachSolver = GetSolverHandle(Model, "SimplyQ reach solver");
	auto Reach       = GetIndexSetHandle(Model, "Reaches");
	
	auto SuspendedSedimentFlux = RegisterEquation(Model, "Reach suspended sediment flux", KgPerDay, ReachSolver);
	auto ReachSedimentInput = RegisterEquation(Model, "Reach sediment input (erosion and entrainment)", KgPerDay, ReachSolver);
	auto ReachSedimentInputUpstream = RegisterEquation(Model, "Reach sediment input (upstream flux)", KgPerDay);
	
	auto SuspendedSedimentMass = RegisterEquationODE(Model, "Reach suspended sediment mass", Kg, ReachSolver);
	SetInitialValue(Model, SuspendedSedimentMass, 0.0);
	
	auto DailyMeanSuspendedSedimentFlux = RegisterEquationODE(Model, "Reach daily mean suspended sediment flux", KgPerDay, ReachSolver);
	SetInitialValue(Model, DailyMeanSuspendedSedimentFlux, 0.0);
	ResetEveryTimestep(Model, DailyMeanSuspendedSedimentFlux);
	
	auto SuspendedSedimentConcentration = RegisterEquation(Model, "Reach suspended sediment concentration", MgPerL); //Volume-weighted daily mean

	
	// Equations already defined
	auto ReachFlow          = GetEquationHandle(Model, "Reach flow (end-of-day)");
	auto ReachFlowInputFromLand = GetEquationHandle(Model, "Flow input from land");
	auto DailyMeanReachFlow = GetEquationHandle(Model, "Reach flow (daily mean, cumecs)");
	auto ReachVolume        = GetEquationHandle(Model, "Reach volume");
	
	// Params already defined
	auto CatchmentArea = GetParameterDoubleHandle(Model, "Catchment area");
	auto ReachSlope    = GetParameterDoubleHandle(Model, "Reach slope");
	auto ReachLength   = GetParameterDoubleHandle(Model, "Reach length");
	
	
	EQUATION(Model, ReachSedimentInput,
		double A_catch = PARAMETER(CatchmentArea);
		double a = PARAMETER(ReachSedimentInputScalingFactor) * 1e-3;
		return A_catch * pow(a * 86400.0*RESULT(ReachFlow) / A_catch, PARAMETER(SedimentInputNonlinearCoefficient));
	)
	
	EQUATION(Model, ReachSedimentInputUpstream,
		double upstreamflux = 0.0;
		for(index_t Input : BRANCH_INPUTS(Reach))
			upstreamflux += RESULT(DailyMeanSuspendedSedimentFlux, Input);
		return upstreamflux;
	)
	
	EQUATION(Model, SuspendedSedimentMass,	
		return RESULT(ReachSedimentInput) + RESULT(ReachSedimentInputUpstream) - RESULT(SuspendedSedimentFlux);
	)
	
	EQUATION(Model, SuspendedSedimentFlux,
		return 86400.0 * RESULT(ReachFlow) * SafeDivide(RESULT(SuspendedSedimentMass), RESULT(ReachVolume));
	)
	
	EQUATION(Model, DailyMeanSuspendedSedimentFlux,
		return RESULT(SuspendedSedimentFlux);
	)
	
	EQUATION(Model, SuspendedSedimentConcentration,
		//Converting flow m3/s->m3/day, and converting kg/m3 to mg/l. TODO: make conversion functions
		return 1e3 * SafeDivide(RESULT(DailyMeanSuspendedSedimentFlux), 86400.0 * RESULT(DailyMeanReachFlow));
	)
}

