


static void
AddEDNAModel(mobius_model *Model)
{
	auto Ng      = RegisterUnit(Model, "ng");
	auto NgPerTs = RegisterUnit(Model, "ng/ts");
	auto NgPerKgPerTs = RegisterUnit(Model, "ng/kg/ts");
	auto M3      = RegisterUnit(Model, "m3");
	auto M3PerS  = RegisterUnit(Model, "m3/s");
	auto PerM    = RegisterUnit(Model, "1/m");
	auto PerTs   = RegisterUnit(Model, "1/ts");
	auto M       = RegisterUnit(Model, "m");
	auto Kg      = RegisterUnit(Model, "kg");
	auto NgPerL  = RegisterUnit(Model, "ng/l");
	
	auto Reach       = RegisterIndexSetBranched(Model, "Reach");
	auto Fish        = RegisterIndexSet(Model, "Fish species");
	
	
	auto GlobalPars  = RegisterParameterGroup(Model, "Global eDNA parameters");
	
	auto SheddingRate = RegisterParameterDouble(Model, GlobalPars, "eDNA shedding rate per fish biomass", NgPerKgPerTs, 0.0);
	auto DecayRate    = RegisterParameterDouble(Model, GlobalPars, "eDNA decay rate", PerTs, 0.0);
	
	auto ReachPars   = RegisterParameterGroup(Model, "Per-reach parameters", Reach);
	
	auto Dist        = RegisterParameterDouble(Model, ReachPars, "Distance from outlet", M, 0.0);
	
	auto FishPars    = RegisterParameterGroup(Model, "Per-species parameters", Fish);
	
	auto BiomassDistr0 = RegisterParameterDouble(Model, FishPars, "Biomass at outlet", Kg, 0.0);
	auto BiomassDistrA = RegisterParameterDouble(Model, FishPars, "Biomass distribution linear coeff", PerM, 0.0);
	auto BiomassDistrB = RegisterParameterDouble(Model, FishPars, "Biomass distribution exponential coeff", PerM, 0.0);
	
	
	auto ReachVolume = RegisterInput(Model, "Reach volume", M3);
	auto ReachFlow   = RegisterInput(Model, "Reach flow", M3PerS);
	
	
	auto Sol              = RegisterSolver(Model, "eDNA solver", 0.1, IncaDascru);
	
	auto EDNAMass = RegisterEquationODE(Model, "eDNA mass", Ng, Sol);
	auto FishBiomass      = RegisterEquation(Model, "Fish biomass", Kg);
	auto EDNAShedding     = RegisterEquation(Model, "eDNA shedding", NgPerTs);
	auto EDNADecay        = RegisterEquation(Model, "eDNA decay", NgPerTs, Sol);
	auto EDNAConc         = RegisterEquation(Model, "eDNA concentration", NgPerL, Sol);
	auto EDNAFromUpstream = RegisterEquation(Model, "eDNA input from upstream", NgPerTs);
	auto EDNAOut          = RegisterEquation(Model, "eDNA output", NgPerTs, Sol);

	// TODO: could be computed parameter
	EQUATION(Model, FishBiomass,
		return PARAMETER(BiomassDistr0) * std::exp(-PARAMETER(BiomassDistrB)*PARAMETER(Dist)) - PARAMETER(BiomassDistrA)*PARAMETER(Dist);
	)
	
	EQUATION(Model, EDNAShedding,
		return RESULT(FishBiomass) * PARAMETER(SheddingRate);
	)
	
	EQUATION(Model, EDNADecay,
		return RESULT(EDNAMass) * PARAMETER(DecayRate);
	)
	
	EQUATION(Model, EDNAConc,
		return (RESULT(EDNAMass) / INPUT(ReachVolume)) * 1e-3;   // 1e-3 converts 1/m3 to 1/l
	)
	
	// May be overkill to have a branched system here...
	EQUATION(Model, EDNAFromUpstream,
		double upstream = 0.0;
		for(auto In : BRANCH_INPUTS(Reach))
			upstream += RESULT(EDNAOut, In);
		return upstream;
	)
	
	EQUATION(Model, EDNAOut,
		double dt = (double)CURRENT_TIME().StepLengthInSeconds;
		return RESULT(EDNAConc) * 1e3 * INPUT(ReachFlow) * dt;  // 1e3 converts 1/l to 1/m3
	)
	
	EQUATION(Model, EDNAMass,
		return
			  RESULT(EDNAShedding)
			- RESULT(EDNADecay)
			+ RESULT(EDNAFromUpstream)
			- RESULT(EDNAOut);
	)
}





