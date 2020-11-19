




static void
AddEasyLakeCNPModule(mobius_model *Model)
{
	BeginModule(Model, "Easy-Lake CNP", "_dev");
	
	SetModuleDescription(Model, R""""(
This is a coupling of Easy-Lake to catchment Carbon, Nitrogen, and Phosphorous models. Some simple in-lake processes are added.

Currently in early development.
)"""");
	
	auto Kg      = RegisterUnit(Model, "kg");
	auto MPerDay = RegisterUnit(Model, "m/day");
	auto MgPerL  = RegisterUnit(Model, "mg/l");
	auto KgPerDay = RegisterUnit(Model, "kg/day");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");

	auto EasyLakeCNP = RegisterParameterGroup(Model, "Lake Phosphorous");
	
	auto TDPSettlingVelocity = RegisterParameterDouble(Model, EasyLakeCNP, "TDP settling velocity", MPerDay, 0.5, 0.0, 10.0);
	auto SSSettlingVelocity  = RegisterParameterDouble(Model, EasyLakeCNP, "Suspended solid settling velocity", MPerDay, 0.5, 0.0, 10.0);
	
	auto LakeSolver = GetSolverHandle(Model, "Lake solver");


	auto EpilimnionSSMass            = RegisterEquationODE(Model, "Epilimnion SS mass", Kg, LakeSolver);
	auto EpilimnionSSConcentration   = RegisterEquation(Model, "Epilimnion SS concentration", MgPerL, LakeSolver);
	auto LakeSSFlux                  = RegisterEquation(Model, "Lake SS flux", KgPerDay, LakeSolver);
	auto DailyMeanLakeSSFlux         = RegisterEquationODE(Model, "Lake daily mean SS flux", KgPerDay, LakeSolver);
	ResetEveryTimestep(Model, DailyMeanLakeSSFlux);
	auto EpilimnionHypolimnionSSFlux = RegisterEquation(Model, "Epilimnion-hypolimnion SS flux", KgPerDay, LakeSolver);
	auto HypolimnionSSMass           = RegisterEquationODE(Model, "Hypolimnion SS mass", Kg, LakeSolver);
	auto HypolimnionSSConcentration  = RegisterEquation(Model, "Hypolimnion SS concentration", MgPerL, LakeSolver);
	auto HypolimnionSSSettling       = RegisterEquation(Model, "Hypolimnion SS settling", KgPerDay, LakeSolver);

	auto EpilimnionTDPMass           = RegisterEquationODE(Model, "Epilimnion TDP mass", Kg, LakeSolver);
	auto EpilimnionTDPConcentration  = RegisterEquation(Model, "Epilimnion TDP concentration", MgPerL, LakeSolver);
	auto LakeTDPFlux                 = RegisterEquation(Model, "Lake TDP flux", KgPerDay, LakeSolver);
	auto DailyMeanLakeTDPFlux        = RegisterEquationODE(Model, "Lake daily mean TDP flux", KgPerDay, LakeSolver);
	ResetEveryTimestep(Model, DailyMeanLakeTDPFlux);
	auto EpilimnionHypolimnionTDPFlux = RegisterEquation(Model, "Epilimnion-hypolimnion TDP flux", KgPerDay, LakeSolver);
	auto HypolimnionTDPMass          = RegisterEquationODE(Model, "Hypolimnion TDP mass", Kg, LakeSolver);
	auto HypolimnionTDPConcentration = RegisterEquation(Model, "Hypolimnion TDP concentration", MgPerL, LakeSolver);
	auto HypolimnionTDPSettling      = RegisterEquation(Model, "Hypolimnion TDP settling", KgPerDay, LakeSolver);
	
	auto EpilimnionPPMass            = RegisterEquationODE(Model, "Epilimnion PP mass", Kg, LakeSolver);
	auto EpilimnionPPConcentration   = RegisterEquation(Model, "Epilimnion PP concentration", MgPerL, LakeSolver);
	auto LakePPFlux                  = RegisterEquation(Model, "Lake PP flux", MgPerL, LakeSolver);
	auto DailyMeanLakePPFlux         = RegisterEquationODE(Model, "Daily mean lake PP flux", KgPerDay, LakeSolver);
	ResetEveryTimestep(Model, DailyMeanLakePPFlux);
	auto EpilimnionHypolimnionPPFlux = RegisterEquation(Model, "Epilimnion-hypolimnion PP flux", KgPerDay, LakeSolver);
	auto HypolimnionPPMass           = RegisterEquationODE(Model, "Hypolimnion PP mass", Kg, LakeSolver);
	auto HypolimnionPPConcentration  = RegisterEquation(Model, "Hypolimnion PP concentration", MgPerL, LakeSolver);
	auto HypolimnionPPSettling       = RegisterEquation(Model, "Hypolimnion PP settling", KgPerDay, LakeSolver);
	
	auto EpilimnionTPConcentration   = RegisterEquation(Model, "Epilimnion TP concentration", MgPerL);
	auto HypolimnionTPConcentration  = RegisterEquation(Model, "Hypolimnion TP concentration", MgPerL);
	
	auto ThisIsALake = GetConditionalHandle(Model, "This is a lake");
	SetConditional(Model, EpilimnionTPConcentration, ThisIsALake);
	SetConditional(Model, HypolimnionTPConcentration, ThisIsALake);
	
	
	auto IsLake               = GetParameterBoolHandle(Model, "This section is a lake");
	
	auto DailyMeanReachSSFlux  = GetEquationHandle(Model, "Reach daily mean suspended sediment flux");
	auto SedimentInputFromLand = GetEquationHandle(Model, "Reach sediment input (erosion and entrainment)");
	auto SedimentInputUpstream = GetEquationHandle(Model, "Reach sediment input (upstream flux)");
	auto DailyMeanReachTDPFlux = GetEquationHandle(Model, "Reach daily mean TDP flux");
	auto TDPInputFromUpstream  = GetEquationHandle(Model, "Reach TDP input from upstream");
	auto PPInputFromErosion    = GetEquationHandle(Model, "Reach PP input from erosion and entrainment");
	auto PPInputFromUpstream   = GetEquationHandle(Model, "Reach PP input from upstream");
	auto ReachDailyMeanPPFlux  = GetEquationHandle(Model, "Reach daily mean PP flux");
	
	auto ReachTDPInputFromCatchment = GetEquationHandle(Model, "Reach TDP input from catchment");
	auto LakeVolume            = GetEquationHandle(Model, "Lake volume");
	auto HypolimnionVolume     = GetEquationHandle(Model, "Hypolimnion volume");
	auto EpilimnionVolume      = GetEquationHandle(Model, "Epilimnion volume");
	auto LakeSurfaceArea       = GetEquationHandle(Model, "Lake surface area");
	auto LakeOutflow           = GetEquationHandle(Model, "Lake outflow");
	auto MixingVelocity        = GetEquationHandle(Model, "Mixing velocity");
	
	
	EQUATION_OVERRIDE(Model, TDPInputFromUpstream,
		double upstreamflux = 0.0;
		
		for(index_t Input : BRANCH_INPUTS(Reach))
		{
			if(PARAMETER(IsLake, Input))
				upstreamflux += RESULT(DailyMeanLakePPFlux, Input);
			else
				upstreamflux += RESULT(ReachDailyMeanPPFlux, Input);
		}
		
		return upstreamflux;
	)
	
	EQUATION_OVERRIDE(Model, PPInputFromUpstream,
		double upstreamflux = 0.0;
		
		for(index_t Input : BRANCH_INPUTS(Reach))
		{
			if(PARAMETER(IsLake, Input))
				upstreamflux += RESULT(DailyMeanLakeTDPFlux, Input);
			else
				upstreamflux += RESULT(DailyMeanReachTDPFlux, Input);
		}
		
		return upstreamflux;
	)
	
	EQUATION_OVERRIDE(Model, SedimentInputUpstream,
		double upstreamflux = 0.0;
		
		for(index_t Input : BRANCH_INPUTS(Reach))
		{
			if(PARAMETER(IsLake, Input))
				upstreamflux += RESULT(DailyMeanLakeSSFlux, Input);
			else
				upstreamflux += RESULT(DailyMeanReachSSFlux, Input);
		}
		
		return upstreamflux;
	)
	
	EQUATION(Model, EpilimnionSSMass,
		return
			  RESULT(SedimentInputFromLand)
			+ RESULT(SedimentInputUpstream)
			- RESULT(LakeSSFlux)
			- RESULT(EpilimnionHypolimnionSSFlux);
	)
	
	EQUATION(Model, EpilimnionSSConcentration,
		return SafeDivide(RESULT(EpilimnionSSMass), RESULT(EpilimnionVolume)) * 1000.0;   // kg/m3 -> mg/l
	)
	
	EQUATION(Model, LakeSSFlux,
		return RESULT(LakeOutflow) * RESULT(EpilimnionSSConcentration) * 86.4;
	)
	
	EQUATION(Model, DailyMeanLakeSSFlux,
		return RESULT(LakeSSFlux);
	)
	
	EQUATION(Model, EpilimnionHypolimnionSSFlux,
		double settling = PARAMETER(SSSettlingVelocity) * RESULT(LakeSurfaceArea) * RESULT(EpilimnionSSConcentration)*1e-3;
		double mixing = RESULT(MixingVelocity)*RESULT(LakeSurfaceArea)*(RESULT(EpilimnionSSConcentration) - RESULT(HypolimnionSSConcentration))*1e-3;
		
		return settling + mixing;
	)
	
	EQUATION(Model, HypolimnionSSMass,
		return
			  RESULT(EpilimnionHypolimnionSSFlux)
			- RESULT(HypolimnionSSSettling);
	)
	
	EQUATION(Model, HypolimnionSSConcentration,
		return SafeDivide(RESULT(HypolimnionSSMass), RESULT(HypolimnionVolume)) * 1000.0;   // kg/m3 -> mg/l
	)
	
	EQUATION(Model, HypolimnionSSSettling,
		return PARAMETER(SSSettlingVelocity) * RESULT(LakeSurfaceArea) * RESULT(HypolimnionSSConcentration) * 1e-3;
	)
	
	
	
	EQUATION(Model, EpilimnionTDPMass,
		return
			  RESULT(ReachTDPInputFromCatchment)
			+ RESULT(TDPInputFromUpstream)
			- RESULT(LakeTDPFlux)
			- RESULT(EpilimnionHypolimnionTDPFlux);
	)
	
	EQUATION(Model, EpilimnionTDPConcentration,
		return SafeDivide(RESULT(EpilimnionTDPMass), RESULT(EpilimnionVolume)) * 1000.0;   // kg/m3 -> mg/l
	)
	
	EQUATION(Model, LakeTDPFlux,
		return RESULT(LakeOutflow) * RESULT(EpilimnionTDPConcentration) * 86.4;
	)
	
	EQUATION(Model, DailyMeanLakeTDPFlux,
		return RESULT(LakeTDPFlux);
	)
	
	EQUATION(Model, EpilimnionHypolimnionTDPFlux,
		double settling = PARAMETER(TDPSettlingVelocity) * RESULT(LakeSurfaceArea) * RESULT(EpilimnionTDPConcentration)*1e-3;
		double mixing = RESULT(MixingVelocity)*RESULT(LakeSurfaceArea)*(RESULT(EpilimnionTDPConcentration) - RESULT(HypolimnionTDPConcentration))*1e-3;
		
		return settling + mixing;
	)
	
	EQUATION(Model, HypolimnionTDPMass,
		return
			  RESULT(EpilimnionHypolimnionTDPFlux)
			- RESULT(HypolimnionTDPSettling);
	)
	
	EQUATION(Model, HypolimnionTDPConcentration,
		return SafeDivide(RESULT(HypolimnionTDPMass), RESULT(HypolimnionVolume)) * 1000.0;   // kg/m3 -> mg/l
	)
	
	EQUATION(Model, HypolimnionTDPSettling,
		return PARAMETER(TDPSettlingVelocity) * RESULT(LakeSurfaceArea) * RESULT(HypolimnionTDPConcentration) * 1e-3;
	)
	
	
	
	EQUATION(Model, EpilimnionPPMass,
		return
			  RESULT(PPInputFromErosion)
			+ RESULT(PPInputFromUpstream)
			- RESULT(LakePPFlux)
			- RESULT(EpilimnionHypolimnionPPFlux);
	)
	
	EQUATION(Model, EpilimnionPPConcentration,
		return SafeDivide(RESULT(EpilimnionPPMass), RESULT(EpilimnionVolume)) * 1000.0;   // kg/m3 -> mg/l
	)
	
	EQUATION(Model, LakePPFlux,
		return RESULT(LakeOutflow) * RESULT(EpilimnionPPConcentration) * 86.4;
	)
	
	EQUATION(Model, DailyMeanLakePPFlux,
		return RESULT(LakePPFlux);
	)
	
	EQUATION(Model, EpilimnionHypolimnionPPFlux,
		double settling = PARAMETER(SSSettlingVelocity) * RESULT(LakeSurfaceArea) * RESULT(EpilimnionPPConcentration)*1e-3;
		double mixing = RESULT(MixingVelocity)*RESULT(LakeSurfaceArea)*(RESULT(EpilimnionPPConcentration) - RESULT(HypolimnionPPConcentration))*1e-3;
		return settling + mixing;
	)
	
	EQUATION(Model, HypolimnionPPMass,
		return
			  RESULT(EpilimnionHypolimnionPPFlux)
			- RESULT(HypolimnionPPSettling);
	)
	
	EQUATION(Model, HypolimnionPPConcentration,
		return SafeDivide(RESULT(HypolimnionPPMass), RESULT(HypolimnionVolume)) * 1000.0;   // kg/m3 -> mg/l
	)
	
	EQUATION(Model, HypolimnionPPSettling,
		return PARAMETER(SSSettlingVelocity) * RESULT(LakeSurfaceArea) * RESULT(HypolimnionPPConcentration) * 1e-3;
	)
	
	EQUATION(Model, EpilimnionTPConcentration,
		return RESULT(EpilimnionPPConcentration) + RESULT(EpilimnionTDPConcentration);
	)
	
	EQUATION(Model, HypolimnionTPConcentration,
		return RESULT(HypolimnionPPConcentration) + RESULT(HypolimnionTDPConcentration);
	)
	

	EndModule(Model);
}