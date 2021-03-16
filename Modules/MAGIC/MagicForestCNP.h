

static void
AddSimpleMagicCarbonNitrogenModel(mobius_model *Model)
{
	
	BeginModule(Model, "MAGIC Forest CNP", "_dev");
	SetModuleDescription(Model, R""""(
A CNP-module for MAGIC Forest. Developed by Bernard J. Cosby.
)"""");

	auto Dimensionless    = RegisterUnit(Model);
	auto MolPerM2         = RegisterUnit(Model, "mol/m2");
	auto MMolPerM2        = RegisterUnit(Model, "mmol/m2");
	auto MMolPerM2PerYear = RegisterUnit(Model, "mmol/m2/year");
	
	
	auto Compartment               = GetIndexSetHandle(Model, "Compartment");
	auto CNPPar                    = RegisterParameterGroup(Model, "C, N, and P", Compartment);
	
	
	auto OrganicCLitter               = RegisterParameterDouble(Model, CNPPar, "Organic C litter", MMolPerM2PerYear, 0.0, 0.0, 1e6);
	auto OrganicCDecomposition        = RegisterParameterDouble(Model, CNPPar, "Organic C decomposition", MMolPerM2PerYear, 0.0, 0.0, 1e6);
	

	auto OrganicCNLitterRatio         = RegisterParameterDouble(Model, CNPPar, "Organic C/N litter ratio", Dimensionless, 0.1, 0.0001, 10.0);
	auto OrganicCNDecompositionRatio  = RegisterParameterDouble(Model, CNPPar, "Organic C/N decomposition ratio", Dimensionless, 0.1, 0.0001, 10.0, "If 0, use the pool C/N instead");
	
	auto OrganicCPLitterRatio         = RegisterParameterDouble(Model, CNPPar, "Organic C/P litter ratio", Dimensionless, 0.1, 0.0001, 10.0);
	auto OrganicCPDecompositionRatio  = RegisterParameterDouble(Model, CNPPar, "Organic C/P decomposition ratio", Dimensionless, 0.1, 0.0001, 10.0, "If 0, use the pool C/N instead");
	
	auto InitialOrganicC              = RegisterParameterDouble(Model, CNPPar, "Initial organic C", MolPerM2, 0.0, 0.0, 1e8);
	auto InitialOrganicN              = RegisterParameterDouble(Model, CNPPar, "Initial organic N", MolPerM2, 0.0, 0.0, 1e8);
	auto InitialOrganicP              = RegisterParameterDouble(Model, CNPPar, "Initial organic P", MolPerM2, 0.0, 0.0, 1e8);

	/*
		auto DecompR0                = RegisterParameterDouble(Model, CNPPar, "C decomposition at 0°C", MMolPerM2PerTs, 0.0, 0.0, 1000.0);
	auto DecompQ10               = RegisterParameterDouble(Model, CNPPar, "C decomposition Q10", Dimensionless, 1.0, 1.0, 5.0);
	auto UptakeR0                = RegisterParameterDouble(Model, CNPPar, "N uptake at 0°C", MMolPerM2PerTs, 0.0, 0.0, 1000.0);
	auto UptakeQ10               = RegisterParameterDouble(Model, CNPPar, "N uptake Q10", Dimensionless, 1.0, 1.0, 5.0);
	auto NO3UptakeScale          = RegisterParameterDouble(Model, CNPPar, "NO3 uptake scale", Dimensionless, 1.0, 0.1, 10.0);
	auto NH4UptakeScale          = RegisterParameterDouble(Model, CNPPar, "NH4 uptake scale", Dimensionless, 1.0, 0.1, 10.0);
	auto LitterCN                = RegisterParameterDouble(Model, CNPPar, "Litter C/N", Dimensionless, 50.0, 0.1, 200.0);
	*/




	auto InitialOrganicCScaled     = RegisterEquationInitialValue(Model, "Initial organic C", MMolPerM2);
	auto OrganicC                  = RegisterEquation(Model, "Organic C", MMolPerM2);
	SetInitialValue(Model, OrganicC, InitialOrganicCScaled);
	
	auto InitialOrganicNScaled     = RegisterEquationInitialValue(Model, "Initial organic N", MMolPerM2);
	auto OrganicN                  = RegisterEquation(Model, "Organic N", MMolPerM2);
	SetInitialValue(Model, OrganicN, InitialOrganicNScaled);
	
	auto InitialOrganicPScaled     = RegisterEquationInitialValue(Model, "Initial organic P", MMolPerM2);
	auto OrganicP                  = RegisterEquation(Model, "Organic P", MMolPerM2);
	SetInitialValue(Model, OrganicP, InitialOrganicPScaled);
	
	auto CNRatio                   = RegisterEquation(Model, "Pool C/N ratio", Dimensionless);
	auto CPRatio                   = RegisterEquation(Model, "Pool C/P ratio", Dimensionless);
	
	/*
	auto Decomposition           = RegisterEquation(Model, "Organic C decomposition", MMolPerM2PerTs);
	auto UptakeBaseline          = RegisterEquation(Model, "N uptake baseline", MMolPerM2PerTs);
	auto DesiredNO3Uptake        = RegisterEquation(Model, "Desired NO3 uptake", MMolPerM2PerTs);
	auto DesiredNH4Uptake        = RegisterEquation(Model, "Desired NH4 uptake", MMolPerM2PerTs);
	auto LitterC                 = RegisterEquation(Model, "Organic C litter", MMolPerM2PerTs);
	*/
	
	auto IsSoil                    = GetParameterDoubleHandle(Model, "This is a soil compartment");
	
	
	
	EQUATION(Model, InitialOrganicCScaled,
		return PARAMETER(InitialOrganicC)*1000.0;
	)
	
	EQUATION(Model, InitialOrganicNScaled,
		return PARAMETER(InitialOrganicN)*1000.0;
	)
	
	EQUATION(Model, InitialOrganicPScaled,
		return PARAMETER(InitialOrganicP)*1000.0;
	)
	
	
	EQUATION(Model, CNRatio,
		double CN = SafeDivide(LAST_RESULT(OrganicC), LAST_RESULT(OrganicN));
		if(!PARAMETER(IsSoil)) CN = 1.0;  //NOTE: to not cause crashes when we are in water. TODO: Make a conditional to exclude the entire computation in that case.
		return CN;
	)
	
	EQUATION(Model, CPRatio,
		double CP = SafeDivide(LAST_RESULT(OrganicC), LAST_RESULT(OrganicP));
		if(!PARAMETER(IsSoil)) CP = 1.0;  //NOTE: to not cause crashes when we are in water. TODO: Make a conditional to exclude the entire computation in that case.
		return CP;
	)
	
	
	//TODO: These should be redone!
	
	/*
	EQUATION(Model, Decomposition,
		return PARAMETER(DecompR0) * std::pow(PARAMETER(DecompQ10), RESULT(Temperature) * 0.1);
	)
	
	EQUATION(Model, UptakeBaseline,
		return PARAMETER(UptakeR0) * std::pow(PARAMETER(UptakeQ10), RESULT(Temperature) * 0.1);
	)
	
	EQUATION(Model, DesiredNO3Uptake,
		return RESULT(UptakeBaseline) * PARAMETER(NO3UptakeScale);
	)
	
	EQUATION(Model, DesiredNH4Uptake,
		return RESULT(UptakeBaseline) * PARAMETER(NH4UptakeScale);
	)
	
	EQUATION(Model, OrganicCLitter,
		return (RESULT(DesiredNO3Uptake) + RESULT(DesiredNH4Uptake))*PARAMETER(OrganicCNLitterRatio);
	)
	*/
	
	
	
	EQUATION(Model, OrganicC,
		double dCdt =
			  PARAMETER(OrganicCLitter)*RESULT(FractionOfYear)
			- RESULT(OrganicCLoss);
		
		if(!PARAMETER(IsSoil)) dCdt = 0.0; //TODO: make conditional exec instead?
			
		return LAST_RESULT(OrganicC) + dCdt;
	)
}