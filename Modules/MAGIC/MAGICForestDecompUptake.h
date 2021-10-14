





static void
AddMAGICForestDecompUptakeModel(mobius_model *Model)
{
	BeginModule(Model, "MAGIC Forest decomposition and uptake", "0.001");
	
	auto Dimensionless   = RegisterUnit(Model);
	auto TPerHa          = RegisterUnit(Model, "tonnes/Ha");
	auto PerYear         = RegisterUnit(Model, "1/year");
	auto TPerHaPerTs     = RegisterUnit(Model, "tonnes/Ha/month");
	auto TPerHaPerYear   = RegisterUnit(Model, "tonnes/Ha/year");
	auto MMolPerKg       = RegisterUnit(Model, "mmol/kg");
	auto MEqPerKg        = RegisterUnit(Model, "meq/kg");
	auto MMolPerM2PerTs  = RegisterUnit(Model, "mmol/m2/month");
	auto MEqPerM2PerTs   = RegisterUnit(Model, "meq/m2/month");
	
	
	auto ForestPatch     = RegisterIndexSet(Model, "Forest patch");
	auto TreeClass       = RegisterIndexSet(Model, "Tree species");
	auto TreeCompartment = RegisterIndexSet(Model, "Tree compartment");
	
	
	
	auto TreeGrowth           = RegisterInput(Model, "Forest growth", TPerHaPerTs, true);
	auto TreeTurnover         = RegisterInput(Model, "Forest turnover", TPerHaPerTs, true);
	auto TreeDisturbance      = RegisterInput(Model, "Forest disturbance", Dimensionless);
	auto LeftAfterDisturbance = RegisterInput(Model, "Biomass left on ground after disturbance", Dimensionless);
	auto TreeAgeInterp        = RegisterInput(Model, "Compartment share interpolation variable", Dimensionless);  //Should go between 1 and 100
	
	
	
	auto PatchPars            = RegisterParameterGroup(Model, "Forest patches", ForestPatch);
	
	auto PatchArea            = RegisterParameterDouble(Model, PatchPars, "Patch relative area", Dimensionless, 1.0, 0.0, 1.0);
	auto CarryingCapacity     = RegisterParameterDouble(Model, PatchPars, "Patch carrying capacity", TPerHa, 0.0, 0.0, 10000.0);
	
	
	auto InitialForestPars    = RegisterParameterGroup(Model, "Initial tree mass", ForestPatch, TreeClass);
	
	auto InitialTreeMass      = RegisterParameterDouble(Model, InitialForestPars, "Initial live tree mass", TPerHa, 0.0, 0.0, 10000.0);
	
	
	auto TreeGrowthPars       = RegisterParameterGroup(Model, "Tree growth", TreeClass);
	
	auto MaxGrowthRate        = RegisterParameterDouble(Model, TreeGrowthPars, "Max tree growth rate", PerYear, 0.0, 0.0, 1000.0);
	
	
	auto TreeDecomp           = RegisterParameterGroup(Model, "Tree (de)composition", TreeClass, TreeCompartment);
	
	auto ShareOfYoung         = RegisterParameterDouble(Model, TreeDecomp, "Share of compartment in young trees", Dimensionless, 0.0, 0.0, 1.0);
	auto ShareOfOld           = RegisterParameterDouble(Model, TreeDecomp, "Share of compartment in old trees", Dimensionless, 0.0, 0.0, 1.0);
	auto TurnoverRate         = RegisterParameterDouble(Model, TreeDecomp, "Tree compartment turnover rate", PerYear, 0.0, 0.0, 1.0);
	auto TreeDecompRate       = RegisterParameterDouble(Model, TreeDecomp, "Tree decomposition rate", PerYear, 0.1, 0.0, 1.0);
	//auto InitialDeadTreeMass  = RegisterParameterDouble(Model, TreeDecomp, "Initial dead tree mass", TPerHa, 0.0, 0.0, 10000.0);
	auto TreeCConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree C concentration", MMolPerKg, 0.0, 0.0, 100000.0);
	auto TreeNConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree N concentration", MMolPerKg, 0.0, 0.0, 100000.0);
	auto TreePConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree P concentration", MMolPerKg, 0.0, 0.0, 100000.0);
	auto TreeCaConc           = RegisterParameterDouble(Model, TreeDecomp, "Tree Ca concentration", MEqPerKg, 0.0, 0.0, 100000.0);
	auto TreeMgConc           = RegisterParameterDouble(Model, TreeDecomp, "Tree Mg concentration", MEqPerKg, 0.0, 0.0, 100000.0);
	auto TreeNaConc           = RegisterParameterDouble(Model, TreeDecomp, "Tree Na concentration", MEqPerKg, 0.0, 0.0, 100000.0);
	auto TreeKConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree K concentration", MEqPerKg, 0.0, 0.0, 100000.0);
	
	
	auto ForestNetGrowth      = RegisterEquation(Model, "Tree net growth", TPerHaPerTs);
	auto LiveTreeMass         = RegisterEquation(Model, "Live tree mass per species", TPerHa);
	//auto TotalLiveTreeMass    = RegisterEquationCumulative(Model, "Total live tree mass", LiveTreeMass, TreeClass);
	auto CompartmentTurnover  = RegisterEquation(Model, "Compartment turnover", TPerHaPerTs);
	
	auto CompartmentTurnoverSummed = RegisterEquationCumulative(Model, "Compartment turnover averaged over patches", CompartmentTurnover, ForestPatch, PatchArea);
	
	SetInitialValue(Model, LiveTreeMass, InitialTreeMass);
	
	auto CompartmentShare     = RegisterEquation(Model, "Compartment share", Dimensionless);
	auto CompartmentGrowth    = RegisterEquation(Model, "Compartment growth", TPerHaPerTs);
	
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
	auto DeadTreeMass         = RegisterEquation(Model, "Dead tree mass", TPerHa);
	auto DeadTreeDecomp       = RegisterEquation(Model, "Dead tree decomposition", TPerHaPerTs);
	
	auto InitialDeadTreeMass  = RegisterEquationInitialValue(Model, "Initial dead tree mass", TPerHa);
	SetInitialValue(Model, DeadTreeMass, InitialDeadTreeMass);
	
	auto TreeDecompCSource    = RegisterEquation(Model, "C source from tree decomposition", MMolPerM2PerTs);
	auto TreeDecompNSource    = RegisterEquation(Model, "N source from tree decomposition", MMolPerM2PerTs);
	auto TreeDecompPSource    = RegisterEquation(Model, "P source from tree decomposition", MMolPerM2PerTs);
	auto TreeDecompCaSource   = RegisterEquation(Model, "Ca source from tree decomposition", MEqPerM2PerTs);
	auto TreeDecompMgSource   = RegisterEquation(Model, "Mg source from tree decomposition", MEqPerM2PerTs);
	auto TreeDecompNaSource   = RegisterEquation(Model, "Na source from tree decomposition", MEqPerM2PerTs);
	auto TreeDecompKSource    = RegisterEquation(Model, "K source from tree decomposition", MEqPerM2PerTs);
	
	auto TotalTreeDecompCSourceSpecies  = RegisterEquationCumulative(Model, "Total C source from tree decomposition per species", TreeDecompCSource, TreeCompartment);
	auto TotalTreeDecompNSourceSpecies  = RegisterEquationCumulative(Model, "Total N source from tree decomposition per species", TreeDecompNSource, TreeCompartment);
	auto TotalTreeDecompPSourceSpecies  = RegisterEquationCumulative(Model, "Total P source from tree decomposition per species", TreeDecompPSource, TreeCompartment);
	auto TotalTreeDecompCaSourceSpecies = RegisterEquationCumulative(Model, "Total Ca source from tree decomposition per species", TreeDecompCaSource, TreeCompartment);
	auto TotalTreeDecompMgSourceSpecies = RegisterEquationCumulative(Model, "Total Mg source from tree decomposition per species", TreeDecompMgSource, TreeCompartment);
	auto TotalTreeDecompNaSourceSpecies = RegisterEquationCumulative(Model, "Total Na source from tree decomposition per species", TreeDecompNaSource, TreeCompartment);
	auto TotalTreeDecompKSourceSpecies  = RegisterEquationCumulative(Model, "Total K source from tree decomposition per species", TreeDecompKSource, TreeCompartment);
	
	auto TotalTreeDecompCSource  = RegisterEquationCumulative(Model, "Total C source from tree decomposition", TotalTreeDecompCSourceSpecies, TreeClass);
	auto TotalTreeDecompNSource  = RegisterEquationCumulative(Model, "Total N source from tree decomposition", TotalTreeDecompNSourceSpecies, TreeClass);
	auto TotalTreeDecompPSource  = RegisterEquationCumulative(Model, "Total P source from tree decomposition", TotalTreeDecompPSourceSpecies, TreeClass);
	auto TotalTreeDecompCaSource = RegisterEquationCumulative(Model, "Total Ca source from tree decomposition", TotalTreeDecompCaSourceSpecies, TreeClass);
	auto TotalTreeDecompMgSource = RegisterEquationCumulative(Model, "Total Mg source from tree decomposition", TotalTreeDecompMgSourceSpecies, TreeClass);
	auto TotalTreeDecompNaSource = RegisterEquationCumulative(Model, "Total Na source from tree decomposition", TotalTreeDecompNaSourceSpecies, TreeClass);
	auto TotalTreeDecompKSource  = RegisterEquationCumulative(Model, "Total K source from tree decomposition", TotalTreeDecompKSourceSpecies, TreeClass);
	
	auto FractionOfYear       = GetEquationHandle(Model, "Fraction of year");
	
	EQUATION(Model, ForestNetGrowth,
		double in = INPUT(TreeGrowth);
		double r = PARAMETER(MaxGrowthRate)*RESULT(FractionOfYear);
		double K = PARAMETER(CarryingCapacity);
		double V = LAST_RESULT(LiveTreeMass);
		
		//NOTE: LAST_RESULT(TotalLiveTreeMass) fails for some obscure reason. Why????
		double Vtot = 0.0;
		index_t Patch = CURRENT_INDEX(ForestPatch);
		for(index_t Species = FIRST_INDEX(TreeClass); Species != INDEX_COUNT(TreeClass); ++Species)
			Vtot += LAST_RESULT(LiveTreeMass, Species, Patch);
		
		if(std::isfinite(in)) return in;
		
		//NOTE: Logistic growth. The way it is formulated here is technically unstable, but the growth rate should be so slow that it shouldn't matter.
		double computed = r*V*(1.0 - Vtot/K);
		
		if(!std::isfinite(computed)) computed = 0.0;
		
		return computed;
	)
	
	EQUATION(Model, LiveTreeMass,
		double before_disturbance = LAST_RESULT(LiveTreeMass) + RESULT(ForestNetGrowth);
		double after_disturbance = before_disturbance * (1.0 - INPUT(TreeDisturbance));
		return std::max(0.0, after_disturbance);
	)

	EQUATION(Model, CompartmentShare,
		const double ln100  = 4.60517018599;
		double interp = INPUT(TreeAgeInterp);
		double young  = PARAMETER(ShareOfYoung);
		double old    = PARAMETER(ShareOfOld);
		if(interp == 0.0) return young;                                      //NOTE: Just to not make the model crash if the time series was not provided.
		return LinearInterpolate(std::log(interp), 0.0, ln100, young, old);
	)
	
	EQUATION(Model, CompartmentTurnover,
		double in = INPUT(TreeTurnover);
		double share = RESULT(CompartmentShare);
	
		double invdt = RESULT(FractionOfYear);
		double r = 1.0 - std::pow(1.0 - PARAMETER(TurnoverRate), invdt);  //NOTE: Turn 1/year to 1/month
		double computed = r * share * RESULT(LiveTreeMass); //NOTE: Is based on current-timestep so that trees that are harvested are not counted for turnover. But does that mean that it is counted doubly in compartmentgrowth below?
		
		if(std::isfinite(in)) return in * share;
		return computed;
	)
	
	EQUATION(Model, CompartmentGrowth,
		return RESULT(ForestNetGrowth) * RESULT(CompartmentShare) + RESULT(CompartmentTurnover);  //Have to add turnover here since that is not taken into account in the net growth.
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
		double share = PARAMETER(ShareOfOld);  //NOTE: Hmm, should be the interpolated one, but can't access it!
		
		//NOTE: Again, we can't use result of cumulative equations in initial value computations for some reason.
		double turnoveravg = 0.0;
		for(index_t Patch = FIRST_INDEX(ForestPatch); Patch < INDEX_COUNT(ForestPatch); ++Patch)
		{
			double turnover = PARAMETER(InitialTreeMass, Patch, CURRENT_INDEX(TreeClass));
			turnoveravg += PARAMETER(PatchArea, Patch) * turnover;
		}
		
		turnoveravg *= (r * share);
		
		return turnoveravg / (1.0 - std::exp(-PARAMETER(TreeDecompRate)*RESULT(FractionOfYear)));
	)
	
	EQUATION(Model, LeftOnGroundByDisturbance,
		double before_disturbance = LAST_RESULT(LiveTreeMass) + RESULT(ForestNetGrowth);
		double disturbance  = before_disturbance * INPUT(TreeDisturbance);
		return INPUT(LeftAfterDisturbance) * disturbance * RESULT(CompartmentShare);
	)
	
	EQUATION(Model, DeadTreeMass,
		return LAST_RESULT(DeadTreeMass) * std::exp(-PARAMETER(TreeDecompRate)*RESULT(FractionOfYear)) + RESULT(TotalLeftOnGroundByDisturbance) + RESULT(CompartmentTurnoverSummed);
	)
	
	EQUATION(Model, TreeDecompCSource,      
		return PARAMETER(TreeCConc) * RESULT(DeadTreeDecomp) * 0.1;  // Convert   tonne/Ha -> kg/m2
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