

static void
AddSimpleMagicCarbonNitrogenModel(mobius_model *Model)
{
	
	BeginModule(Model, "MAGIC simple carbon and nitrogen", "_dev");
	
	auto MMolPerM2PerYear = RegisterUnit(Model, "mmol/m2/year");
	auto MMolPerM2PerTs = RegisterUnit(Model, "mmol/m2/month");
	
	auto Compartment        = GetIndexSetHandle(Model, "Compartment");
	
	auto CAndN              = RegisterParameterGroup(Model, "Carbon and Nitrogen by subcatchment", Compartment);
	
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
	
	auto FractionOfYear = GetEquationHandle(Model, "Fraction of year");
	auto NO3BasicInputs = GetEquationHandle(Model, "NO3 basic inputs");
	auto NH4BasicInputs = GetEquationHandle(Model, "NH4 basic inputs");
	
	EQUATION(Model, NO3Inputs,
		return RESULT(NO3BasicInputs) + RESULT(NitrificationEq);
	)
	
	EQUATION(Model, NH4Inputs,
		return RESULT(NH4BasicInputs) + RESULT(FractionOfYear) * PARAMETER(Mineralisation);
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



/*
	auto OrganicCInput                    = RegisterParameterDouble(Model, CAndN, "Organic C input", MMolPerM2PerYear, 0.0, 0.0, 1e6); 
	auto OrganicCSink                     = RegisterParameterDouble(Model, CAndN, "Organic C sink", MMolPerM2PerYear, 0.0, 0.0, 1e6);
	auto OrganicCDecomposition            = RegisterParameterDouble(Model, CAndN, "Organic C decomposition", MMolPerM2PerYear, 0.0, 0.0, 1e6);
	auto InitialOrganicC                  = RegisterParameterDouble(Model, CAndN, "Initial organic C", MMolPerM2, 0.0, 0.0, 1e8);

	auto OrganicCNInputRatio              = RegisterParameterDouble(Model, CAndN, "Organic C/N input ratio", Dimensionless, 0.1, 0.0001, 5.0);
	auto OrganicCNSinkRatio               = RegisterParameterDouble(Model, CAndN, "Organic C/N sink ratio", Dimensionless, 0.1, 0.0001, 5.0);
	auto OrganicCNDecompositionRation     = RegisterParameterDouble(Model, CAndN, "Organic C/N decomposition ratio", Dimensionless, 0.1, 0.0001, 5.0);
	auto InitialOrganicN                  = RegisterParameterDouble(Model, CAndN, "Initial organic N", MMolPerM2, 0.0, 0.0, 1e8);

	auto UpperCNThresholdForNO3Immobilisation = RegisterParameterDouble(Model, CAndN, "Upper C/N threshold for NO3 immobilisation", Percent, 0.5, 0.0, 5.0, "C/N above this value - 100% NO3 immobilisation");
	auto UpperCNThresholdForNH4Immobilisation = RegisterParameterDouble(Model, CAndN, "Upper C/N threshold for NH4 immobilisation", Percent, 0.5, 0.0, 5.0, "C/N above this value - 100% NH4 immobilisation");
	auto LowerCNThresholdForNO3Immobilisation = RegisterParameterDouble(Model, CAndN, "Lower C/N threshold for NO3 immobilisation", Percent, 0.5, 0.0, 5.0,
	"C/N below this value - 0% NO3 immobilisation");
	auto LowerCNThresholdForNH4Immobilisation = RegisterParameterDouble(Model, CAndN, "Lower C/N threshold for NH4 immobilisation", Percent, 0.5, 0.0, 5.0,
	"C/N below this value - 0% NH4 immobilisation");
	
	auto NO3PlantUptake                       = RegisterParameterDouble(Model, CAndN, "NO3 plant uptake", MMolPerM2PerYear, 0.0, 0.0, 1000.0);
	auto NH4PlantUptake                       = RegisterParameterDouble(Model, CAndN, "NH4 plant uptake", MMolPerM2PerYear, 0.0, 0.0, 1000.0);
	auto Nitrification                        = RegisterParameterDouble(Model, CAndN, "Nitrification", MEqPerM2PerYear, 0.0, 0.0, 1000.0);
	auto Denitrification                      = RegisterParameterDouble(Model, CAndN, "Denitrification", MEqPerM2PerYear, 0.0, 0.0, 1000.0);

	auto OrganicNMineralisation    = RegisterEquation(Model, "Organic N mineralisation", MMolPerM2PerYear);

	auto OrganicC                  = RegisterEquationODE(Model, "Organic C", MMolPerM2, CompartmentSolver);
	SetInitialValue(Model, OrganicC, InitialOrganicC);
	
	auto OrganicN                  = RegisterEquationODE(Model, "Organic N", MMolPerM2, CompartmentSolver);
	SetInitialValue(Model, OrganicN, InitialOrganicN);

	auto NO3ImmobilisationFraction = RegisterEquation(Model, "NO3 immobilisation fraction", Dimensionless, CompartmentSolver);
	auto NO3Immobilisation         = RegisterEquation(Model, "NO3 immobilisation", MMolPerM2PerYear, CompartmentSolver);
	auto NH4ImmobilisationFraction = RegisterEquation(Model, "NH4 immobilisation fraction", Dimensionless, CompartmentSolver);
	auto NH4Immobilisation         = RegisterEquation(Model, "NH4 immobilisation", MMolPerM2PerYear, CompartmentSolver);
	auto CNRatio                   = RegisterEquation(Model, "C/N ratio", Dimensionless, CompartmentSolver);
	
	auto NO3ExternalFluxWithoutImmobilisation = RegisterEquation(Model, "NO3 flux disregarding discharge and immobilisation", MEqPerM2PerTs);
	auto NH4ExternalFluxWithoutImmobilisation = RegisterEquation(Model, "NH4 flux disregarding discharge and immobilisation", MEqPerM2PerTs);
	*/
	
	/*
	
	EQUATION(Model, OrganicNMineralisation,
		return PARAMETER(OrganicCDecomposition) / PARAMETER(OrganicCNDecompositionRation);
	)
	
	EQUATION(Model, OrganicC,
		return RESULT(FractionOfYear)*(PARAMETER(OrganicCInput) - PARAMETER(OrganicCSink) - PARAMETER(OrganicCDecomposition));
	)
	
	//TODO: Needs modification for uptake etc.
	EQUATION(Model, OrganicN,
		return
			  RESULT(FractionOfYear)*(
				  RESULT(NO3Immobilisation)
				+ RESULT(NH4Immobilisation)
				- RESULT(OrganicNMineralisation)
				+ PARAMETER(OrganicCInput) / PARAMETER(OrganicCNInputRatio)
				- PARAMETER(OrganicCSink)  / PARAMETER(OrganicCNSinkRatio));
	)
	
	EQUATION(Model, CNRatio,
		return SafeDivide(RESULT(OrganicC), RESULT(OrganicN));
	)
	
	EQUATION(Model, NO3ImmobilisationFraction,
		return LinearResponse(RESULT(CNRatio), PARAMETER(LowerCNThresholdForNO3Immobilisation)*0.01, PARAMETER(UpperCNThresholdForNO3Immobilisation)*0.01, 0.0, 1.0);
	)
	
	EQUATION(Model, NH4ImmobilisationFraction,
		return LinearResponse(RESULT(CNRatio), PARAMETER(LowerCNThresholdForNH4Immobilisation)*0.01, PARAMETER(UpperCNThresholdForNH4Immobilisation)*0.01, 0.0, 1.0);
	)
	
	EQUATION(Model, NO3ExternalFluxWithoutImmobilisation,
		return RESULT(NH4Deposition) + PARAMETER(NH4Weathering)*RESULT(FractionOfYear);   // sources+sinks, uptake etc.
	)
	
	EQUATION(Model, NH4ExternalFluxWithoutImmobilisation,
		return RESULT(NO3Deposition) + PARAMETER(NO3Weathering)*RESULT(FractionOfYear);   // sources+sinks, uptake etc.
	)
	
	EQUATION(Model, NO3Immobilisation,
		return RESULT(NO3ImmobilisationFraction) * RESULT(NO3ExternalFluxWithoutImmobilisation);
	)
	
	EQUATION(Model, NH4Immobilisation,
		return RESULT(NH4ImmobilisationFraction) * RESULT(NH4ExternalFluxWithoutImmobilisation);
	)

	*/