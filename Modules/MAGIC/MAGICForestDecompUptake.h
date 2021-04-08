





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
	auto MEqPerKgPerTs   = RegisterUnit(Model, "meq/kg/month");
	
	
	auto TreeCompartment = RegisterIndexSet(Model, "Tree compartment");
	auto Needles         = RequireIndex(Model, TreeCompartment, "Needles");
	auto Bark            = RequireIndex(Model, TreeCompartment, "Bark");
	auto Branches        = RequireIndex(Model, TreeCompartment, "Branches+roots");
	auto Wood            = RequireIndex(Model, TreeCompartment, "Wood");
	
	
	auto TreeGrowth           = RegisterInput(Model, "Tree growth", TPerHa);
	auto TreeToDecomp         = RegisterInput(Model, "New dead tree biomass", TPerHa);
	
	
	auto TreeDecomp           = RegisterParameterGroup(Model, "Tree (de)composition", TreeCompartment);
	
	auto ShareOfYoung         = RegisterParameterDouble(Model, TreeDecomp, "Share of compartment in young trees", Dimensionless, 0.0, 0.0, 1.0);
	auto ShareOfOld           = RegisterParameterDouble(Model, TreeDecomp, "Share of compartment in old trees", Dimensionless, 0.0, 0.0, 1.0);
	auto TreeDecompRate       = RegisterParameterDouble(Model, TreeDecomp, "Tree decomposition rate", PerYear, 0.1, 0.0, 1.0);
	auto TreeCConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree C concentration", MEqPerKg);
	auto TreeNConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree N concentration", MEqPerKg);
	auto TreePConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree P concentration", MEqPerKg);
	auto TreeCaConc           = RegisterParameterDouble(Model, TreeDecomp, "Tree Ca concentration", MEqPerKg);
	auto TreeMgConc           = RegisterParameterDouble(Model, TreeDecomp, "Tree Mg concentration", MEqPerKg);
	auto TreeNaConc           = RegisterParameterDouble(Model, TreeDecomp, "Tree Na concentration", MEqPerKg);
	auto TreeKConc            = RegisterParameterDouble(Model, TreeDecomp, "Tree K concentration", MEqPerKg);
	
	
	auto DeadTreeMass         = RegisterEquation(Model, "Dead tree mass", TPerHa);
	auto DeadTreeDecomp       = RegisterEquation(Model, "Dead tree decomposition", TPerHaPerTs);
	
	auto TreeDecompCSource    = RegisterEquation(Model, "C source from tree decomposition", MMolPerM2PerTs);
	auto TreeDecompNSource    = RegisterEquation(Model, "N source from tree decomposition", MMolPerM2PerTs);
	auto TreeDecompPSource    = RegisterEquation(Model, "P source from tree decomposition", MMolPerM2PerTs);
	auto TreeDecompCaSource   = RegisterEquation(Model, "Ca source from tree decomposition", MEqPerKgPerTs);
	auto TreeDecompMgSource   = RegisterEquation(Model, "Mg source from tree decomposition", MEqPerKgPerTs);
	auto TreeDecompNaSource   = RegisterEquation(Model, "Na source from tree decomposition", MEqPerKgPerTs);
	auto TreeDecompKSource    = RegisterEquation(Model, "K source from tree decomposition", MEqPerKgPerTs);
	
	auto TotalTreeDecompCSource  = RegisterEquationCumulative(Model, "Total C source from tree decomposition", TreeDecompCSource, TreeCompartment);
	auto TotalTreeDecompNSource  = RegisterEquationCumulative(Model, "Total N source from tree decomposition", TreeDecompNSource, TreeCompartment);
	auto TotalTreeDecompPSource  = RegisterEquationCumulative(Model, "Total P source from tree decomposition", TreeDecompNSource, TreeCompartment);
	auto TotalTreeDecompCaSource = RegisterEquationCumulative(Model, "Total Ca source from tree decomposition", TreeDecompNSource, TreeCompartment);
	auto TotalTreeDecompMgSource = RegisterEquationCumulative(Model, "Total Mg source from tree decomposition", TreeDecompNSource, TreeCompartment);
	auto TotalTreeDecompNaSource = RegisterEquationCumulative(Model, "Total Na source from tree decomposition", TreeDecompNSource, TreeCompartment);
	auto TotalTreeDecompKSource  = RegisterEquationCumulative(Model, "Total K source from tree decomposition", TreeDecompNSource, TreeCompartment);
	
	auto FractionOfYear       = GetEquationHandle(Model, "Fraction of year");
	
	
	
	EQUATION(Model, DeadTreeDecomp,
		return LAST_RESULT(DeadTreeMass) * std::exp(-PARAMETER(TreeDecompRate)*RESULT(FractionOfYear));
	)
	
	EQUATION(Model, DeadTreeMass,
		return LAST_RESULT(DeadTreeMass) - RESULT(DeadTreeDecomp) + INPUT(TreeToDecomp)*PARAMETER(ShareOfOld);
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