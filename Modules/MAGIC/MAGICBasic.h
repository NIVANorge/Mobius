

// NOTE: This model is in a very early development stage.

// This is (will be) a standard basic "driver" for the MAGIC Core.

// This will be an adaptation of MAGIC (the Model of Acidification of Groundwater In Catchments)
// B.J Cosby, R. C. Ferrier, A. Jenkins and R. F. Wright, 2001, Modelling the effects of acid deposition: refinements, adjustments and inclusion of nitrogen dynamics in the MAGIC model. Hydrol. Earth Syst. Sci, 5(3), 499-517


void AddMagicModel(mobius_model *Model)
{
	BeginModule(Model, "MAGIC driver", "_dev");
	
	auto Dimensionless  = RegisterUnit(Model);
	auto MPerTs         = RegisterUnit(Model, "m/timestep");
	auto MPerYear       = RegisterUnit(Model, "m/year");
	auto MEqPerM2PerTs  = RegisterUnit(Model, "meq/m2/timestep");
	auto MEqPerM3       = RegisterUnit(Model, "meq/m3");
	
	auto Compartment    = RegisterIndexSet(Model, "Compartment");
	
	auto DepositionParams      = RegisterParameterGroup(Model, "Deposition");
	auto DepositionCompartment = RegisterParameterGroup(Model, "Deposition by compartment", Compartment);
	auto CompartmentParams = GetParameterGroupHandle(Model, "Compartment parameters");
	
	auto PrecipPar             = RegisterParameterDouble(Model, DepositionParams, "Precipitation", MPerYear, 0.0, 0.0, 100.0, "Default value for timesteps where no input series value is provided");
	auto CaWetDeposition       = RegisterParameterDouble(Model, DepositionParams, "Ca wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation");
	auto MgWetDeposition       = RegisterParameterDouble(Model, DepositionParams, "Mg wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation");
	auto NaWetDeposition       = RegisterParameterDouble(Model, DepositionParams, "Na wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation");
	auto KWetDeposition        = RegisterParameterDouble(Model, DepositionParams, "K wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation");
	auto NH4WetDeposition      = RegisterParameterDouble(Model, DepositionParams, "NH4 wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation");
	auto SO4WetDeposition      = RegisterParameterDouble(Model, DepositionParams, "SO4 wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation");
	auto ClWetDeposition       = RegisterParameterDouble(Model, DepositionParams, "Cl wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation");
	auto NO3WetDeposition      = RegisterParameterDouble(Model, DepositionParams, "NO3 wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation");
	auto FWetDeposition        = RegisterParameterDouble(Model, DepositionParams, "F wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation");
	
	
	auto DischargePar          = RegisterParameterDouble(Model, CompartmentParams, "Discharge", MPerYear, 0.0, 0.0, 100.0, "Default value for timesteps where no input series value is provided");
	
	auto CaDryDepositionFactor  = RegisterParameterDouble(Model, DepositionCompartment, "Ca dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto MgDryDepositionFactor  = RegisterParameterDouble(Model, DepositionCompartment, "Mg dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto NaDryDepositionFactor  = RegisterParameterDouble(Model, DepositionCompartment, "Na dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto KDryDepositionFactor   = RegisterParameterDouble(Model, DepositionCompartment, "K dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto NH4DryDepositionFactor = RegisterParameterDouble(Model, DepositionCompartment, "NH4 dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto SO4DryDepositionFactor = RegisterParameterDouble(Model, DepositionCompartment, "SO4 dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto ClDryDepositionFactor  = RegisterParameterDouble(Model, DepositionCompartment, "Cl dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto NO3DryDepositionFactor = RegisterParameterDouble(Model, DepositionCompartment, "NO3 dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto FDryDepositionFactor   = RegisterParameterDouble(Model, DepositionCompartment, "F dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");

	
	
	auto Discharge          = RegisterEquation(Model, "Discharge", MPerTs);
	auto Precipitation      = RegisterEquation(Model, "Precipitation", MPerTs);
	
	auto CaExternalFlux     = RegisterEquation(Model, "Sum of Ca fluxes not related to discharge", MEqPerM2PerTs);
	auto MgExternalFlux     = RegisterEquation(Model, "Sum of Mg fluxes not related to discharge", MEqPerM2PerTs);
	auto NaExternalFlux     = RegisterEquation(Model, "Sum of Na fluxes not related to discharge", MEqPerM2PerTs);
	auto KExternalFlux      = RegisterEquation(Model, "Sum of K fluxes not related to discharge", MEqPerM2PerTs);
	auto NH4ExternalFlux    = RegisterEquation(Model, "Sum of NH4 fluxes not related to discharge", MEqPerM2PerTs);
	auto SO4ExternalFlux    = RegisterEquation(Model, "Sum of SO4 fluxes not related to discharge", MEqPerM2PerTs);
	auto ClExternalFlux     = RegisterEquation(Model, "Sum of Cl fluxes not related to discharge", MEqPerM2PerTs);
	auto NO3ExternalFlux    = RegisterEquation(Model, "Sum of NO3 fluxes not related to discharge", MEqPerM2PerTs);
	auto FExternalFlux      = RegisterEquation(Model, "Sum of F fluxes not related to discharge", MEqPerM2PerTs);
	
	
	auto CaDeposition       = RegisterEquation(Model, "Ca deposition", MEqPerM2PerTs);
	auto MgDeposition       = RegisterEquation(Model, "Mg deposition", MEqPerM2PerTs);
	auto NaDeposition       = RegisterEquation(Model, "Na deposition", MEqPerM2PerTs);
	auto KDeposition        = RegisterEquation(Model, "K deposition", MEqPerM2PerTs);
	auto NH4Deposition      = RegisterEquation(Model, "NH4 deposition", MEqPerM2PerTs);
	auto SO4Deposition      = RegisterEquation(Model, "SO4 deposition", MEqPerM2PerTs);
	auto ClDeposition       = RegisterEquation(Model, "Cl deposition", MEqPerM2PerTs);
	auto NO3Deposition      = RegisterEquation(Model, "NO3 deposition", MEqPerM2PerTs);
	auto FDeposition        = RegisterEquation(Model, "F deposition", MEqPerM2PerTs);
	
	auto PrecipIn           = RegisterInput(Model, "Precipitation", MPerTs);
	auto DischargeIn        = RegisterInput(Model, "Discharge", MPerTs);
	
	EQUATION(Model, Precipitation,
		double prin  = INPUT(PrecipIn);
		double prpar = PARAMETER(PrecipPar) * (double) CURRENT_TIME().StepLengthInSeconds / (86400.0*(double)CURRENT_TIME().DaysThisYear);
		if(prin > 0.0) return prin;
		return prpar;
	)
	
	EQUATION(Model, Discharge,
		double disin  = INPUT(DischargeIn);
		double dispar = PARAMETER(DischargePar) * (double) CURRENT_TIME().StepLengthInSeconds / (86400.0*(double)CURRENT_TIME().DaysThisYear);
		if(disin > 0.0) return disin;
		return dispar;
	)
	
	
	
	EQUATION(Model, CaDeposition,
		return RESULT(Precipitation)*PARAMETER(CaWetDeposition)*PARAMETER(CaDryDepositionFactor);
	)
	
	EQUATION(Model, MgDeposition,
		return RESULT(Precipitation)*PARAMETER(MgWetDeposition)*PARAMETER(MgDryDepositionFactor);
	)
	
	EQUATION(Model, NaDeposition,
		return RESULT(Precipitation)*PARAMETER(NaWetDeposition)*PARAMETER(NaDryDepositionFactor);
	)
	
	EQUATION(Model, KDeposition,
		return RESULT(Precipitation)*PARAMETER(KWetDeposition)*PARAMETER(KDryDepositionFactor);
	)
	
	EQUATION(Model, NH4Deposition,
		return RESULT(Precipitation)*PARAMETER(NH4WetDeposition)*PARAMETER(NH4DryDepositionFactor);
	)
	
	EQUATION(Model, SO4Deposition,
		return RESULT(Precipitation)*PARAMETER(SO4WetDeposition)*PARAMETER(SO4DryDepositionFactor);
	)
	
	EQUATION(Model, ClDeposition,
		return RESULT(Precipitation)*PARAMETER(ClWetDeposition)*PARAMETER(ClDryDepositionFactor);
	)
	
	EQUATION(Model, NO3Deposition,
		return RESULT(Precipitation)*PARAMETER(NO3WetDeposition)*PARAMETER(NO3DryDepositionFactor);
	)
	
	EQUATION(Model, FDeposition,
		return RESULT(Precipitation)*PARAMETER(FWetDeposition)*PARAMETER(FDryDepositionFactor);
	)
	
	
	
	
	EQUATION(Model, CaExternalFlux,
		return RESULT(CaDeposition);   // weathering, sources+sinks, etc.
	)
	
	EQUATION(Model, MgExternalFlux,
		return RESULT(MgDeposition);   // weathering, sources+sinks, etc.
	)
	
	EQUATION(Model, NaExternalFlux,
		return RESULT(NaDeposition);   // weathering, sources+sinks, etc.
	)
	
	EQUATION(Model, KExternalFlux,
		return RESULT(KDeposition);    // weathering, sources+sinks, etc.
	)
	
	EQUATION(Model, NH4ExternalFlux,
		return RESULT(NH4Deposition);   // weathering, sources+sinks, etc.
	)
	
	EQUATION(Model, SO4ExternalFlux,
		return RESULT(SO4Deposition);   // weathering, sources+sinks, etc.
	)
	
	EQUATION(Model, ClExternalFlux,
		return RESULT(ClDeposition);   // weathering, sources+sinks, etc.
	)
	
	EQUATION(Model, NO3ExternalFlux,
		return RESULT(NO3Deposition);   // weathering, sources+sinks, etc.
	)
	
	EQUATION(Model, FExternalFlux,
		return RESULT(FDeposition);   // weathering, sources+sinks, etc.
	)
	
	
	/*
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
	
	*/

	
	EndModule(Model);
}