

static void
AddSimplySedimentModule(mobius_model *Model)
{
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
	
	auto ReachSedimentInputScalingFactor         = RegisterParameterDouble(Model, Sediment, "Reach sediment input scaling factor", KgPerM3, 150.0, 0.0, 1000.0, "Calibrated parameter linking simulated sediment input from land to simulated flow from land");
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
	
	auto ReachSolver = GetSolverHandle(Model, "SimplyQ reach solver");
	
	// Equations already defined
	auto ReachFlow          = GetEquationHandle(Model, "Reach flow (end-of-day)");
	auto ReachFlowInputFromLand = GetEquationHandle(Model, "Reach flow input from land");
	auto DailyMeanReachFlow = GetEquationHandle(Model, "Reach flow (daily mean, cumecs)");
	auto ReachVolume        = GetEquationHandle(Model, "Reach volume");
	
	// Sediment equations
	auto TimeDependentVegetationCoverFactor = RegisterEquation(Model, "Time dependent vegetation cover factor", Dimensionless);
	auto ReachSedimentInputCoefficient         = RegisterEquation(Model, "Sediment input coefficient", KgPerM3);
	auto TotalReachSedimentInputCoefficient    = RegisterEquationCumulative(Model, "Sediment input coefficient summed over land classes", ReachSedimentInputCoefficient, LandscapeUnits);
	
	auto SuspendedSedimentFlux = RegisterEquation(Model, "Reach suspended sediment flux", KgPerDay);
	SetSolver(Model, SuspendedSedimentFlux, ReachSolver);
	
	auto ReachSedimentInput = RegisterEquation(Model, "Reach sediment input (erosion and entrainment)", KgPerDay);
	SetSolver(Model, ReachSedimentInput, ReachSolver);
	
	auto SuspendedSedimentMass = RegisterEquationODE(Model, "Reach suspended sediment mass", Kg);
	SetInitialValue(Model, SuspendedSedimentMass, 0.0);
	SetSolver(Model, SuspendedSedimentMass, ReachSolver);
	
	auto DailyMeanSuspendedSedimentFlux = RegisterEquationODE(Model, "Reach daily mean suspended sediment flux", KgPerDay);
	SetInitialValue(Model, DailyMeanSuspendedSedimentFlux, 0.0);
	SetSolver(Model, DailyMeanSuspendedSedimentFlux, ReachSolver);
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
		//# Reach sed input coefficient per land use class (kg/m3). This was recently changed from kg/mm which had a different rationalization.
		double Esus_i =
			  PARAMETER(ReachSedimentInputScalingFactor) * 1e6 //1e6 just for convenient range in input parameter
			* PARAMETER(ReachSlope)
			* PARAMETER(MeanSlopeOfLand)
			* RESULT(TimeDependentVegetationCoverFactor)
			* (1.0 - PARAMETER(ReductionOfLoadInSediment));
		
		return Esus_i * PARAMETER(LandUseProportions); //NOTE: This is to make them sum up correctly for use later
	)	
	
	EQUATION(Model, ReachSedimentInput,
		// This does not take into account the greater sediment mobilisation capacity of reaches which are further downstream in the catchment, and may need revisiting. May need to split sediment delivery into two (terrestrial versus instream), which would likely require two of these equations which we want to avoid as long as possible.
		//Note: if this changes, also needs to change in the particulate P equations
		double A_catch = PARAMETER(CatchmentArea);
		double flowfromland = RESULT(ReachFlowInputFromLand);
		return RESULT(TotalReachSedimentInputCoefficient) * A_catch * pow(flowfromland/A_catch, PARAMETER(SedimentInputNonlinearCoefficient));
	)
	
	EQUATION(Model, SuspendedSedimentMass,	
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(DailyMeanSuspendedSedimentFlux, *Input);
		)
		
		return RESULT(ReachSedimentInput) + upstreamflux - RESULT(SuspendedSedimentFlux);
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