

static void
AddMagicForestModule(mobius_model *Model)
{
	BeginModule(Model, "MAGIC Forest", "_dev");
	
	SetModuleDescription(Model, R""""(
Forest growth driver module developed as part of the CatchCAN project.
)"""");
	
	auto Dimensionless   = RegisterUnit(Model);
	auto DegreesCelsius	 = RegisterUnit(Model, "°C");
	//auto MgPerL          = RegisterUnit(Model, "mg/l");
	auto MPerTs          = RegisterUnit(Model, "m/month");
	auto MmPerTs         = RegisterUnit(Model, "mm/month");
	auto MEqPerM3        = RegisterUnit(Model, "meq/m3");
	auto MMolPerM3       = RegisterUnit(Model, "mmol/m3");
	auto MEqPerM2PerTs   = RegisterUnit(Model, "meq/m2/month");
	auto MMolPerM2PerTs  = RegisterUnit(Model, "mmol/m2/month");
	auto MEqPerM2PerYear = RegisterUnit(Model, "meq/m2/year");
	auto YearPerTs       = RegisterUnit(Model, "year/month");
	auto Percent         = RegisterUnit(Model, "%");
	
	//auto Compartment             = GetIndexSetHandle(Model, "Compartment");
	auto Compartment             = RegisterIndexSet(Model, "Compartment");
	
	auto Climate                 = RegisterParameterGroup(Model, "Climate", Compartment);
	
	
	auto PartialPressureCO2Par   = RegisterParameterDouble(Model, Climate, "CO2 partial pressure", Percent, 0.3, 0.1, 2.0, "Default value for timesteps where no input series value is provided");
	auto OAConcentrationPar      = RegisterParameterDouble(Model, Climate, "Organic acid concentration", MMolPerM3, 0.0, 0.0, 200.0, "Default value for timesteps where no input series value is provided");
	auto MinCompartmentTemp      = RegisterParameterDouble(Model, Climate, "Minimal compartment temperature", DegreesCelsius, 0.0, -10.0, 10.0);
	auto UseMeasuredRunoff       = RegisterParameterBool(Model, Climate, "Use measured runoff when available", false, "If this is off, it will always use the value computed by the hydrology module.");
	
	auto Weathering              = RegisterParameterGroup(Model, "Weathering", Compartment);
	
	auto CaWeathering            = RegisterParameterDouble(Model, Weathering, "Ca weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0);
	auto MgWeathering            = RegisterParameterDouble(Model, Weathering, "Mg weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0);
	auto NaWeathering            = RegisterParameterDouble(Model, Weathering, "Na weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0);
	auto KWeathering             = RegisterParameterDouble(Model, Weathering, "K weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0);
	auto NH4Weathering           = RegisterParameterDouble(Model, Weathering, "NH4 weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0);
	auto SO4Weathering           = RegisterParameterDouble(Model, Weathering, "SO4 weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0);
	auto ClWeathering            = RegisterParameterDouble(Model, Weathering, "Cl weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0);
	auto NO3Weathering           = RegisterParameterDouble(Model, Weathering, "NO3 weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0);
	auto FWeathering             = RegisterParameterDouble(Model, Weathering, "F weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0);
	
	
	

	
	auto AirTemperature          = RegisterInput(Model, "Air temperature", DegreesCelsius);
	auto Precipitation           = RegisterInput(Model, "Precipitation", MmPerTs);
	
	//NOTE: These are auto-cleared to NaN for missing values
	auto RunoffIn                = RegisterInput(Model, "Runoff", MmPerTs, true);
	auto PartialPressureCO2In    = RegisterInput(Model, "CO2 partial pressure", Percent, true);           
	auto OAConcentrationIn       = RegisterInput(Model, "Organic acid concentration", MMolPerM3, true);
	
	auto CaPrecipConc            = RegisterInput(Model, "Ca conc in precip", MEqPerM3);
	auto MgPrecipConc            = RegisterInput(Model, "Mg conc in precip", MEqPerM3);
	auto NaPrecipConc            = RegisterInput(Model, "Na conc in precip", MEqPerM3);
	auto KPrecipConc             = RegisterInput(Model, "K conc in precip", MEqPerM3);
	auto NH4PrecipConc           = RegisterInput(Model, "NH4 conc in precip", MEqPerM3);
	auto SO4PrecipConc           = RegisterInput(Model, "SO4 conc in precip", MEqPerM3);
	auto ClPrecipConc            = RegisterInput(Model, "Cl conc in precip", MEqPerM3);
	auto NO3PrecipConc           = RegisterInput(Model, "NO3 conc in precip", MEqPerM3);
	auto FPrecipConc             = RegisterInput(Model, "F conc in precip", MEqPerM3);
	
	
	
	
	
	auto FractionOfYear          = RegisterEquation(Model, "Fraction of year", YearPerTs);
	
	
	auto MgDeposition            = RegisterEquation(Model, "Mg deposition", MEqPerM2PerTs);
	auto NH4Deposition           = RegisterEquation(Model, "NH4 deposition", MEqPerM2PerTs);
	auto NO3Deposition           = RegisterEquation(Model, "NO3 deposition", MEqPerM2PerTs);
	auto ClDeposition            = RegisterEquation(Model, "Cl deposition", MEqPerM2PerTs);
	auto NaDeposition            = RegisterEquation(Model, "Na deposition", MEqPerM2PerTs);
	auto KDeposition             = RegisterEquation(Model, "K deposition", MEqPerM2PerTs);
	auto SO4Deposition           = RegisterEquation(Model, "SO4 deposition", MEqPerM2PerTs);
	auto CaDeposition            = RegisterEquation(Model, "Ca deposition", MEqPerM2PerTs);
	auto FDeposition             = RegisterEquation(Model, "F deposition", MEqPerM2PerTs);
	
	
	// These are required by the MAGIC Core:
	auto Discharge               = RegisterEquation(Model, "Discharge", MPerTs);
	auto Temperature             = RegisterEquation(Model, "Temperature", DegreesCelsius);
	auto PartialPressureCO2      = RegisterEquation(Model, "CO2 partial pressure", Percent);
	auto OAConcentration         = RegisterEquation(Model, "Organic acid concentration", MMolPerM3);
	
	auto CaExternalFlux          = RegisterEquation(Model, "Sum of Ca fluxes not related to discharge", MEqPerM2PerTs);
	auto MgExternalFlux          = RegisterEquation(Model, "Sum of Mg fluxes not related to discharge", MEqPerM2PerTs);
	auto NaExternalFlux          = RegisterEquation(Model, "Sum of Na fluxes not related to discharge", MEqPerM2PerTs);
	auto KExternalFlux           = RegisterEquation(Model, "Sum of K fluxes not related to discharge", MEqPerM2PerTs);
	auto NH4ExternalFlux         = RegisterEquation(Model, "Sum of NH4 fluxes not related to discharge", MEqPerM2PerTs);
	auto SO4ExternalFlux         = RegisterEquation(Model, "Sum of SO4 fluxes not related to discharge", MEqPerM2PerTs);
	auto ClExternalFlux          = RegisterEquation(Model, "Sum of Cl fluxes not related to discharge", MEqPerM2PerTs);
	auto NO3ExternalFlux         = RegisterEquation(Model, "Sum of NO3 fluxes not related to discharge", MEqPerM2PerTs);
	auto FExternalFlux           = RegisterEquation(Model, "Sum of F fluxes not related to discharge", MEqPerM2PerTs);
	
	
	//The following four are re-registered and defined in the Carbon Nitrogen module
	auto NO3Inputs               = RegisterEquation(Model, "NO3 inputs", MMolPerM2PerTs);
	auto NH4Inputs               = RegisterEquation(Model, "NH4 inputs", MMolPerM2PerTs);
	auto NO3ProcessesLoss        = RegisterEquation(Model, "NO3 processes loss", MMolPerM2PerTs);
	auto NH4ProcessesLoss        = RegisterEquation(Model, "NH4 processes loss", MMolPerM2PerTs);
	
	auto NO3BasicInputs          = RegisterEquation(Model, "NO3 basic inputs", MMolPerM2PerTs);
	auto NH4BasicInputs          = RegisterEquation(Model, "NH4 basic inputs", MMolPerM2PerTs);
	
	
	
	// From WASMOD:
	auto Runoff                  = GetEquationHandle(Model, "Runoff");
	
	
	EQUATION(Model, FractionOfYear,
		return (double)CURRENT_TIME().StepLengthInSeconds / (86400.0*(double)CURRENT_TIME().DaysThisYear);
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
	
	
	EQUATION(Model, Discharge,
		//TODO: We need to do something more sophisticated in how we do routing between multiple compartments, and how we handle lakes!
		
		double invalue  = INPUT(RunoffIn);
		double computed = RESULT(Runoff) * 1e-3;
		
		if(PARAMETER(UseMeasuredRunoff) && std::isfinite(invalue)) return invalue * 1e-3;
		return computed;
	)
	
	
	EQUATION(Model, Temperature,
		return Max(INPUT(AirTemperature), PARAMETER(MinCompartmentTemp));
	)
	
	
	EQUATION(Model, CaDeposition,
		return INPUT(CaPrecipConc) * INPUT(Precipitation) * 1e-3;
	)
	
	EQUATION(Model, MgDeposition,
		return INPUT(MgPrecipConc) * INPUT(Precipitation) * 1e-3;
	)
	
	EQUATION(Model, NaDeposition,
		return INPUT(NaPrecipConc) * INPUT(Precipitation) * 1e-3;
	)

	EQUATION(Model, KDeposition,
		return INPUT(KPrecipConc) * INPUT(Precipitation) * 1e-3;
	)
	
	EQUATION(Model, NH4Deposition,
		return INPUT(NH4PrecipConc) * INPUT(Precipitation) * 1e-3;
	)
	
	EQUATION(Model, SO4Deposition,
		return INPUT(SO4PrecipConc) * INPUT(Precipitation) * 1e-3;
	)
	
	EQUATION(Model, NO3Deposition,
		return INPUT(NO3PrecipConc) * INPUT(Precipitation) * 1e-3;
	)
	
	EQUATION(Model, ClDeposition,
		return INPUT(ClPrecipConc) * INPUT(Precipitation) * 1e-3;
	)
	
	EQUATION(Model, FDeposition,
		return INPUT(FPrecipConc) * INPUT(Precipitation) * 1e-3;
	)
	
	
	
	
	EQUATION(Model, NO3BasicInputs,
		return RESULT(NO3Deposition) + PARAMETER(NO3Weathering)*RESULT(FractionOfYear);
	)
	
	EQUATION(Model, NH4BasicInputs,
		return RESULT(NH4Deposition) + PARAMETER(NH4Weathering)*RESULT(FractionOfYear);
	)
	
	
	
	EQUATION(Model, CaExternalFlux,
		return RESULT(CaDeposition) + RESULT(FractionOfYear)*PARAMETER(CaWeathering);
	)
	
	EQUATION(Model, MgExternalFlux,
		return RESULT(MgDeposition) + RESULT(FractionOfYear)*PARAMETER(MgWeathering);
	)
	
	EQUATION(Model, NaExternalFlux,
		return RESULT(NaDeposition) + RESULT(FractionOfYear)*PARAMETER(NaWeathering);
	)
	
	EQUATION(Model, KExternalFlux,
		return RESULT(KDeposition) + RESULT(FractionOfYear)*PARAMETER(KWeathering);
	)
	
	EQUATION(Model, NH4ExternalFlux,
		return RESULT(NH4Inputs) - RESULT(NH4ProcessesLoss);
	)
	
	EQUATION(Model, SO4ExternalFlux,
		return RESULT(SO4Deposition) + RESULT(FractionOfYear)*PARAMETER(SO4Weathering);
	)
	
	EQUATION(Model, NO3ExternalFlux,
		return RESULT(NO3Inputs) - RESULT(NO3ProcessesLoss);
	)
	
	EQUATION(Model, ClExternalFlux,
		return RESULT(ClDeposition) + RESULT(FractionOfYear)*PARAMETER(ClWeathering);
	)
	
	EQUATION(Model, FExternalFlux,
		return RESULT(FDeposition) + RESULT(FractionOfYear)*PARAMETER(FWeathering);
	)
	
	
	EndModule(Model);
}