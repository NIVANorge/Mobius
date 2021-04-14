





static void
AddMAGICForestDecompUptakeModel(mobius_model *Model)
{
	BeginModule(Model, "MAGIC Forest decomposition and uptake", "0.0");
	
	auto Dimensionless   = RegisterUnit(Model);
	auto TPerHa          = RegisterUnit(Model, "tonnes/Ha");
	auto PerYear         = RegisterUnit(Model, "1/year");
	auto TPerHaPerTs     = RegisterUnit(Model, "tonnes/Ha/month");
	auto MMolPerKg       = RegisterUnit(Model, "mmol/kg");
	auto MEqPerKg        = RegisterUnit(Model, "meq/kg");
	auto MMolPerM2PerTs  = RegisterUnit(Model, "mmol/m2/month");
	auto MEqPerM2PerTs   = RegisterUnit(Model, "meq/m2/month");
	
	
	auto TreeCompartment = RegisterIndexSet(Model, "Tree compartment");
	
	//NOTE: We should probably just allow dynamic indexing here?
	//auto Needles         = RequireIndex(Model, TreeCompartment, "Needles");
	//auto Bark            = RequireIndex(Model, TreeCompartment, "Bark");
	//auto Branches        = RequireIndex(Model, TreeCompartment, "Branches+roots");
	//auto Wood            = RequireIndex(Model, TreeCompartment, "Wood");
	
	
	auto TreeGrowth           = RegisterInput(Model, "Tree growth", TPerHaPerTs);
	auto TreeAgeInterp        = RegisterInput(Model, "Compartment share interpolation variable", Dimensionless);  //Should go between 1 and 100
	auto TreeToDecomp         = RegisterInput(Model, "New dead tree biomass", TPerHaPerTs);
	
	
	auto TreeDecomp           = RegisterParameterGroup(Model, "Tree (de)composition", TreeCompartment);
	
	auto ShareOfYoung         = RegisterParameterDouble(Model, TreeDecomp, "Share of compartment in young trees", Dimensionless, 0.0, 0.0, 1.0);
	auto ShareOfOld           = RegisterParameterDouble(Model, TreeDecomp, "Share of compartment in old trees", Dimensionless, 0.0, 0.0, 1.0);
	auto TreeDecompRate       = RegisterParameterDouble(Model, TreeDecomp, "Tree decomposition rate", PerYear, 0.1, 0.0, 1.0);
	auto TreeCConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree C concentration", MMolPerKg, 0.0, 0.0, 100.0);
	auto TreeNConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree N concentration", MMolPerKg, 0.0, 0.0, 100.0);
	auto TreePConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree P concentration", MMolPerKg, 0.0, 0.0, 100.0);
	auto TreeCaConc           = RegisterParameterDouble(Model, TreeDecomp, "Tree Ca concentration", MEqPerKg, 0.0, 0.0, 100.0);
	auto TreeMgConc           = RegisterParameterDouble(Model, TreeDecomp, "Tree Mg concentration", MEqPerKg, 0.0, 0.0, 100.0);
	auto TreeNaConc           = RegisterParameterDouble(Model, TreeDecomp, "Tree Na concentration", MEqPerKg, 0.0, 0.0, 100.0);
	auto TreeKConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree K concentration", MEqPerKg, 0.0, 0.0, 100.0);
	
	
	auto CompartmentShare     = RegisterEquation(Model, "Compartment share", Dimensionless);
	auto CompartmentGrowth    = RegisterEquation(Model, "Compartment growth", TPerHaPerTs);
	auto TreeNUptake          = RegisterEquation(Model, "Tree N uptake", MMolPerM2PerTs);
	auto TreePUptake          = RegisterEquation(Model, "Tree P uptake", MMolPerM2PerTs);
	auto TreeCaUptake         = RegisterEquation(Model, "Tree Ca uptake", MEqPerM2PerTs);
	auto TreeMgUptake         = RegisterEquation(Model, "Tree Mg uptake", MEqPerM2PerTs);
	auto TreeNaUptake         = RegisterEquation(Model, "Tree Na uptake", MEqPerM2PerTs);
	auto TreeKUptake          = RegisterEquation(Model, "Tree K uptake", MEqPerM2PerTs);
	
	auto TotalTreeNUptake     = RegisterEquationCumulative(Model, "Total tree N uptake", TreeNUptake, TreeCompartment);
	auto TotalTreePUptake     = RegisterEquationCumulative(Model, "Total tree P uptake", TreePUptake, TreeCompartment);
	auto TotalTreeCaUptake    = RegisterEquationCumulative(Model, "Total tree Ca uptake", TreeCaUptake, TreeCompartment);
	auto TotalTreeMgUptake    = RegisterEquationCumulative(Model, "Total tree Mg uptake", TreeMgUptake, TreeCompartment);
	auto TotalTreeNaUptake    = RegisterEquationCumulative(Model, "Total tree Na uptake", TreeNaUptake, TreeCompartment);
	auto TotalTreeKUptake     = RegisterEquationCumulative(Model, "Total tree K uptake", TreeKUptake, TreeCompartment);
	
	
	auto DeadTreeMass         = RegisterEquation(Model, "Dead tree mass", TPerHa);
	auto DeadTreeDecomp       = RegisterEquation(Model, "Dead tree decomposition", TPerHaPerTs);
	
	SetInitialValue(Model, DeadTreeMass, 0.0); //So that it does not try to compute it using the equation. Mabe replace with a parameter later.
	
	auto TreeDecompCSource    = RegisterEquation(Model, "C source from tree decomposition", MMolPerM2PerTs);
	auto TreeDecompNSource    = RegisterEquation(Model, "N source from tree decomposition", MMolPerM2PerTs);
	auto TreeDecompPSource    = RegisterEquation(Model, "P source from tree decomposition", MMolPerM2PerTs);
	auto TreeDecompCaSource   = RegisterEquation(Model, "Ca source from tree decomposition", MEqPerM2PerTs);
	auto TreeDecompMgSource   = RegisterEquation(Model, "Mg source from tree decomposition", MEqPerM2PerTs);
	auto TreeDecompNaSource   = RegisterEquation(Model, "Na source from tree decomposition", MEqPerM2PerTs);
	auto TreeDecompKSource    = RegisterEquation(Model, "K source from tree decomposition", MEqPerM2PerTs);
	
	auto TotalTreeDecompCSource  = RegisterEquationCumulative(Model, "Total C source from tree decomposition", TreeDecompCSource, TreeCompartment);
	auto TotalTreeDecompNSource  = RegisterEquationCumulative(Model, "Total N source from tree decomposition", TreeDecompNSource, TreeCompartment);
	auto TotalTreeDecompPSource  = RegisterEquationCumulative(Model, "Total P source from tree decomposition", TreeDecompPSource, TreeCompartment);
	auto TotalTreeDecompCaSource = RegisterEquationCumulative(Model, "Total Ca source from tree decomposition", TreeDecompCaSource, TreeCompartment);
	auto TotalTreeDecompMgSource = RegisterEquationCumulative(Model, "Total Mg source from tree decomposition", TreeDecompMgSource, TreeCompartment);
	auto TotalTreeDecompNaSource = RegisterEquationCumulative(Model, "Total Na source from tree decomposition", TreeDecompNaSource, TreeCompartment);
	auto TotalTreeDecompKSource  = RegisterEquationCumulative(Model, "Total K source from tree decomposition", TreeDecompKSource, TreeCompartment);
	
	auto FractionOfYear       = GetEquationHandle(Model, "Fraction of year");
	
	
	EQUATION(Model, CompartmentShare,
		double ln100  = 4.60517018599;
		double interp = INPUT(TreeAgeInterp);
		double young  = PARAMETER(ShareOfYoung);
		double old    = PARAMETER(ShareOfOld);
		if(interp == 0.0) return young;                                      //NOTE: Just to not make the model crash if the time series was not provided.
		return LinearInterpolate(std::log(interp), 0.0, ln100, young, old);
	)
	
	EQUATION(Model, CompartmentGrowth,
		return INPUT(TreeGrowth) * RESULT(CompartmentShare);
	)
	
	EQUATION(Model, TreeNUptake,
		return RESULT(CompartmentGrowth) * PARAMETER(TreeNConc);
	)
	
	EQUATION(Model, TreePUptake,
		return RESULT(CompartmentGrowth) * PARAMETER(TreePConc);
	)
	
	EQUATION(Model, TreeCaUptake,
		return RESULT(CompartmentGrowth) * PARAMETER(TreeCaConc);
	)
	
	EQUATION(Model, TreeMgUptake,
		return RESULT(CompartmentGrowth) * PARAMETER(TreeMgConc);
	)
	
	EQUATION(Model, TreeNaUptake,
		return RESULT(CompartmentGrowth) * PARAMETER(TreeNaConc);
	)
	
	EQUATION(Model, TreeKUptake,
		return RESULT(CompartmentGrowth) * PARAMETER(TreeKConc);
	)
	
	
	EQUATION(Model, DeadTreeDecomp,
		return LAST_RESULT(DeadTreeMass) * (1.0 - std::exp(-PARAMETER(TreeDecompRate)*RESULT(FractionOfYear)));
	)
	
	EQUATION(Model, DeadTreeMass,
		return LAST_RESULT(DeadTreeMass) * std::exp(-PARAMETER(TreeDecompRate)*RESULT(FractionOfYear)) + INPUT(TreeToDecomp)*PARAMETER(ShareOfOld);
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