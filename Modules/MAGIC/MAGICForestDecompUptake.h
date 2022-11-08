





static void
AddMAGICForestDecompUptakeModel(mobius_model *Model)
{
	BeginModule(Model, "MAGIC Forest decomposition and uptake", "0.0.3");
	
	auto Dimensionless   = RegisterUnit(Model);
	auto M3PerHa         = RegisterUnit(Model, "m3/Ha");
	auto TPerHa          = RegisterUnit(Model, "tonnes/Ha");
	auto TPerM3          = RegisterUnit(Model, "tonnes/m3");
	auto Years           = RegisterUnit(Model, "years");
	auto PerYear         = RegisterUnit(Model, "1/year");
	auto M3PerHaPerTs    = RegisterUnit(Model, "m3/Ha/ts");
	auto TPerHaPerTs     = RegisterUnit(Model, "tonnes/Ha/ts");
	//auto TPerHaPerYear   = RegisterUnit(Model, "tonnes/Ha/year");
	auto MMolPerKg       = RegisterUnit(Model, "mmol/kg");
	auto MEqPerKg        = RegisterUnit(Model, "meq/kg");
	auto MMolPerM2PerTs  = RegisterUnit(Model, "mmol/m2/ts");
	auto MEqPerM2PerTs   = RegisterUnit(Model, "meq/m2/ts");
	auto KgPerM2PerTs    = RegisterUnit(Model, "kg/m2/ts");
	
	
	auto ForestPatch     = RegisterIndexSet(Model, "Forest patch");
	auto TreeClass       = RegisterIndexSet(Model, "Tree species");
	auto TreeCompartment = RegisterIndexSet(Model, "Tree compartment");
	
	
	
	auto TreeGrowth           = RegisterInput(Model, "Forest growth", M3PerHaPerTs, true);
	//auto TreeTurnover         = RegisterInput(Model, "Forest turnover", TPerHaPerTs, true);
	auto TreeDisturbance      = RegisterInput(Model, "Forest disturbance", Dimensionless);
	auto LeftAfterDisturbance = RegisterInput(Model, "Biomass left on ground after disturbance", Dimensionless);
	//auto TreeAgeInterp        = RegisterInput(Model, "Compartment share interpolation variable", Dimensionless);  //Should go between 1 and 100
	auto StandAge             = RegisterInput(Model, "Stand age", Years);
	
	
	
	auto PatchPars            = RegisterParameterGroup(Model, "Forest patches", ForestPatch);
	
	auto PatchArea            = RegisterParameterDouble(Model, PatchPars, "Patch relative area", Dimensionless, 1.0, 0.0, 1.0);
	auto CarryingCapacity     = RegisterParameterDouble(Model, PatchPars, "Patch carrying capacity", M3PerHa, 0.0, 0.0, 10000.0);
	auto VolumeAtFullCover    = RegisterParameterDouble(Model, PatchPars, "Forest volume at full forest cover", M3PerHa, 0.0, 0.0, 10000.0, "Forest volume when dry deposition reaches its maximum");
	
	auto InitialForestPars    = RegisterParameterGroup(Model, "Initial tree volume", ForestPatch, TreeClass);
	
	auto InitialTreeVolume    = RegisterParameterDouble(Model, InitialForestPars, "Initial live forest volume", M3PerHa, 0.0, 0.0, 10000.0);
	
	
	auto TreeGrowthPars       = RegisterParameterGroup(Model, "Tree growth", TreeClass);
	
	auto MaxGrowthRate        = RegisterParameterDouble(Model, TreeGrowthPars, "Un-restricted tree growth rate at 20°C", PerYear, 0.0, 0.0, 1000.0);
	auto GrowthQ10            = RegisterParameterDouble(Model, TreeGrowthPars, "Growth rate Q10", Dimensionless, 1.0, 1.0, 5.0);
	
	
	auto TreeDecomp           = RegisterParameterGroup(Model, "Tree (de)composition", TreeClass, TreeCompartment);
	
	auto BefYoung             = RegisterParameterDouble(Model, TreeDecomp, "Biomass expansion factor in old trees", TPerM3, 0.0, 0.0, 1.0, "The a in bef = a + b*exp(-stand_age/100)");
	auto BefAgeDep            = RegisterParameterDouble(Model, TreeDecomp, "Biomass expansion factor age dependence", TPerM3, 0.0, -1.0, 1.0, "The b in bef = a + b*exp(-stand_age/100)");
	auto TurnoverRate         = RegisterParameterDouble(Model, TreeDecomp, "Tree compartment turnover rate", PerYear, 0.0, 0.0, 1.0);
	auto TreeDecompRate       = RegisterParameterDouble(Model, TreeDecomp, "Tree decomposition rate", PerYear, 0.1, 0.0, 1.0); // at 20°C
	//auto DecompQ10            = RegisterParameterDouble(Model, TreeDecomp, "Tree decomposition Q10", Dimensionless, 1.0, 1.0, 5.0);
	auto TreeNConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree N concentration",  MMolPerKg, 0.0, 0.0, 100000.0, "mmol N per kg of tree biomass");
	auto TreePConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree P concentration",  MMolPerKg, 0.0, 0.0, 100000.0, "mmol P per kg of tree biomass");
	auto TreeCaConc           = RegisterParameterDouble(Model, TreeDecomp, "Tree Ca concentration", MEqPerKg, 0.0, 0.0, 100000.0, "meq Ca per kg of tree biomass");
	auto TreeMgConc           = RegisterParameterDouble(Model, TreeDecomp, "Tree Mg concentration", MEqPerKg, 0.0, 0.0, 100000.0, "meq Mg per kg of tree biomass");
	auto TreeNaConc           = RegisterParameterDouble(Model, TreeDecomp, "Tree Na concentration", MEqPerKg, 0.0, 0.0, 100000.0, "meq Na per kg of tree biomass");
	auto TreeKConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree K concentration",  MEqPerKg, 0.0, 0.0, 100000.0, "meq K per kg of tree biomass");
	
	
	
	auto ForestNetGrowth      = RegisterEquation(Model, "Tree net growth", M3PerHaPerTs);
	auto LiveTreeVolume       = RegisterEquation(Model, "Live tree volume per species", M3PerHa);
	auto TotalLiveTreeVolume  = RegisterEquationCumulative(Model, "Total live tree volume per patch", LiveTreeVolume, TreeClass);
	auto ForestCoverPatch     = RegisterEquation(Model, "Forest cover per patch", Dimensionless);
	auto ForestCoverAvg       = RegisterEquationCumulative(Model, "Forest cover averaged over patches", ForestCoverPatch, ForestPatch, PatchArea);

	auto CompartmentTurnover  = RegisterEquation(Model, "Compartment turnover", TPerHaPerTs);
	SetInitialValue(Model, CompartmentTurnover, CompartmentTurnover);
	
	auto CompartmentTurnoverSummed = RegisterEquationCumulative(Model, "Compartment turnover averaged over patches", CompartmentTurnover, ForestPatch, PatchArea);
	SetInitialValue(Model, CompartmentTurnoverSummed, CompartmentTurnoverSummed);
	
	SetInitialValue(Model, LiveTreeVolume, InitialTreeVolume);
	
	//auto CompartmentShare     = RegisterEquation(Model, "Compartment share", Dimensionless);
	auto CompartmentBef       = RegisterEquation(Model, "Compartment biomass expansion factor", TPerM3);
	auto CompartmentBiomass   = RegisterEquation(Model, "Compartment biomass", TPerHa);
	auto CompartmentGrowth    = RegisterEquation(Model, "Compartment growth", TPerHaPerTs);
	
	//SetInitialValue(Model, CompartmentBef, CompartmentBef);
	auto InitialCompartmentBiomass = RegisterEquationInitialValue(Model, "Initial compartment biomass", TPerHa);
	SetInitialValue(Model, CompartmentBiomass, InitialCompartmentBiomass);
	
	auto CompartmentGrowthSummed = RegisterEquationCumulative(Model, "Compartment growth averaged over patches", CompartmentGrowth, ForestPatch, PatchArea);
	
	auto TreeNUptake          = RegisterEquation(Model, "Tree N uptake", MMolPerM2PerTs);
	auto TreePUptake          = RegisterEquation(Model, "Tree P uptake", MMolPerM2PerTs);
	auto TreeCaUptake         = RegisterEquation(Model, "Tree Ca uptake", MEqPerM2PerTs);
	auto TreeMgUptake         = RegisterEquation(Model, "Tree Mg uptake", MEqPerM2PerTs);
	auto TreeNaUptake         = RegisterEquation(Model, "Tree Na uptake", MEqPerM2PerTs);
	auto TreeKUptake          = RegisterEquation(Model, "Tree K uptake", MEqPerM2PerTs);
	
	auto TotalTreeNUptakeSpecies  = RegisterEquationCumulative(Model, "Total tree N uptake per species", TreeNUptake, TreeCompartment);
	auto TotalTreePUptakeSpecies  = RegisterEquationCumulative(Model, "Total tree P uptake per species", TreePUptake, TreeCompartment);
	auto TotalTreeCaUptakeSpecies = RegisterEquationCumulative(Model, "Total tree Ca uptake per species", TreeCaUptake, TreeCompartment);
	auto TotalTreeMgUptakeSpecies = RegisterEquationCumulative(Model, "Total tree Mg uptake per species", TreeMgUptake, TreeCompartment);
	auto TotalTreeNaUptakeSpecies = RegisterEquationCumulative(Model, "Total tree Na uptake per species", TreeNaUptake, TreeCompartment);
	auto TotalTreeKUptakeSpecies  = RegisterEquationCumulative(Model, "Total tree K uptake per species", TreeKUptake, TreeCompartment);
	
	auto TotalTreeNUptake     = RegisterEquationCumulative(Model, "Total tree N uptake", TotalTreeNUptakeSpecies, TreeClass);
	auto TotalTreePUptake     = RegisterEquationCumulative(Model, "Total tree P uptake", TotalTreePUptakeSpecies, TreeClass);
	auto TotalTreeCaUptake    = RegisterEquationCumulative(Model, "Total tree Ca uptake", TotalTreeCaUptakeSpecies, TreeClass);
	auto TotalTreeMgUptake    = RegisterEquationCumulative(Model, "Total tree Mg uptake", TotalTreeMgUptakeSpecies, TreeClass);
	auto TotalTreeNaUptake    = RegisterEquationCumulative(Model, "Total tree Na uptake", TotalTreeNaUptakeSpecies, TreeClass);
	auto TotalTreeKUptake     = RegisterEquationCumulative(Model, "Total tree K uptake", TotalTreeKUptakeSpecies, TreeClass);

	auto LeftOnGroundByDisturbance = RegisterEquation(Model, "Tree mass left on ground by disturbance", TPerHa);
	auto TotalLeftOnGroundByDisturbance = RegisterEquationCumulative(Model, "Tree mass left on ground by disturbance averaged over forest patches", LeftOnGroundByDisturbance, ForestPatch, PatchArea);
	auto LitterPerCompartment = RegisterEquation(Model, "Litter per compartment", TPerHaPerTs);
	auto DeadTreeMass         = RegisterEquation(Model, "Dead tree biomass", TPerHa);
	auto DeadTreeDecomp       = RegisterEquation(Model, "Dead tree decomposition", TPerHaPerTs);
	
	SetInitialValue(Model, LitterPerCompartment, LitterPerCompartment);
	
	auto InitialDeadTreeMass  = RegisterEquationInitialValue(Model, "Initial dead tree mass", TPerHa);
	SetInitialValue(Model, DeadTreeMass, InitialDeadTreeMass);
	
	auto TreeDecompCSourceMass = RegisterEquation(Model, "C source from tree decomposition (mass)", KgPerM2PerTs);
	auto TreeDecompCSource    = RegisterEquation(Model, "C source from tree decomposition", MMolPerM2PerTs);
	auto TreeDecompNSource    = RegisterEquation(Model, "N source from tree decomposition", MMolPerM2PerTs);
	auto TreeDecompPSource    = RegisterEquation(Model, "P source from tree decomposition", MMolPerM2PerTs);
	auto TreeDecompCaSource   = RegisterEquation(Model, "Ca source from tree decomposition", MEqPerM2PerTs);
	auto TreeDecompMgSource   = RegisterEquation(Model, "Mg source from tree decomposition", MEqPerM2PerTs);
	auto TreeDecompNaSource   = RegisterEquation(Model, "Na source from tree decomposition", MEqPerM2PerTs);
	auto TreeDecompKSource    = RegisterEquation(Model, "K source from tree decomposition", MEqPerM2PerTs);
	
	SetInitialValue(Model, TreeDecompCSource, TreeDecompCSource);
	
	auto TotalTreeDecompCSourceSpecies  = RegisterEquationCumulative(Model, "Total C source from tree decomposition per species", TreeDecompCSource, TreeCompartment);
	auto TotalTreeDecompNSourceSpecies  = RegisterEquationCumulative(Model, "Total N source from tree decomposition per species", TreeDecompNSource, TreeCompartment);
	auto TotalTreeDecompPSourceSpecies  = RegisterEquationCumulative(Model, "Total P source from tree decomposition per species", TreeDecompPSource, TreeCompartment);
	auto TotalTreeDecompCaSourceSpecies = RegisterEquationCumulative(Model, "Total Ca source from tree decomposition per species", TreeDecompCaSource, TreeCompartment);
	auto TotalTreeDecompMgSourceSpecies = RegisterEquationCumulative(Model, "Total Mg source from tree decomposition per species", TreeDecompMgSource, TreeCompartment);
	auto TotalTreeDecompNaSourceSpecies = RegisterEquationCumulative(Model, "Total Na source from tree decomposition per species", TreeDecompNaSource, TreeCompartment);
	auto TotalTreeDecompKSourceSpecies  = RegisterEquationCumulative(Model, "Total K source from tree decomposition per species", TreeDecompKSource, TreeCompartment);
	
	auto TotalTreeDecompCSourcePerCompartment = RegisterEquationCumulative(Model, "Total C source from tree decomposition per compartment (mass)", TreeDecompCSourceMass, TreeClass);
	auto TotalTreeDecompCSource  = RegisterEquationCumulative(Model, "Total C source from tree decomposition", TotalTreeDecompCSourceSpecies, TreeClass);
	auto TotalTreeDecompNSource  = RegisterEquationCumulative(Model, "Total N source from tree decomposition", TotalTreeDecompNSourceSpecies, TreeClass);
	auto TotalTreeDecompPSource  = RegisterEquationCumulative(Model, "Total P source from tree decomposition", TotalTreeDecompPSourceSpecies, TreeClass);
	auto TotalTreeDecompCaSource = RegisterEquationCumulative(Model, "Total Ca source from tree decomposition", TotalTreeDecompCaSourceSpecies, TreeClass);
	auto TotalTreeDecompMgSource = RegisterEquationCumulative(Model, "Total Mg source from tree decomposition", TotalTreeDecompMgSourceSpecies, TreeClass);
	auto TotalTreeDecompNaSource = RegisterEquationCumulative(Model, "Total Na source from tree decomposition", TotalTreeDecompNaSourceSpecies, TreeClass);
	auto TotalTreeDecompKSource  = RegisterEquationCumulative(Model, "Total K source from tree decomposition", TotalTreeDecompKSourceSpecies, TreeClass);
	
#ifdef FOREST_STANDALONE
	auto FractionOfYear       = RegisterEquation(Model, "Fraction of year", Dimensionless);
	SetInitialValue(Model, FractionOfYear, FractionOfYear);
	
	EQUATION(Model, FractionOfYear,
		return (double)CURRENT_TIME().StepLengthInSeconds / (86400.0*(double)CURRENT_TIME().DaysThisYear);
	)
#else
	auto FractionOfYear       = GetEquationHandle(Model, "Fraction of year");
#endif

	auto Temperature          = GetEquationHandle(Model, "Temperature");

	EQUATION(Model, ForestNetGrowth,
		double in = INPUT(TreeGrowth);
		double r = PARAMETER(MaxGrowthRate)*RESULT(FractionOfYear);
		r = r*std::pow(PARAMETER(GrowthQ10), (RESULT(Temperature) - 20.0)/10.0);
		double K = PARAMETER(CarryingCapacity);
		double V = LAST_RESULT(LiveTreeVolume);
		
		//NOTE: LAST_RESULT(TotalLiveTreeVolume) fails for some obscure reason. Why????
		double Vtot = 0.0;
		index_t Patch = CURRENT_INDEX(ForestPatch);
		for(index_t Species = FIRST_INDEX(TreeClass); Species != INDEX_COUNT(TreeClass); ++Species)
			Vtot += LAST_RESULT(LiveTreeVolume, Species, Patch);
		
		if(std::isfinite(in)) return in;
		
		//double computed = r*V*(1.0 - Vtot/K);
		
		// TODO: This isn't really an exact solution. Reason being that if you have multiple species they would compete during the growth. May not be a problem.
		//double exprt = std::exp(r);
		//double computed = (K*V*exprt) / (K + Vtot*(exprt-1)) - V;
		double expmrt = std::exp(-r);
		double computed = K / (1.0 + ((K -V)/Vtot)*expmrt) - V;
		
		if(!std::isfinite(computed)) computed = 0.0;
		
		return computed;
	)
	
	EQUATION(Model, LiveTreeVolume,
		double before_disturbance = LAST_RESULT(LiveTreeVolume) + RESULT(ForestNetGrowth);
		double after_disturbance = before_disturbance * (1.0 - INPUT(TreeDisturbance));
		return std::max(0.0, after_disturbance);
	)
	
	EQUATION(Model, ForestCoverPatch,
		return std::min(1.0, RESULT(TotalLiveTreeVolume) / PARAMETER(VolumeAtFullCover));
	)
	
	EQUATION(Model, CompartmentBef,
		return PARAMETER(BefYoung) + PARAMETER(BefAgeDep)*std::exp(-INPUT(StandAge)*0.01);
	)
	
	// Ugh, broken initial value system...
	EQUATION(Model, InitialCompartmentBiomass,
		double bef = PARAMETER(BefYoung) + PARAMETER(BefAgeDep)*std::exp(-INPUT(StandAge)*0.01);
		return RESULT(LiveTreeVolume) * bef; 
	)
	
	EQUATION(Model, CompartmentBiomass,
		return RESULT(LiveTreeVolume) * RESULT(CompartmentBef);
	)
	
	EQUATION(Model, CompartmentTurnover,
		//double in = INPUT(TreeTurnover);
	
		double invdt = RESULT(FractionOfYear);
		double r = 1.0 - std::pow(1.0 - PARAMETER(TurnoverRate), invdt);  //NOTE: Turn 1/year to 1/month
		double computed = r * RESULT(CompartmentBiomass); //NOTE: Is based on current-timestep so that trees that are harvested are not counted for turnover. But does that mean that it is counted doubly in compartmentgrowth below?
		
		//if(std::isfinite(in)) return in * share;
		return computed;
	)
	
	EQUATION(Model, CompartmentGrowth,
		return RESULT(ForestNetGrowth) * RESULT(CompartmentBef) + RESULT(CompartmentTurnover);  //Have to add turnover here since that is not taken into account in the net growth.
	)
	
	EQUATION(Model, TreeNUptake,
		return RESULT(CompartmentGrowthSummed) * PARAMETER(TreeNConc) * 0.1;  // Convert   tonne/Ha -> kg/m2
	)
	
	EQUATION(Model, TreePUptake,
		return RESULT(CompartmentGrowthSummed) * PARAMETER(TreePConc) * 0.1;  // Convert   tonne/Ha -> kg/m2
	)
	
	EQUATION(Model, TreeCaUptake,
		return RESULT(CompartmentGrowthSummed) * PARAMETER(TreeCaConc) * 0.1;  // Convert   tonne/Ha -> kg/m2
	)
	
	EQUATION(Model, TreeMgUptake,
		return RESULT(CompartmentGrowthSummed) * PARAMETER(TreeMgConc) * 0.1;  // Convert   tonne/Ha -> kg/m2
	)
	
	EQUATION(Model, TreeNaUptake,
		return RESULT(CompartmentGrowthSummed) * PARAMETER(TreeNaConc) * 0.1;  // Convert   tonne/Ha -> kg/m2
	)
	
	EQUATION(Model, TreeKUptake,
		return RESULT(CompartmentGrowthSummed) * PARAMETER(TreeKConc) * 0.1;  // Convert   tonne/Ha -> kg/m2
	)
	
	EQUATION(Model, DeadTreeDecomp,
		return LAST_RESULT(DeadTreeMass) * (1.0 - std::exp(-PARAMETER(TreeDecompRate)*RESULT(FractionOfYear)));
	)
	
	EQUATION(Model, InitialDeadTreeMass,
		double invdt = RESULT(FractionOfYear);
		double r = 1.0 - std::pow(1.0 - PARAMETER(TurnoverRate), invdt);
		double bef = PARAMETER(BefYoung);  //NOTE: Hmm, should be the full bef, but can't access it
		
		//NOTE: Again, we can't use result of cumulative equations in initial value computations for some reason.
		double volavg = 0.0;
		for(index_t Patch = FIRST_INDEX(ForestPatch); Patch < INDEX_COUNT(ForestPatch); ++Patch)
		{
			double vol = PARAMETER(InitialTreeVolume, Patch, CURRENT_INDEX(TreeClass));
			volavg += PARAMETER(PatchArea, Patch) * vol;
		}
		
		double turnoveravg = volavg * bef * r;
		
		return turnoveravg / (1.0 - std::exp(-PARAMETER(TreeDecompRate)*RESULT(FractionOfYear))); // Steady state:   turnover = decomposition
	)
	
	EQUATION(Model, LeftOnGroundByDisturbance,
		double before_disturbance = LAST_RESULT(LiveTreeVolume) + RESULT(ForestNetGrowth);
		double disturbance  = before_disturbance * INPUT(TreeDisturbance);
		return INPUT(LeftAfterDisturbance) * disturbance * RESULT(CompartmentBef);
	)
	
	EQUATION(Model, LitterPerCompartment,
		return RESULT(TotalLeftOnGroundByDisturbance) + RESULT(CompartmentTurnoverSummed);
	)
	
	EQUATION(Model, DeadTreeMass,
		return LAST_RESULT(DeadTreeMass) * std::exp(-PARAMETER(TreeDecompRate)*RESULT(FractionOfYear)) + RESULT(LitterPerCompartment);
	)
	
	EQUATION(Model, TreeDecompCSourceMass,
		return RESULT(DeadTreeDecomp) * 0.1 * 0.5;   // convert tonne/Ha -> kg/m2, then assume half of biomass is C
	)
	
	EQUATION(Model, TreeDecompCSource,
		constexpr double c_molar_weight = 12.011; // g/mol
		return RESULT(TreeDecompCSourceMass) * 1e6/c_molar_weight;   // kg/m2 * mol/g -> mmol/m2
	)
	
	EQUATION(Model, TreeDecompNSource,      
		return PARAMETER(TreeNConc) * RESULT(DeadTreeDecomp) * 0.1;  // Convert   tonne/Ha -> kg/m2
	)
	
	EQUATION(Model, TreeDecompPSource,      
		return PARAMETER(TreePConc) * RESULT(DeadTreeDecomp) * 0.1;  // Convert   tonne/Ha -> kg/m2
	)
	
	EQUATION(Model, TreeDecompCaSource,      
		return PARAMETER(TreeCaConc) * RESULT(DeadTreeDecomp) * 0.1;  // Convert   tonne/Ha -> kg/m2
	)
	
	EQUATION(Model, TreeDecompMgSource,      
		return PARAMETER(TreeMgConc) * RESULT(DeadTreeDecomp) * 0.1;  // Convert   tonne/Ha -> kg/m2
	)
	
	EQUATION(Model, TreeDecompNaSource,      
		return PARAMETER(TreeNaConc) * RESULT(DeadTreeDecomp) * 0.1;  // Convert   tonne/Ha -> kg/m2
	)
	
	EQUATION(Model, TreeDecompKSource,      
		return PARAMETER(TreeKConc) * RESULT(DeadTreeDecomp) * 0.1;  // Convert   tonne/Ha -> kg/m2
	)
	
	

	
	
	
	
	EndModule(Model);
}