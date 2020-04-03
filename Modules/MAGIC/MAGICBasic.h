

// NOTE: This model is in a very early development stage.

// This is (will be) a standard basic "driver" for the MAGIC Core.

// This will be an adaptation of MAGIC (the Model of Acidification of Groundwater In Catchments)
// B.J Cosby, R. C. Ferrier, A. Jenkins and R. F. Wright, 2001, Modelling the effects of acid deposition: refinements, adjustments and inclusion of nitrogen dynamics in the MAGIC model. Hydrol. Earth Syst. Sci, 5(3), 499-517


void AddMagicModel(mobius_model *Model)
{
	BeginModule(Model, "MAGIC driver", "_dev");
	
	auto Dimensionless  = RegisterUnit(Model);
	auto DegreesCelsius	= RegisterUnit(Model, "°C");
	auto MPerTs         = RegisterUnit(Model, "m/month");
	auto MPerYear       = RegisterUnit(Model, "m/year");
	auto YearPerTs      = RegisterUnit(Model, "year/month");
	auto MEqPerM2       = RegisterUnit(Model, "meq/m2");
	auto MEqPerM2PerYear = RegisterUnit(Model, "meq/m2/year");
	auto MEqPerM2PerTs  = RegisterUnit(Model, "meq/m2/month");
	auto MEqPerM3       = RegisterUnit(Model, "meq/m3");
	auto MMolPerM2      = RegisterUnit(Model, "mmol/m2");
	auto MMolPerM3      = RegisterUnit(Model, "mmol/m3");
	auto MMolPerM2PerYear = RegisterUnit(Model, "mmol/m2/year");
	auto MMolPerM2PerTs = RegisterUnit(Model, "mmol/m2/month");
	auto Percent        = RegisterUnit(Model, "%");
	
	auto Compartment    = RegisterIndexSet(Model, "Compartment");
	
	auto DepositionParams      = RegisterParameterGroup(Model, "Deposition");
	auto DepositionCompartment = RegisterParameterGroup(Model, "Deposition by compartment", Compartment);
	auto WeatheringCompartment = RegisterParameterGroup(Model, "Weathering by compartment", Compartment);
	auto SourcesSinksCompartment = RegisterParameterGroup(Model, "Sources and sinks by compartment", Compartment);
	auto ClimateParams         = RegisterParameterGroup(Model, "Climate by compartment", Compartment);
	
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
	
	
	auto DischargePar          = RegisterParameterDouble(Model, ClimateParams, "Discharge", MPerYear, 0.0, 0.0, 100.0, "Default value for timesteps where no input series value is provided");
	auto TemperaturePar        = RegisterParameterDouble(Model, ClimateParams, "Temperature", DegreesCelsius, 0.0, -20.0, 40.0, "Default value for timesteps where no input series value is provided");
	auto PartialPressureCO2Par = RegisterParameterDouble(Model, ClimateParams, "CO2 partial pressure", Percent, 20.0, 0.1, 40.0, "Default value for timesteps where no input series value is provided");
	auto DOCConcentrationPar   = RegisterParameterDouble(Model, ClimateParams, "DOC concentration", MMolPerM3, 0.0, 0.0, 500.0, "Default value for timesteps where no input series value is provided");

	
	
	auto CaDryDepositionFactor  = RegisterParameterDouble(Model, DepositionCompartment, "Ca dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto MgDryDepositionFactor  = RegisterParameterDouble(Model, DepositionCompartment, "Mg dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto NaDryDepositionFactor  = RegisterParameterDouble(Model, DepositionCompartment, "Na dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto KDryDepositionFactor   = RegisterParameterDouble(Model, DepositionCompartment, "K dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto NH4DryDepositionFactor = RegisterParameterDouble(Model, DepositionCompartment, "NH4 dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto SO4DryDepositionFactor = RegisterParameterDouble(Model, DepositionCompartment, "SO4 dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto ClDryDepositionFactor  = RegisterParameterDouble(Model, DepositionCompartment, "Cl dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto NO3DryDepositionFactor = RegisterParameterDouble(Model, DepositionCompartment, "NO3 dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto FDryDepositionFactor   = RegisterParameterDouble(Model, DepositionCompartment, "F dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");

	auto PrecipIn                  = RegisterInput(Model, "Precipitation", MPerTs);
	auto PrecipSeasonal            = RegisterInput(Model, "Precipitation seasonal distribution", Percent);
	auto DischargeIn               = RegisterInput(Model, "Discharge", MPerTs);
	auto DischargeSeasonal         = RegisterInput(Model, "Discharge seasonal distribution", Percent);
	auto TemperatureIn             = RegisterInput(Model, "Temperature", DegreesCelsius);
	auto PartialPressureCO2In      = RegisterInput(Model, "CO2 partial pressure", Percent);
	auto DOCConcentrationIn        = RegisterInput(Model, "DOC concentration", MMolPerM3);

	auto CaWetDepositionScale      = RegisterInput(Model, "Ca wet deposition scaling factor", Dimensionless);
	auto MgWetDepositionScale      = RegisterInput(Model, "Mg wet deposition scaling factor", Dimensionless);
	auto NaWetDepositionScale      = RegisterInput(Model, "Na wet deposition scaling factor", Dimensionless);
	auto KWetDepositionScale       = RegisterInput(Model, "K wet deposition scaling factor", Dimensionless);
	auto NH4WetDepositionScale     = RegisterInput(Model, "NH4 wet deposition scaling factor", Dimensionless);
	auto SO4WetDepositionScale     = RegisterInput(Model, "SO4 wet deposition scaling factor", Dimensionless);
	auto ClWetDepositionScale      = RegisterInput(Model, "Cl wet deposition scaling factor", Dimensionless);
	auto NO3WetDepositionScale     = RegisterInput(Model, "NO3 wet deposition scaling factor", Dimensionless);
	auto FWetDepositionScale       = RegisterInput(Model, "F wet deposition scaling factor", Dimensionless);

	


	auto CaWeathering           = RegisterParameterDouble(Model, WeatheringCompartment, "Ca weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto MgWeathering           = RegisterParameterDouble(Model, WeatheringCompartment, "Mg weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NaWeathering           = RegisterParameterDouble(Model, WeatheringCompartment, "Na weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto KWeathering            = RegisterParameterDouble(Model, WeatheringCompartment, "K weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NH4Weathering          = RegisterParameterDouble(Model, WeatheringCompartment, "NH4 weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto SO4Weathering          = RegisterParameterDouble(Model, WeatheringCompartment, "SO4 weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto ClWeathering           = RegisterParameterDouble(Model, WeatheringCompartment, "Cl weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NO3Weathering          = RegisterParameterDouble(Model, WeatheringCompartment, "NO3 weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto FWeathering            = RegisterParameterDouble(Model, WeatheringCompartment, "F weathering", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	
	auto CaSinks                = RegisterParameterDouble(Model, SourcesSinksCompartment, "Ca sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto MgSinks                = RegisterParameterDouble(Model, SourcesSinksCompartment, "Mg sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NaSinks                = RegisterParameterDouble(Model, SourcesSinksCompartment, "Na sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto KSinks                 = RegisterParameterDouble(Model, SourcesSinksCompartment, "K sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NH4Sinks               = RegisterParameterDouble(Model, SourcesSinksCompartment, "NH4 sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto SO4Sinks               = RegisterParameterDouble(Model, SourcesSinksCompartment, "SO4 sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto ClSinks                = RegisterParameterDouble(Model, SourcesSinksCompartment, "Cl sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NO3Sinks               = RegisterParameterDouble(Model, SourcesSinksCompartment, "NO3 sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto FSinks                 = RegisterParameterDouble(Model, SourcesSinksCompartment, "F sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	
	auto CaSources              = RegisterParameterDouble(Model, SourcesSinksCompartment, "Ca sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto MgSources              = RegisterParameterDouble(Model, SourcesSinksCompartment, "Mg sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NaSources              = RegisterParameterDouble(Model, SourcesSinksCompartment, "Na sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto KSources               = RegisterParameterDouble(Model, SourcesSinksCompartment, "K sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NH4Sources             = RegisterParameterDouble(Model, SourcesSinksCompartment, "NH4 sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto SO4Sources             = RegisterParameterDouble(Model, SourcesSinksCompartment, "SO4 sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto ClSources              = RegisterParameterDouble(Model, SourcesSinksCompartment, "Cl sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NO3Sources             = RegisterParameterDouble(Model, SourcesSinksCompartment, "NO3 sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto FSources               = RegisterParameterDouble(Model, SourcesSinksCompartment, "F sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	
	
	auto FractionOfYear     = RegisterEquation(Model, "Fraction of year", YearPerTs);
	
	auto Precipitation      = RegisterEquation(Model, "Precipitation", MPerTs);
	auto Discharge          = RegisterEquation(Model, "Discharge", MPerTs);
	auto Temperature        = RegisterEquation(Model, "Temperature", DegreesCelsius);
	auto PartialPressureCO2 = RegisterEquation(Model, "CO2 partial pressure", Percent);
	auto DOCConcentration   = RegisterEquation(Model, "DOC concentration", MMolPerM3);
	
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
	
	auto Nitrification      = RegisterParameterDouble(Model, CAndN, "Nitrification", MMolPerM2PerYear, 0.0, -100.0, 500.0, "Negative rate sets value as % of inputs");
	auto Denitrification    = RegisterParameterDouble(Model, CAndN, "Denitrification", MMolPerM2PerYear, 0.0, -100.0, 500.0, "Negative rate sets value as % of inputs");
	auto NO3Immobilisation  = RegisterParameterDouble(Model, CAndN, "NO3 immobilisation", MMolPerM2PerYear, 0.0, -100.0, 500.0, "Negative rate sets value as % of inputs");
	auto NH4Immobilisation  = RegisterParameterDouble(Model, CAndN, "NH4 immobilisation", MMolPerM2PerYear, 0.0, -100.0, 500.0, "Negative rate sets value as % of inputs");
	auto Mineralisation     = RegisterParameterDouble(Model, CAndN, "Mineralisation", MMolPerM2PerYear, 0.0, 0.0, 500.0);
	
	auto NO3Inputs           = RegisterEquation(Model, "NO3 inputs", MMolPerM2PerTs);
	auto NH4Inputs           = RegisterEquation(Model, "NH4 inputs", MMolPerM2PerTs);
	auto NitrificationEq     = RegisterEquation(Model, "Nitrification", MMolPerM2PerTs);
	auto DenitrificationEq   = RegisterEquation(Model, "Denitrification", MMolPerM2PerTs);
	auto NO3ImmobilisationEq = RegisterEquation(Model, "NO3 immobilisation", MMolPerM2PerTs);
	auto NH4ImmobilisationEq = RegisterEquation(Model, "NH4 immobilisation", MMolPerM2PerTs);


	
	
	
	EQUATION(Model, FractionOfYear,
		return (double)CURRENT_TIME().StepLengthInSeconds / (86400.0*(double)CURRENT_TIME().DaysThisYear);
	)
	
	EQUATION(Model, Precipitation,
		double in  = INPUT(PrecipIn);
		double fraction = RESULT(FractionOfYear);
		if(INPUT_WAS_PROVIDED(PrecipSeasonal)) fraction = INPUT(PrecipSeasonal)*0.01;
		double par = PARAMETER(PrecipPar) * fraction;
		if(in > 0.0) return in;
		return par;
	)
	
	EQUATION(Model, Discharge,
		double in  = INPUT(DischargeIn);
		double fraction = RESULT(FractionOfYear);
		if(INPUT_WAS_PROVIDED(DischargeSeasonal)) fraction = INPUT(DischargeSeasonal)*0.01;
		double par = PARAMETER(DischargePar) * fraction;
		if(in > 0.0) return in;
		return par;
	)
	
	EQUATION(Model, Temperature,
		//TODO: Hmm, it is not that good that this works differently to the others, but the problem is that 0 is a valid temperature value... Should we have an option to always clear an input series to NaN?
		double par = PARAMETER(TemperaturePar);
		if(INPUT_WAS_PROVIDED(TemperatureIn)) return INPUT(TemperatureIn);
		return par;
	)
	
	EQUATION(Model, PartialPressureCO2,
		double par = PARAMETER(PartialPressureCO2Par);
		double in  = INPUT(PartialPressureCO2In);
		if(in > 0.0) return in;
		return par;
	)
	
	EQUATION(Model, DOCConcentration,
		//TODO: 0 is actually a legitimate value here too though...
		double par = PARAMETER(DOCConcentrationPar);
		double in = INPUT(DOCConcentrationIn);
		if(in > 0.0) return in;
		return par;
	)
	
	EQUATION(Model, CaDeposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(CaWetDepositionScale)) scale = INPUT(CaWetDepositionScale);
		return RESULT(Precipitation)*PARAMETER(CaWetDeposition)*scale*PARAMETER(CaDryDepositionFactor);
	)
	
	EQUATION(Model, MgDeposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(MgWetDepositionScale)) scale = INPUT(MgWetDepositionScale);
		return RESULT(Precipitation)*PARAMETER(MgWetDeposition)*scale*PARAMETER(MgDryDepositionFactor);
	)
	
	EQUATION(Model, NaDeposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(NaWetDepositionScale)) scale = INPUT(NaWetDepositionScale);
		return RESULT(Precipitation)*PARAMETER(NaWetDeposition)*scale*PARAMETER(NaDryDepositionFactor);
	)
	
	EQUATION(Model, KDeposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(KWetDepositionScale)) scale = INPUT(KWetDepositionScale);
		return RESULT(Precipitation)*PARAMETER(KWetDeposition)*scale*PARAMETER(KDryDepositionFactor);
	)
	
	EQUATION(Model, NH4Deposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(NH4WetDepositionScale)) scale = INPUT(NH4WetDepositionScale);
		return RESULT(Precipitation)*PARAMETER(NH4WetDeposition)*scale*PARAMETER(NH4DryDepositionFactor);
	)
	
	EQUATION(Model, SO4Deposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(SO4WetDepositionScale)) scale = INPUT(SO4WetDepositionScale);
		return RESULT(Precipitation)*PARAMETER(SO4WetDeposition)*scale*PARAMETER(SO4DryDepositionFactor);
	)
	
	EQUATION(Model, ClDeposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(ClWetDepositionScale)) scale = INPUT(ClWetDepositionScale);
		return RESULT(Precipitation)*PARAMETER(ClWetDeposition)*scale*PARAMETER(ClDryDepositionFactor);
	)
	
	EQUATION(Model, NO3Deposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(NO3WetDepositionScale)) scale = INPUT(NO3WetDepositionScale);
		return RESULT(Precipitation)*PARAMETER(NO3WetDeposition)*scale*PARAMETER(NO3DryDepositionFactor);
	)
	
	EQUATION(Model, FDeposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(FWetDepositionScale)) scale = INPUT(FWetDepositionScale);
		return RESULT(Precipitation)*PARAMETER(FWetDeposition)*scale*PARAMETER(FDryDepositionFactor);
	)
	
	
	
	
	EQUATION(Model, NO3Inputs,
		return RESULT(NO3Deposition) + RESULT(FractionOfYear) * (PARAMETER(NO3Weathering) + PARAMETER(NO3Sources) - PARAMETER(NO3Sinks)) + RESULT(NitrificationEq);   // TODO: is it correct to subtract sinks here?
	)
	
	EQUATION(Model, NH4Inputs,
		return RESULT(NH4Deposition) + RESULT(FractionOfYear) * (PARAMETER(NH4Weathering) + PARAMETER(Mineralisation) + PARAMETER(NH4Sources) - PARAMETER(NH4Sinks));   // TODO: is it correct to subtract sinks here?
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
	
	
	EQUATION(Model, CaExternalFlux,
		return RESULT(CaDeposition) + (PARAMETER(CaWeathering) + PARAMETER(CaSources) - PARAMETER(CaSinks))*RESULT(FractionOfYear);   // sources+sinks, etc.
	)
	
	EQUATION(Model, MgExternalFlux,
		return RESULT(MgDeposition) + (PARAMETER(MgWeathering) + PARAMETER(MgSources) - PARAMETER(MgSinks))*RESULT(FractionOfYear);
	)
	
	EQUATION(Model, NaExternalFlux,
		return RESULT(NaDeposition) + (PARAMETER(NaWeathering) + PARAMETER(NaSources) - PARAMETER(NaSinks))*RESULT(FractionOfYear);
	)
	
	EQUATION(Model, KExternalFlux,
		return RESULT(KDeposition) + (PARAMETER(KWeathering) + PARAMETER(KSources) - PARAMETER(KSinks))*RESULT(FractionOfYear);
	)
	
	EQUATION(Model, SO4ExternalFlux,
		return RESULT(SO4Deposition) + (PARAMETER(SO4Weathering) + PARAMETER(SO4Sources) - PARAMETER(SO4Sinks))*RESULT(FractionOfYear);
	)
	
	EQUATION(Model, ClExternalFlux,
		return RESULT(ClDeposition) + (PARAMETER(ClWeathering) + PARAMETER(ClSources) - PARAMETER(ClSinks))*RESULT(FractionOfYear);
	)
	
	EQUATION(Model, FExternalFlux,
		return RESULT(FDeposition) + (PARAMETER(FWeathering) + PARAMETER(FSources) - PARAMETER(FSinks))*RESULT(FractionOfYear);
	)
	
	EQUATION(Model, NO3ExternalFlux,
		return RESULT(NO3Inputs) - RESULT(DenitrificationEq) - RESULT(NO3ImmobilisationEq);
	)
	
	EQUATION(Model, NH4ExternalFlux, 
		return RESULT(NH4Inputs) - RESULT(NitrificationEq) - RESULT(NH4ImmobilisationEq);
	)
	
	
	
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
	
	EndModule(Model);
}