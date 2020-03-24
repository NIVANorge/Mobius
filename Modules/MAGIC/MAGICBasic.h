

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
	auto YearPerTs      = RegisterUnit(Model, "year/timestep");
	auto MEqPerM2PerYear = RegisterUnit(Model, "meq/m2/year");
	auto MEqPerM2PerTs  = RegisterUnit(Model, "meq/m2/timestep");
	auto MEqPerM3       = RegisterUnit(Model, "meq/m3");
	auto MMolPerM2      = RegisterUnit(Model, "mmol/m2");
	auto MMolPerM2PerYear = RegisterUnit(Model, "mmol/m2/year");
	
	auto Compartment    = RegisterIndexSet(Model, "Compartment");
	
	auto DepositionParams      = RegisterParameterGroup(Model, "Deposition");
	auto DepositionCompartment = RegisterParameterGroup(Model, "Deposition by compartment", Compartment);
	auto WeatheringCompartment = RegisterParameterGroup(Model, "Weathering by compartment", Compartment);
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

	auto CaWeathering           = RegisterParameterDouble(Model, WeatheringCompartment, "Ca weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto MgWeathering           = RegisterParameterDouble(Model, WeatheringCompartment, "Mg weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NaWeathering           = RegisterParameterDouble(Model, WeatheringCompartment, "Na weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto KWeathering            = RegisterParameterDouble(Model, WeatheringCompartment, "K weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NH4Weathering          = RegisterParameterDouble(Model, WeatheringCompartment, "NH4 weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto SO4Weathering          = RegisterParameterDouble(Model, WeatheringCompartment, "SO4 weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto ClWeathering           = RegisterParameterDouble(Model, WeatheringCompartment, "Cl weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NO3Weathering          = RegisterParameterDouble(Model, WeatheringCompartment, "NO3 weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto FWeathering            = RegisterParameterDouble(Model, WeatheringCompartment, "F weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	
	
	auto FractionOfYear     = RegisterEquation(Model, "Fraction of year", YearPerTs);
	
	auto Discharge          = RegisterEquation(Model, "Discharge", MPerTs);
	auto Precipitation      = RegisterEquation(Model, "Precipitation", MPerTs);
	
	
	auto CompartmentSolver = GetSolverHandle(Model, "Compartment solver");
	
	auto CaExternalFlux     = RegisterEquation(Model, "Sum of Ca fluxes not related to discharge", MEqPerM2PerTs);
	auto MgExternalFlux     = RegisterEquation(Model, "Sum of Mg fluxes not related to discharge", MEqPerM2PerTs);
	auto NaExternalFlux     = RegisterEquation(Model, "Sum of Na fluxes not related to discharge", MEqPerM2PerTs);
	auto KExternalFlux      = RegisterEquation(Model, "Sum of K fluxes not related to discharge", MEqPerM2PerTs);
	auto NH4ExternalFlux    = RegisterEquation(Model, "Sum of NH4 fluxes not related to discharge", MEqPerM2PerTs, CompartmentSolver);
	auto SO4ExternalFlux    = RegisterEquation(Model, "Sum of SO4 fluxes not related to discharge", MEqPerM2PerTs);
	auto ClExternalFlux     = RegisterEquation(Model, "Sum of Cl fluxes not related to discharge", MEqPerM2PerTs);
	auto NO3ExternalFlux    = RegisterEquation(Model, "Sum of NO3 fluxes not related to discharge", MEqPerM2PerTs, CompartmentSolver);
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
	

	
	auto CAndN             = RegisterParameterGroup(Model, "Carbon and Nitrogen", Compartment);
	
	auto OrganicCInput                    = RegisterParameterDouble(Model, CAndN, "Organic C input", MMolPerM2PerYear, 0.0, 0.0, 1e6); 
	auto OrganicCSink                     = RegisterParameterDouble(Model, CAndN, "Organic C sink", MMolPerM2PerYear, 0.0, 0.0, 1e6);
	auto OrganicCDecomposition            = RegisterParameterDouble(Model, CAndN, "Organic C decomposition", MMolPerM2PerYear, 0.0, 0.0, 1e6);
	auto InitialOrganicC                  = RegisterParameterDouble(Model, CAndN, "Initial organic C", MMolPerM2, 0.0, 0.0, 1e8);

	auto OrganicCNInputRatio              = RegisterParameterDouble(Model, CAndN, "Organic C/N input ratio", Dimensionless, 0.1, 0.0001, 5.0);
	auto OrganicCNSinkRatio               = RegisterParameterDouble(Model, CAndN, "Organic C/N sink ratio", Dimensionless, 0.1, 0.0001, 5.0);
	auto OrganicCNDecompositionRation     = RegisterParameterDouble(Model, CAndN, "Organic C/N decomposition ratio", Dimensionless, 0.1, 0.0001, 5.0);
	auto InitialOrganicN                  = RegisterParameterDouble(Model, CAndN, "Initial organic N", MMolPerM2, 0.0, 0.0, 1e8);

	auto UpperCNThresholdForNO3Immobilisation = RegisterParameterDouble(Model, CAndN, "Upper C/N threshold for NO3 immobilisation", Dimensionless, 0.5, 0.0, 5.0, "C/N above this value - 100% NO3 immobilisation");
	auto UpperCNThresholdForNH4Immobilisation = RegisterParameterDouble(Model, CAndN, "Upper C/N threshold for NH4 immobilisation", Dimensionless, 0.5, 0.0, 5.0, "C/N above this value - 100% NH4 immobilisation");
	auto LowerCNThresholdForNO3Immobilisation = RegisterParameterDouble(Model, CAndN, "Lower C/N threshold for NO3 immobilisation", Dimensionless, 0.5, 0.0, 5.0,
	"C/N below this value - 0% NO3 immobilisation");
	auto LowerCNThresholdForNH4Immobilisation = RegisterParameterDouble(Model, CAndN, "Lower C/N threshold for NH4 immobilisation", Dimensionless, 0.5, 0.0, 5.0,
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
	
	auto PrecipIn           = RegisterInput(Model, "Precipitation", MPerTs);
	auto DischargeIn        = RegisterInput(Model, "Discharge", MPerTs);
	
	
	EQUATION(Model, FractionOfYear,
		return (double)CURRENT_TIME().StepLengthInSeconds / (86400.0*(double)CURRENT_TIME().DaysThisYear);
	)
	
	EQUATION(Model, Precipitation,
		double prin  = INPUT(PrecipIn);
		double prpar = PARAMETER(PrecipPar) * RESULT(FractionOfYear);
		if(prin > 0.0) return prin;
		return prpar;
	)
	
	EQUATION(Model, Discharge,
		double disin  = INPUT(DischargeIn);
		double dispar = PARAMETER(DischargePar) * RESULT(FractionOfYear);
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
		return RESULT(CaDeposition) + PARAMETER(CaWeathering)*RESULT(FractionOfYear);   // sources+sinks, etc.
	)
	
	EQUATION(Model, MgExternalFlux,
		return RESULT(MgDeposition) + PARAMETER(MgWeathering)*RESULT(FractionOfYear);   // sources+sinks, etc.
	)
	
	EQUATION(Model, NaExternalFlux,
		return RESULT(NaDeposition) + PARAMETER(NaWeathering)*RESULT(FractionOfYear);   // sources+sinks, etc.
	)
	
	EQUATION(Model, KExternalFlux,
		return RESULT(KDeposition) + PARAMETER(KWeathering)*RESULT(FractionOfYear);   // sources+sinks, etc.
	)
	
	EQUATION(Model, NH4ExternalFlux,
		return RESULT(NH4ExternalFluxWithoutImmobilisation) - RESULT(NH4Immobilisation);
	)
	
	EQUATION(Model, SO4ExternalFlux,
		return RESULT(SO4Deposition) + PARAMETER(SO4Weathering)*RESULT(FractionOfYear);   // sources+sinks, etc.
	)
	
	EQUATION(Model, ClExternalFlux,
		return RESULT(ClDeposition) + PARAMETER(ClWeathering)*RESULT(FractionOfYear);   // sources+sinks, etc.
	)
	
	EQUATION(Model, NO3ExternalFlux,
		return RESULT(NO3ExternalFluxWithoutImmobilisation) - RESULT(NO3Immobilisation);
	)
	
	EQUATION(Model, FExternalFlux,
		return RESULT(FDeposition) + PARAMETER(FWeathering)*RESULT(FractionOfYear);   // sources+sinks, etc.
	)
	
	
	
	
	
	
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
		return LinearResponse(RESULT(CNRatio), PARAMETER(LowerCNThresholdForNO3Immobilisation), PARAMETER(UpperCNThresholdForNO3Immobilisation), 0.0, 1.0);
	)
	
	EQUATION(Model, NH4ImmobilisationFraction,
		return LinearResponse(RESULT(CNRatio), PARAMETER(LowerCNThresholdForNH4Immobilisation), PARAMETER(UpperCNThresholdForNH4Immobilisation), 0.0, 1.0);
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

	
	EndModule(Model);
}