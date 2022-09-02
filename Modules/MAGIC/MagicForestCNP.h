

static void
AddMagicForestCNPModel(mobius_model *Model)
{
	
	BeginModule(Model, "MAGIC Forest CNP", "0.0.1");
	SetModuleDescription(Model, R""""(
A CNP-module for MAGIC Forest. Based on previous MAGIC CN model developed by Bernard J. Cosby.
)"""");

	auto Dimensionless    = RegisterUnit(Model);
	auto MolPerM2         = RegisterUnit(Model, "mol/m2");
	auto MMolPerM2        = RegisterUnit(Model, "mmol/m2");
	auto MMolPerM2PerTs   = RegisterUnit(Model, "mmol/m2/month");
	auto MMolPerM2PerYear = RegisterUnit(Model, "mmol/m2/year");
	
	
	auto Compartment               = GetIndexSetHandle(Model, "Compartment");
	auto CNPPar                    = RegisterParameterGroup(Model, "C, N, and P", Compartment);
	
	
	auto NUseEfficiency               = RegisterParameterDouble(Model, CNPPar, "N use efficiency", Dimensionless, 0.0, 0.0, 1.0, "Fraction of non-solubilized decomposed organic N that becomes biomass and is returned to the organic N pool. The rest is mineralized as NH4.");
	auto PUseEfficiency               = RegisterParameterDouble(Model, CNPPar, "P use efficiency", Dimensionless, 0.0, 0.0, 1.0, "Fraction of non-solubilized decomposed organic P that becomes biomass and is returned to the organic P pool. The rest is mineralized as PO4.");
	
	auto LitterCNRatio                = RegisterParameterDouble(Model, CNPPar, "C/N ratio of litter", Dimensionless, 10, 0.1, 1000.0, "Only for litter that is not computed by the forest module");
	auto LitterCPRatio                = RegisterParameterDouble(Model, CNPPar, "C/P ratio of litter", Dimensionless, 10, 0.1, 1000.0, "Only for litter that is not computed by the forest module");
	
	auto InitialPoolCN                = RegisterParameterDouble(Model, CNPPar, "Initial pool C/N ratio", MolPerM2, 10, 0.1, 1000);
	auto InitialPoolCP                = RegisterParameterDouble(Model, CNPPar, "Initial pool C/P ratio", MolPerM2, 10, 0.1, 1000);


	auto Nitrification                = RegisterParameterDouble(Model, CNPPar, "Nitrification", MMolPerM2PerYear, 0.0, 0.0, 500.0, "NH4->NO3. Negative rate sets value as % of inputs");
	auto Denitrification              = RegisterParameterDouble(Model, CNPPar, "Denitrification", MMolPerM2PerYear, 0.0, 0.0, 500.0, "NO3->N2. Negative rate sets value as % of inputs");
	
	auto UptakeR0                     = RegisterParameterDouble(Model, CNPPar, "N uptake at 0°C", MMolPerM2PerYear, 0.0, 0.0, 1000.0, "Uptake in addition to what is computed by the forest module");
	auto PUptakeR0                    = RegisterParameterDouble(Model, CNPPar, "P uptake at 0°C", MMolPerM2PerYear, 0.0, 0.0, 1000.0, "Uptake in addition to what is computed by the forest module");
	auto UptakeQ10                    = RegisterParameterDouble(Model, CNPPar, "Uptake Q10", Dimensionless, 1.0, 1.0, 5.0);
	auto NH4UptakeScale               = RegisterParameterDouble(Model, CNPPar, "NH4 uptake scale", Dimensionless, 1.0, 0.0, 1.0, "Proportion of NH4 uptake of total inorganic N uptake");

	auto RetentionModel               = RegisterParameterEnum(Model, CNPPar, "Retention model", {"Simple", "Gundersen", "Microbial"}, "Microbial");
	auto Simple          = EnumValue(Model, RetentionModel, "Simple");
	auto Gundersen       = EnumValue(Model, RetentionModel, "Gundersen");
	auto Microbial       = EnumValue(Model, RetentionModel, "Microbial");
	
	
	auto DesiredNO3Retention          = RegisterParameterDouble(Model, CNPPar, "Desired NO3 immobilisation", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Simple only. Negative rate sets value as % of inputs");
	auto DesiredNH4Retention          = RegisterParameterDouble(Model, CNPPar, "Desired NH4 immobilisation", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Simple only. Negative rate sets value as % of inputs");
	auto DesiredPO4Retention          = RegisterParameterDouble(Model, CNPPar, "Desired PO4 immobilisation", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Simple only. Negative rate sets value as % of inputs");
	auto NMineralization              = RegisterParameterDouble(Model, CNPPar, "N mineralization", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Simple only. Organic N -> NH4");
	auto PMineralization              = RegisterParameterDouble(Model, CNPPar, "P mineralization", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Simple only. Organic P -> PO4");
	auto MicrobeCNRatio               = RegisterParameterDouble(Model, CNPPar, "C/N ratio of soil microbial community", Dimensionless, 0.1, 0.0001, 10.0, "Gundersen and microbial only.");
	auto MicrobeCPRatio               = RegisterParameterDouble(Model, CNPPar, "C/P ratio of soil microbial community", Dimensionless, 0.1, 0.0001, 10.0, "Gundersen and microbial only.");
	auto LowerCNThresholdForNO3Immobilisation = RegisterParameterDouble(Model, CNPPar, "Lower C/N threshold for NO3 immobilisation", Dimensionless, 30.0, 0.0, 100.0,
	"Gundersen only. C/N below this value - 0% NO3 immobilisation");
	auto UpperCNThresholdForNO3Immobilisation = RegisterParameterDouble(Model, CNPPar, "Upper C/N threshold for NO3 immobilisation", Dimensionless, 30.0, 0.0, 100.0, "Gundersen only. C/N above this value - 100% NO3 immobilisation");
	auto LowerCNThresholdForNH4Immobilisation = RegisterParameterDouble(Model, CNPPar, "Lower C/N threshold for NH4 immobilisation", Dimensionless, 30.0, 0.0, 100.0,
	"Gundersen only. C/N below this value - 0% NH4 immobilisation");
	auto UpperCNThresholdForNH4Immobilisation = RegisterParameterDouble(Model, CNPPar, "Upper C/N threshold for NH4 immobilisation", Dimensionless, 30.0, 0.0, 100.0, "Gundersen only. C/N above this value - 100% NH4 immobilisation");
	auto LowerCPThresholdForPO4Immobilisation = RegisterParameterDouble(Model, CNPPar, "Lower C/P threshold for PO4 immobilisation", Dimensionless, 30.0, 0.0, 100.0,
	"Gundersen only. C/P below this value - 0% PO4 immobilisation");
	auto UpperCPThresholdForPO4Immobilisation = RegisterParameterDouble(Model, CNPPar, "Upper C/P threshold for PO4 immobilisation", Dimensionless, 30.0, 0.0, 100.0, "Gundersen only. C/P above this value - 100% PO4 immobilisation");

	auto DoImmobilisation             = RegisterParameterBool(Model, CNPPar, "Microbes immobilize inorganic N and P if necessary", true);
	auto PlantsUseInorganicFirst      = RegisterParameterBool(Model, CNPPar, "Plants use inorganic N and P before soil microbes", true);
	auto PlantsUseOrganic             = RegisterParameterBool(Model, CNPPar, "Plants have access to organic N and P", true);

	
	auto InitialOrganicNScaled     = RegisterEquationInitialValue(Model, "Initial organic N", MMolPerM2);
	auto OrganicN                  = RegisterEquation(Model, "Organic N", MMolPerM2);
	SetInitialValue(Model, OrganicN, InitialOrganicNScaled);
	
	auto InitialOrganicPScaled     = RegisterEquationInitialValue(Model, "Initial organic P", MMolPerM2);
	auto OrganicP                  = RegisterEquation(Model, "Organic P", MMolPerM2);
	SetInitialValue(Model, OrganicP, InitialOrganicPScaled);
	
	auto CNRatio                   = RegisterEquation(Model, "Pool C/N ratio", Dimensionless);
	auto CPRatio                   = RegisterEquation(Model, "Pool C/P ratio", Dimensionless);

	auto OrganicNDecomposition    = RegisterEquation(Model, "Organic N decomposition", MMolPerM2PerTs);
	auto OrganicNSolubilized      = RegisterEquation(Model, "Organic N solubilized", MMolPerM2PerTs);
	auto OrganicNMineralized      = RegisterEquation(Model, "Organic N mineralized", MMolPerM2PerTs);
	auto DesiredNImmobilisation   = RegisterEquation(Model, "Desired N immobilization (microbial model only)", MMolPerM2PerTs);
	
	
	auto DesiredNO3Immobilisation = RegisterEquation(Model, "Desired NO3 immobilisation", MMolPerM2PerTs);
	auto DesiredNH4Immobilisation = RegisterEquation(Model, "Desired NH4 immobilisation", MMolPerM2PerTs);
	
	auto DesiredNO3Uptake         = RegisterEquation(Model, "Desired NO3 uptake", MMolPerM2PerTs);
	auto DesiredNH4Uptake         = RegisterEquation(Model, "Desired NH4 uptake", MMolPerM2PerTs);
	
	auto NO3Immobilisation        = RegisterEquation(Model, "NO3 immobilisation", MMolPerM2PerTs);
	auto NH4Immobilisation        = RegisterEquation(Model, "NH4 immobilisation", MMolPerM2PerTs);
	
	auto NO3Uptake                = RegisterEquation(Model, "NO3 uptake", MMolPerM2PerTs);
	auto NH4Uptake                = RegisterEquation(Model, "NH4 uptake", MMolPerM2PerTs);
	
	auto OrganicNUptake           = RegisterEquation(Model, "Organic N uptake", MMolPerM2PerTs);
	
	auto NitrificationEq          = RegisterEquation(Model, "Nitrification", MMolPerM2PerTs);
	auto DenitrificationEq        = RegisterEquation(Model, "Denitrification", MMolPerM2PerTs);
	
	//auto OrganicPLitter           = RegisterEquation(Model, "Organic P litter", MMolPerM2PerTs);
	auto OrganicPDecomposition    = RegisterEquation(Model, "Organic P decomposition", MMolPerM2PerTs);
	auto OrganicPSolubilized      = RegisterEquation(Model, "Organic P solubilized", MMolPerM2PerTs);
	auto OrganicPMineralized      = RegisterEquation(Model, "Organic P mineralized", MMolPerM2PerTs);
	auto DesiredPImmobilisation   = RegisterEquation(Model, "Desired P immobilization", MMolPerM2PerTs);
	auto DesiredPUptake           = RegisterEquation(Model, "Desired P uptake", MMolPerM2PerTs);
	auto PO4Immobilisation        = RegisterEquation(Model, "PO4 immobilization", MMolPerM2PerTs);
	auto PO4Uptake                = RegisterEquation(Model, "PO4 uptake", MMolPerM2PerTs);
	auto OrganicPUptake           = RegisterEquation(Model, "Organic P uptake", MMolPerM2PerTs);
	
	
	//NOTE: The following 6 are required as an "interface" to the rest of the MAGICForest model
	auto NO3Inputs                = RegisterEquation(Model, "NO3 inputs", MMolPerM2PerTs);
	auto NH4Inputs                = RegisterEquation(Model, "NH4 inputs", MMolPerM2PerTs);
	auto PO4Inputs                = RegisterEquation(Model, "PO4 inputs", MMolPerM2PerTs);
	auto NO3ProcessesLoss         = RegisterEquation(Model, "NO3 processes loss", MMolPerM2PerTs);
	auto NH4ProcessesLoss         = RegisterEquation(Model, "NH4 processes loss", MMolPerM2PerTs);	
	auto PO4ProcessesLoss         = RegisterEquation(Model, "PO4 processes loss", MMolPerM2PerTs);
	
	auto NO3Inflow                = GetEquationHandle(Model, "NO3 input from other compartments");
	auto NH4Inflow                = GetEquationHandle(Model, "NH4 input from other compartments");
	auto PO4Inflow                = GetEquationHandle(Model, "PO4 input from other compartments");
	
	
	//From soil carbon module.
	auto OrganicCDecompositionEq = GetEquationHandle(Model, "Organic C turnover");
	auto OrganicC                = GetEquationHandle(Model, "Organic C");
	auto OrganicCSolubilized     = GetEquationHandle(Model, "Organic C solubilized");
	auto OrganicCInBiomass       = GetEquationHandle(Model, "Organic C in soil microbial biomass");
	auto OrganicCLitter          = GetParameterDoubleHandle(Model, "Organic C litter");
	
	auto Solubilization          = GetParameterDoubleHandle(Model, "Solubilization");
	
	auto IsSoil                    = GetParameterBoolHandle(Model, "This is a soil compartment");
	auto IsTop                     = GetParameterBoolHandle(Model, "This is a top compartment");
	
	auto FractionOfYear            = GetEquationHandle(Model, "Fraction of year");
	auto Temperature               = GetEquationHandle(Model, "Temperature");
	auto NO3BasicInputs            = GetEquationHandle(Model, "NO3 non-process inputs");
	auto NH4BasicInputs            = GetEquationHandle(Model, "NH4 non-process inputs");
	auto PO4BasicInputs            = GetEquationHandle(Model, "PO4 non-process inputs");
	
	// From the forest decomp-uptake module
	auto TotalTreeNUptake       = GetEquationHandle(Model, "Total tree N uptake");
	auto TotalTreePUptake       = GetEquationHandle(Model, "Total tree P uptake");
	auto TotalTreeDecompCSource = GetEquationHandle(Model, "Total C source from tree decomposition");
	auto TotalTreeDecompNSource = GetEquationHandle(Model, "Total N source from tree decomposition");
	auto TotalTreeDecompPSource = GetEquationHandle(Model, "Total P source from tree decomposition");
	

	
	EQUATION(Model, InitialOrganicNScaled,
		//WarningPrint("Organic C is ", RESULT(OrganicC), " init cn is ", PARAMETER(InitialPoolCN));
		return RESULT(OrganicC) / PARAMETER(InitialPoolCN);
	)
	
	EQUATION(Model, InitialOrganicPScaled,
		return RESULT(OrganicC) / PARAMETER(InitialPoolCP);
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
	
	EQUATION(Model, OrganicNDecomposition,
		return SafeDivide(RESULT(OrganicCDecompositionEq), RESULT(CNRatio));
	)
	
	EQUATION(Model, OrganicNSolubilized,
		return SafeDivide(RESULT(OrganicCSolubilized), RESULT(CNRatio));
	)
	
	EQUATION(Model, OrganicNMineralized,
		double microbial = (1.0 - PARAMETER(Solubilization)) * (1.0 - PARAMETER(NUseEfficiency)) * RESULT(OrganicNDecomposition);
		
		double simple = PARAMETER(NMineralization)*RESULT(FractionOfYear);
		
		u64 retmodel = PARAMETER(RetentionModel);
		
		if(CURRENT_TIMESTEP()==-1) return 0.0;  //NOTE: Stopgap because we don't compute forest uptake in the initial step to balance this..
		
		if(retmodel == Simple)
			return simple;
		else
			return microbial;   // Gundersen and microbial are the same for this value
	)
	
	EQUATION(Model, DesiredNImmobilisation,
		double available_n_in_decomp = (1.0 - PARAMETER(Solubilization)) * PARAMETER(NUseEfficiency) * RESULT(OrganicNDecomposition);
		double desired_n             = RESULT(OrganicCInBiomass) / PARAMETER(MicrobeCNRatio);
		double potential = std::max(0.0, desired_n - available_n_in_decomp);
		
		if(!PARAMETER(DoImmobilisation))
			potential = 0.0;
		
		if(PARAMETER(RetentionModel) != Microbial)
			potential = 0.0;
		
		return potential;
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
		double treeuptake = RESULT(TotalTreeNUptake);
		if(!PARAMETER(IsTop) || !PARAMETER(IsSoil) || CURRENT_TIMESTEP()==-1) treeuptake = 0.0;        //Hmm, plantuptake should maybe also be 0. But trees could have deep roots ideally
		double plantuptake = RESULT(FractionOfYear) * PARAMETER(UptakeR0) * std::pow(PARAMETER(UptakeQ10), RESULT(Temperature) * 0.1);
		return (plantuptake + treeuptake) * (1.0 - PARAMETER(NH4UptakeScale));
	)
	
	EQUATION(Model, DesiredNH4Uptake,
		double treeuptake = RESULT(TotalTreeNUptake);
		if(!PARAMETER(IsTop) || !PARAMETER(IsSoil) || CURRENT_TIMESTEP()==-1) treeuptake = 0.0;        //Hmm, plantuptake should maybe also be 0. But trees could have deep roots ideally
		double plantuptake = RESULT(FractionOfYear) * PARAMETER(UptakeR0) * std::pow(PARAMETER(UptakeQ10), RESULT(Temperature) * 0.1);
		return (plantuptake + treeuptake) * PARAMETER(NH4UptakeScale);
	)
	
	EQUATION(Model, NO3Inputs,
		return RESULT(NO3BasicInputs) + RESULT(NitrificationEq);
	)
	
	EQUATION(Model, NH4Inputs,
		return RESULT(NH4BasicInputs) + RESULT(OrganicNMineralized);
	)
	
	
	EQUATION(Model, DesiredNO3Immobilisation,
		double inno3 = RESULT(NO3Inputs) - RESULT(DenitrificationEq) + RESULT(NO3Inflow);
		double innh4 = RESULT(NH4Inputs) - RESULT(NitrificationEq)   + RESULT(NH4Inflow);
		double microbial = inno3 * SafeDivide(RESULT(DesiredNImmobilisation), inno3 + innh4);
		
		double simple = PARAMETER(DesiredNO3Retention);
		double result_simple = simple * RESULT(FractionOfYear);
		if(simple < 0.0)
			result_simple = -simple*0.01*inno3;
		
		double gundersen = inno3 * LinearResponse(RESULT(CNRatio), PARAMETER(LowerCNThresholdForNO3Immobilisation), PARAMETER(UpperCNThresholdForNO3Immobilisation), 0.0, 1.0);
		
		if(!PARAMETER(DoImmobilisation)) return 0.0;
		
		u64 retmodel = PARAMETER(RetentionModel);
		if(retmodel == Simple)
			return result_simple;
		else if(retmodel == Gundersen)
			return gundersen;
		else
			return microbial;
	)
	
	EQUATION(Model, DesiredNH4Immobilisation,
		double inno3 = RESULT(NO3Inputs) - RESULT(DenitrificationEq) + RESULT(NO3Inflow);
		double innh4 = RESULT(NH4Inputs) - RESULT(NitrificationEq)   + RESULT(NH4Inflow);
		double microbial = innh4 * SafeDivide(RESULT(DesiredNImmobilisation), inno3 + innh4);
		
		double simple = PARAMETER(DesiredNH4Retention);
		double result_simple = simple * RESULT(FractionOfYear);
		if(simple < 0.0)
			result_simple = -simple*0.01*innh4;
		
		double gundersen = innh4 * LinearResponse(RESULT(CNRatio), PARAMETER(LowerCNThresholdForNH4Immobilisation), PARAMETER(UpperCNThresholdForNH4Immobilisation), 0.0, 1.0);
		
		u64 retmodel = PARAMETER(RetentionModel);
		
		if(!PARAMETER(DoImmobilisation)) return 0.0;
		
		if(retmodel == Simple)
			return result_simple;
		else if(retmodel == Gundersen)
			return gundersen;
		else
			return microbial;
	)
	
	
	EQUATION(Model, NO3Uptake,
		double in             = RESULT(NO3Inputs) - RESULT(DenitrificationEq) + RESULT(NO3Inflow);
		double desired_uptake = RESULT(DesiredNO3Uptake);
		double desired_immob  = std::min(in, RESULT(DesiredNO3Immobilisation));
		if(!PARAMETER(PlantsUseInorganicFirst))
			in -= desired_immob;
		return std::min(in, desired_uptake);
	)
	
	EQUATION(Model, NH4Uptake,
		double in             = RESULT(NH4Inputs) - RESULT(NitrificationEq) + RESULT(NH4Inflow);
		double desired_uptake = RESULT(DesiredNH4Uptake);
		double desired_immob  = std::min(in, RESULT(DesiredNH4Immobilisation));
		if(!PARAMETER(PlantsUseInorganicFirst))
			in -= desired_immob;
		return std::min(in, desired_uptake);
	)
	
	EQUATION(Model, NO3Immobilisation,
		double in             = RESULT(NO3Inputs) - RESULT(DenitrificationEq) + RESULT(NO3Inflow);
		double desired_immob  = RESULT(DesiredNO3Immobilisation);
		double desired_uptake = std::min(in, RESULT(DesiredNO3Uptake));
		if(PARAMETER(PlantsUseInorganicFirst))
			in -= desired_uptake;
		return std::min(in, desired_immob);
	)
	
	EQUATION(Model, NH4Immobilisation,
		double in             = RESULT(NH4Inputs) - RESULT(NitrificationEq) + RESULT(NH4Inflow);
		double desired_immob  = RESULT(DesiredNH4Immobilisation);
		double desired_uptake = std::min(in, RESULT(DesiredNH4Uptake));
		if(PARAMETER(PlantsUseInorganicFirst))
			in -= desired_uptake;
		return std::min(in, desired_immob);
	)
	
	EQUATION(Model, OrganicNUptake,
		double potential = 
			  RESULT(DesiredNO3Uptake) - RESULT(NO3Uptake)
			+ RESULT(DesiredNH4Uptake) - RESULT(NH4Uptake);
		potential = std::max(0.0, potential);
		
		if(!PARAMETER(PlantsUseOrganic))
			potential = 0.0;
		
		return std::min(potential, LAST_RESULT(OrganicN));	  
	)
	
	EQUATION(Model, NO3ProcessesLoss,
		return RESULT(DenitrificationEq) + RESULT(NO3Immobilisation) + RESULT(NO3Uptake);
	)
	
	EQUATION(Model, NH4ProcessesLoss,
		return RESULT(NitrificationEq) + RESULT(NH4Immobilisation) + RESULT(NH4Uptake);
	)
	
	
	EQUATION(Model, OrganicN,
		double forest = RESULT(TotalTreeDecompNSource);
		if(!PARAMETER(IsTop) || !PARAMETER(IsSoil)) forest = 0.0;
	
		double dNdt =
			  PARAMETER(OrganicCLitter) / PARAMETER(LitterCNRatio)
			+ forest
			- RESULT(OrganicNSolubilized)
			- RESULT(OrganicNMineralized)
			- RESULT(OrganicNUptake)
			+ RESULT(NO3Immobilisation)
			+ RESULT(NH4Immobilisation);
		
		if(!PARAMETER(IsSoil)) dNdt = 0.0; //TODO: make conditional exec instead?
			
		return LAST_RESULT(OrganicN) + dNdt;
	)
	
	EQUATION(Model, OrganicPDecomposition,
		return SafeDivide(RESULT(OrganicCDecompositionEq), RESULT(CPRatio));
	)
	
	EQUATION(Model, OrganicPSolubilized,
		return SafeDivide(RESULT(OrganicCSolubilized), RESULT(CPRatio));
	)
	
	EQUATION(Model, OrganicPMineralized,
		double microbial = (1.0 - PARAMETER(Solubilization)) * (1.0 - PARAMETER(PUseEfficiency)) * RESULT(OrganicPDecomposition);
		
		double simple = PARAMETER(PMineralization)*RESULT(FractionOfYear);
		
		u64 retmodel = PARAMETER(RetentionModel);
		
		if(CURRENT_TIMESTEP()==-1) return 0.0;  //NOTE: Stopgap because we don't compute forest uptake in the initial step to balance this..
		
		if(retmodel == Simple)
			return simple;
		else
			return microbial;   // Gundersen and microbial are the same for this value
	)
	
	EQUATION(Model, DesiredPImmobilisation,
		double available_p_in_decomp = (1.0 - PARAMETER(Solubilization)) * PARAMETER(PUseEfficiency) * RESULT(OrganicPDecomposition);
		double desired_p             = RESULT(OrganicCInBiomass) / PARAMETER(MicrobeCPRatio);
		double microbial = std::max(0.0, desired_p - available_p_in_decomp);
		
		double inpo4 = RESULT(PO4Inputs);
		
		double simple = PARAMETER(DesiredPO4Retention);
		double result_simple = simple * RESULT(FractionOfYear);
		if(simple < 0.0)
			result_simple = -simple*0.01*inpo4;
		
		double gundersen = inpo4 * LinearResponse(RESULT(CNRatio), PARAMETER(LowerCPThresholdForPO4Immobilisation), PARAMETER(UpperCPThresholdForPO4Immobilisation), 0.0, 1.0);
		
		u64 retmodel = PARAMETER(RetentionModel);
		
		if(!PARAMETER(DoImmobilisation)) return 0.0;
		
		if(retmodel == Simple)
			return result_simple;
		else if(retmodel == Gundersen)
			return gundersen;
		else
			return microbial;
	)
	
	EQUATION(Model, DesiredPUptake,
		double treeuptake  = RESULT(TotalTreePUptake);
		if(!PARAMETER(IsTop) || !PARAMETER(IsSoil)) treeuptake = 0.0;
		double plantuptake = RESULT(FractionOfYear) * PARAMETER(PUptakeR0) * std::pow(PARAMETER(UptakeQ10), RESULT(Temperature) * 0.1);
		return plantuptake + treeuptake;
	)
	
	EQUATION(Model, PO4Uptake,
		double in             = RESULT(PO4Inputs) + RESULT(PO4Inflow);
		double desired_uptake = RESULT(DesiredPUptake);
		double desired_immob  = std::min(in, RESULT(DesiredPImmobilisation));
		if(!PARAMETER(PlantsUseInorganicFirst))
			in -= desired_immob;
		return std::min(in, desired_uptake);
	)
	
	EQUATION(Model, PO4Immobilisation,
		double in             = RESULT(PO4Inputs) + RESULT(PO4Inflow);
		double desired_immob  = RESULT(DesiredPImmobilisation);
		double desired_uptake = std::min(in, RESULT(DesiredPUptake));
		if(PARAMETER(PlantsUseInorganicFirst))
			in -= desired_uptake;
		return std::min(in, desired_immob);
	)
	
	EQUATION(Model, OrganicPUptake,
		double potential = std::max(0.0, RESULT(DesiredPUptake) - RESULT(PO4Uptake));
		
		if(!PARAMETER(PlantsUseOrganic))
			potential = 0.0;
		
		return std::min(potential, LAST_RESULT(OrganicP));	  
	)
	
	EQUATION(Model, OrganicP,
		double dPdt =
			  PARAMETER(OrganicCLitter) / PARAMETER(LitterCPRatio)
			- RESULT(OrganicPSolubilized)
			- RESULT(OrganicPMineralized)
			- RESULT(OrganicPUptake)
			+ RESULT(PO4Immobilisation);
		
		if(!PARAMETER(IsSoil)) dPdt = 0.0; //TODO: make conditional exec instead?
			
		return LAST_RESULT(OrganicP) + dPdt;
	)
	
	
	EQUATION(Model, PO4Inputs,
		double forest = RESULT(TotalTreeDecompPSource);
		if(!PARAMETER(IsTop) || !PARAMETER(IsSoil)) forest = 0.0;
		return RESULT(PO4BasicInputs) + RESULT(OrganicPMineralized) + forest;
	)
	
	EQUATION(Model, PO4ProcessesLoss,
		return RESULT(PO4Immobilisation) + RESULT(PO4Uptake);
	)
}