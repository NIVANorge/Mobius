

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
	
	
	auto General                 = RegisterParameterGroup(Model, "General");
	
	auto UseMeasuredRunoff       = RegisterParameterBool(Model, General, "Use measured runoff when available", false, "If this is off, it will always use the value computed by the hydrology module.");
	
	auto Climate                 = RegisterParameterGroup(Model, "Climate", Compartment);
	
	auto PartialPressureCO2Par   = RegisterParameterDouble(Model, Climate, "CO2 partial pressure", Percent, 0.3, 0.1, 2.0, "Default value for timesteps where no input series value is provided");
	auto OAConcentrationPar      = RegisterParameterDouble(Model, Climate, "Organic acid concentration", MMolPerM3, 0.0, 0.0, 200.0, "Default value for timesteps where no input series value is provided", "OA");
	auto ComputeOAConc           = RegisterParameterBool(Model, Climate, "Adjust OA concentration based on SO4 concentration", false);
	auto OAConcSO4Scale          = RegisterParameterDouble(Model, Climate, "Reduction in OA by SO4", Dimensionless, 0.0, 0.0, 10.0);
	auto MinCompartmentTemp      = RegisterParameterDouble(Model, Climate, "Minimal compartment temperature", DegreesCelsius, 0.0, -10.0, 10.0);
	auto ThisIsATopCompartment   = RegisterParameterBool(Model, Climate, "This is a top compartment", true, "True if it receives deposition. Also, if it interacts with the forest module");
	
	
	
	auto Weathering              = RegisterParameterGroup(Model, "Weathering", Compartment);
	
	auto CaWeathering            = RegisterParameterDouble(Model, Weathering, "Ca weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0, "", "WCa");
	auto MgWeathering            = RegisterParameterDouble(Model, Weathering, "Mg weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0, "", "WMg");
	auto NaWeathering            = RegisterParameterDouble(Model, Weathering, "Na weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0, "", "WNa");
	auto KWeathering             = RegisterParameterDouble(Model, Weathering, "K weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0, "", "WK");
	auto NH4Weathering           = RegisterParameterDouble(Model, Weathering, "NH4 weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0, "", "WNH4");
	auto SO4Weathering           = RegisterParameterDouble(Model, Weathering, "SO4 weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0, "", "WSO4");
	auto ClWeathering            = RegisterParameterDouble(Model, Weathering, "Cl weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0, "", "WCl");
	auto NO3Weathering           = RegisterParameterDouble(Model, Weathering, "NO3 weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0, "", "WNO3");
	auto FWeathering             = RegisterParameterDouble(Model, Weathering, "F weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0, "", "WF");
	auto PO4Weathering           = RegisterParameterDouble(Model, Weathering, "PO4 weathering", MEqPerM2PerYear, 0.0, 0.0, 200.0, "", "WPO4");
	
	

	
	auto AirTemperature          = RegisterInput(Model, "Air temperature", DegreesCelsius);
	auto Precipitation           = RegisterInput(Model, "Precipitation", MmPerTs);
	
	//NOTE: These three are auto-cleared to NaN for missing values
	auto RunoffIn                = RegisterInput(Model, "Runoff", MmPerTs, true);
	auto PartialPressureCO2In    = RegisterInput(Model, "CO2 partial pressure", Percent, true);           
	auto OAConcentrationIn       = RegisterInput(Model, "Organic acid concentration", MMolPerM3, true);
	
	
	auto MgDeposition            = RegisterInput(Model, "Total Mg deposition", MEqPerM2PerTs);
	auto NH4Deposition           = RegisterInput(Model, "Total NH4 deposition", MEqPerM2PerTs);
	auto NO3Deposition           = RegisterInput(Model, "Total NO3 deposition", MEqPerM2PerTs);
	auto ClDeposition            = RegisterInput(Model, "Total Cl deposition", MEqPerM2PerTs);
	auto NaDeposition            = RegisterInput(Model, "Total Na deposition", MEqPerM2PerTs);
	auto KDeposition             = RegisterInput(Model, "Total K deposition", MEqPerM2PerTs);
	auto SO4Deposition           = RegisterInput(Model, "Total SO4 deposition", MEqPerM2PerTs);
	auto CaDeposition            = RegisterInput(Model, "Total Ca deposition", MEqPerM2PerTs);
	auto FDeposition             = RegisterInput(Model, "Total F deposition", MEqPerM2PerTs);
	auto PO4Deposition           = RegisterInput(Model, "Total PO4 deposition", MEqPerM2PerTs);
	
	
	auto FractionOfYear          = RegisterEquation(Model, "Fraction of year", YearPerTs);
	
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
	auto PO4ExternalFlux         = RegisterEquation(Model, "Sum of PO4 fluxes not related to discharge", MEqPerM2PerTs);
	
	
	//The following four are re-registered and defined in the CNP module
	auto NO3Inputs               = RegisterEquation(Model, "NO3 inputs", MMolPerM2PerTs);
	auto NH4Inputs               = RegisterEquation(Model, "NH4 inputs", MMolPerM2PerTs);
	auto PO4Inputs               = RegisterEquation(Model, "PO4 inputs", MMolPerM2PerTs);
	auto NO3ProcessesLoss        = RegisterEquation(Model, "NO3 processes loss", MMolPerM2PerTs);
	auto NH4ProcessesLoss        = RegisterEquation(Model, "NH4 processes loss", MMolPerM2PerTs);
	auto PO4ProcessesLoss        = RegisterEquation(Model, "PO4 processes loss", MMolPerM2PerTs);
	
	auto NO3BasicInputs          = RegisterEquation(Model, "NO3 non-process inputs", MMolPerM2PerTs);
	auto NH4BasicInputs          = RegisterEquation(Model, "NH4 non-process inputs", MMolPerM2PerTs);
	auto PO4BasicInputs          = RegisterEquation(Model, "PO4 non-process inputs", MMolPerM2PerTs);
	
	// The following are re-registered and defined in the CNP module:
	auto TotalTreeDecompCaSource = RegisterEquation(Model, "Total Ca source from tree decomposition", MEqPerM2PerTs);
	auto TotalTreeDecompMgSource = RegisterEquation(Model, "Total Mg source from tree decomposition", MEqPerM2PerTs);
	auto TotalTreeDecompNaSource = RegisterEquation(Model, "Total Na source from tree decomposition", MEqPerM2PerTs);
	auto TotalTreeDecompKSource  = RegisterEquation(Model, "Total K source from tree decomposition", MEqPerM2PerTs);
	auto TotalTreeCaUptake       = RegisterEquation(Model, "Total tree Ca uptake", MEqPerM2PerTs);
	auto TotalTreeMgUptake       = RegisterEquation(Model, "Total tree Mg uptake", MEqPerM2PerTs);
	auto TotalTreeNaUptake       = RegisterEquation(Model, "Total tree Na uptake", MEqPerM2PerTs);
	auto TotalTreeKUptake        = RegisterEquation(Model, "Total tree K uptake", MEqPerM2PerTs);
	
	
	// From WASMOD:
	auto Runoff                  = GetEquationHandle(Model, "Runoff");
	
	// From the core wrapper:
	auto IsSoil                  = GetParameterBoolHandle(Model, "This is a soil compartment");
	
	auto ConcSO4                 = GetEquationHandle(Model, "SO4(2-) ionic concentration");
	

	
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
		
		bool compute = PARAMETER(ComputeOAConc);
		double so4factor = PARAMETER(OAConcSO4Scale);
		double so4 = LAST_RESULT(ConcSO4);
		if(compute) par -= so4*so4factor;
		
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
	
	
	EQUATION(Model, NO3BasicInputs,
		double deposition = INPUT(NO3Deposition);
		double weathering = PARAMETER(NO3Weathering)*RESULT(FractionOfYear);
		if(!PARAMETER(ThisIsATopCompartment)) deposition = 0.0;
		return deposition + weathering;
	)
	
	EQUATION(Model, NH4BasicInputs,
		double deposition = INPUT(NH4Deposition);
		double weathering = PARAMETER(NH4Weathering)*RESULT(FractionOfYear);
		if(!PARAMETER(ThisIsATopCompartment)) deposition = 0.0;
		return deposition + weathering;
	)
	
	EQUATION(Model, PO4BasicInputs,
		double deposition = INPUT(PO4Deposition);
		double weathering = PARAMETER(PO4Weathering)*RESULT(FractionOfYear);
		if(!PARAMETER(ThisIsATopCompartment)) deposition = 0.0;
		return deposition + weathering;
	)
	
	
	//TODO: The uptake should maybe be limited so that the mass never goes in the negative, but it may not be a problem
	EQUATION(Model, CaExternalFlux,
		double deposition = INPUT(CaDeposition);
		double weathering = RESULT(FractionOfYear)*PARAMETER(CaWeathering);
		double forest     = RESULT(TotalTreeDecompCaSource) - RESULT(TotalTreeCaUptake);
		if(!PARAMETER(ThisIsATopCompartment)) { deposition = 0.0; forest = 0.0; }
		if(!PARAMETER(IsSoil)) forest = 0.0;
		return deposition + weathering + forest;
	)
	
	EQUATION(Model, MgExternalFlux,
		double deposition = INPUT(MgDeposition);
		double weathering = RESULT(FractionOfYear)*PARAMETER(MgWeathering);
		double forest     = RESULT(TotalTreeDecompMgSource) - RESULT(TotalTreeMgUptake);
		if(!PARAMETER(ThisIsATopCompartment)) { deposition = 0.0; forest = 0.0; }
		if(!PARAMETER(IsSoil)) forest = 0.0;
		return deposition + weathering + forest;
	)
	
	EQUATION(Model, NaExternalFlux,
		double deposition = INPUT(NaDeposition);
		double weathering = RESULT(FractionOfYear)*PARAMETER(NaWeathering);
		double forest     = RESULT(TotalTreeDecompNaSource) - RESULT(TotalTreeNaUptake);
		if(!PARAMETER(ThisIsATopCompartment)) { deposition = 0.0; forest = 0.0; }
		if(!PARAMETER(IsSoil)) forest = 0.0;
		return deposition + weathering + forest;
	)
	
	EQUATION(Model, KExternalFlux,
		double deposition = INPUT(KDeposition);
		double weathering = RESULT(FractionOfYear)*PARAMETER(KWeathering);
		double forest     = RESULT(TotalTreeDecompKSource) - RESULT(TotalTreeKUptake);
		if(!PARAMETER(ThisIsATopCompartment)) { deposition = 0.0; forest = 0.0; }
		if(!PARAMETER(IsSoil)) forest = 0.0;
		return deposition + weathering + forest;
	)
	
	EQUATION(Model, NH4ExternalFlux,
		return RESULT(NH4Inputs) - RESULT(NH4ProcessesLoss);
	)
	
	EQUATION(Model, SO4ExternalFlux,
		double deposition = INPUT(SO4Deposition);
		double weathering = RESULT(FractionOfYear)*PARAMETER(SO4Weathering);
		if(!PARAMETER(ThisIsATopCompartment)) deposition = 0.0;
		return deposition + weathering;
	)
	
	EQUATION(Model, NO3ExternalFlux,
		return RESULT(NO3Inputs) - RESULT(NO3ProcessesLoss);
	)
	
	EQUATION(Model, ClExternalFlux,
		double deposition = INPUT(ClDeposition);
		double weathering = RESULT(FractionOfYear)*PARAMETER(ClWeathering);
		if(!PARAMETER(ThisIsATopCompartment)) deposition = 0.0;
		return deposition + weathering;
	)
	
	EQUATION(Model, FExternalFlux,
		double deposition = INPUT(FDeposition);
		double weathering = RESULT(FractionOfYear)*PARAMETER(FWeathering);
		if(!PARAMETER(ThisIsATopCompartment)) deposition = 0.0;
		return deposition + weathering;
	)
	
	EQUATION(Model, PO4ExternalFlux,
		return RESULT(PO4Inputs) - RESULT(PO4ProcessesLoss);
	)
	
	
	EndModule(Model);
}