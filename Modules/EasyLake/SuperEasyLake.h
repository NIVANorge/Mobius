

static void
AddSuperEasyLakeModule(mobius_model *Model)
{
	BeginModule(Model, "SuperEasyLake", "0.0");
	
	SetModuleDescription(Model, R""""(
Simplified version of EasyLake.
)"""");
	
	
	auto Dimensionless = RegisterUnit(Model);
	auto M             = RegisterUnit(Model, "m");
	auto M2            = RegisterUnit(Model, "m2");
	auto M3            = RegisterUnit(Model, "m3");
	auto MmPerDay      = RegisterUnit(Model, "mm/day");
	auto MPerS         = RegisterUnit(Model, "m/s");
	auto M3PerS        = RegisterUnit(Model, "m3/s");
	auto M3PerDay      = RegisterUnit(Model, "m3/day");
	auto MPerM         = RegisterUnit(Model, "m/m");
	auto DegreesCelsius = RegisterUnit(Model, "°C");
	

	
	auto Reach      = GetIndexSetHandle(Model, "Reaches");
	auto PhysParams = RegisterParameterGroup(Model, "Lake physical", Reach);
	
	auto IsLake                         = RegisterParameterBool(Model, PhysParams, "This section is a lake", false, "If false this is a river section: ignore the parameters below");
	auto InitialLakeSurfaceArea         = RegisterParameterDouble(Model, PhysParams, "Initial lake surface area", M2, 1e3, 0.0, 371e9);
	auto LakeLength                     = RegisterParameterDouble(Model, PhysParams, "Lake length", M, 300.0, 0.0, 1.03e6);
	auto LakeShoreSlope                 = RegisterParameterDouble(Model, PhysParams, "Lake shore slope", MPerM, 0.2, 0.0, 4.0, "This parameter should be adjusted when calibrating lake outflow. Slope is roughly 2*depth/width");
	auto WaterLevelAtWhichOutflowIsZero = RegisterParameterDouble(Model, PhysParams, "Water level at which outflow is 0", M, 10.0, 0.0, 1642.0);
	auto OutflowRatingCurveMagnitude    = RegisterParameterDouble(Model, PhysParams, "Outflow rating curve magnitude", MPerS, 1.0, 0.01, 100.0);
	
	auto Precipitation    = RegisterInput(Model, "Precipitation", MmPerDay);
	auto AirTemperature   = RegisterInput(Model, "Air temperature", DegreesCelsius);
	
	
	
	auto DailyMeanReachFlow    = GetEquationHandle(Model, "Reach flow (daily mean, cumecs)");
	auto FlowInputFromUpstream = GetEquationHandle(Model, "Flow input from upstream");
	auto FlowInputFromLand     = GetEquationHandle(Model, "Flow input from land");
	
	
	auto LakeSolver = RegisterSolver(Model, "Lake solver", 0.1, IncaDascru);
	
	auto LakeVolume        = RegisterEquationODE(Model, "Lake volume", M3, LakeSolver);
	auto InitialLakeVolume = RegisterEquationInitialValue(Model, "Initial lake volume", M3);
	SetInitialValue(Model, LakeVolume, InitialLakeVolume);
	
	auto LakeSurfaceArea   = RegisterEquation(Model, "Lake surface area", M2, LakeSolver);
	SetInitialValue(Model, LakeSurfaceArea, InitialLakeSurfaceArea);
	
	auto WaterLevel = RegisterEquationODE(Model, "Water level", M, LakeSolver);
	SetInitialValue(Model, WaterLevel, WaterLevelAtWhichOutflowIsZero);
	
	//auto EpilimnionVolume  = RegisterEquation(Model, "Epilimnion volume", M3, LakeSolver);  //NOTE: In their current form, they are just put on the solver to not cause circular references in EasyLakeCNP.
	//auto HypolimnionVolume = RegisterEquation(Model, "Hypolimnion volume", M3, LakeSolver);
	
	auto DVDT        = RegisterEquation(Model, "Change in lake volume", M3PerDay, LakeSolver);
	auto OutletWaterLevel = RegisterEquation(Model, "Outlet water level", M, LakeSolver);
	auto LakeOutflow = RegisterEquation(Model, "Lake outflow", M3PerS, LakeSolver);
	//auto Evaporation = RegisterEquation(Model, "Evaporation", MmPerDay, LakeSolver);       // TODO: Add degree-day or something?
	
	auto DailyMeanLakeOutflow = RegisterEquationODE(Model, "Lake outflow (daily mean)", M3PerS, LakeSolver);
	ResetEveryTimestep(Model, DailyMeanLakeOutflow);
	
	
	auto LakeLengthComputation = RegisterEquationInitialValue(Model, "Lake length computation", M);
	ParameterIsComputedBy(Model, LakeLength, LakeLengthComputation, true);
	
	
	
	auto ThisIsALake  = RegisterConditionalExecution(Model, "This is a lake", IsLake, true);
	auto ThisIsARiver = RegisterConditionalExecution(Model, "This is a river", IsLake, false);
	
	SetConditional(Model, LakeSolver, ThisIsALake);
	
	EQUATION_OVERRIDE(Model, FlowInputFromUpstream,
		double upstreamflow = 0.0;

		for(index_t Input : BRANCH_INPUTS(Reach))
		{
			if(PARAMETER(IsLake, Input))
				upstreamflow += RESULT(DailyMeanLakeOutflow, Input);
			else
				upstreamflow += RESULT(DailyMeanReachFlow, Input);
		}
		return upstreamflow;
	)
	
	
	EQUATION(Model, LakeLengthComputation,
		return 0.5*PARAMETER(InitialLakeSurfaceArea)*PARAMETER(LakeShoreSlope)/PARAMETER(WaterLevelAtWhichOutflowIsZero);
	)
	
	
	EQUATION(Model, InitialLakeVolume,
		return 0.5 * PARAMETER(WaterLevelAtWhichOutflowIsZero) * PARAMETER(InitialLakeSurfaceArea);
	)
	
	EQUATION(Model, DVDT,
		double inflow = RESULT(FlowInputFromUpstream) + RESULT(FlowInputFromLand);
		return (inflow - RESULT(LakeOutflow)) * 86400.0 + 1e-3 * (INPUT(Precipitation) /*- RESULT(Evaporation)*/) * RESULT(LakeSurfaceArea);
	)
	
	EQUATION(Model, LakeVolume,
		return RESULT(DVDT);
	)
	
	EQUATION(Model, LakeSurfaceArea,
		return 2.0 * RESULT(WaterLevel)*PARAMETER(LakeLength)/PARAMETER(LakeShoreSlope);
	)
	
	EQUATION(Model, WaterLevel,
		return 0.5 * (PARAMETER(LakeShoreSlope) / (PARAMETER(LakeLength) * RESULT(WaterLevel))) * RESULT(DVDT);
	)
	
	EQUATION(Model, OutletWaterLevel,
		return Max(0.0, RESULT(WaterLevel) - PARAMETER(WaterLevelAtWhichOutflowIsZero));
	)
	
	EQUATION(Model, LakeOutflow,
		double outletlevel = RESULT(OutletWaterLevel);
		return PARAMETER(OutflowRatingCurveMagnitude)*outletlevel*outletlevel;
	)
	
	EQUATION(Model, DailyMeanLakeOutflow,
		return RESULT(LakeOutflow);
	)
	
	
	EndModule(Model);
}




static void
AddSuperEasyLakeCModule(mobius_model *Model)
{
	BeginModule(Model, "SuperEasyLakeC", "0.0");
	
	SetModuleDescription(Model, R""""(
Simple DOC processing for SuperEasyLake.
)"""");
	
	auto Kg         = RegisterUnit(Model, "kg");
	auto KgPerDay   = RegisterUnit(Model, "kg/day");
	auto MgPerL     = RegisterUnit(Model, "mg/l");
	auto Days       = RegisterUnit(Model, "days");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	
	auto LakeC  = RegisterParameterGroup(Model, "Lake Carbon");
	
	auto LakeDOCHl = RegisterParameterDouble(Model, LakeC, "Lake DOC half life", Days, 50.0, 1.0, 10000.0);
	
	auto LakeSolver = GetSolverHandle(Model, "Lake solver");
	
	auto LakeDOCMass          = RegisterEquationODE(Model, "Lake DOC mass", Kg, LakeSolver);
	auto LakeDOCConcentration = RegisterEquation(Model, "Lake DOC concentration", MgPerL, LakeSolver);
	auto LakeDOCLoss          = RegisterEquation(Model, "Lake DOC loss", KgPerDay, LakeSolver);
	auto LakeDOCFlux          = RegisterEquation(Model, "Lake DOC flux", KgPerDay, LakeSolver);
	auto DailyMeanLakeDOCFlux = RegisterEquationODE(Model, "Daily mean lake DOC flux", KgPerDay, LakeSolver);
	ResetEveryTimestep(Model, DailyMeanLakeDOCFlux);
	
	
	auto ReachDailyMeanDOCFlux = GetEquationHandle(Model, "DOC flux from reach, daily mean");
	auto DOCInputFromCatchment = GetEquationHandle(Model, "DOC input from catchment");
	auto DOCInputFromUpstream  = GetEquationHandle(Model, "DOC input from upstream");
	
	auto LakeOutflow           = GetEquationHandle(Model, "Lake outflow");
	auto LakeVolume            = GetEquationHandle(Model, "Lake volume");
	
	auto IsLake                = GetParameterBoolHandle(Model, "This section is a lake");
	
	
	
	EQUATION_OVERRIDE(Model, DOCInputFromUpstream,
		double upstreamflux = 0.0;
		
		for(index_t Input : BRANCH_INPUTS(Reach))
		{
			if(PARAMETER(IsLake, Input))
				upstreamflux += RESULT(DailyMeanLakeDOCFlux, Input);
			else
				upstreamflux += RESULT(ReachDailyMeanDOCFlux, Input);
		}
		
		return upstreamflux;
	)
	
	EQUATION(Model, LakeDOCMass,
		return
			  RESULT(DOCInputFromCatchment)
			+ RESULT(DOCInputFromUpstream)
			- RESULT(LakeDOCLoss)
			- RESULT(LakeDOCFlux);
	)
	
	EQUATION(Model, LakeDOCConcentration,
		return SafeDivide(RESULT(LakeDOCMass), RESULT(LakeVolume)) * 1000.0;
	)
	
	EQUATION(Model, LakeDOCFlux,
		return RESULT(LakeOutflow) * RESULT(LakeDOCConcentration) * 86.4;
	)
	
	EQUATION(Model, DailyMeanLakeDOCFlux,
		return RESULT(LakeDOCFlux);
	)
	
	EQUATION(Model, LakeDOCLoss,
		return RESULT(LakeDOCMass) * HalfLifeToRate(PARAMETER(LakeDOCHl));
	)
	
	EndModule(Model);
}





