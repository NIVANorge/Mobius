

// NOTE: This model is in a very early development stage.

// This is (will be) a standard basic "driver" for the MAGIC Core.

// This will be an adaptation of MAGIC (the Model of Acidification of Groundwater In Catchments)
// B.J Cosby, R. C. Ferrier, A. Jenkins and R. F. Wright, 2001, Modelling the effects of acid deposition: refinements, adjustments and inclusion of nitrogen dynamics in the MAGIC model. Hydrol. Earth Syst. Sci, 5(3), 499-517


void AddMagicModel(mobius_model *Model)
{
	BeginModule(Model, "MAGIC driver", "_dev");
	
	
	auto CompartmentParams = RegisterParameterGroup(Model, "Soil"); //TODO: figure out group structure later. This should index over multiple soil boxes
	
	
	
	//TODO: More of these should be time series instead
	auto SoilOrganicCInput         = RegisterParameterDouble(Model, SoilParams, "Soil organic C input", MolPerM2PerYear, 0.0, 0.0, 1e6); 
	auto SoilOrganicCSink          = RegisterParameterDouble(Model, SoilParams, "Soil organic C sink", MolPerM2PerYear, 0.0, 0.0, 1e6);
	auto SoilOrganicCDecomposition = RegisterParameterDouble(Model, SoilParams, "Soil organic C decomposition", MolPerM2PerYear, 0.0, 0.0, 1e6);
	auto InitialSoilOrganicC       = RegisterParameterDouble(Model, SoilParams, "Initial soil organic C", MolPerM2, 0.0, 0.0, 1e8);

	auto SoilOrganicCNInputRatio          = RegisterParameterDouble(Model, SoilParams, "Soil organic C/N input ratio", Dimensionless, 0.1, 0.0001, 5.0);
	auto SoilOrganicCNSinkRatio           = RegisterParameterDouble(Model, SoilParams, "Soil organic C/N sink ratio", Dimensionless, 0.1, 0.0001, 5.0);
	auto SoilOrganicCNDecompositionRation = RegisterParameterDouble(Model, SoilParams, "Soil organic C/N decomposition ratio", Dimensionless, 0.1, 0.0001, 5.0);
	auto InitialSoilOrganicN              = RegisterParameterDouble(Model, SoilParams, "Initial soil organic N", MolPerM2, 0.0, 0.0, 1e8);

	auto UpperCNThresholdForNO3Immobilisation = RegisterParameterDouble(Model, SoilParams, "Upper C/N threshold for NO3 immobilisation", Dimensionless, 0.5, 0.0, 5.0, "C/N above this value - 100% NO3 immobilisation");
	auto UpperCNThresholdForNH4Immobilisation = RegisterParameterDouble(Model, SoilParams, "Upper C/N threshold for NH4 immobilisation", Dimensionless, 0.5, 0.0, 5.0, "C/N above this value - 100% NH4 immobilisation");
	auto LowerCNThresholdForNO3Immobilisation = RegisterParameterDouble(Model, SoilParams, "Lower C/N threshold for HO3 immobilisation", Dimensionless, 0.5, 0.0, 5.0,
	"C/N below this value - 0% NO3 immobilisation");
	auto LowerCNThresholdForNH4Immobilisation = RegisterParameterDouble(Model, SoilParams, "Lower C/N threshold for NH4 immobilisation", Dimensionless, 0.5, 0.0, 5.0,
	"C/N below this value - 0% NH4 immobilisation");
	
	
	auto NO3PlantUptake            = RegisterParameterDouble(Model, SoilParams, "NO3 plant uptake", MolPerM2PerYear, 0.0, 0.0, 1000.0);
	auto NH4PlantUptake            = RegisterParameterDouble(Model, SoilParams, "NH4 plant uptake", MolPerM2PerYear, 0.0, 0.0, 1000.0);

	auto NO3AtmosphericDeposition  = RegisterParameterDouble(Model, SoilParams, "NO3 atmospheric deposition", EqPerM2PerYear, 0.0, 0.0, 1000.0);
	auto NH4AtmosphericDeposition  = RegisterParameterDouble(Model, SoilParams, "NH4 atmospheric deposition", EqPerM2PerYear, 0.0, 0.0, 1000.0);
	auto CaAtmosphericDeposition   = RegisterParameterDouble(Model, SoilParams, "Ca atmospheric deposition",  EqPerM2PerYear, 0.0, 0.0, 1000.0);
	
	auto NO3SourcesAndSinks        = RegisterParameterDouble(Model, SoilParams, "NO3 sources and sinks", EqPerM2PerYear, 0.0, 0.0, 1000.0);
	auto NH4SourcesAndSinks        = RegisterParameterDouble(Model, SoilParams, "NH4 sources and sinks", EqPerM2PerYear, 0.0, 0.0, 1000.0);
	auto CaSourcesAndSinks         = RegisterParameterDouble(Model, SoilParams, "Ca sources and sinks",  EqPerM2PerYear, 0.0, 0.0, 1000.0);
	
	//TODO: Paper says weathering rates could be pH dependent. What is the equation for that?
	auto NO3WeatheringRate         = RegisterParameterDouble(Model, SoilParams, "NO3 weathering rate", EqPerM2PerYear, 0.0, 0.0, 1000.0);
	auto NH4WeatheringRate         = RegisterParameterDouble(Model, SoilParams, "NH4 weathering rate", EqPerM2PerYear, 0.0, 0.0, 1000.0);
	auto CaWeatheringRate          = RegisterParameterDouble(Model, SoilParams, "Ca weathering rate",  EqPerM2PerYear, 0.0, 0.0, 1000.0);
	
	auto Nitrification             = RegisterParameterDouble(Model, SoilParams, "Nitrification", EqPerM2PerYear, 0.0, 0.0, 1000.0);
	auto Denitrification           = RegisterParameterDouble(Model, SoilParams, "Denitrification", EqPerM2PerYear, 0.0, 0.0, 1000.0);
	



	auto Discharge      = RegisterEquation(Model, "Discharge", MPerYear);
	

	auto SoilOrganicNMineralisation = RegisterEquation(Model, "Soil organic N mineralisation", MolPerM2PerYear);

	auto SoilOrganicC = RegisterEquation(Model, "Soil organic C", MolPerM2);
	SetInitialValue(Model, SoilOrganicC, InitialSoilOrganicC);
	
	auto SoilOrganicN = RegisterEquation(Model, "Soil organic N", MolPerM2);
	SetInitialValue(Model, SoilOrganicN, InitialSoilOrganicN);

	auto SoilNO3ImmobilisationFraction = RegisterEquation(Model, "Soil NO3 immobilisation fraction", Dimensionless);
	auto SoilNO3Immobilisation         = RegisterEquation(Model, "Soil NO3 immobilisation", MolPerM2PerYear);
	auto SoilNH4ImmobilisationFraction = RegisterEquation(Model, "Soil NH4 immobilisation fraction", Dimensionless);
	auto SoilNH4Immobilisation         = RegisterEquation(Model, "Soil NH4 immobilisation", MolPerM2PerYear);
	auto SoilCNRatio                   = RegisterEquation(Model, "Soil C/N ratio", Dimensionless);
	
	
	EQUATION(Model, Discharge,
		return 0.0;   //TODO
	)
	
	
	
	
	
	
	EQUATION(Model, SoilOrganicNMineralisation,
		return PARAMETER(SoilOrganicCDecomposition) / PARAMETER(SoilOrganicCNDecompositionRation);
	)
	
	EQUATION(Model, SoilOrganicC,
		return LAST_RESULT(SoilOrganicC) + PARAMETER(SoilOrganicCInput) - PARAMETER(SoilOrganicCSink) - PARAMETER(SoilOrganicCDecomposition);
	)
	
	EQUATION(Model, SoilOrganicN,
		return
			  LAST_RESULT(SoilOrganicN)
			+ RESULT(SoilNO3Immobilisation)
			+ RESULT(SoilNH4Immobilisation)
			- RESULT(SoilOrganicNMineralisation)
			+ PARAMETER(SoilOrganicCInput) / PARAMETER(SoilOrganicCNInputRatio)
			- PARAMETER(SoilOrganicCSink)  / PARAMETER(SoilOrganicCNSinkRatio);
	)
	
	EQUATION(Model, SoilCNRatio,
		return SafeDivide(RESULT(SoilOrganicC), RESULT(SoilOrganicN));
	)
	
	EQUATION(Model, SoilNO3ImmobilisationFraction,
		return LinearResponse(RESULT(SoilCNRatio), PARAMETER(LowerCNThresholdForNO3Immobilisation), PARAMETER(UpperCNThresholdForNO3Immobilisation), 0.0, 1.0);
	)
	
	EQUATION(Model, SoilNH4ImmobilisationFraction,
		return LinearResponse(RESULT(SoilCNRatio), PARAMETER(LowerCNThresholdForNH4Immobilisation), PARAMETER(UpperCNThresholdForNH4Immobilisation), 0.0, 1.0);
	)
	
	EQUATION(Model, SoilNO3Immobilisation,
		return
			RESULT(SoilNO3ImmobilisationFraction) *
			(
				  PARAMETER(NO3AtmosphericDeposition)
				+ PARAMETER(NO3SourcesAndSinks)
				+ PARAMETER(NO3WeatheringRate)
				+ PARAMETER(Nitrification)
				- PARAMETER(NO3PlantUptake)
			);
	)
	
	EQUATION(Model, SoilNH4Immobilisation,
		return
			RESULT(SoilNH4ImmobilisationFraction) *
			(
				  PARAMETER(NH4AtmosphericDeposition)
				+ PARAMETER(NH4SourcesAndSinks)
				+ PARAMETER(NH4WeatheringRate)
				+ RESULT(SoilOrganicNMineralisation)
				- PARAMETER(Nitrification)
				- PARAMETER(NH4PlantUptake)
			);
	)
	
	EQUATION(Model, NO3ExternalFlux,
		return
			  PARAMETER(NO3AtmosphericDeposition)
			+ PARAMETER(NO3WeatheringRate)
			+ PARAMETER(NO3SourcesAndSinks)
			+ PARAMETER(Nitrification)
			- RESULT(SoilNO3Immobilisation)
			- PARAMETER(NO3PlantUptake)
			- PARAMETER(Denitrification)
	)
	

	EQUATION(Model, NH4ExternalFlux,
		return
			  PARAMETER(NH4AtmosphericDeposition)
			+ PARAMETER(NH4WeatheringRate)
			+ PARAMETER(NH4SourcesAndSinks)
			+ RESULT(SoilOrganicNMineralisation)
			- RESULT(SoilNH4Immobilisation)
			- PARAMETER(NH4PlantUptake)
			- PARAMETER(Nitrification)
	)
	

	
	EndModule(Model);
}