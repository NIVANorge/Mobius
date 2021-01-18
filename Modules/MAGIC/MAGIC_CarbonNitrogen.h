


//TODO: There is some overlap between these. We should factor some of it out.


static void
AddSimpleMagicCarbonNitrogenModel(mobius_model *Model)
{
	
	BeginModule(Model, "MAGIC simple carbon and nitrogen", "_dev");
	SetModuleDescription(Model, R""""(
This is a basic module that provides Carbon and Nitrogen dynamics as drivers for the MAGIC core.
)"""");
	
	auto MMolPerM2PerYear = RegisterUnit(Model, "mmol/m2/year");
	auto MMolPerM2PerTs   = RegisterUnit(Model, "mmol/m2/month");
	
	auto Compartment        = GetIndexSetHandle(Model, "Compartment");
	
	auto CAndN              = RegisterParameterGroup(Model, "Carbon and Nitrogen by compartment", Compartment);
	
	auto Nitrification      = RegisterParameterDouble(Model, CAndN, "Nitrification", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs");
	auto Denitrification    = RegisterParameterDouble(Model, CAndN, "Denitrification", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs");
	auto NO3Immobilisation  = RegisterParameterDouble(Model, CAndN, "NO3 immobilisation", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs");
	auto NH4Immobilisation  = RegisterParameterDouble(Model, CAndN, "NH4 immobilisation", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs");
	auto Mineralisation     = RegisterParameterDouble(Model, CAndN, "Mineralisation", MMolPerM2PerYear, 0.0, 0.0, 500.0);
	
	//NOTE: The following 4 are required as an "interface" to the rest of the MAGIC model
	auto NO3Inputs           = RegisterEquation(Model, "NO3 inputs", MMolPerM2PerTs);
	auto NH4Inputs           = RegisterEquation(Model, "NH4 inputs", MMolPerM2PerTs);
	auto NO3ProcessesLoss    = RegisterEquation(Model, "NO3 processes loss", MMolPerM2PerTs);
	auto NH4ProcessesLoss    = RegisterEquation(Model, "NH4 processes loss", MMolPerM2PerTs);
	
	auto NitrificationEq     = RegisterEquation(Model, "Nitrification", MMolPerM2PerTs);
	auto DenitrificationEq   = RegisterEquation(Model, "Denitrification", MMolPerM2PerTs);
	auto NO3ImmobilisationEq = RegisterEquation(Model, "NO3 immobilisation", MMolPerM2PerTs);
	auto NH4ImmobilisationEq = RegisterEquation(Model, "NH4 immobilisation", MMolPerM2PerTs);
	auto MineralisationEq    = RegisterEquation(Model, "Mineralisation", MMolPerM2PerTs);
	
	auto FractionOfYear = GetEquationHandle(Model, "Fraction of year");
	auto NO3BasicInputs = GetEquationHandle(Model, "NO3 basic inputs");
	auto NH4BasicInputs = GetEquationHandle(Model, "NH4 basic inputs");
	
	EQUATION(Model, NO3Inputs,
		return RESULT(NO3BasicInputs) + RESULT(NitrificationEq);
	)
	
	EQUATION(Model, NH4Inputs,
		return RESULT(NH4BasicInputs) + RESULT(MineralisationEq);
	)
	
	EQUATION(Model, MineralisationEq,
		return RESULT(FractionOfYear) * PARAMETER(Mineralisation);
	)
	
	EQUATION(Model, NitrificationEq,
		double nitr = PARAMETER(Nitrification);
		double result = nitr * RESULT(FractionOfYear);
		double in = RESULT(NH4Inputs);
		if(nitr < 0.0) result = -nitr*0.01*in;
		return result;
	)
	
	EQUATION(Model, DenitrificationEq,
		double denitr = PARAMETER(Denitrification);
		double result = denitr * RESULT(FractionOfYear);
		double in = RESULT(NO3Inputs);
		if(denitr < 0.0) result = -denitr*0.01*in;
		return result;
	)
	
	EQUATION(Model, NO3ImmobilisationEq,
		double immob = PARAMETER(NO3Immobilisation);
		double result = immob * RESULT(FractionOfYear);
		double in = RESULT(NO3Inputs);
		if(immob < 0.0) result = -immob*0.01*in;
		return result;
	)
	
	EQUATION(Model, NH4ImmobilisationEq,
		double immob = PARAMETER(NH4Immobilisation);
		double result = immob * RESULT(FractionOfYear);
		double in = RESULT(NH4Inputs);
		if(immob < 0.0) result = -immob*0.01*in;
		return result;
	)
	
	EQUATION(Model, NO3ProcessesLoss,
		return RESULT(DenitrificationEq) + RESULT(NO3ImmobilisationEq);
	)
	
	EQUATION(Model, NH4ProcessesLoss,
		return RESULT(NitrificationEq) + RESULT(NH4ImmobilisationEq);
	)
	
	
	EndModule(Model);
}

static void
AddRatioMagicCarbonNitrogenModel(mobius_model *Model)
{
	AddSimpleMagicCarbonNitrogenModel(Model);
	
	BeginModule(Model, "MAGIC C/N ratio carbon and nitrogen", "_dev");
	
	auto Dimensionless    = RegisterUnit(Model);
	auto Percent          = RegisterUnit(Model, "%");
	auto MolPerM2         = RegisterUnit(Model, "mol/m2");
	auto MMolPerM2        = RegisterUnit(Model, "mmol/m2");
	auto MMolPerM2PerYear = RegisterUnit(Model, "mmol/m2/year");
	auto MMolPerM2PerTs   = RegisterUnit(Model, "mmol/m2/month");
	auto MolPerMol        = RegisterUnit(Model, "mol/mol");
	
	auto Compartment = GetIndexSetHandle(Model, "Compartment");
	//NOTE: this transfers ownership of this group from the simple module to this module.
	auto CAndN              = RegisterParameterGroup(Model, "Carbon and Nitrogen by compartment", Compartment);
	
	auto OrganicCInput                    = RegisterParameterDouble(Model, CAndN, "Organic C input", MMolPerM2PerYear, 0.0, 0.0, 1e6);
	auto OrganicCLitter                   = RegisterParameterDouble(Model, CAndN, "Organic C litter", MMolPerM2PerYear, 0.0, 0.0, 1e6);
	auto OrganicCSink                     = RegisterParameterDouble(Model, CAndN, "Organic C sink", MMolPerM2PerYear, 0.0, 0.0, 1e6);
	auto OrganicCDecomposition            = RegisterParameterDouble(Model, CAndN, "Organic C decomposition", MMolPerM2PerYear, 0.0, 0.0, 1e6);
	auto InitialOrganicC                  = RegisterParameterDouble(Model, CAndN, "Initial organic C", MolPerM2, 0.0, 0.0, 1e8);

	auto OrganicCNInputRatio              = RegisterParameterDouble(Model, CAndN, "Organic C/N input ratio", Dimensionless, 0.1, 0.0001, 100.0);
	auto OrganicCNLitterRatio             = RegisterParameterDouble(Model, CAndN, "Organic C/N litter ratio", Dimensionless, 0.1, 0.0001, 100.0);
	auto OrganicCNSinkRatio               = RegisterParameterDouble(Model, CAndN, "Organic C/N sink ratio", Dimensionless, 0.1, 0.0001, 100.0, "If 0, use the pool C/N instead");
	auto OrganicCNDecompositionRatio      = RegisterParameterDouble(Model, CAndN, "Organic C/N decomposition ratio", Dimensionless, 0.1, 0.0001, 100.0, "If 0, use the pool C/N instead");
	auto InitialOrganicN                  = RegisterParameterDouble(Model, CAndN, "Initial organic N", MolPerM2, 0.0, 0.0, 1e8);

	auto LowerCNThresholdForNO3Immobilisation = RegisterParameterDouble(Model, CAndN, "Lower C/N threshold for NO3 immobilisation", Percent, 0.5, 0.0, 5.0,
	"C/N below this value - 0% NO3 immobilisation");
	auto UpperCNThresholdForNO3Immobilisation = RegisterParameterDouble(Model, CAndN, "Upper C/N threshold for NO3 immobilisation", Percent, 0.5, 0.0, 5.0, "C/N above this value - 100% NO3 immobilisation");
	
	auto LowerCNThresholdForNH4Immobilisation = RegisterParameterDouble(Model, CAndN, "Lower C/N threshold for NH4 immobilisation", Percent, 0.5, 0.0, 5.0,
	"C/N below this value - 0% NH4 immobilisation");
	auto UpperCNThresholdForNH4Immobilisation = RegisterParameterDouble(Model, CAndN, "Upper C/N threshold for NH4 immobilisation", Percent, 0.5, 0.0, 5.0, "C/N above this value - 100% NH4 immobilisation");
	
	auto NO3PlantUptake                       = RegisterParameterDouble(Model, CAndN, "NO3 plant uptake", MMolPerM2PerYear, 0.0, 0.0, 1000.0);
	auto NH4PlantUptake                       = RegisterParameterDouble(Model, CAndN, "NH4 plant uptake", MMolPerM2PerYear, 0.0, 0.0, 1000.0);

	//NOTE: Mineralisation and immobilisation is instead determined by C/N pool
	HideParameter(Model, GetParameterHandle(Model, "Mineralisation")); 
	HideParameter(Model, GetParameterHandle(Model, "NO3 immobilisation"));
	HideParameter(Model, GetParameterHandle(Model, "NH4 immobilisation"));

	//NOTE: Takes ownership of this one from the simple module:
	auto MineralisationEq          = RegisterEquation(Model, "Mineralisation", MMolPerM2PerTs);

	auto InitialOrganicCScaled     = RegisterEquationInitialValue(Model, "Initial organic C", MMolPerM2);
	auto OrganicC                  = RegisterEquation(Model, "Organic C", MMolPerM2);
	SetInitialValue(Model, OrganicC, InitialOrganicCScaled);
	
	auto InitialOrganicNScaled     = RegisterEquationInitialValue(Model, "Initial organic N", MMolPerM2);
	auto OrganicN                  = RegisterEquation(Model, "Organic N", MMolPerM2);
	SetInitialValue(Model, OrganicN, InitialOrganicNScaled);

	auto NO3ImmobilisationFraction = RegisterEquation(Model, "NO3 immobilisation fraction", Dimensionless);
	auto NO3ImmobilisationEq       = RegisterEquation(Model, "NO3 immobilisation", MMolPerM2PerTs);
	auto NH4ImmobilisationFraction = RegisterEquation(Model, "NH4 immobilisation fraction", Dimensionless);
	auto NH4ImmobilisationEq       = RegisterEquation(Model, "NH4 immobilisation", MMolPerM2PerTs);
	auto CNRatio                   = RegisterEquation(Model, "Pool C/N ratio", Dimensionless);

	auto NO3UptakeEq               = RegisterEquation(Model, "NO3 plant uptake", MMolPerM2PerTs);
	auto NH4UptakeEq               = RegisterEquation(Model, "NH4 plant uptake", MMolPerM2PerTs);

	//NOTE: The following 4 are required as an "interface" to the rest of the MAGIC model
	// We take ownership of them from the simple model
	auto NO3Inputs           = RegisterEquation(Model, "NO3 inputs", MMolPerM2PerTs);
	auto NH4Inputs           = RegisterEquation(Model, "NH4 inputs", MMolPerM2PerTs);
	auto NO3ProcessesLoss    = RegisterEquation(Model, "NO3 processes loss", MMolPerM2PerTs);
	auto NH4ProcessesLoss    = RegisterEquation(Model, "NH4 processes loss", MMolPerM2PerTs);
	
	//NOTE: The following is provided by the driver
	auto FractionOfYear      = GetEquationHandle(Model, "Fraction of year");
	auto NO3BasicInputs      = GetEquationHandle(Model, "NO3 basic inputs");
	auto NH4BasicInputs      = GetEquationHandle(Model, "NH4 basic inputs");  
	
	
	EQUATION(Model, InitialOrganicCScaled,
		return PARAMETER(InitialOrganicC)*1000.0;
	)
	
	EQUATION(Model, InitialOrganicNScaled,
		return PARAMETER(InitialOrganicN)*1000.0;
	)
	
	EQUATION(Model, CNRatio,
		return SafeDivide(LAST_RESULT(OrganicC), LAST_RESULT(OrganicN));
	)
	
	EQUATION_OVERRIDE(Model, MineralisationEq,
		double decomp_CN = PARAMETER(OrganicCNDecompositionRatio);
		double pool_CN   = RESULT(CNRatio);
		if(decomp_CN == 0.0) decomp_CN = pool_CN;
		
		return RESULT(FractionOfYear) * PARAMETER(OrganicCDecomposition) / decomp_CN;
	)
	
	//TODO: Carbon fixation?
	
	EQUATION(Model, OrganicC,
		return 
		  LAST_RESULT(OrganicC)
		+ RESULT(FractionOfYear)*(
			  PARAMETER(OrganicCInput)
			+ PARAMETER(OrganicCLitter)
			- PARAMETER(OrganicCSink)
			- PARAMETER(OrganicCDecomposition));
	)
	
	
	EQUATION(Model, OrganicN,
		double sink_CN = PARAMETER(OrganicCNSinkRatio);
		double pool_CN = RESULT(CNRatio);
		if(sink_CN == 0.0) sink_CN = pool_CN;
	
		//TODO: Needs to be able to do uptake, immob etc in different orders.
	
		return
			  LAST_RESULT(OrganicN)
			+ RESULT(FractionOfYear)*(
				+ PARAMETER(OrganicCInput) / PARAMETER(OrganicCNInputRatio)
				+ PARAMETER(OrganicCLitter) / PARAMETER(OrganicCNLitterRatio)
				- PARAMETER(OrganicCSink)  / sink_CN
				)
			+ RESULT(NO3ImmobilisationEq)
			+ RESULT(NH4ImmobilisationEq)
			- RESULT(MineralisationEq);
	)
	
	EQUATION(Model, NO3UptakeEq,
		return RESULT(FractionOfYear) * PARAMETER(NO3PlantUptake);
	)
	
	EQUATION(Model, NH4UptakeEq,
		return RESULT(FractionOfYear) * PARAMETER(NH4PlantUptake);
	)
	
	EQUATION(Model, NO3ImmobilisationFraction,
		//TODO: There should be a retention potential.
		return LinearResponse(RESULT(CNRatio), PARAMETER(LowerCNThresholdForNO3Immobilisation)*0.01, PARAMETER(UpperCNThresholdForNO3Immobilisation)*0.01, 0.0, 1.0);
	)
	
	EQUATION(Model, NH4ImmobilisationFraction,
		return LinearResponse(RESULT(CNRatio), PARAMETER(LowerCNThresholdForNH4Immobilisation)*0.01, PARAMETER(UpperCNThresholdForNH4Immobilisation)*0.01, 0.0, 1.0);
	)
	
	EQUATION_OVERRIDE(Model, NH4ImmobilisationEq,
		return RESULT(FractionOfYear) * RESULT(NH4ImmobilisationFraction) * RESULT(NH4Inputs);
	)
	
	EQUATION_OVERRIDE(Model, NO3ImmobilisationEq,
		return RESULT(FractionOfYear) * RESULT(NO3ImmobilisationFraction) * RESULT(NO3Inputs);
	)
	
	/*
	//TODO: These seem to remain the same?
	
	EQUATION_OVERRIDE(Model, NO3Inputs,
		return RESULT(NO3BasicInputs) + RESULT(NitrificationEq);
	)
	
	EQUATION_OVERRIDE(Model, NH4Inputs,
		return RESULT(NH4BasicInputs) + RESULT(MineralisationEq);
	)
	*/
	
	auto DenitrificationEq = GetEquationHandle(Model, "Denitrification");
	auto NitrificationEq   = GetEquationHandle(Model, "Nitrification");
	
	EQUATION_OVERRIDE(Model, NO3ProcessesLoss,
		return RESULT(DenitrificationEq) + RESULT(NO3ImmobilisationEq) + RESULT(NO3UptakeEq);
	)
	
	EQUATION_OVERRIDE(Model, NH4ProcessesLoss,
		return RESULT(NitrificationEq) + RESULT(NH4ImmobilisationEq) + RESULT(NH4UptakeEq);
	)
	
	
	EndModule(Model);
}


static void
AddMicrobialMagicCarbonNitrogenModel(mobius_model *Model)
{
	BeginModule(Model, "MAGIC microbial carbon and nitrogen", "_dev");
	
	auto Dimensionless    = RegisterUnit(Model);
	auto Percent          = RegisterUnit(Model, "%");
	auto MolPerM2         = RegisterUnit(Model, "mol/m2");
	auto MMolPerM2        = RegisterUnit(Model, "mmol/m2");
	auto MMolPerM2PerYear = RegisterUnit(Model, "mmol/m2/year");
	auto MMolPerM2PerTs   = RegisterUnit(Model, "mmol/m2/month");
	auto MolPerMol        = RegisterUnit(Model, "mol/mol");
	
	auto Compartment        = GetIndexSetHandle(Model, "Compartment");
	
	auto CAndN              = RegisterParameterGroup(Model, "Carbon and Nitrogen by compartment", Compartment);
	
	auto BiomassCN          = RegisterParameterDouble(Model, CAndN, "Biomass C/N", Dimensionless, 10.0, 0.0, 1000.0);
	auto CarbEff            = RegisterParameterDouble(Model, CAndN, "Carbon efficiency", Percent, 25.0, 0.0, 100.0);
	auto NitrEff            = RegisterParameterDouble(Model, CAndN, "Nitrogen efficiency", Percent, 50.0, 0.0, 100.0);
	auto DecompEff          = RegisterParameterDouble(Model, CAndN, "Decomposition efficiency", Percent, 5.0, 0.0, 100.0);
	
	//auto PlantsAccessNitrogen    = RegisterParameterBool(Model, CAndN, "Plants have access to organic nitrogen", true);
	//auto PlantsUseInorganicFirst = RegisterParameterBool(Model, CAndN, "Plants use inorganic nitrogen before soil", true);
	//auto PoreWaterNAvailableForRetention
	//auto MicrobesUtilizeSoilNH4BeforeNO3
	auto InitialOrganicC    = RegisterParameterDouble(Model, CAndN, "Initial organic C", MolPerM2, 0.0, 0.0, 5000.0);
	auto InitialOrganicN    = RegisterParameterDouble(Model, CAndN, "Initial organic N", MolPerM2, 0.0, 0.0, 5000.0);
	
	auto PlantNH4Uptake     = RegisterParameterDouble(Model, CAndN, "Plant NH4 uptake", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs");
	auto PlantNO3Uptake     = RegisterParameterDouble(Model, CAndN, "Plant NO3 uptake", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs");
	auto Nitrification      = RegisterParameterDouble(Model, CAndN, "Nitrification", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs");
	auto Denitrification    = RegisterParameterDouble(Model, CAndN, "Denitrification", MMolPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs");
	
	auto OrganicCInput      = RegisterParameterDouble(Model, CAndN, "Organic C input", MMolPerM2PerYear, 0.0, 0.0, 50.0, "Organic C input");
	auto InputCN            = RegisterParameterDouble(Model, CAndN, "Input C/N", MolPerMol, 25.0, 0.01, 1000.0);
	auto OrganicCOutput     = RegisterParameterDouble(Model, CAndN, "Organic C output", MMolPerM2PerYear, 0.0, 0.0, 50.0, "Organic C output");
	auto OutputCN           = RegisterParameterDouble(Model, CAndN, "Output C/N", MolPerMol, 25.0, 0.01, 1000.0, "If value is 0, use pool C/N");
	auto OrganicCDecomp     = RegisterParameterDouble(Model, CAndN, "Organic C decomposition", MMolPerM2PerYear, 25e3, 0.0, 100e3);
	auto DecompCN           = RegisterParameterDouble(Model, CAndN, "Decomp C/N", MolPerMol, 0.0, 0.01, 1000.0, "If value is 0, use pool C/N");
	auto OrganicCLitter     = RegisterParameterDouble(Model, CAndN, "Organic C litter", MMolPerM2PerYear, 0.0, 0.0, 50e3, "Organic C litter");
	auto LitterCN           = RegisterParameterDouble(Model, CAndN, "Litter C/N", MolPerMol, 50.0, 0.01, 1000.0);
	auto ActualNRetentionPot = RegisterParameterDouble(Model, CAndN, "Actual N retention potential", Percent, 100.0, 0.0, 100.0);
	
	auto CompartmentSolver = GetSolverHandle(Model, "Compartment solver");
	
	auto OrganicC           = RegisterEquation(Model, "Organic C", MMolPerM2);
	auto InitialOrganicCScaled = RegisterEquationInitialValue(Model, "Initial organic C", MMolPerM2);
	SetInitialValue(Model, OrganicC, InitialOrganicCScaled);
	auto OrganicN           = RegisterEquation(Model, "Organic N", MMolPerM2);
	auto InitialOrganicNScaled = RegisterEquationInitialValue(Model, "Initial organic N", MMolPerM2);
	SetInitialValue(Model, OrganicN, InitialOrganicNScaled);
	
	auto PoolCN             = RegisterEquation(Model, "Pool C/N", MolPerMol);
	auto InitialPoolCN      = RegisterEquationInitialValue(Model, "Initial pool C/N", MolPerMol);
	SetInitialValue(Model, PoolCN, InitialPoolCN);
	
	EQUATION(Model, InitialOrganicCScaled,
		return PARAMETER(InitialOrganicC)*1000.0;
	)
	
	EQUATION(Model, InitialOrganicNScaled,
		return PARAMETER(InitialOrganicN)*1000.0;
	)
	
	EQUATION(Model, InitialPoolCN,
		return SafeDivide(PARAMETER(InitialOrganicC), PARAMETER(InitialOrganicN));
	)
	
	
	
	auto TurnoverC          = RegisterEquation(Model, "Organic C turnover", MMolPerM2PerTs);
	auto RetainedC          = RegisterEquation(Model, "Organic C retained as decomposer biomass", MMolPerM2PerTs);
	auto DepletedC          = RegisterEquation(Model, "Organic C depleted", MMolPerM2PerTs);
	auto RespiredC          = RegisterEquation(Model, "Organic C respired as CO2", MMolPerM2PerTs);
	auto SolubilizedC       = RegisterEquation(Model, "Organic C solubilized as DOC", MMolPerM2PerTs);
	
	auto DecomposerCN       = RegisterEquation(Model, "Decomposer C/N", MolPerMol);
	auto TurnoverN          = RegisterEquation(Model, "Organic N turnover", MMolPerM2PerTs);
	auto RetainedN          = RegisterEquation(Model, "Organic N retained as decomposer biomass", MMolPerM2PerTs);
	auto SolubilizedN       = RegisterEquation(Model, "Organic N solubilized", MMolPerM2PerTs);
	auto NUsedByDecomp      = RegisterEquation(Model, "Oragnic N used by decomposers", MMolPerM2PerTs);
	auto NDeficit           = RegisterEquation(Model, "Excess organic N needed by decomposers", MMolPerM2PerTs);
	auto MineralizedN       = RegisterEquation(Model, "Organic N mineralized as NH4", MMolPerM2PerTs);
	auto DepletedN          = RegisterEquation(Model, "Organic N depleted", MMolPerM2PerTs);
	
	
	auto NitrificationEq     = RegisterEquation(Model, "Nitrification", MMolPerM2PerTs);
	auto DenitrificationEq   = RegisterEquation(Model, "Denitrification", MMolPerM2PerTs);
	auto NO3ImmobilisationEq = RegisterEquation(Model, "NO3 immobilisation", MMolPerM2PerTs);
	auto NH4ImmobilisationEq = RegisterEquation(Model, "NH4 immobilisation", MMolPerM2PerTs);
	auto NO3UptakeEq         = RegisterEquation(Model, "NO3 plant uptake", MMolPerM2PerTs);
	auto NH4UptakeEq         = RegisterEquation(Model, "NH4 plant uptake", MMolPerM2PerTs);
	
	
	//NOTE: The following 4 are required as an "interface" to the rest of the MAGIC model
	auto NO3Inputs           = RegisterEquation(Model, "NO3 inputs", MMolPerM2PerTs);
	auto NH4Inputs           = RegisterEquation(Model, "NH4 inputs", MMolPerM2PerTs);
	auto NO3ProcessesLoss    = RegisterEquation(Model, "NO3 processes loss", MMolPerM2PerTs);
	auto NH4ProcessesLoss    = RegisterEquation(Model, "NH4 processes loss", MMolPerM2PerTs);
	
	
	//NOTE! If we don't say these are 0 in initialization we get a nan computation
	SetInitialValue(Model, NO3ImmobilisationEq, 0.0);
	SetInitialValue(Model, NH4ImmobilisationEq, 0.0);
	
	
	auto FractionOfYear = GetEquationHandle(Model, "Fraction of year");
	auto NO3BasicInputs = GetEquationHandle(Model, "NO3 basic inputs");
	auto NH4BasicInputs = GetEquationHandle(Model, "NH4 basic inputs");
	auto TotalNO3       = GetEquationHandle(Model, "Total NO3 mass");
	auto TotalNH4       = GetEquationHandle(Model, "Total NH4 mass");
	
	/*
	1) d(SOC)/dt = litter C – (FC3 + FC4)   		- SOM C mass balance
	2) FC1 = FC2 + (FC3 + FC4)              		- decomposer turnover C balance
	3) FC2 = (Cfrac) x FC1                      	- C retained as decomposer biomass
	4) FC3 = (Dfrac) x (FC1 – FC2)         		    - C depleted as CO2 (respired) 
	5) FC4 = (1-Dfrac) x (FC1 – FC2)     		    - C depleted as DOC (solubilized)
	*/
	
	EQUATION(Model, PoolCN,
		return SafeDivide(LAST_RESULT(OrganicC), LAST_RESULT(OrganicN));
	)
	
	EQUATION(Model, TurnoverC,
		return PARAMETER(OrganicCDecomp)*RESULT(FractionOfYear);     //FC1
	)
	
	EQUATION(Model, RetainedC,
		return RESULT(TurnoverC)*PARAMETER(CarbEff)*0.01;              //FC2
	)
	
	EQUATION(Model, DepletedC,
		return RESULT(TurnoverC)*(1.0 - PARAMETER(CarbEff)*0.01);      //FC1 - FC2
	)
	
	EQUATION(Model, RespiredC,
		return RESULT(DepletedC) * PARAMETER(DecompEff)*0.01;           //FC3
	)
	
	EQUATION(Model, SolubilizedC,
		return RESULT(DepletedC) * (1.0 - PARAMETER(DecompEff)*0.01);   //FC4
	)
	
	EQUATION(Model, OrganicC,
		double in     = PARAMETER(OrganicCInput)*RESULT(FractionOfYear);
		double out    = PARAMETER(OrganicCOutput)*RESULT(FractionOfYear);
		double litter = PARAMETER(OrganicCLitter)*RESULT(FractionOfYear);

		return 
			  LAST_RESULT(OrganicC)
			+ in - out + litter - RESULT(DepletedC);
	)
	
	/*
	1) litter N = litter C / (C/N)litter  
	2) FN1 = FC1 / (C/N)SOM                       - org N processed in turnover
	3) FN4 = FC4 / (C/N)SOM                       - SOM org N solubilized
	4) FN5 = (FN1 – FN4) x (Nfrac)                - org N from SOM used by decomposers
	5) FN3 = FN1 - FN4 - FN5                      - SOM org N mineralized as NH4
	6) FN2 = FC2 / (C/N)biomass                   - N in new decomposer biomass
	*/
	
	EQUATION(Model, DecomposerCN,
		double poolCN = RESULT(PoolCN);
		double dcmpCN = PARAMETER(DecompCN);
		if(dcmpCN <= 0.0) dcmpCN = poolCN;
		return dcmpCN;
	)
	
	EQUATION(Model, TurnoverN,
		return RESULT(TurnoverC)/RESULT(DecomposerCN);    //FN1
	)
	
	EQUATION(Model, RetainedN,
		return RESULT(RetainedC)/RESULT(DecomposerCN);       //FN2
	)
	
	EQUATION(Model, SolubilizedN,
		return RESULT(SolubilizedC)/RESULT(DecomposerCN);    //FN4
	)
	
	EQUATION(Model, NUsedByDecomp,
		return (RESULT(TurnoverN) - RESULT(SolubilizedN))*PARAMETER(NitrEff)*0.01;  //FN5
	)
	
	EQUATION(Model, NDeficit,
		return Max(0.0, RESULT(NUsedByDecomp) - RESULT(RetainedN)); //-FN6
	)
	
	EQUATION(Model, MineralizedN,
		double retainedN = RESULT(RetainedN);
		double Nused     = RESULT(NUsedByDecomp);
		double mineralized = RESULT(TurnoverN) - RESULT(SolubilizedN) - RESULT(NUsedByDecomp);  //FN3
		
		if(Nused > retainedN)
		{
			//FN3 = FN3 + (FN5 – FN2)
			mineralized += (retainedN - Nused);
		}
		return mineralized;
	)
	
	EQUATION(Model, DepletedN,
		return RESULT(SolubilizedN) + RESULT(MineralizedN);
	)
	
	
	EQUATION(Model, OrganicN,
		double poolCN = RESULT(PoolCN);
		double outCN  = PARAMETER(OutputCN);
		if(outCN == 0.0)  outCN  = poolCN;
		
		double in     = PARAMETER(OrganicCInput)*RESULT(FractionOfYear)/PARAMETER(InputCN);
		double out    = PARAMETER(OrganicCOutput)*RESULT(FractionOfYear)/outCN;
		double litter = PARAMETER(OrganicCLitter)*RESULT(FractionOfYear)/PARAMETER(LitterCN);
		
		return
			  LAST_RESULT(OrganicN)
			+ in - out + litter - RESULT(DepletedN);
	)
	
	//TODO: The following has to be updated to match the new fluxes above:
	
	EQUATION(Model, NitrificationEq,
		double nitr = PARAMETER(Nitrification);
		double result = nitr * RESULT(FractionOfYear);
		double in = RESULT(NH4Inputs);
		if(nitr < 0.0) result = -nitr*0.01*in;
		return result;
	)
	
	EQUATION(Model, DenitrificationEq,
		double denitr = PARAMETER(Denitrification);
		double result = denitr * RESULT(FractionOfYear);
		double in = RESULT(NO3Inputs);
		if(denitr < 0.0) result = -denitr*0.01*in;
		return result;
	)
	
	EQUATION(Model, NO3ImmobilisationEq,
		//TODO: allow toggling preferential immob.
		//TODO: guard against going below 0
		//std::cout << "timestep is " << CURRENT_TIMESTEP() << " last totalno3 is " << LAST_RESULT(TotalNO3) << '\n';
		
		double deficit = RESULT(NDeficit);
		double frac = LAST_RESULT(TotalNO3) / (LAST_RESULT(TotalNO3) + LAST_RESULT(TotalNH4));
		return deficit * frac;
	)
	
	EQUATION(Model, NH4ImmobilisationEq,
		//TODO: allow toggling preferential immob.
		//TODO: guard against going below 0
		//std::cout << "timestep is " << CURRENT_TIMESTEP() << " last totalno3 is " << LAST_RESULT(TotalNO3) << '\n';
		
		double deficit = RESULT(NDeficit);
		double frac = LAST_RESULT(TotalNH4) / (LAST_RESULT(TotalNO3) + LAST_RESULT(TotalNH4));
		return deficit * frac;
	)
	
	//TODO: Plants should have access to uptake of organic N (if toggled)
	//TODO: Budged immob and plant uptake to be restrained by each other (toggling preferentiality)
	
	EQUATION(Model, NO3UptakeEq,
		double uptake = PARAMETER(PlantNO3Uptake);
		double result = uptake * RESULT(FractionOfYear);
		double in = RESULT(NO3Inputs);
		double total = LAST_RESULT(TotalNO3);
		if(uptake < 0.0) result = -uptake*0.01*in;
		return result;
	)
	
	EQUATION(Model, NH4UptakeEq,
		double uptake = PARAMETER(PlantNH4Uptake);
		double result = uptake * RESULT(FractionOfYear);
		double in = RESULT(NH4Inputs);
		if(uptake < 0.0) result = -uptake*0.01*in;
		return result;
	)
	
	
	
	EQUATION(Model, NO3Inputs,
		return RESULT(NO3BasicInputs) + RESULT(NitrificationEq);
	)
	
	EQUATION(Model, NH4Inputs,
		return RESULT(NH4BasicInputs) + RESULT(MineralizedN);
	)
	
	EQUATION(Model, NO3ProcessesLoss,
		return RESULT(DenitrificationEq) + RESULT(NO3ImmobilisationEq) + RESULT(NO3UptakeEq);
	)
	
	EQUATION(Model, NH4ProcessesLoss,
		return RESULT(NitrificationEq) + RESULT(NH4ImmobilisationEq) + RESULT(NH4UptakeEq);
	)
	
	
	EndModule(Model);
}
