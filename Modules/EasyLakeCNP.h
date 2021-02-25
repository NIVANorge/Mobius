




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

	auto EasyLakeCNP = RegisterParameterGroup(Model, "Lake Carbon Nitrogen Phosphorous");
	
	auto TDPSettlingVelocity = RegisterParameterDouble(Model, EasyLakeCNP, "TDP settling velocity", MPerDay, 0.5, 0.0, 10.0);
	auto SSSettlingVelocity  = RegisterParameterDouble(Model, EasyLakeCNP, "Suspended solid settling velocity", MPerDay, 0.5, 0.0, 10.0);
	auto DINSettlingVelocity = RegisterParameterDouble(Model, EasyLakeCNP, "DIN settling velocity", MPerDay, 0.5, 0.0, 10.0);
	auto DOCSettlingVelocity = RegisterParameterDouble(Model, EasyLakeCNP, "DOC settling velocity", MPerDay, 0.5, 0.0, 10.0);
	
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
	
	auto EpilimnionDINMass           = RegisterEquationODE(Model, "Epilimnion DIN mass", Kg, LakeSolver);
	auto EpilimnionDINConcentration  = RegisterEquation(Model, "Epilimnion DIN concentration", MgPerL, LakeSolver);
	auto LakeDINFlux                 = RegisterEquation(Model, "Lake DIN flux", MgPerL, LakeSolver);
	auto DailyMeanLakeDINFlux        = RegisterEquationODE(Model, "Daily mean lake DIN flux", KgPerDay, LakeSolver);
	ResetEveryTimestep(Model, DailyMeanLakeDINFlux);
	auto EpilimnionHypolimnionDINFlux= RegisterEquation(Model, "Epilimnion-hypolimnion DIN flux", KgPerDay, LakeSolver);
	auto HypolimnionDINMass          = RegisterEquationODE(Model, "Hypolimnion DIN mass", Kg, LakeSolver);
	auto HypolimnionDINConcentration = RegisterEquation(Model, "Hypolimnion DIN concentration", MgPerL, LakeSolver);
	auto HypolimnionDINSettling      = RegisterEquation(Model, "Hypolimnion DIN settling", KgPerDay, LakeSolver);
	
	
	auto EpilimnionDOCPhotoMineralization = RegisterEquation(Model, "Epilimnion DOC photomineralization", KgPerDay, LakeSolver);
	auto EpilimnionDOCMass           = RegisterEquationODE(Model, "Epilimnion DOC mass", Kg, LakeSolver);
	auto EpilimnionDOCConcentration  = RegisterEquation(Model, "Epilimnion DOC concentration", MgPerL, LakeSolver);
	auto LakeDOCFlux                 = RegisterEquation(Model, "Lake DOC flux", MgPerL, LakeSolver);
	auto DailyMeanLakeDOCFlux        = RegisterEquationODE(Model, "Daily mean lake DOC flux", KgPerDay, LakeSolver);
	ResetEveryTimestep(Model, DailyMeanLakeDOCFlux);
	auto EpilimnionHypolimnionDOCFlux= RegisterEquation(Model, "Epilimnion-hypolimnion DOC flux", KgPerDay, LakeSolver);
	auto HypolimnionDOCMass          = RegisterEquationODE(Model, "Hypolimnion DOC mass", Kg, LakeSolver);
	auto HypolimnionDOCConcentration = RegisterEquation(Model, "Hypolimnion DOC concentration", MgPerL, LakeSolver);
	auto HypolimnionDOCSettling      = RegisterEquation(Model, "Hypolimnion DOC settling", KgPerDay, LakeSolver);
	
	
	auto ThisIsALake = GetConditionalHandle(Model, "This is a lake");
	SetConditional(Model, EpilimnionTPConcentration, ThisIsALake);
	SetConditional(Model, HypolimnionTPConcentration, ThisIsALake);
	
	
	auto IsLake               = GetParameterBoolHandle(Model, "This section is a lake");
	
	auto DailyMeanReachSSFlux  = GetEquationHandle(Model, "Reach daily mean suspended sediment flux");
	auto SedimentInputFromLand = GetEquationHandle(Model, "Reach sediment input (erosion and entrainment)");
	auto SedimentInputUpstream = GetEquationHandle(Model, "Reach sediment input (upstream flux)");
	
	auto DailyMeanReachTDPFlux = GetEquationHandle(Model, "Reach daily mean TDP flux");
	auto ReachTDPInputFromCatchment = GetEquationHandle(Model, "Reach TDP input from catchment");
	auto TDPInputFromUpstream  = GetEquationHandle(Model, "Reach TDP input from upstream");
	
	auto PPInputFromErosion    = GetEquationHandle(Model, "Reach PP input from erosion and entrainment");
	auto PPInputFromUpstream   = GetEquationHandle(Model, "Reach PP input from upstream");
	auto ReachDailyMeanPPFlux  = GetEquationHandle(Model, "Reach daily mean PP flux");
	
	auto ReachDailyMeanDINFlux = GetEquationHandle(Model, "Daily mean reach DIN flux");
	auto DINInputFromCatchment = GetEquationHandle(Model, "DIN input from catchment");
	auto DINInputFromUpstream  = GetEquationHandle(Model, "DIN input from upstream");
	
	auto ReachDailyMeanDOCFlux = GetEquationHandle(Model, "DOC flux from reach, daily mean");
	auto DOCInputFromCatchment = GetEquationHandle(Model, "DOC input from catchment");
	auto DOCInputFromUpstream  = GetEquationHandle(Model, "DOC input from upstream");
	
	auto LakeVolume            = GetEquationHandle(Model, "Lake volume");
	auto HypolimnionVolume     = GetEquationHandle(Model, "Hypolimnion volume");
	auto EpilimnionVolume      = GetEquationHandle(Model, "Epilimnion volume");
	auto LakeSurfaceArea       = GetEquationHandle(Model, "Lake surface area");
	auto LakeOutflow           = GetEquationHandle(Model, "Lake outflow");
	auto MixingVelocity        = GetEquationHandle(Model, "Mixing velocity");
	auto EpilimnionShortwave   = GetEquationHandle(Model, "Epilimnion incoming shortwave radiation");
	
	
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
	
	EQUATION_OVERRIDE(Model, DINInputFromUpstream,
		double upstreamflux = 0.0;
		
		for(index_t Input : BRANCH_INPUTS(Reach))
		{
			if(PARAMETER(IsLake, Input))
				upstreamflux += RESULT(DailyMeanLakeDINFlux, Input);
			else
				upstreamflux += RESULT(ReachDailyMeanDINFlux, Input);
		}
		
		return  upstreamflux;
	)
	
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
	
	
	
	EQUATION(Model, EpilimnionDINMass,
		return
			  RESULT(DINInputFromCatchment)   // + effluents?
			+ RESULT(DINInputFromUpstream)
			- RESULT(LakeDINFlux)
			- RESULT(EpilimnionHypolimnionDINFlux);
	)
	
	EQUATION(Model, EpilimnionDINConcentration,
		return SafeDivide(RESULT(EpilimnionDINMass), RESULT(EpilimnionVolume)) * 1000.0;   // kg/m3 -> mg/l
	)
	
	EQUATION(Model, LakeDINFlux,
		return RESULT(LakeOutflow) * RESULT(EpilimnionDINConcentration) * 86.4;
	)
	
	EQUATION(Model, DailyMeanLakeDINFlux,
		return RESULT(LakeDINFlux);
	)
	
	EQUATION(Model, EpilimnionHypolimnionDINFlux,
		double settling = PARAMETER(DINSettlingVelocity) * RESULT(LakeSurfaceArea) * RESULT(EpilimnionDINConcentration)*1e-3;
		double mixing = RESULT(MixingVelocity)*RESULT(LakeSurfaceArea)*(RESULT(EpilimnionDINConcentration) - RESULT(HypolimnionDINConcentration))*1e-3;
		return settling + mixing;
	)
	
	EQUATION(Model, HypolimnionDINMass,
		return
			  RESULT(EpilimnionHypolimnionDINFlux)
			- RESULT(HypolimnionDINSettling);
	)
	
	EQUATION(Model, HypolimnionDINConcentration,
		return SafeDivide(RESULT(HypolimnionDINMass), RESULT(HypolimnionVolume)) * 1000.0;   // kg/m3 -> mg/l
	)
	
	EQUATION(Model, HypolimnionDINSettling,
		return PARAMETER(DINSettlingVelocity) * RESULT(LakeSurfaceArea) * RESULT(HypolimnionDINConcentration) * 1e-3;
	)
	
	
	
	
	EQUATION(Model, EpilimnionDOCPhotoMineralization,
	
		//TODO: Some of these could be parameters
		double oc_DOC = 0.01;   // Optical cross-section of DOM
		double qy_DOC = 0.5;    // mol DOC /mol quanta   Quantum yield
		
		double f_par = 0.45;     //Fract.   of PAR in incoming solar radiation
		double e_par = 240800.0; // J/mol   Average energy of PAR photons
		
		//‒oc_DOC * qy_DOC f_par(1/e_par)*(86400)*Qsw*Attn_epilimnion * [Nitrosamines]" in mg N m-3 d-1
		double shortwave = RESULT(EpilimnionShortwave);
		
		double epilimnionattn = 1.0;  //TODO: This should be computed, and actually depends on the DOC conc. Moreover, the excess should go into the hypolimnion and cause photomineralization there.
		
		return oc_DOC * qy_DOC * (f_par / e_par) * 86400.0 * shortwave * epilimnionattn * RESULT(EpilimnionDOCMass);
	)
	
	EQUATION(Model, EpilimnionDOCMass,
		return
			  RESULT(DOCInputFromCatchment)
			+ RESULT(DOCInputFromUpstream)
			- RESULT(LakeDOCFlux)
			- RESULT(EpilimnionHypolimnionDOCFlux)
			- RESULT(EpilimnionDOCPhotoMineralization);
	)
	
	EQUATION(Model, EpilimnionDOCConcentration,
		return SafeDivide(RESULT(EpilimnionDOCMass), RESULT(EpilimnionVolume)) * 1000.0;   // kg/m3 -> mg/l
	)
	
	EQUATION(Model, LakeDOCFlux,
		return RESULT(LakeOutflow) * RESULT(EpilimnionDOCConcentration) * 86.4;
	)
	
	EQUATION(Model, DailyMeanLakeDOCFlux,
		return RESULT(LakeDOCFlux);
	)
	
	EQUATION(Model, EpilimnionHypolimnionDOCFlux,
		double settling = PARAMETER(DOCSettlingVelocity) * RESULT(LakeSurfaceArea) * RESULT(EpilimnionDOCConcentration)*1e-3;
		double mixing = RESULT(MixingVelocity)*RESULT(LakeSurfaceArea)*(RESULT(EpilimnionDOCConcentration) - RESULT(HypolimnionDOCConcentration))*1e-3;
		return settling + mixing;
	)
	
	EQUATION(Model, HypolimnionDOCMass,
		return
			  RESULT(EpilimnionHypolimnionDOCFlux)
			- RESULT(HypolimnionDOCSettling);
	)
	
	EQUATION(Model, HypolimnionDOCConcentration,
		return SafeDivide(RESULT(HypolimnionDOCMass), RESULT(HypolimnionVolume)) * 1000.0;   // kg/m3 -> mg/l
	)
	
	EQUATION(Model, HypolimnionDOCSettling,
		return PARAMETER(DOCSettlingVelocity) * RESULT(LakeSurfaceArea) * RESULT(HypolimnionDOCConcentration) * 1e-3;
	)
	

	EndModule(Model);
}