

static void
AddMagicForestSoilCarbonModel(mobius_model *Model)
{
	BeginModule(Model, "MAGIC Forest soil carbon", "_dev");
	SetModuleDescription(Model, R""""(
A soil carbon module for MAGIC Forest.
)"""");

	auto Dimensionless = RegisterUnit(Model);
	auto MolPerM2      = RegisterUnit(Model, "mol/m2");
	auto MMolPerM2     = RegisterUnit(Model, "mmol/m2");
	auto MMolPerM2PerTs = RegisterUnit(Model, "mmol/m2/month");
	auto PerYear       = RegisterUnit(Model, "1/year");


	auto Compartment = GetIndexSetHandle(Model, "Compartment");
	
	auto SoilCarbon = RegisterParameterGroup(Model, "Soil carbon", Compartment);
	
	auto InitialOrganicC              = RegisterParameterDouble(Model, SoilCarbon, "Initial organic C", MolPerM2, 0.0, 0.0, 1e8);
	auto OrganicCFastFraction         = RegisterParameterDouble(Model, SoilCarbon, "Relative size of fast C compartment", Dimensionless, 0.5, 0.0, 1.0);
	auto TurnoverRateFast             = RegisterParameterDouble(Model, SoilCarbon, "Turnover rate of fast-decomposable C", PerYear, 0.1, 0.0, 1.0);
	auto TurnoverRateSlow             = RegisterParameterDouble(Model, SoilCarbon, "Turnover rate of slow-decomposable C", PerYear, 0.01, 0.0, 1.0);
	auto FastSlowMassFlowRate         = RegisterParameterDouble(Model, SoilCarbon, "Mass flow rate from fast to slow C pool", PerYear, 0.01, 0.0, 1.0);
	
	
	auto Solubilization               = RegisterParameterDouble(Model, SoilCarbon, "Solubilization", Dimensionless, 0.0, 0.0, 1.0, "Fraction of decomposed organic C,N and P that is solubilized as DOC, DON or DOP.");
	auto CUseEfficiency               = RegisterParameterDouble(Model, SoilCarbon, "C use efficiency", Dimensionless, 0.0, 0.0, 1.0, "Fraction of non-solubilized decomposed organic C that becomes biomass and is returned to the organic C pool. The rest is mineralized/respired as CO2.");
	
	auto TurnoverFast           = RegisterEquation(Model, "Fast C turnover", MMolPerM2PerTs);
	auto TurnoverSlow           = RegisterEquation(Model, "Slow C turnover", MMolPerM2PerTs);
	auto TotalTurnover          = RegisterEquation(Model, "Organic C turnover", MMolPerM2PerTs);
	auto FastSlowMassFlow       = RegisterEquation(Model, "Fast-slow mass flow", MMolPerM2PerTs);
	auto OrganicCFast           = RegisterEquation(Model, "Organic C fast-decomposable pool", MMolPerM2);
	auto OrganicCSlow           = RegisterEquation(Model, "Organic C slow-decomposable pool", MMolPerM2);
	auto OrganicCSolubilized      = RegisterEquation(Model, "Organic C solubilized", MMolPerM2PerTs);
	auto OrganicCMineralized      = RegisterEquation(Model, "Organic C mineralized", MMolPerM2PerTs);
	auto OrganicCInBiomass        = RegisterEquation(Model, "Organic C in soil microbial biomass", MMolPerM2PerTs);
	auto OrganicC               = RegisterEquation(Model, "Organic C", MMolPerM2);
	
	auto InitialOrganicCFast    = RegisterEquationInitialValue(Model, "Initial organic C fast-decomposable", MMolPerM2);
	auto InitialOrganicCSlow    = RegisterEquationInitialValue(Model, "Initial organic C slow-decomposable", MMolPerM2);
	
	SetInitialValue(Model, OrganicCFast, InitialOrganicCFast);
	SetInitialValue(Model, OrganicCSlow, InitialOrganicCSlow);
	
	
	auto TotalTreeDecompCSource    = GetEquationHandle(Model, "Total C source from tree decomposition");
	auto FractionOfYear            = GetEquationHandle(Model, "Fraction of year");
	
	auto IsSoil                    = GetParameterBoolHandle(Model, "This is a soil compartment");
	auto IsTop                     = GetParameterBoolHandle(Model, "This is a top compartment");
	
	
	EQUATION(Model, InitialOrganicCFast,
		return PARAMETER(InitialOrganicC)*PARAMETER(OrganicCFastFraction)*1000.0; //mol->mmol
	)
	
	EQUATION(Model, InitialOrganicCSlow,
		return PARAMETER(InitialOrganicC)*(1.0 - PARAMETER(OrganicCFastFraction))*1000.0; //mol->mmol
	)
	
	EQUATION(Model, TurnoverFast,
		double r = 1.0 - std::pow(1.0 - PARAMETER(TurnoverRateFast), RESULT(FractionOfYear));
		return LAST_RESULT(OrganicCFast)*r;
	)
	
	EQUATION(Model, TurnoverSlow,
		double r = 1.0 - std::pow(1.0 - PARAMETER(TurnoverRateSlow), RESULT(FractionOfYear));
		return LAST_RESULT(OrganicCSlow)*r;
	)
	
	EQUATION(Model, TotalTurnover,
		return RESULT(TurnoverFast) + RESULT(TurnoverSlow);
	)
	
	EQUATION(Model, OrganicCSolubilized,
		return PARAMETER(Solubilization) * RESULT(TotalTurnover);
	)
	
	EQUATION(Model, OrganicCMineralized,
		return (1.0 - PARAMETER(Solubilization)) * (1.0 - PARAMETER(CUseEfficiency)) * RESULT(TotalTurnover);
	)
	
	EQUATION(Model, OrganicCInBiomass,
		return (1.0 - PARAMETER(Solubilization)) * PARAMETER(CUseEfficiency) * RESULT(TotalTurnover);
	)
	
	EQUATION(Model, FastSlowMassFlow,
		double r = 1.0 - std::pow(1.0 - PARAMETER(FastSlowMassFlowRate), RESULT(FractionOfYear));
		return LAST_RESULT(OrganicCFast)*r;
	)
	
	EQUATION(Model, OrganicCFast,
		double forest = RESULT(TotalTreeDecompCSource);
		if(!PARAMETER(IsTop) || !PARAMETER(IsSoil)) forest = 0.0;
		return
			  LAST_RESULT(OrganicCFast)
			+ forest
			- RESULT(TurnoverFast)
			+ RESULT(OrganicCInBiomass)
			- RESULT(FastSlowMassFlow);
	)
	
	EQUATION(Model, OrganicCSlow,
		return
			  LAST_RESULT(OrganicCSlow)
			- RESULT(TurnoverSlow)
			+ RESULT(FastSlowMassFlow);
	)
	
	EQUATION(Model, OrganicC,
		return RESULT(OrganicCFast) + RESULT(OrganicCSlow);
	)
	
}