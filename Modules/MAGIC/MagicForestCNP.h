

static void
AddSimpleMagicForestCNPModel(mobius_model *Model)
{
	
	BeginModule(Model, "MAGIC Forest CNP", "_dev");
	SetModuleDescription(Model, R""""(
A CNP-module for MAGIC Forest. Developed by Bernard J. Cosby.
)"""");

	auto Dimensionless    = RegisterUnit(Model);
	auto MolPerM2         = RegisterUnit(Model, "mol/m2");
	auto MMolPerM2        = RegisterUnit(Model, "mmol/m2");
	auto MMolPerM2PerTs   = RegisterUnit(Model, "mmol/m2/month");
	auto MMolPerM2PerYear = RegisterUnit(Model, "mmol/m2/year");
	
	
	auto Compartment               = GetIndexSetHandle(Model, "Compartment");
	auto CNPPar                    = RegisterParameterGroup(Model, "C, N, and P", Compartment);
	
	
	auto OrganicCLitter               = RegisterParameterDouble(Model, CNPPar, "Organic C litter", MMolPerM2PerYear, 0.0, 0.0, 1e6);
	auto OrganicCDecomposition        = RegisterParameterDouble(Model, CNPPar, "Organic C decomposition", MMolPerM2PerYear, 0.0, 0.0, 1e6);
	auto Solubilization               = RegisterParameterDouble(Model, CNPPar, "Solubilization", Dimensionless, 0.0, 0.0, 1.0, "Fraction of decomposed organic C,N and P that is solubilized as DOC, DON or DOP.");
	auto CUseEfficiency               = RegisterParameterDouble(Model, CNPPar, "C use efficiency", Dimensionless, 0.0, 0.0, 1.0, "Fraction of non-solubilized decomposed organic C that becomes biomass and is returned to the organic C pool. The rest is mineralized/respired as CO2.");
	auto NUseEfficiency               = RegisterParameterDouble(Model, CNPPar, "N use efficiency", Dimensionless, 0.0, 0.0, 1.0, "Fraction of non-solubilized decomposed organic N that becomes biomass and is returned to the organic N pool. The rest is mineralized as NH4.");
	auto PUseEfficiency               = RegisterParameterDouble(Model, CNPPar, "P use efficiency", Dimensionless, 0.0, 0.0, 1.0, "Fraction of non-solubilized decomposed organic P that becomes biomass and is returned to the organic P pool. The rest is mineralized as PO4.");
	

	auto LitterCNRatio                = RegisterParameterDouble(Model, CNPPar, "C/N ratio of litter", Dimensionless, 0.1, 0.0001, 10.0);
	auto MicrobeCNRatio               = RegisterParameterDouble(Model, CNPPar, "C/N ratio of soil microbial community", Dimensionless, 0.1, 0.0001, 10.0);
	
	auto LitterCPRatio                = RegisterParameterDouble(Model, CNPPar, "C/P ratio of litter", Dimensionless, 0.1, 0.0001, 10.0);
	auto MicrobeCPRatio               = RegisterParameterDouble(Model, CNPPar, "C/P ratio of soil microbial community", Dimensionless, 0.1, 0.0001, 10.0);
	
	auto InitialOrganicC              = RegisterParameterDouble(Model, CNPPar, "Initial organic C", MolPerM2, 0.0, 0.0, 1e8);
	auto InitialOrganicN              = RegisterParameterDouble(Model, CNPPar, "Initial organic N", MolPerM2, 0.0, 0.0, 1e8);
	auto InitialOrganicP              = RegisterParameterDouble(Model, CNPPar, "Initial organic P", MolPerM2, 0.0, 0.0, 1e8);


	auto Nitrification                = RegisterParameterDouble(Model, CNPPar, "Nitrification", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs");
	auto Denitrification              = RegisterParameterDouble(Model, CNPPar, "Denitrification", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs");
	
	auto UptakeR0                     = RegisterParameterDouble(Model, CNPPar, "N uptake at 0°C", MMolPerM2PerYear, 0.0, 0.0, 1000.0);
	auto UptakeQ10                    = RegisterParameterDouble(Model, CNPPar, "N uptake Q10", Dimensionless, 1.0, 1.0, 5.0);
	auto NH4UptakeScale               = RegisterParameterDouble(Model, CNPPar, "NH4 uptake scale", Dimensionless, 1.0, 0.0, 10.0, "amount of NH4 uptake relative to amount of NO3 uptake");

	auto PlantsUseOrganicN            = RegisterParameterBool(Model, CAndN, "Plants have access to organic nitrogen", true);
	auto PlantsUseInorganicFirst      = RegisterParameterBool(Model, CAndN, "Plants use inorganic nitrogen before soil", true);

	/*
		auto DecompR0                = RegisterParameterDouble(Model, CNPPar, "C decomposition at 0°C", MMolPerM2PerTs, 0.0, 0.0, 1000.0);
		auto DecompQ10               = RegisterParameterDouble(Model, CNPPar, "C decomposition Q10", Dimensionless, 1.0, 1.0, 5.0);
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
	
	auto LitterC                 = RegisterEquation(Model, "Organic C litter", MMolPerM2PerTs);
	*/
	
	
	auto OrganicCLitterEq         = RegisterEquation(Model, "Organic C litter", MMolPerM2PerTs);
	auto OrganicCDecompositionEq  = RegisterEquation(Model, "Organic C decomposition", MMolPerM2PerTs);
	auto OrganicCSolubilized      = RegisterEquation(Model, "Organic C solubilized", MMolPerM2PerTs);
	auto OrganicCMineralized      = RegisterEquation(Model, "Organic C mineralized", MMolPerM2PerTs);
	auto OrganicCInBiomass        = RegisterEquation(Model, "Organic C in soil microbial biomass", MMolPerM2PerTs);
	
	auto OrganicNLitter           = RegisterEquation(Model, "Organic N litter", MMolPerM2PerTs);
	auto OrganicNDecomposition    = RegisterEquation(Model, "Organic N decomposition", MMolPerM2PerTs);
	auto OrganicNSolubilized      = RegisterEquation(Model, "Organic N solubilized", MMolPerM2PerTs);
	auto OrganicNMineralized      = RegisterEquation(Model, "Organic N mineralized", MMolPerM2PerTs);
	auto DesiredNImmobilisation   = RegisterEquation(Model, "Desired N immobilization", MMolPerM2PerTs);
	
	auto NitrificationEq          = RegisterEquation(Model, "Nitrification", MMolPerM2PerTs);
	auto DenitrificationEq        = RegisterEquation(Model, "Denitrification", MMolPerM2PerTs);
	
	auto DesiredNO3Uptake         = RegisterEquation(Model, "Desired NO3 uptake", MMolPerM2PerTs);
	auto DesiredNH4Uptake         = RegisterEquation(Model, "Desired NH4 uptake", MMolPerM2PerTs);
	
	auto NO3Immobilisation        = RegisterEquation(Model, "NO3 immobilisation", MMolPerM2PerTs);
	auto NH4Immobilisation        = RegisterEquation(Model, "NH4 immobilisation", MMolPerM2PerTs);
	
	auto NO3Uptake                = RegisterEquation(Model, "NO3 uptake", MMolPerM2PerTs);
	auto NH4Uptake                = RegisterEquation(Model, "NH4 uptake", MMolPerM2PerTs);
	
	auto OrganicNUptake           = RegisterEquation(Model, "Organic N uptake", MMolPerM2PerTs);
	
	
	//NOTE: The following 4 are required as an "interface" to the rest of the MAGIC model
	auto NO3Inputs           = RegisterEquation(Model, "NO3 inputs", MMolPerM2PerTs);
	auto NH4Inputs           = RegisterEquation(Model, "NH4 inputs", MMolPerM2PerTs);
	auto NO3ProcessesLoss    = RegisterEquation(Model, "NO3 processes loss", MMolPerM2PerTs);
	auto NH4ProcessesLoss    = RegisterEquation(Model, "NH4 processes loss", MMolPerM2PerTs);	
	
	
	
	
	auto IsSoil                    = GetParameterDoubleHandle(Model, "This is a soil compartment");
	
	auto FractionOfYear            = GetEquationHandle(Model, "Fraction of year");
	auto Temperature               = GetEquationHandle(Model, "Temperature");
	auto NO3BasicInputs            = GetEquationHandle(Model, "NO3 basic inputs");
	auto NH4BasicInputs            = GetEquationHandle(Model, "NH4 basic inputs");
	
	
	
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
	
	
	
	EQUATION(Model, OrganicCLitter,
		return (RESULT(DesiredNO3Uptake) + RESULT(DesiredNH4Uptake))*PARAMETER(OrganicCNLitterRatio);
	)
	*/
	
	EQUATION(Model, OrganicCLitterEq,
		return RESULT(FractionOfYear) * PARAMETER(OrganicCLitter);
	)
	
	EQUATION(Model, OrganicCDecompositionEq,
		return RESULT(FractionOfYear) * PARAMETER(OrganicCDecomposition);
	)
	
	EQUATION(Model, OrganicCSolubilized,
		return PARAMETER(Solubilization) * RESULT(OrganicCDecompositionEq);
	)
	
	EQUATION(Model, OrganicCMineralized,
		return (1.0 - PARAMETER(Solubilization)) * (1.0 - PARAMETER(CUseEfficiency)) * RESULT(OrganicCDecompositionEq);
	)
	
	EQUATION(Model, OrganicCInBiomass,
		return (1.0 - PARAMETER(Solubilization)) * PARAMETER(CUseEfficiency) * RESULT(OrganicCDecompositionEq);
	)
	
	EQUATION(Model, OrganicC,
		double dCdt =
			  RESULT(OrganicCLitterEq)
			- RESULT(OrganicCSolubilized)
			- RESULT(OrganicCMineralized);
		
		if(!PARAMETER(IsSoil)) dCdt = 0.0; //TODO: make conditional exec instead?
			
		return LAST_RESULT(OrganicC) + dCdt;
	)
	
	EQUATION(Model, OrganicNLitter,
		return RESULT(OrganicCLitterEq) / PARAMETER(LitterCNRatio);
	)
	
	EQUATION(Model, OrganicNDecomposition,
		return SafeDivide(RESULT(OrganicCDecompositionEq), RESULT(CNRatio));
	)
	
	EQUATION(Model, OrganicNSolubilized,
		return SafeDivide(RESULT(OrganicCSolubilized), RESULT(CNRatio));
	)
	
	EQUATION(Model, OrganicNMineralized,
		return (1.0 - PARAMETER(Solubilization)) * (1.0 - PARAMETER(NUseEfficiency)) * RESULT(OrganicNDecomposition);
	)
	
	EQUATION(Model, DesiredNImmobilisation,
		double available_n_in_decomp = (1.0 - PARAMETER(Solubilization)) * PARAMETER(NUseEfficiency) * RESULT(OrganicNDecomposition);
		double desired_n             = RESULT(OrganicCInBiomass) / PARAMETER(MicrobeCNRatio);
		return std::max(0.0, desired_n - available_n_in_decomp);
	)
	
	EQUATION(Model, NitrificationEq,
		double nitr = PARAMETER(Nitrification);
		double result = nitr * RESULT(FractionOfYear);
		double in = RESULT(NH4Inputs);
		if(nitr < 0.0)
			result = -nitr*0.01*in;
		else
			result = std::min(result, in);
		return result;
	)
	
	EQUATION(Model, DenitrificationEq,
		double denitr = PARAMETER(Denitrification);
		double result = denitr * RESULT(FractionOfYear);
		double in = RESULT(NO3Inputs);
		if(denitr < 0.0)
			result = -denitr*0.01*in;
		else
			result = std::min(result, in);
		return result;
	)
	
	EQUATION(Model, DesiredNO3Uptake,
		return RESULT(FractionOfYear) * (PARAMETER(UptakeR0) / (1.0 + PARAMETER(NH4UptakeScale))) * std::pow(PARAMETER(UptakeQ10), RESULT(Temperature) * 0.1);
	)
	
	EQUATION(Model, DesiredNH4Uptake,
		return RESULT(DesiredNO3Uptake) * PARAMETER(NH4UptakeScale);
	)
	
	EQUATION(Model, NO3Inputs,
		return RESULT(NO3BasicInputs) + RESULT(NitrificationEq);
	)
	
	EQUATION(Model, NH4Inputs,
		return RESULT(NH4BasicInputs) + RESULT(OrganicNMineralized);
	)
	
	
	
	EQUATION(Model, NO3Uptake,
		return RESULT(DesiredNO3Uptake); //TODO: Must be limited by availabillity!
	)
	
	EQUATION(Model, NH4Uptake,
		return RESULT(DesiredNH4Uptake); //TODO: Must be limited by availability!
	)
	
	EQUATION(Model, NO3Immobilisation,
		return RESULT(DesiredNImmobilisation);    //TODO: Must be limited by availability!
	)
	
	EQUATION(Model, NH4Immobilisation,
		return 0.0;
	)
	
	
	EQUATION(Model, OrganicNUptake,
		return 
			  RESULT(DesiredNO3Uptake) - RESULT(NO3Uptake)
			+ RESULT(DesiredNH4Uptake) - RESULT(NH4Uptake);  //TODO!
	)
	
	EQUATION(Model, NO3ProcessesLoss,
		return RESULT(DenitrificationEq) + RESULT(NO3Immobilisation) + RESULT(NO3Uptake);
	)
	
	EQUATION(Model, NH4ProcessesLoss,
		return RESULT(NitrificationEq) + RESULT(NH4Immobilisation) + RESULT(NH4Uptake);
	)
	
	
	EQUATION(Model, OrganicN,
		double dNdt =
			  RESULT(OrganicNLitter)
			- RESULT(OrganicNSolubilized)
			- RESULT(OrganicNMineralized)
			- RESULT(OrganicNUptake)
			+ RESULT(NO3Immobilisation)
			+ RESULT(NH4Immobilisation);
		
		if(!PARAMETER(IsSoil)) dNdt = 0.0; //TODO: make conditional exec instead?
			
		return LAST_RESULT(OrganicN) + dNdt;
	)
	
	
	EQUATION(Model, OrganicP,
		return 1.0; //TODO!
	)
}