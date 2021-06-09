

// NOTE: This model is in a very early development stage.

// This is (will be) a standard basic "driver" for the MAGIC Core.

// This will be an adaptation of MAGIC (the Model of Acidification of Groundwater In Catchments)
// B.J Cosby, R. C. Ferrier, A. Jenkins and R. F. Wright, 2001, Modelling the effects of acid deposition: refinements, adjustments and inclusion of nitrogen dynamics in the MAGIC model. Hydrol. Earth Syst. Sci, 5(3), 499-517


void AddMagicModel(mobius_model *Model)
{
	BeginModule(Model, "MAGIC driver", "_dev");
	
	SetModuleDescription(Model, R""""(
This is a basic module that provides flow, temperature, and other drivers for the MAGIC core.
)"""");
	
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
	auto CaWetDeposition       = RegisterParameterDouble(Model, DepositionParams, "Ca wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation", "DCa");
	auto MgWetDeposition       = RegisterParameterDouble(Model, DepositionParams, "Mg wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation", "DMg");
	auto NaWetDeposition       = RegisterParameterDouble(Model, DepositionParams, "Na wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation", "DNa");
	auto KWetDeposition        = RegisterParameterDouble(Model, DepositionParams, "K wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation", "DK");
	auto NH4WetDeposition      = RegisterParameterDouble(Model, DepositionParams, "NH4 wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation", "DNH4");
	auto SO4WetDeposition      = RegisterParameterDouble(Model, DepositionParams, "SO4 wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation", "DSO4");
	auto ClWetDeposition       = RegisterParameterDouble(Model, DepositionParams, "Cl wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation", "DCl");
	auto NO3WetDeposition      = RegisterParameterDouble(Model, DepositionParams, "NO3 wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation", "DNO3");
	auto FWetDeposition        = RegisterParameterDouble(Model, DepositionParams, "F wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation", "DF");
	auto PO4WetDeposition      = RegisterParameterDouble(Model, DepositionParams, "PO4 wet deposition", MEqPerM3, 0.0, 0.0, 500.0, "Concentration in precipitation", "DPO4");
	
	
	auto DischargePar          = RegisterParameterDouble(Model, ClimateParams, "Discharge", MPerYear, 0.0, 0.0, 100.0, "Default value for timesteps where no input series value is provided");
	auto TemperaturePar        = RegisterParameterDouble(Model, ClimateParams, "Temperature", DegreesCelsius, 0.0, -20.0, 40.0, "Default value for timesteps where no input series value is provided");
	auto PartialPressureCO2Par = RegisterParameterDouble(Model, ClimateParams, "CO2 partial pressure", Percent, 0.3, 0.1, 2.0, "Default value for timesteps where no input series value is provided");
	auto OAConcentrationPar   = RegisterParameterDouble(Model, ClimateParams, "Organic acid concentration", MMolPerM3, 0.0, 0.0, 500.0, "Default value for timesteps where no input series value is provided");

	
	auto IsTopLayer                = RegisterParameterBool(Model, DepositionCompartment, "This is a top layer", true, "Whether or not this compartment should receive deposition.");
	auto CaDryDepositionFactor  = RegisterParameterDouble(Model, DepositionCompartment, "Ca dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto MgDryDepositionFactor  = RegisterParameterDouble(Model, DepositionCompartment, "Mg dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto NaDryDepositionFactor  = RegisterParameterDouble(Model, DepositionCompartment, "Na dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto KDryDepositionFactor   = RegisterParameterDouble(Model, DepositionCompartment, "K dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto NH4DryDepositionFactor = RegisterParameterDouble(Model, DepositionCompartment, "NH4 dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto SO4DryDepositionFactor = RegisterParameterDouble(Model, DepositionCompartment, "SO4 dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto ClDryDepositionFactor  = RegisterParameterDouble(Model, DepositionCompartment, "Cl dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto NO3DryDepositionFactor = RegisterParameterDouble(Model, DepositionCompartment, "NO3 dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto FDryDepositionFactor   = RegisterParameterDouble(Model, DepositionCompartment, "F dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");
	auto PO4DryDepositionFactor = RegisterParameterDouble(Model, DepositionCompartment, "PO4 dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition with to get total deposition");

	auto PrecipIn                  = RegisterInput(Model, "Precipitation", MPerTs, true);
	auto PrecipSeasonal            = RegisterInput(Model, "Precipitation seasonal distribution", Percent);
	auto DischargeIn               = RegisterInput(Model, "Discharge", MPerTs, true);
	auto DischargeSeasonal         = RegisterInput(Model, "Discharge seasonal distribution", Percent);
	auto TemperatureIn             = RegisterInput(Model, "Temperature", DegreesCelsius, true);
	auto PartialPressureCO2In      = RegisterInput(Model, "CO2 partial pressure", Percent, true);
	auto OAConcentrationIn         = RegisterInput(Model, "Organic acid concentration", MMolPerM3, true);

	auto CaWetDepositionScale      = RegisterInput(Model, "Ca wet deposition scaling factor", Dimensionless);
	auto MgWetDepositionScale      = RegisterInput(Model, "Mg wet deposition scaling factor", Dimensionless);
	auto NaWetDepositionScale      = RegisterInput(Model, "Na wet deposition scaling factor", Dimensionless);
	auto KWetDepositionScale       = RegisterInput(Model, "K wet deposition scaling factor", Dimensionless);
	auto NH4WetDepositionScale     = RegisterInput(Model, "NH4 wet deposition scaling factor", Dimensionless);
	auto SO4WetDepositionScale     = RegisterInput(Model, "SO4 wet deposition scaling factor", Dimensionless);
	auto ClWetDepositionScale      = RegisterInput(Model, "Cl wet deposition scaling factor", Dimensionless);
	auto NO3WetDepositionScale     = RegisterInput(Model, "NO3 wet deposition scaling factor", Dimensionless);
	auto FWetDepositionScale       = RegisterInput(Model, "F wet deposition scaling factor", Dimensionless);
	auto PO4WetDepositionScale     = RegisterInput(Model, "PO4 wet deposition scaling factor", Dimensionless);

	// NOTE: The 'true' means that we clear these to NaN!
	auto CaWetDepositionIn         = RegisterInput(Model, "Ca wet deposition", MEqPerM3, true);
	auto MgWetDepositionIn         = RegisterInput(Model, "Mg wet deposition", MEqPerM3, true);
	auto NaWetDepositionIn         = RegisterInput(Model, "Na wet deposition", MEqPerM3, true);
	auto KWetDepositionIn          = RegisterInput(Model, "K wet deposition", MEqPerM3, true);
	auto NH4WetDepositionIn        = RegisterInput(Model, "NH4 wet deposition", MEqPerM3, true);
	auto SO4WetDepositionIn        = RegisterInput(Model, "SO4 wet deposition", MEqPerM3, true);
	auto ClWetDepositionIn         = RegisterInput(Model, "Cl wet deposition", MEqPerM3, true);
	auto NO3WetDepositionIn        = RegisterInput(Model, "NO3 wet deposition", MEqPerM3, true);
	auto FWetDepositionIn          = RegisterInput(Model, "F wet deposition", MEqPerM3, true);
	auto PO4WetDepositionIn        = RegisterInput(Model, "PO4 wet deposition", MEqPerM3, true);
	


	auto CaWeathering           = RegisterParameterDouble(Model, WeatheringCompartment, "Ca weathering", MEqPerM2PerYear, 0.0, 0.0, 5000.0, "", "WCa");
	auto MgWeathering           = RegisterParameterDouble(Model, WeatheringCompartment, "Mg weathering", MEqPerM2PerYear, 0.0, 0.0, 5000.0, "", "WMg");
	auto NaWeathering           = RegisterParameterDouble(Model, WeatheringCompartment, "Na weathering", MEqPerM2PerYear, 0.0, 0.0, 5000.0, "", "WNa");
	auto KWeathering            = RegisterParameterDouble(Model, WeatheringCompartment, "K weathering", MEqPerM2PerYear, 0.0, 0.0, 5000.0, "", "WK");
	auto NH4Weathering          = RegisterParameterDouble(Model, WeatheringCompartment, "NH4 weathering", MEqPerM2PerYear, 0.0, 0.0, 5000.0, "", "WNH4");
	auto SO4Weathering          = RegisterParameterDouble(Model, WeatheringCompartment, "SO4 weathering", MEqPerM2PerYear, 0.0, 0.0, 5000.0, "", "WSO4");
	auto ClWeathering           = RegisterParameterDouble(Model, WeatheringCompartment, "Cl weathering", MEqPerM2PerYear, 0.0, 0.0, 5000.0, "", "WCl");
	auto NO3Weathering          = RegisterParameterDouble(Model, WeatheringCompartment, "NO3 weathering", MEqPerM2PerYear, 0.0, 0.0, 5000.0, "", "WNO3");
	auto FWeathering            = RegisterParameterDouble(Model, WeatheringCompartment, "F weathering", MEqPerM2PerYear, 0.0, 0.0, 5000.0, "", "WF");
	auto PO4Weathering          = RegisterParameterDouble(Model, WeatheringCompartment, "F weathering", MEqPerM2PerYear, 0.0, 0.0, 5000.0, "", "WPO4");
	
	auto CaSinks                = RegisterParameterDouble(Model, SourcesSinksCompartment, "Ca sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs", "CaSink");
	auto MgSinks                = RegisterParameterDouble(Model, SourcesSinksCompartment, "Mg sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs", "MgSink");
	auto NaSinks                = RegisterParameterDouble(Model, SourcesSinksCompartment, "Na sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs", "NaSink");
	auto KSinks                 = RegisterParameterDouble(Model, SourcesSinksCompartment, "K sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs", "KSink");
	auto NH4Sinks               = RegisterParameterDouble(Model, SourcesSinksCompartment, "NH4 sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs", "NH4Sink");
	auto SO4Sinks               = RegisterParameterDouble(Model, SourcesSinksCompartment, "SO4 sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs", "SO4Sink");
	auto ClSinks                = RegisterParameterDouble(Model, SourcesSinksCompartment, "Cl sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs", "ClSink");
	auto NO3Sinks               = RegisterParameterDouble(Model, SourcesSinksCompartment, "NO3 sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs", "NO3Sink");
	auto FSinks                 = RegisterParameterDouble(Model, SourcesSinksCompartment, "F sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs", "FSink");
	auto PO4Sinks               = RegisterParameterDouble(Model, SourcesSinksCompartment, "PO4 sinks", MEqPerM2PerYear, 0.0, 0.0, 500.0, "Negative rate sets value as % of inputs", "PO4Sink");
	
	auto CaSources              = RegisterParameterDouble(Model, SourcesSinksCompartment, "Ca sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto MgSources              = RegisterParameterDouble(Model, SourcesSinksCompartment, "Mg sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NaSources              = RegisterParameterDouble(Model, SourcesSinksCompartment, "Na sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto KSources               = RegisterParameterDouble(Model, SourcesSinksCompartment, "K sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NH4Sources             = RegisterParameterDouble(Model, SourcesSinksCompartment, "NH4 sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto SO4Sources             = RegisterParameterDouble(Model, SourcesSinksCompartment, "SO4 sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto ClSources              = RegisterParameterDouble(Model, SourcesSinksCompartment, "Cl sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto NO3Sources             = RegisterParameterDouble(Model, SourcesSinksCompartment, "NO3 sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto FSources               = RegisterParameterDouble(Model, SourcesSinksCompartment, "F sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	auto PO4Sources             = RegisterParameterDouble(Model, SourcesSinksCompartment, "PO4 sources", MEqPerM2PerYear, 0.0, 0.0, 500.0);
	
	
	auto FractionOfYear     = RegisterEquation(Model, "Fraction of year", YearPerTs);
	
	auto Precipitation      = RegisterEquation(Model, "Precipitation", MPerTs);
	auto Discharge          = RegisterEquation(Model, "Discharge", MPerTs);
	auto Temperature        = RegisterEquation(Model, "Temperature", DegreesCelsius);
	auto PartialPressureCO2 = RegisterEquation(Model, "CO2 partial pressure", Percent);
	auto OAConcentration   = RegisterEquation(Model, "Organic acid concentration", MMolPerM3);
	
	auto CompartmentSolver = GetSolverHandle(Model, "Compartment solver");
	
	//The following four are re-registered and defined in the Carbon Nitrogen module
	auto NO3Inputs          = RegisterEquation(Model, "NO3 inputs", MMolPerM2PerTs);
	auto NH4Inputs          = RegisterEquation(Model, "NH4 inputs", MMolPerM2PerTs);
	auto NO3ProcessesLoss   = RegisterEquation(Model, "NO3 processes loss", MMolPerM2PerTs);
	auto NH4ProcessesLoss   = RegisterEquation(Model, "NH4 processes loss", MMolPerM2PerTs);
	
	auto NO3BasicInputs     = RegisterEquation(Model, "NO3 non-process inputs", MMolPerM2PerTs);
	auto NH4BasicInputs     = RegisterEquation(Model, "NH4 non-process inputs", MMolPerM2PerTs);
	
	auto CaExternalFlux     = RegisterEquation(Model, "Sum of Ca fluxes not related to discharge", MEqPerM2PerTs);
	auto MgExternalFlux     = RegisterEquation(Model, "Sum of Mg fluxes not related to discharge", MEqPerM2PerTs);
	auto NaExternalFlux     = RegisterEquation(Model, "Sum of Na fluxes not related to discharge", MEqPerM2PerTs);
	auto KExternalFlux      = RegisterEquation(Model, "Sum of K fluxes not related to discharge", MEqPerM2PerTs);
	auto NH4ExternalFlux    = RegisterEquation(Model, "Sum of NH4 fluxes not related to discharge", MEqPerM2PerTs);//, CompartmentSolver);
	auto SO4ExternalFlux    = RegisterEquation(Model, "Sum of SO4 fluxes not related to discharge", MEqPerM2PerTs);
	auto ClExternalFlux     = RegisterEquation(Model, "Sum of Cl fluxes not related to discharge", MEqPerM2PerTs);
	auto NO3ExternalFlux    = RegisterEquation(Model, "Sum of NO3 fluxes not related to discharge", MEqPerM2PerTs);//, CompartmentSolver);
	auto FExternalFlux      = RegisterEquation(Model, "Sum of F fluxes not related to discharge", MEqPerM2PerTs);
	auto PO4ExternalFlux    = RegisterEquation(Model, "Sum of PO4 fluxes not related to discharge", MEqPerM2PerTs);
	
	
	auto CaDeposition       = RegisterEquation(Model, "Ca deposition", MEqPerM2PerTs);
	auto MgDeposition       = RegisterEquation(Model, "Mg deposition", MEqPerM2PerTs);
	auto NaDeposition       = RegisterEquation(Model, "Na deposition", MEqPerM2PerTs);
	auto KDeposition        = RegisterEquation(Model, "K deposition", MEqPerM2PerTs);
	auto NH4Deposition      = RegisterEquation(Model, "NH4 deposition", MEqPerM2PerTs);
	auto SO4Deposition      = RegisterEquation(Model, "SO4 deposition", MEqPerM2PerTs);
	auto ClDeposition       = RegisterEquation(Model, "Cl deposition", MEqPerM2PerTs);
	auto NO3Deposition      = RegisterEquation(Model, "NO3 deposition", MEqPerM2PerTs);
	auto FDeposition        = RegisterEquation(Model, "F deposition", MEqPerM2PerTs);
	auto PO4Deposition      = RegisterEquation(Model, "PO4 deposition", MEqPerM2PerTs);

	
	
	EQUATION(Model, FractionOfYear,
		return (double)CURRENT_TIME().StepLengthInSeconds / (86400.0*(double)CURRENT_TIME().DaysThisYear);
	)
	
	EQUATION(Model, Precipitation,
		double in  = INPUT(PrecipIn);
		double fraction = RESULT(FractionOfYear);
		if(INPUT_WAS_PROVIDED(PrecipSeasonal)) fraction = INPUT(PrecipSeasonal)*0.01;
		double par = PARAMETER(PrecipPar) * fraction;
		if(std::isfinite(in)) return in;
		return par;
	)
	
	EQUATION(Model, Discharge,
		double in  = INPUT(DischargeIn);
		double fraction = RESULT(FractionOfYear);
		if(INPUT_WAS_PROVIDED(DischargeSeasonal)) fraction = INPUT(DischargeSeasonal)*0.01;
		double par = PARAMETER(DischargePar) * fraction;
		if(std::isfinite(in)) return in;
		return par;
	)
	
	EQUATION(Model, Temperature,
		double par = PARAMETER(TemperaturePar);
		double in  = INPUT(TemperatureIn);
		if(std::isfinite(in)) return INPUT(TemperatureIn);
		return par;
	)
	
	EQUATION(Model, PartialPressureCO2,
		double par = PARAMETER(PartialPressureCO2Par);
		double in  = INPUT(PartialPressureCO2In);
		if(std::isfinite(in)) return in;
		return par;
	)
	
	EQUATION(Model, OAConcentration,
		double par = PARAMETER(OAConcentrationPar);
		double in = INPUT(OAConcentrationIn);
		if(std::isfinite(in)) return in;
		return par;
	)
	
	EQUATION(Model, CaDeposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(CaWetDepositionScale)) scale = INPUT(CaWetDepositionScale);
		if(!PARAMETER(IsTopLayer)) scale = 0.0;
		double conc = PARAMETER(CaWetDeposition)*scale;
		double in = INPUT(CaWetDepositionIn);
		if(std::isfinite(in)) conc = in;
		return RESULT(Precipitation)*conc*PARAMETER(CaDryDepositionFactor);
	)
	
	EQUATION(Model, MgDeposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(MgWetDepositionScale)) scale = INPUT(MgWetDepositionScale);
		if(!PARAMETER(IsTopLayer)) scale = 0.0;
		double conc = PARAMETER(MgWetDeposition)*scale;
		double in = INPUT(MgWetDepositionIn);
		if(std::isfinite(in)) conc = in;
		return RESULT(Precipitation)*conc*PARAMETER(MgDryDepositionFactor);
	)
	
	EQUATION(Model, NaDeposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(NaWetDepositionScale)) scale = INPUT(NaWetDepositionScale);
		if(!PARAMETER(IsTopLayer)) scale = 0.0;
		double conc = PARAMETER(NaWetDeposition)*scale;
		double in = INPUT(NaWetDepositionIn);
		if(std::isfinite(in)) conc = in;
		return RESULT(Precipitation)*conc*PARAMETER(NaDryDepositionFactor);
	)
	
	EQUATION(Model, KDeposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(KWetDepositionScale)) scale = INPUT(KWetDepositionScale);
		if(!PARAMETER(IsTopLayer)) scale = 0.0;
		double conc = PARAMETER(KWetDeposition)*scale;
		double in = INPUT(KWetDepositionIn);
		if(std::isfinite(in)) conc = in;
		return RESULT(Precipitation)*conc*PARAMETER(KDryDepositionFactor);
	)
	
	EQUATION(Model, NH4Deposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(NH4WetDepositionScale)) scale = INPUT(NH4WetDepositionScale);
		if(!PARAMETER(IsTopLayer)) scale = 0.0;
		double conc = PARAMETER(NH4WetDeposition)*scale;
		double in = INPUT(NH4WetDepositionIn);
		if(std::isfinite(in)) conc = in;
		return RESULT(Precipitation)*conc*PARAMETER(NH4DryDepositionFactor);
	)
	
	EQUATION(Model, SO4Deposition,  //TODO: Something similar may have to be done for other sea salt-affected inputs that have a scaling factor
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(SO4WetDepositionScale)) scale = INPUT(SO4WetDepositionScale);
#if 1
		bool toplayer = PARAMETER(IsTopLayer);
		double conc_ref = PARAMETER(SO4WetDeposition);
		double in = INPUT(SO4WetDepositionIn);
		double precip = RESULT(Precipitation);
		double ddf = PARAMETER(SO4DryDepositionFactor);
		
		if(!toplayer) return 0.0;
		if(std::isfinite(in)) return in*ddf*precip;
		
		double sea_salt = RESULT(ClDeposition)*0.103;   //Hmm, does not account for Cl ddf. Should factor out.
		double excess = conc_ref*precip - sea_salt;
		return (sea_salt + excess*scale)*ddf;
#else
		
		if(!PARAMETER(IsTopLayer)) scale = 0.0;
		double conc = PARAMETER(SO4WetDeposition)*scale;
		double in = INPUT(SO4WetDepositionIn);
		if(std::isfinite(in)) conc = in;
		return RESULT(Precipitation)*conc*PARAMETER(SO4DryDepositionFactor);
#endif		
	)
	
	EQUATION(Model, ClDeposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(ClWetDepositionScale)) scale = INPUT(ClWetDepositionScale);
		if(!PARAMETER(IsTopLayer)) scale = 0.0;
		double conc = PARAMETER(ClWetDeposition)*scale;
		double in = INPUT(ClWetDepositionIn);
		if(std::isfinite(in)) conc = in;
		return RESULT(Precipitation)*conc*PARAMETER(ClDryDepositionFactor);
	)
	
	EQUATION(Model, NO3Deposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(NO3WetDepositionScale)) scale = INPUT(NO3WetDepositionScale);
		if(!PARAMETER(IsTopLayer)) scale = 0.0;
		double conc = PARAMETER(NO3WetDeposition)*scale;
		double in = INPUT(NO3WetDepositionIn);
		if(std::isfinite(in)) conc = in;
		return RESULT(Precipitation)*conc*PARAMETER(NO3DryDepositionFactor);
	)
	
	EQUATION(Model, FDeposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(FWetDepositionScale)) scale = INPUT(FWetDepositionScale);
		if(!PARAMETER(IsTopLayer)) scale = 0.0;
		double conc = PARAMETER(FWetDeposition)*scale;
		double in = INPUT(FWetDepositionIn);
		if(std::isfinite(in)) conc = in;
		return RESULT(Precipitation)*conc*PARAMETER(FDryDepositionFactor);
	)
	
	EQUATION(Model, PO4Deposition,
		double scale = 1.0;
		if(INPUT_WAS_PROVIDED(PO4WetDepositionScale)) scale = INPUT(PO4WetDepositionScale);
		if(!PARAMETER(IsTopLayer)) scale = 0.0;
		double conc = PARAMETER(PO4WetDeposition)*scale;
		double in = INPUT(PO4WetDepositionIn);
		if(std::isfinite(in)) conc = in;
		return RESULT(Precipitation)*conc*PARAMETER(PO4DryDepositionFactor);
	)
	
	
	EQUATION(Model, CaExternalFlux,
		double in = RESULT(CaDeposition) + (PARAMETER(CaWeathering) + PARAMETER(CaSources))*RESULT(FractionOfYear);
		double sink = PARAMETER(CaSinks);
		double sink2 = sink*RESULT(FractionOfYear);
		if(sink < 0.0) sink2 = -sink*0.01*in;
		return in - sink2;
	)
	
	EQUATION(Model, MgExternalFlux,
		double in = RESULT(MgDeposition) + (PARAMETER(MgWeathering) + PARAMETER(MgSources))*RESULT(FractionOfYear);
		double sink = PARAMETER(MgSinks);
		double sink2 = sink*RESULT(FractionOfYear);
		if(sink < 0.0) sink2 = -sink*0.01*in;
		return in - sink2;
	)
	
	EQUATION(Model, NaExternalFlux,
		double in = RESULT(NaDeposition) + (PARAMETER(NaWeathering) + PARAMETER(NaSources))*RESULT(FractionOfYear);
		double sink = PARAMETER(NaSinks);
		double sink2 = sink*RESULT(FractionOfYear);
		if(sink < 0.0) sink2 = -sink*0.01*in;
		return in - sink2;
	)
	
	EQUATION(Model, KExternalFlux,
		double in = RESULT(KDeposition) + (PARAMETER(KWeathering) + PARAMETER(KSources))*RESULT(FractionOfYear);
		double sink = PARAMETER(KSinks);
		double sink2 = sink*RESULT(FractionOfYear);
		if(sink < 0.0) sink2 = -sink*0.01*in;
		return in - sink2;
	)
	
	EQUATION(Model, SO4ExternalFlux,
		double in = RESULT(SO4Deposition) + (PARAMETER(SO4Weathering) + PARAMETER(SO4Sources))*RESULT(FractionOfYear);
		double sink = PARAMETER(SO4Sinks);
		double sink2 = sink*RESULT(FractionOfYear);
		if(sink < 0.0) sink2 = -sink*0.01*in;
		return in - sink2;
	)
	
	EQUATION(Model, ClExternalFlux,
		double in = RESULT(ClDeposition) + (PARAMETER(ClWeathering) + PARAMETER(ClSources))*RESULT(FractionOfYear);
		double sink = PARAMETER(ClSinks);
		double sink2 = sink*RESULT(FractionOfYear);
		if(sink < 0.0) sink2 = -sink*0.01*in;
		return in - sink2;
	)
	
	EQUATION(Model, FExternalFlux,
		double in = RESULT(FDeposition) + (PARAMETER(FWeathering) + PARAMETER(FSources))*RESULT(FractionOfYear);
		double sink = PARAMETER(FSinks);
		double sink2 = sink*RESULT(FractionOfYear);
		if(sink < 0.0) sink2 = -sink*0.01*in;
		return in - sink2;
	)

	EQUATION(Model, PO4ExternalFlux,
		double in = RESULT(PO4Deposition) + (PARAMETER(PO4Weathering) + PARAMETER(PO4Sources))*RESULT(FractionOfYear);
		double sink = PARAMETER(PO4Sinks);
		double sink2 = sink*RESULT(FractionOfYear);
		if(sink < 0.0) sink2 = -sink*0.01*in;
		return in - sink2;
	)	
	
	EQUATION(Model, NO3ExternalFlux,
		double in = RESULT(NO3Inputs) - RESULT(NO3ProcessesLoss);
		double sink = PARAMETER(NO3Sinks);
		double sink2 = sink*RESULT(FractionOfYear);
		if(sink < 0.0) sink2 = -sink*0.01*in;
		return in - sink2;
	)
	
	EQUATION(Model, NH4ExternalFlux,
		double in = RESULT(NH4Inputs) - RESULT(NH4ProcessesLoss);
		double sink = PARAMETER(NH4Sinks);
		double sink2 = sink*RESULT(FractionOfYear);
		if(sink < 0.0) sink2 = -sink*0.01*in;
		return in - sink2;
	)
	
	EQUATION(Model, NO3BasicInputs,
		return RESULT(NO3Deposition) + (PARAMETER(NO3Weathering) + PARAMETER(NO3Sources))*RESULT(FractionOfYear);
	)
	
	EQUATION(Model, NH4BasicInputs,
		return RESULT(NH4Deposition) + (PARAMETER(NH4Weathering) + PARAMETER(NH4Sources))*RESULT(FractionOfYear);
	)
	
	EndModule(Model);
}