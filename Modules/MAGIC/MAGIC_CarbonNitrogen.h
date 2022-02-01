


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
	auto NO3BasicInputs = GetEquationHandle(Model, "NO3 non-process inputs");
	auto NH4BasicInputs = GetEquationHandle(Model, "NH4 non-process inputs");
	
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
		if(nitr < 0.0)
			result = -nitr*0.01*in;
		else
			result = Min(result, in);
		return result;
	)
	
	EQUATION(Model, DenitrificationEq,
		double denitr = PARAMETER(Denitrification);
		double result = denitr * RESULT(FractionOfYear);
		double in = RESULT(NO3Inputs);
		if(denitr < 0.0)
			result = -denitr*0.01*in;
		else
			result = Min(result, in);
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
	auto OrganicCOutput                   = RegisterParameterDouble(Model, CAndN, "Organic C output", MMolPerM2PerYear, 0.0, 0.0, 1e6);
	auto OrganicCDecomposition            = RegisterParameterDouble(Model, CAndN, "Organic C decomposition", MMolPerM2PerYear, 0.0, 0.0, 1e6);
	auto InitialOrganicC                  = RegisterParameterDouble(Model, CAndN, "Initial organic C", MolPerM2, 0.0, 0.0, 1e8);

	auto OrganicCNInputRatio              = RegisterParameterDouble(Model, CAndN, "Organic C/N input ratio", Dimensionless, 0.1, 0.0001, 100.0);
	auto OrganicCNLitterRatio             = RegisterParameterDouble(Model, CAndN, "Organic C/N litter ratio", Dimensionless, 0.1, 0.0001, 100.0);
	auto OrganicCNOutputRatio               = RegisterParameterDouble(Model, CAndN, "Organic C/N output ratio", Dimensionless, 0.1, 0.0001, 100.0, "If 0, use the pool C/N instead");
	auto OrganicCNDecompositionRatio      = RegisterParameterDouble(Model, CAndN, "Organic C/N decomposition ratio", Dimensionless, 0.1, 0.0001, 100.0, "If 0, use the pool C/N instead");
	auto InitialOrganicN                  = RegisterParameterDouble(Model, CAndN, "Initial organic N", MolPerM2, 0.0, 0.0, 1e8);

	auto LowerCNThresholdForNO3Immobilisation = RegisterParameterDouble(Model, CAndN, "Lower C/N threshold for NO3 immobilisation", Dimensionless, 0.5, 0.0, 5.0,
	"C/N below this value - 0% NO3 immobilisation");
	auto UpperCNThresholdForNO3Immobilisation = RegisterParameterDouble(Model, CAndN, "Upper C/N threshold for NO3 immobilisation", Dimensionless, 0.5, 0.0, 5.0, "C/N above this value - 100% NO3 immobilisation");
	
	auto LowerCNThresholdForNH4Immobilisation = RegisterParameterDouble(Model, CAndN, "Lower C/N threshold for NH4 immobilisation", Dimensionless, 0.5, 0.0, 5.0,
	"C/N below this value - 0% NH4 immobilisation");
	auto UpperCNThresholdForNH4Immobilisation = RegisterParameterDouble(Model, CAndN, "Upper C/N threshold for NH4 immobilisation", Dimensionless, 0.5, 0.0, 5.0, "C/N above this value - 100% NH4 immobilisation");
	
	auto NO3PlantUptake                       = RegisterParameterDouble(Model, CAndN, "NO3 plant uptake", MMolPerM2PerYear, 0.0, 0.0, 1000.0);
	auto NH4PlantUptake                       = RegisterParameterDouble(Model, CAndN, "NH4 plant uptake", MMolPerM2PerYear, 0.0, 0.0, 1000.0);
	
	auto PlantsUseOrganicN                    = RegisterParameterBool(Model, CAndN, "Plants have access to organic nitrogen", true);
	auto PlantsUseInorganicFirst              = RegisterParameterBool(Model, CAndN, "Plants use inorganic nitrogen before soil", true);

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
	auto NH4ImmobilisationFraction = RegisterEquation(Model, "NH4 immobilisation fraction", Dimensionless);

	auto DesiredNO3Uptake          = RegisterEquation(Model, "Desired NO3 plant uptake", MMolPerM2PerTs);
	auto DesiredNH4Uptake          = RegisterEquation(Model, "Desired NH4 plant uptake", MMolPerM2PerTs);
	auto DesiredNO3Immobilisation  = RegisterEquation(Model, "Desired NO3 immobilisation", MMolPerM2PerTs);
	auto DesiredNH4Immobilisation  = RegisterEquation(Model, "Desired NH4 immobilisation", MMolPerM2PerTs);
	
	auto NO3ImmobilisationEq       = RegisterEquation(Model, "NO3 immobilisation", MMolPerM2PerTs);
	auto NH4ImmobilisationEq       = RegisterEquation(Model, "NH4 immobilisation", MMolPerM2PerTs);
	auto CNRatio                   = RegisterEquation(Model, "Pool C/N ratio", Dimensionless);
	
	auto NO3UptakeEq               = RegisterEquation(Model, "NO3 plant uptake", MMolPerM2PerTs);
	auto NH4UptakeEq               = RegisterEquation(Model, "NH4 plant uptake", MMolPerM2PerTs);
	
	auto OrganicNUptake            = RegisterEquation(Model, "Organic N uptake", MMolPerM2PerTs);

	//NOTE: The following 4 are required as an "interface" to the rest of the MAGIC model
	// We take ownership of them from the simple model
	auto NO3Inputs           = RegisterEquation(Model, "NO3 inputs", MMolPerM2PerTs);
	auto NH4Inputs           = RegisterEquation(Model, "NH4 inputs", MMolPerM2PerTs);
	auto NO3ProcessesLoss    = RegisterEquation(Model, "NO3 processes loss", MMolPerM2PerTs);
	auto NH4ProcessesLoss    = RegisterEquation(Model, "NH4 processes loss", MMolPerM2PerTs);
	
	//NOTE: The following is provided by the driver
	auto FractionOfYear      = GetEquationHandle(Model, "Fraction of year");
	auto NO3BasicInputs      = GetEquationHandle(Model, "NO3 non-process inputs");
	auto NH4BasicInputs      = GetEquationHandle(Model, "NH4 non-process inputs");  
	
	auto IsSoil = GetParameterDoubleHandle(Model, "This is a soil compartment");
	
	
	EQUATION(Model, InitialOrganicCScaled,
		return PARAMETER(InitialOrganicC)*1000.0;
	)
	
	EQUATION(Model, InitialOrganicNScaled,
		return PARAMETER(InitialOrganicN)*1000.0;
	)
	
	EQUATION(Model, CNRatio,
		double CN = SafeDivide(LAST_RESULT(OrganicC), LAST_RESULT(OrganicN));
		if(!PARAMETER(IsSoil)) CN = 1.0;  //NOTE: to not cause crashes when we are in water. TODO: Make a conditional to exclude the entire computation in that case.
		return CN;
	)
	
	EQUATION_OVERRIDE(Model, MineralisationEq,
		double decomp_CN = PARAMETER(OrganicCNDecompositionRatio);
		double pool_CN   = RESULT(CNRatio);
		if(decomp_CN == 0.0) decomp_CN = pool_CN;
		
		return RESULT(FractionOfYear) * PARAMETER(OrganicCDecomposition) / decomp_CN;
	)
	
	//TODO: Carbon fixation?
	
	EQUATION(Model, OrganicC,
		double dCdt = RESULT(FractionOfYear) * (
			  PARAMETER(OrganicCInput)
			+ PARAMETER(OrganicCLitter)
			- PARAMETER(OrganicCOutput)
			- PARAMETER(OrganicCDecomposition)
			);
		
		if(!PARAMETER(IsSoil)) dCdt = 0.0; //TODO: make conditional exec instead?
			
		return LAST_RESULT(OrganicC) + dCdt;
	)
	
	
	EQUATION(Model, OrganicN,
		double output_CN = PARAMETER(OrganicCNOutputRatio);
		double pool_CN = RESULT(CNRatio);
		if(output_CN == 0.0) output_CN = pool_CN;
	
		double dNdt = RESULT(FractionOfYear)*(
				+ PARAMETER(OrganicCInput) / PARAMETER(OrganicCNInputRatio)
				+ PARAMETER(OrganicCLitter) / PARAMETER(OrganicCNLitterRatio)
				- PARAMETER(OrganicCOutput)  / output_CN
				)
			+ RESULT(NO3ImmobilisationEq)
			+ RESULT(NH4ImmobilisationEq)
			- RESULT(MineralisationEq)
			- RESULT(OrganicNUptake);
		
		if(!PARAMETER(IsSoil)) dNdt = 0.0; //TODO: make conditional exec instead?
	
		return LAST_RESULT(OrganicN) + dNdt;
	)
	
	EQUATION(Model, NO3ImmobilisationFraction,
		//TODO: There should be a retention potential.
		return LinearResponse(RESULT(CNRatio), PARAMETER(LowerCNThresholdForNO3Immobilisation), PARAMETER(UpperCNThresholdForNO3Immobilisation), 0.0, 1.0);
	)
	
	EQUATION(Model, NH4ImmobilisationFraction,
		return LinearResponse(RESULT(CNRatio), PARAMETER(LowerCNThresholdForNH4Immobilisation), PARAMETER(UpperCNThresholdForNH4Immobilisation), 0.0, 1.0);
	)
	
	
	EQUATION(Model, DesiredNO3Uptake,
		return RESULT(FractionOfYear) * PARAMETER(NO3PlantUptake);
	)
	
	EQUATION(Model, DesiredNH4Uptake,
		return RESULT(FractionOfYear) * PARAMETER(NH4PlantUptake);
	)
	
	EQUATION(Model, DesiredNO3Immobilisation,
		double immob = RESULT(NO3ImmobilisationFraction) * RESULT(NO3Inputs);
		if(!PARAMETER(IsSoil)) immob = 0.0;
		return immob;
	)
	
	EQUATION(Model, DesiredNH4Immobilisation,
		double immob = RESULT(NH4ImmobilisationFraction) * RESULT(NH4Inputs);
		if(!PARAMETER(IsSoil)) immob = 0.0;
		return immob;
	)
	
	
	EQUATION(Model, NO3UptakeEq,
		double in = RESULT(NO3Inputs);
		double desired_uptake = RESULT(DesiredNO3Uptake);
		double desired_immob = Min(in, RESULT(DesiredNO3Immobilisation));
		if(!PARAMETER(PlantsUseInorganicFirst))
			in -= desired_immob;
		return Min(in, desired_uptake);
	)
	
	EQUATION(Model, NH4UptakeEq,
		double in = RESULT(NH4Inputs);
		double desired_uptake = RESULT(DesiredNH4Uptake);
		double desired_immob = Min(in, RESULT(DesiredNH4Immobilisation));
		if(!PARAMETER(PlantsUseInorganicFirst))
			in -= desired_immob;
		return Min(in, desired_uptake);
	)
	
	
	EQUATION_OVERRIDE(Model, NO3ImmobilisationEq,
		double in = RESULT(NO3Inputs);
		double desired_immob = RESULT(DesiredNO3Immobilisation);
		double desired_uptake = Min(in, RESULT(DesiredNO3Uptake));
		if(PARAMETER(PlantsUseInorganicFirst))
			in -= desired_uptake;
		return Min(in, desired_immob);
	)
	
	EQUATION_OVERRIDE(Model, NH4ImmobilisationEq,
		double in = RESULT(NH4Inputs);
		double desired_immob = RESULT(DesiredNH4Immobilisation);
		double desired_uptake = Min(in, RESULT(DesiredNH4Uptake));
		if(PARAMETER(PlantsUseInorganicFirst))
			in -= desired_uptake;
		return Min(in, desired_immob);
	)
	
	EQUATION(Model, OrganicNUptake,
		double pot = (RESULT(DesiredNO3Uptake) - RESULT(NO3UptakeEq)) + (RESULT(DesiredNH4Uptake) - RESULT(NH4UptakeEq));
		if(!PARAMETER(PlantsUseOrganicN))
			pot = 0.0;
		return pot;
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
	
	auto Nitrification     = GetParameterDoubleHandle(Model, "Nitrification");
	auto Denitrification   = GetParameterDoubleHandle(Model, "Denitrification");
	
	auto DenitrificationEq = GetEquationHandle(Model, "Denitrification");
	auto NitrificationEq   = GetEquationHandle(Model, "Nitrification");
	
	
	EQUATION_OVERRIDE(Model, NitrificationEq,
		double nitr = PARAMETER(Nitrification);
		double result = nitr * RESULT(FractionOfYear);
		double in = RESULT(NH4Inputs) - RESULT(NH4ImmobilisationEq) - RESULT(NH4UptakeEq); //NOTE: immob. and uptake prioritized over nitrification.
		if(nitr < 0.0)
			result = -nitr*0.01*in; //TODO: should this instead be the desired value, and the actual is limited by what remains?
		else
			result = Min(result, in);
		return result;
	)
	
	EQUATION_OVERRIDE(Model, DenitrificationEq,
		double denitr = PARAMETER(Denitrification);
		double result = denitr * RESULT(FractionOfYear);
		double in = RESULT(NO3Inputs) - RESULT(NO3ImmobilisationEq) - RESULT(NO3UptakeEq); //NOTE: immob. and uptake are prioritized over denitrification
		if(denitr < 0.0)
			result = -denitr*0.01*in; //TODO: see above!
		else
			result = Min(result, in);
		return result;
	)
	
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
	AddRatioMagicCarbonNitrogenModel(Model);
	
	BeginModule(Model, "MAGIC microbial carbon and nitrogen", "_dev");
	
	auto Dimensionless    = RegisterUnit(Model);
	auto Percent          = RegisterUnit(Model, "%");
	auto MolPerM2         = RegisterUnit(Model, "mol/m2");
	auto MMolPerM2        = RegisterUnit(Model, "mmol/m2");
	auto MMolPerM2PerYear = RegisterUnit(Model, "mmol/m2/year");
	auto MMolPerM2PerTs   = RegisterUnit(Model, "mmol/m2/month");
	auto MolPerMol        = RegisterUnit(Model, "mol/mol");
	
	auto Compartment        = GetIndexSetHandle(Model, "Compartment");
	
	//Take ownership of the parameter group
	auto CAndN              = RegisterParameterGroup(Model, "Carbon and Nitrogen by compartment", Compartment);
	
	auto CarbEff            = RegisterParameterDouble(Model, CAndN, "Carbon efficiency", Percent, 25.0, 0.0, 100.0);
	auto NitrEff            = RegisterParameterDouble(Model, CAndN, "Nitrogen efficiency", Percent, 50.0, 0.0, 100.0);
	auto DecompEff          = RegisterParameterDouble(Model, CAndN, "Decomposition efficiency", Percent, 5.0, 0.0, 100.0);
	
	auto ImmobType          = RegisterParameterEnum(Model, CAndN, "Immobilisation order", {"NO3_first", "NH4_first", "proportionally"}, "proportionally");
	
	auto TurnoverC          = RegisterEquation(Model, "Organic C turnover", MMolPerM2PerTs);
	auto RetainedC          = RegisterEquation(Model, "Organic C retained as decomposer biomass", MMolPerM2PerTs);
	auto DepletedC          = RegisterEquation(Model, "Organic C depleted", MMolPerM2PerTs);
	auto RespiredC          = RegisterEquation(Model, "Organic C respired as CO2", MMolPerM2PerTs);
	auto SolubilizedC       = RegisterEquation(Model, "Organic C solubilized as DOC", MMolPerM2PerTs);
	
	auto TurnoverN          = RegisterEquation(Model, "Organic N turnover", MMolPerM2PerTs);
	auto RetainedN          = RegisterEquation(Model, "Organic N retained as decomposer biomass", MMolPerM2PerTs);
	auto DepletedN          = RegisterEquation(Model, "Organic N depleted", MMolPerM2PerTs);
	auto WasteCN            = RegisterEquation(Model, "Waste C/N ratio", Dimensionless);
	auto SolubilizedN       = RegisterEquation(Model, "Organic N solubilized", MMolPerM2PerTs);
	auto TotalDesiredImmobilisation = RegisterEquation(Model, "Total desired N immobilisation", MMolPerM2PerTs);
	
	//NOTE: Take ownership of these from the previous module
	auto DesiredNH4Immobilisation = RegisterEquation(Model, "Desired NH4 immobilisation", MMolPerM2PerTs);
	auto DesiredNO3Immobilisation = RegisterEquation(Model, "Desired NO3 immobilisation", MMolPerM2PerTs);
	auto OrganicC                 = RegisterEquation(Model, "Organic C", MMolPerM2PerTs);
	auto OrganicN                 = RegisterEquation(Model, "Organic N", MMolPerM2PerTs);
	auto MineralisationEq         = RegisterEquation(Model, "Mineralisation", MMolPerM2PerTs);
	
	auto DesiredNO3Uptake         = GetEquationHandle(Model, "Desired NO3 plant uptake");
	auto DesiredNH4Uptake         = GetEquationHandle(Model, "Desired NH4 plant uptake");
	auto NH4ImmobilisationEq      = GetEquationHandle(Model, "NH4 immobilisation");
	auto NO3ImmobilisationEq      = GetEquationHandle(Model, "NO3 immobilisation");
	auto FractionOfYear           = GetEquationHandle(Model, "Fraction of year");
	auto CNRatio                  = GetEquationHandle(Model, "Pool C/N ratio");
	auto OrganicNUptake           = GetEquationHandle(Model, "Organic N uptake");
	auto NO3Inputs                = GetEquationHandle(Model, "NO3 inputs");
	auto NH4Inputs                = GetEquationHandle(Model, "NH4 inputs");
	auto NO3BasicInputs           = GetEquationHandle(Model, "NO3 non-process inputs");
	
	auto OrganicCDecomposition    = GetParameterDoubleHandle(Model, "Organic C decomposition");
	auto OrganicCInput            = GetParameterDoubleHandle(Model, "Organic C input");
	auto OrganicCLitter           = GetParameterDoubleHandle(Model, "Organic C litter");
	auto OrganicCOutput           = GetParameterDoubleHandle(Model, "Organic C output");
	auto IsSoil                   = GetParameterBoolHandle(Model, "This is a soil compartment");
	auto PlantsUseInorganicFirst  = GetParameterBoolHandle(Model, "Plants use inorganic nitrogen before soil");
	
	auto OrganicCNInputRatio      = GetParameterDoubleHandle(Model, "Organic C/N input ratio");
	auto OrganicCNLitterRatio     = GetParameterDoubleHandle(Model, "Organic C/N litter ratio");
	auto OrganicCNOutputRatio     = GetParameterDoubleHandle(Model, "Organic C/N output ratio");
	auto OrganicCNDecompositionRatio = GetParameterDoubleHandle(Model, "Organic C/N decomposition ratio");
	
	HideParameter(Model, GetParameterDoubleHandle(Model, "Lower C/N threshold for NO3 immobilisation"));
	HideParameter(Model, GetParameterDoubleHandle(Model, "Upper C/N threshold for NO3 immobilisation"));
	HideParameter(Model, GetParameterDoubleHandle(Model, "Lower C/N threshold for NH4 immobilisation"));
	HideParameter(Model, GetParameterDoubleHandle(Model, "Upper C/N threshold for NH4 immobilisation"));
	
	
	EQUATION(Model, RespiredC,
		return PARAMETER(OrganicCDecomposition)*RESULT(FractionOfYear); // FC6
	)
	
	EQUATION(Model, DepletedC,
		double Deff = PARAMETER(DecompEff) * 0.01;
		double respired = RESULT(RespiredC);
		if(Deff == 0.0) return respired;
		return respired / Deff;                                        // FC4
	)
	
	EQUATION(Model, SolubilizedC,
		return RESULT(DepletedC) * (1.0 - PARAMETER(DecompEff)*0.01);   //FC5
	)
	
	EQUATION(Model, TurnoverC,
		return RESULT(DepletedC) / (1.0 - PARAMETER(CarbEff)*0.01);     //FC2
	)
	
	EQUATION(Model, RetainedC,
		return RESULT(TurnoverC)*PARAMETER(CarbEff)*0.01;              //FC3
	)
	
	EQUATION_OVERRIDE(Model, OrganicC,
		double in     = PARAMETER(OrganicCInput)*RESULT(FractionOfYear);
		double out    = PARAMETER(OrganicCOutput)*RESULT(FractionOfYear);
		double litter = PARAMETER(OrganicCLitter)*RESULT(FractionOfYear);
	
		double dCdt = in + litter - out - RESULT(DepletedC);
		
		if(!PARAMETER(IsSoil)) dCdt = 0.0; //TODO: make conditional exec instead?

		return LAST_RESULT(OrganicC) + dCdt;
	)
	
	EQUATION(Model, TurnoverN,
		return RESULT(TurnoverC) / RESULT(CNRatio);            //FN2
	)
	
	EQUATION(Model, RetainedN,
		double poolCN = RESULT(CNRatio);
		double dcmpCN = PARAMETER(OrganicCNDecompositionRatio);
		if(dcmpCN <= 0.0) dcmpCN = poolCN;
		return RESULT(RetainedC) / dcmpCN;                     //FN3
	)
	
	EQUATION(Model, DepletedN,
		return RESULT(TurnoverN)*PARAMETER(NitrEff)*0.01;      //FN4
	)
	
	EQUATION(Model, WasteCN,
		return SafeDivide(RESULT(DepletedC), RESULT(DepletedN)); // waste (C/N)
	)
	
	EQUATION(Model, SolubilizedN,
		return SafeDivide(RESULT(SolubilizedC), RESULT(WasteCN)); //FN5
	)
	
	EQUATION_OVERRIDE(Model, MineralisationEq,
		return SafeDivide(RESULT(RespiredC), RESULT(WasteCN));    //FN6
	)
	
	EQUATION(Model, TotalDesiredImmobilisation,                //FN7
		double NDeficit = RESULT(RetainedN) + RESULT(DepletedN) - RESULT(TurnoverN);
		return Max(0.0, NDeficit);
	)
	
	EQUATION_OVERRIDE(Model, DesiredNH4Immobilisation,
		double inNO3 = RESULT(NO3BasicInputs);        //NOTE: We can't use NO3Inputs here since that relies on Nitrification, which relies on NH4 immob.
		double upNO3 = RESULT(DesiredNO3Uptake);
		double inNH4 = RESULT(NH4Inputs);
		double upNH4 = RESULT(DesiredNH4Uptake);
		
		if(PARAMETER(PlantsUseInorganicFirst))
		{
			inNO3 -= upNO3;
			inNH4 -= upNH4;
		}
		inNO3 = Max(inNO3, 0.0);
		inNH4 = Max(inNH4, 0.0);
	
		double desired_tot = RESULT(TotalDesiredImmobilisation);
		
		u64 type = PARAMETER(ImmobType);
		if(type == 0)      // NO3 first
			return Max(0.0, desired_tot - inNO3);
		else if(type == 1) // NH4 first
			return desired_tot;
		else               // proportionally
			return desired_tot * SafeDivide(inNH4, inNH4 + inNO3);
	)

	EQUATION_OVERRIDE(Model, DesiredNO3Immobilisation,
		double inNO3 = RESULT(NO3Inputs);
		double upNO3 = RESULT(DesiredNO3Uptake);
		double inNH4 = RESULT(NH4Inputs);
		double upNH4 = RESULT(DesiredNH4Uptake);
		
		if(PARAMETER(PlantsUseInorganicFirst))
		{
			inNO3 -= upNO3;
			inNH4 -= upNH4;
		}
		inNO3 = Max(inNO3, 0.0);
		inNH4 = Max(inNH4, 0.0);
	
		double desired_tot = RESULT(TotalDesiredImmobilisation);
		
		u64 type = PARAMETER(ImmobType);
		if(type == 0)      // NO3 first
			return desired_tot;
		else if(type == 1) // NH4 first
			return Max(0.0, desired_tot - inNH4);
		else               // proportionally
			return desired_tot * SafeDivide(inNO3, inNH4 + inNO3);
	)
	
	//Below is not done
	
	EQUATION_OVERRIDE(Model, OrganicN,
		double poolCN = RESULT(CNRatio);
		double outCN  = PARAMETER(OrganicCNOutputRatio);
		if(outCN == 0.0) outCN  = poolCN;
		
		double in     = PARAMETER(OrganicCInput)*RESULT(FractionOfYear)/PARAMETER(OrganicCNInputRatio);
		double litter = PARAMETER(OrganicCLitter)*RESULT(FractionOfYear)/PARAMETER(OrganicCNLitterRatio);
		double out    = PARAMETER(OrganicCOutput)*RESULT(FractionOfYear)/outCN;
		
		double dNdt   = 
			  in + litter - out
			- RESULT(DepletedN)
			+ RESULT(NO3ImmobilisationEq)   //TODO: Is immob already counted into DepletedN implicitly??
			+ RESULT(NH4ImmobilisationEq)
			- RESULT(OrganicNUptake);
		
		if(!PARAMETER(IsSoil)) dNdt = 0.0; //TODO: make conditional exec instead?
	
		return LAST_RESULT(OrganicN) + dNdt;
	)
}

