

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
	auto MMolPerM3     = RegisterUnit(Model, "mmol/m3");
	auto MMolPerM2PerYear = RegisterUnit(Model, "mmol/m2/year");
	auto MMolPerM2PerTs = RegisterUnit(Model, "mmol/m2/month");
	auto PerYear       = RegisterUnit(Model, "1/year");
	auto PerTs         = RegisterUnit(Model, "1/month");
	auto KgPerM2       = RegisterUnit(Model, "kg/m2");
	auto M2PerKg       = RegisterUnit(Model, "m2/kg");


	auto Compartment = GetIndexSetHandle(Model, "Compartment");
	
	auto SoilCarbon = RegisterParameterGroup(Model, "Soil carbon", Compartment);
	
	auto InitialSteady                = RegisterParameterBool(Model, SoilCarbon, "Initial steady state", false);
	auto InitialOrganicC              = RegisterParameterDouble(Model, SoilCarbon, "Initial organic C", MolPerM2, 0.0, 0.0, 1e8, "Only used if not in steady state initially");
	auto OrganicCFastFraction         = RegisterParameterDouble(Model, SoilCarbon, "Initial relative size of fast C pool", Dimensionless, 0.5, 0.0, 1.0, "Only used if not in steady state initially");
	auto OrganicCLitter               = RegisterParameterDouble(Model, SoilCarbon, "Organic C litter", MMolPerM2PerYear, 0.0, 0.0, 1e6, "Litter in addition to what is computed by the forest module");
	auto LossRateFast                 = RegisterParameterDouble(Model, SoilCarbon, "Loss rate of fast-decomposable C", PerYear, 0.1, 0.0, 1.0, "Solubilized and mineralized C");
	auto LossRateSlow                 = RegisterParameterDouble(Model, SoilCarbon, "Loss rate of slow-decomposable C", PerYear, 0.01, 0.0, 1.0, "Solubilized and mineralized C");
	auto MicrobialQ10                 = RegisterParameterDouble(Model, SoilCarbon, "Microbial Q10", Dimensionless, 1, 1, 5, "If different from 1, does not work that well with the steady-state");
	auto FastSlowMassFlowRate         = RegisterParameterDouble(Model, SoilCarbon, "Mass flow rate from fast to slow C pool", PerYear, 0.01, 0.0, 1.0);
	
	auto Solubilization               = RegisterParameterDouble(Model, SoilCarbon, "Solubilization", Dimensionless, 0.0, 0.0, 1.0, "Fraction of decomposed organic C,N and P that is solubilized as DOC, DON or DOP.");
	auto CUseEfficiency               = RegisterParameterDouble(Model, SoilCarbon, "C use efficiency", Dimensionless, 0.0, 0.0, 1.0, "Fraction of non-solubilized decomposed organic C that becomes biomass and is returned to the organic C pool. The rest is mineralized/respired as CO2.");
	
	auto CECParams = RegisterParameterGroup(Model, "Cation exchange capacity", Compartment);
	
	auto CECVariesByOrgC              = RegisterParameterBool(Model, CECParams, "CEC varies by organic C", false);
	auto CECOrgCLinearCoeff           = RegisterParameterDouble(Model, CECParams, "CEC - org C linear coefficient", M2PerKg, 0.0, 0.0, 100.0, "Relative to the initial organic C parameter (not steady state)");
	
	
	auto LossRateFastEq         = RegisterEquation(Model, "Loss rate fast", PerTs);
	auto LossRateSlowEq         = RegisterEquation(Model, "Loss rate slow", PerTs);
	auto TurnoverFast           = RegisterEquation(Model, "Fast C turnover", MMolPerM2PerTs);
	auto TurnoverSlow           = RegisterEquation(Model, "Slow C turnover", MMolPerM2PerTs);
	auto TotalTurnover          = RegisterEquation(Model, "Organic C turnover", MMolPerM2PerTs);
	auto FastSlowMassFlow       = RegisterEquation(Model, "Fast-slow mass flow", MMolPerM2PerTs);
	auto OrganicCFast           = RegisterEquation(Model, "Organic C fast-decomposable pool", MMolPerM2);
	auto OrganicCSlow           = RegisterEquation(Model, "Organic C slow-decomposable pool", MMolPerM2);
	auto OrganicCSolubilized    = RegisterEquation(Model, "Organic C solubilized", MMolPerM2PerTs);
	auto OrganicCMineralized    = RegisterEquation(Model, "Organic C mineralized", MMolPerM2PerTs);
	auto OrganicCInBiomass      = RegisterEquation(Model, "Organic C in soil microbial biomass", MMolPerM2PerTs);
	auto OrganicC               = RegisterEquation(Model, "Organic C", MMolPerM2);
	
	auto OrganicCFastMass       = RegisterEquation(Model, "Organic C fast pool (mass)", KgPerM2);
	auto OrganicCSlowMass       = RegisterEquation(Model, "Organic C slow pool (mass)", KgPerM2);
	auto OrganicCMass           = RegisterEquation(Model, "Organic C (mass)", KgPerM2);
	
	auto DOCConc                = RegisterEquation(Model, "DOC concentration", MMolPerM3);
	
	auto InitialOrganicCFast    = RegisterEquationInitialValue(Model, "Initial organic C fast-decomposable", MMolPerM2);
	auto InitialOrganicCSlow    = RegisterEquationInitialValue(Model, "Initial organic C slow-decomposable", MMolPerM2);
	
	SetInitialValue(Model, OrganicCFast, InitialOrganicCFast);
	SetInitialValue(Model, OrganicCSlow, InitialOrganicCSlow);
	
	auto Discharge                 = GetEquationHandle(Model, "Discharge");
	auto Temperature               = GetEquationHandle(Model, "Temperature");
	
	auto CationExchangeCapacityEq  = GetEquationHandle(Model, "Cation exchange capacity");
	auto CationExchangeCapacity    = GetParameterDoubleHandle(Model, "Cation exchange capacity");
	
	auto TreeSpecies               = GetIndexSetHandle(Model, "Tree species");
	auto TreeCompartment           = GetIndexSetHandle(Model, "Tree compartment");
	auto TotalTreeDecompCSource    = GetEquationHandle(Model, "Total C source from litter decomposition");
	auto CSourceTreeDecomp         = GetEquationHandle(Model, "C source from litter decomposition");
	auto FractionOfYear            = GetEquationHandle(Model, "Fraction of year");
	
	auto IsSoil                    = GetParameterBoolHandle(Model, "This is a soil compartment");
	auto IsTop                     = GetParameterBoolHandle(Model, "This is a top compartment");
	
	EQUATION(Model, InitialOrganicCFast,
		double provided = PARAMETER(InitialOrganicC)*PARAMETER(OrganicCFastFraction)*1000.0; //mol->mmol
		
		//TODO: Make cumulation equations work well with initial value system so that we don't have to do this:
		double forest = 0.0;
		for(index_t Species = FIRST_INDEX(TreeSpecies); Species < INDEX_COUNT(TreeSpecies); ++Species)
		{
			for(index_t Compartment = FIRST_INDEX(TreeCompartment); Compartment < INDEX_COUNT(TreeCompartment); ++Compartment)
				forest += RESULT(CSourceTreeDecomp, Species, Compartment);
		}
		if(!PARAMETER(IsTop) || !PARAMETER(IsSoil)) forest = 0.0;
		
		double f = RESULT(FractionOfYear);
		double In = forest + f*PARAMETER(OrganicCLitter);
		double r_F = 1.0 - std::pow(1.0 - PARAMETER(LossRateFast), f);
		double r_FS = 1.0 - std::pow(1.0 - PARAMETER(FastSlowMassFlowRate), f);
		double steady = In / ((r_F + r_FS));
		
		//WarningPrint("Steady is ", steady, "\n");
		
		if(PARAMETER(InitialSteady))
			return steady;
		
		return provided;
	)
	
	EQUATION(Model, InitialOrganicCSlow,
		double provided = PARAMETER(InitialOrganicC)*(1.0 - PARAMETER(OrganicCFastFraction))*1000.0; //mol->mmol
		
		double f = RESULT(FractionOfYear);
		double r_S = 1.0 - std::pow(1.0 - PARAMETER(LossRateSlow), f);
		double r_FS = 1.0 - std::pow(1.0 - PARAMETER(FastSlowMassFlowRate), f);
		double steady = RESULT(OrganicCFast) * r_FS / r_S;
		
		if(PARAMETER(InitialSteady))
			return steady;
		
		return provided;
	)
	
	EQUATION(Model, LossRateFastEq,
		double ry = PARAMETER(LossRateFast)*std::pow(PARAMETER(MicrobialQ10), (RESULT(Temperature) - 20.0) / 10.0);
		return 1.0 - std::pow(1.0 - ry, RESULT(FractionOfYear));
	)
	
	EQUATION(Model, LossRateSlowEq,
		double ry = PARAMETER(LossRateSlow)*std::pow(PARAMETER(MicrobialQ10), (RESULT(Temperature) - 20.0) / 10.0);
		return 1.0 - std::pow(1.0 - ry, RESULT(FractionOfYear));
	)
	
	EQUATION(Model, TurnoverFast,
		double r = RESULT(LossRateFastEq);
		double s = PARAMETER(Solubilization);
		double c = PARAMETER(CUseEfficiency);
		//double r = std::exp(-PARAMETER(LossRateFast)*RESULT(FractionOfYear)) - 1;
		return LAST_RESULT(OrganicCFast)*r/(s + (1-s)*(1-c));
	)
	
	EQUATION(Model, TurnoverSlow,
		double r = RESULT(LossRateSlowEq);
		double s = PARAMETER(Solubilization);
		double c = PARAMETER(CUseEfficiency);
		//double r = std::exp(-PARAMETER(LossRateSlow)*RESULT(FractionOfYear)) - 1;
		return LAST_RESULT(OrganicCSlow)*r/(s + (1-s)*(1-c));
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
		double r_FS = 1.0 - std::pow(1.0 - PARAMETER(FastSlowMassFlowRate), RESULT(FractionOfYear));
		//double r_FS = std::exp(-PARAMETER(FastSlowMassFlowRate)*RESULT(FractionOfYear)) - 1;
		return LAST_RESULT(OrganicCFast)*r_FS;
	)
	
	EQUATION(Model, OrganicCFast,
		double forest = RESULT(TotalTreeDecompCSource);
		if(!PARAMETER(IsTop) || !PARAMETER(IsSoil)) forest = 0.0;
		return
			  LAST_RESULT(OrganicCFast) * (1.0 - RESULT(LossRateFastEq))
			+ forest
			+ PARAMETER(OrganicCLitter)*RESULT(FractionOfYear)
			- RESULT(FastSlowMassFlow);
	)
	
	EQUATION(Model, OrganicCSlow,
		return
			  LAST_RESULT(OrganicCSlow)*(1.0 - RESULT(LossRateSlowEq))
			+ RESULT(FastSlowMassFlow);
	)
	
	EQUATION(Model, OrganicC,
		return RESULT(OrganicCFast) + RESULT(OrganicCSlow);
	)
	
	constexpr double c_molar_weight = 12.011; // g/mol
	
	EQUATION(Model, OrganicCFastMass,
		return RESULT(OrganicCFast) * 1e-6 * c_molar_weight; // mmol/m2 * g/mol -> kg/m2
	)
	
	EQUATION(Model, OrganicCSlowMass,
		return RESULT(OrganicCSlow) * 1e-6 * c_molar_weight;
	)
	
	EQUATION(Model, OrganicCMass,
		return RESULT(OrganicCFastMass) + RESULT(OrganicCSlowMass);
	)
	
	EQUATION(Model, DOCConc,
		return RESULT(OrganicCSolubilized) / RESULT(Discharge);
	)
	
	
	EQUATION_OVERRIDE(Model, CationExchangeCapacityEq,
		double cec = PARAMETER(CationExchangeCapacity);
		double computed = cec + (LAST_RESULT(OrganicC) - PARAMETER(InitialOrganicC)*1e3)*PARAMETER(CECOrgCLinearCoeff);
		if(PARAMETER(CECVariesByOrgC)) return computed;
		return cec;
	)
	
}