

static void
AddSuperEasyLakeModule(mobius_model *Model)
{
	BeginModule(Model, "SuperEasyLake", "0.1");
	
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
	auto LakeVolume                     = RegisterParameterDouble(Model, PhysParams, "Lake volume", M3, 1e3, 0.0, 371e9);
	auto LakeSurfaceArea                = RegisterParameterDouble(Model, PhysParams, "Lake surface area", M2, 1e3, 0.0, 371e9);
	
	auto Precipitation    = RegisterInput(Model, "Precipitation", MmPerDay);
	auto AirTemperature   = RegisterInput(Model, "Air temperature", DegreesCelsius);
	
	
	
	auto DailyMeanReachFlow    = GetEquationHandle(Model, "Reach flow (daily mean, cumecs)");
	auto FlowInputFromUpstream = GetEquationHandle(Model, "Flow input from upstream");
	auto FlowInputFromLand     = GetEquationHandle(Model, "Flow input from land");
	
	auto LakeSolver = RegisterSolver(Model, "Lake solver", 0.1, IncaDascru);
	
	auto LakeOutflow = RegisterEquation(Model, "Lake outflow", M3PerS, LakeSolver);
	//auto Evaporation = RegisterEquation(Model, "Evaporation", MmPerDay, LakeSolver);       // TODO: Add degree-day or something?
	
	auto DailyMeanLakeOutflow = RegisterEquationODE(Model, "Lake outflow (daily mean)", M3PerS, LakeSolver);
	ResetEveryTimestep(Model, DailyMeanLakeOutflow);
	
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
	
	EQUATION(Model, LakeOutflow,
		// flow in = flow out
		double inflow = RESULT(FlowInputFromUpstream) + RESULT(FlowInputFromLand);
		double precip = 1e-3 * INPUT(Precipitation) * PARAMETER(LakeSurfaceArea) / 86400.0;
		return inflow + precip;
	)
	
	EQUATION(Model, DailyMeanLakeOutflow,
		return RESULT(LakeOutflow);
	)
	
	
	EndModule(Model);
}




static void
AddSuperEasyLakeCModule(mobius_model *Model)
{
	BeginModule(Model, "SuperEasyLakeC", "0.0.1");
	
	SetModuleDescription(Model, R""""(
Simple DOC processing for SuperEasyLake.
)"""");
	
	auto Kg         = RegisterUnit(Model, "kg");
	auto KgPerDay   = RegisterUnit(Model, "kg/day");
	auto MgPerL     = RegisterUnit(Model, "mg/l");
	auto Days       = RegisterUnit(Model, "days");
	auto PerDay     = RegisterUnit(Model, "1/day");
	auto M2PerMJ       = RegisterUnit(Model, "m2/MJ");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	
	auto LakeC  = RegisterParameterGroup(Model, "Lake Carbon");
	
	//auto LakeDOCHl = RegisterParameterDouble(Model, LakeC, "Lake DOC half life", Days, 50.0, 1.0, 10000.0, "", "DOChllake");
	//auto LakeDOCBreak         = RegisterParameterDouble(Model, LakeC, "Lake DOC breakdown in darkness", PerDay, 0.0, 0.0, 1.0, "", "DOCdark");
	auto LakeDOCRad           = RegisterParameterDouble(Model, LakeC, "Lake DOC radiation breakdown", M2PerMJ, 0.0, 0.0, 1.0, "", "DOCrad");
	   
	
	auto LakeSolver = GetSolverHandle(Model, "Lake solver");
	
	auto InitialLakeDOCMass   = RegisterEquationInitialValue(Model, "Initial lake DOC mass", Kg);
	auto LakeDOCMass          = RegisterEquationODE(Model, "Lake DOC mass", Kg, LakeSolver);
	auto LakeDOCConcentration = RegisterEquation(Model, "Lake DOC concentration", MgPerL, LakeSolver);
	auto LakeDOCLoss          = RegisterEquation(Model, "Lake DOC loss", KgPerDay, LakeSolver);
	auto LakeDOCFlux          = RegisterEquation(Model, "Lake DOC flux", KgPerDay, LakeSolver);
	auto DailyMeanLakeDOCFlux = RegisterEquationODE(Model, "Daily mean lake DOC flux", KgPerDay, LakeSolver);
	ResetEveryTimestep(Model, DailyMeanLakeDOCFlux);
	
	auto Shortwave            = GetInputHandle(Model, "Net shortwave radiation");  // TODO: Probably need removal to depend on shortwave for larger lakes.
	
	
	auto ReachDailyMeanDOCFlux = GetEquationHandle(Model, "DOC flux from reach, daily mean");
	auto DOCInputFromCatchment = GetEquationHandle(Model, "DOC input from catchment");
	auto DOCInputFromUpstream  = GetEquationHandle(Model, "DOC input from upstream");
	auto GroundwaterDOCConcentration = GetEquationHandle(Model, "Groundwater DOC concentration");
	
	auto LakeOutflow           = GetEquationHandle(Model, "Lake outflow");
	auto LakeVolume            = GetParameterDoubleHandle(Model, "Lake volume");
	
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
	
	EQUATION(Model, InitialLakeDOCMass,
		//TODO: This is not that good. Should use lake's own steady state in computation...
		return RESULT(GroundwaterDOCConcentration)*PARAMETER(LakeVolume)*1e-3;
	)
	
	EQUATION(Model, LakeDOCMass,
		return
			  RESULT(DOCInputFromCatchment)
			+ RESULT(DOCInputFromUpstream)
			- RESULT(LakeDOCLoss)
			- RESULT(LakeDOCFlux);
	)
	
	EQUATION(Model, LakeDOCConcentration,
		return SafeDivide(RESULT(LakeDOCMass), PARAMETER(LakeVolume)) * 1000.0;
	)
	
	EQUATION(Model, LakeDOCFlux,
		return RESULT(LakeOutflow) * RESULT(LakeDOCConcentration) * 86.4;
	)
	
	EQUATION(Model, DailyMeanLakeDOCFlux,
		return RESULT(LakeDOCFlux);
	)
	
	EQUATION(Model, LakeDOCLoss,
		//return RESULT(LakeDOCMass) * HalfLifeToRate(PARAMETER(LakeDOCHl));
		//double rate = PARAMETER(LakeDOCBreak) + PARAMETER(LakeDOCRad)*INPUT(Shortwave);
		double rate = PARAMETER(LakeDOCRad)*INPUT(Shortwave);
		return RESULT(LakeDOCMass) * rate;
	)
	
	EndModule(Model);
}





