

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
	auto MPerYear        = RegisterUnit(Model, "m/year");
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
	
	auto Deposition              = RegisterParameterGroup(Model, "Deposition");
	
	auto CaDepConc               = RegisterParameterDouble(Model, Deposition, "Ca conc in precipitation", MEqPerM3, 0.0, 0.0, 200.0, "Multiplied with Precipitation to get wet deposition if Wet deposition or Total deposition is not provided for this element", "DCa");
	auto MgDepConc               = RegisterParameterDouble(Model, Deposition, "Mg conc in precipitation", MEqPerM3, 0.0, 0.0, 200.0, "Multiplied with Precipitation to get wet deposition if Wet deposition or Total deposition is not provided for this element", "DMg");
	auto NaDepConc               = RegisterParameterDouble(Model, Deposition, "Na conc in precipitation", MEqPerM3, 0.0, 0.0, 200.0, "Multiplied with Precipitation to get wet deposition if Wet deposition or Total deposition is not provided for this element", "DNa");
	auto KDepConc                = RegisterParameterDouble(Model, Deposition, "K conc in precipitation", MEqPerM3, 0.0, 0.0, 200.0, "Multiplied with Precipitation to get wet deposition if Wet deposition or Total deposition is not provided for this element", "DK");
	auto NH4DepConc              = RegisterParameterDouble(Model, Deposition, "NH4 conc in precipitation", MEqPerM3, 0.0, 0.0, 200.0, "Multiplied with Precipitation to get wet deposition if Wet deposition or Total deposition is not provided for this element", "DNH4");
	auto SO4DepConc              = RegisterParameterDouble(Model, Deposition, "SO4 conc in precipitation", MEqPerM3, 0.0, 0.0, 200.0, "Multiplied with Precipitation to get wet deposition if Wet deposition or Total deposition is not provided for this element", "DSO4");
	auto ClDepConc               = RegisterParameterDouble(Model, Deposition, "Cl conc in precipitation", MEqPerM3, 0.0, 0.0, 200.0, "Multiplied with Precipitation to get wet deposition if Wet deposition or Total deposition is not provided for this element", "DCl");
	auto NO3DepConc              = RegisterParameterDouble(Model, Deposition, "NO3 conc in precipitation", MEqPerM3, 0.0, 0.0, 200.0, "Multiplied with Precipitation to get wet deposition if Wet deposition or Total deposition is not provided for this element", "DNO3");
	auto FDepConc                = RegisterParameterDouble(Model, Deposition, "F conc in precipitation", MEqPerM3, 0.0, 0.0, 200.0, "Multiplied with Precipitation to get wet deposition if Wet deposition or Total deposition is not provided for this element", "DF");
	auto PO4DepConc              = RegisterParameterDouble(Model, Deposition, "PO4 conc in precipitation", MEqPerM3, 0.0, 0.0, 200.0, "Multiplied with Precipitation to get wet deposition if Wet deposition or Total deposition is not provided for this element", "DPO4");
	
	auto CaDryDepFactor          = RegisterParameterDouble(Model, Deposition, "Ca dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition to get total deposition if Total deposition is not provided for this element", "DDFCa");
	auto MgDryDepFactor          = RegisterParameterDouble(Model, Deposition, "Mg dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition to get total deposition if Total deposition is not provided for this element", "DDFMg");
	auto NaDryDepFactor          = RegisterParameterDouble(Model, Deposition, "Na dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition to get total deposition if Total deposition is not provided for this element", "DDFNa");
	auto KDryDepFactor           = RegisterParameterDouble(Model, Deposition, "K dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition to get total deposition if Total deposition is not provided for this element", "DDFK");
	auto NH4DryDepFactor         = RegisterParameterDouble(Model, Deposition, "NH4 dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition to get total deposition if Total deposition is not provided for this element", "DDFNH4");
	auto SO4DryDepFactor         = RegisterParameterDouble(Model, Deposition, "SO4 dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition to get total deposition if Total deposition is not provided for this element", "DDFSO4");
	auto ClDryDepFactor          = RegisterParameterDouble(Model, Deposition, "Cl dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition to get total deposition if Total deposition is not provided for this element", "DDFCl");
	auto NO3DryDepFactor         = RegisterParameterDouble(Model, Deposition, "NO3 dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition to get total deposition if Total deposition is not provided for this element", "DDFNO3");
	auto FDryDepFactor           = RegisterParameterDouble(Model, Deposition, "F dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition to get total deposition if Total deposition is not provided for this element", "DDFF");
	auto PO4DryDepFactor         = RegisterParameterDouble(Model, Deposition, "PO4 dry deposition factor", Dimensionless, 1.0, 1.0, 5.0, "Factor to multiply wet deposition to get total deposition if Total deposition is not provided for this element", "DDFPO4");
	
	auto Sedimentation           = RegisterParameterGroup(Model, "Sedimentation", Compartment);
	
	auto CaSedimentation         = RegisterParameterDouble(Model, Sedimentation, "Ca sedimentation", MPerYear, 0.0, 0.0, 10.0, "For lakes");
	auto MgSedimentation         = RegisterParameterDouble(Model, Sedimentation, "Mg sedimentation", MPerYear, 0.0, 0.0, 10.0, "For lakes");
	auto NaSedimentation         = RegisterParameterDouble(Model, Sedimentation, "Na sedimentation", MPerYear, 0.0, 0.0, 10.0, "For lakes");
	auto KSedimentation          = RegisterParameterDouble(Model, Sedimentation, "K sedimentation", MPerYear, 0.0, 0.0, 10.0, "For lakes");
	auto NH4Sedimentation        = RegisterParameterDouble(Model, Sedimentation, "NH4 sedimentation", MPerYear, 0.0, 0.0, 10.0, "For lakes");
	auto SO4Sedimentation        = RegisterParameterDouble(Model, Sedimentation, "SO4 sedimentation", MPerYear, 0.0, 0.0, 10.0, "For lakes");
	auto ClSedimentation         = RegisterParameterDouble(Model, Sedimentation, "Cl sedimentation", MPerYear, 0.0, 0.0, 10.0, "For lakes");
	auto NO3Sedimentation        = RegisterParameterDouble(Model, Sedimentation, "NO3 sedimentation", MPerYear, 0.0, 0.0, 10.0, "For lakes");
	auto FSedimentation          = RegisterParameterDouble(Model, Sedimentation, "F sedimentation", MPerYear, 0.0, 0.0, 10.0, "For lakes");
	auto PO4Sedimentation        = RegisterParameterDouble(Model, Sedimentation, "PO4 sedimentation", MPerYear, 0.0, 0.0, 10.0, "For lakes");
	

	
	auto AirTemperature          = RegisterInput(Model, "Air temperature", DegreesCelsius);
	auto Precipitation           = RegisterInput(Model, "Precipitation", MmPerTs);
	
	//NOTE: These three are auto-cleared to NaN for missing values
	auto RunoffIn                = RegisterInput(Model, "Runoff", MmPerTs, true);
	auto PartialPressureCO2In    = RegisterInput(Model, "CO2 partial pressure", Percent, true);           
	auto OAConcentrationIn       = RegisterInput(Model, "Organic acid concentration", MMolPerM3, true);
	
	auto CaDepositionTot         = RegisterInput(Model, "Total Ca deposition", MEqPerM2PerTs, true);
	auto MgDepositionTot         = RegisterInput(Model, "Total Mg deposition", MEqPerM2PerTs, true);
	auto NaDepositionTot         = RegisterInput(Model, "Total Na deposition", MEqPerM2PerTs, true);
	auto KDepositionTot          = RegisterInput(Model, "Total K deposition", MEqPerM2PerTs, true);
	auto NH4DepositionTot        = RegisterInput(Model, "Total NH4 deposition", MEqPerM2PerTs, true);
	auto SO4DepositionTot        = RegisterInput(Model, "Total SO4 deposition", MEqPerM2PerTs, true);
	auto ClDepositionTot         = RegisterInput(Model, "Total Cl deposition", MEqPerM2PerTs, true);
	auto NO3DepositionTot        = RegisterInput(Model, "Total NO3 deposition", MEqPerM2PerTs, true);
	auto FDepositionTot          = RegisterInput(Model, "Total F deposition", MEqPerM2PerTs, true);
	auto PO4DepositionTot        = RegisterInput(Model, "Total PO4 deposition", MEqPerM2PerTs, true);
	
	auto CaWetDepositionIn       = RegisterInput(Model, "Ca conc in precipitation", MEqPerM2PerTs);
	auto MgWetDepositionIn       = RegisterInput(Model, "Mg conc in precipitation", MEqPerM2PerTs);
	auto NaWetDepositionIn       = RegisterInput(Model, "Na conc in precipitation", MEqPerM2PerTs);
	auto KWetDepositionIn        = RegisterInput(Model, "K conc in precipitation", MEqPerM2PerTs);
	auto NH4WetDepositionIn      = RegisterInput(Model, "NH4 conc in precipitation", MEqPerM2PerTs);
	auto SO4WetDepositionIn      = RegisterInput(Model, "SO4 conc in precipitation", MEqPerM2PerTs);
	auto ClWetDepositionIn       = RegisterInput(Model, "Cl conc in precipitation", MEqPerM2PerTs);
	auto NO3WetDepositionIn      = RegisterInput(Model, "NO3 conc in precipitation", MEqPerM2PerTs);
	auto FWetDepositionIn        = RegisterInput(Model, "F conc in precipitation", MEqPerM2PerTs);
	auto PO4WetDepositionIn      = RegisterInput(Model, "PO4 conc in precipitation", MEqPerM2PerTs);
	
	auto CaWetDepositionScale    = RegisterInput(Model, "Wet Ca deposition scale", Dimensionless);
	auto MgWetDepositionScale    = RegisterInput(Model, "Wet Mg deposition scale", Dimensionless);
	auto NaWetDepositionScale    = RegisterInput(Model, "Wet Na deposition scale", Dimensionless);
	auto KWetDepositionScale     = RegisterInput(Model, "Wet K deposition scale", Dimensionless);
	auto NH4WetDepositionScale   = RegisterInput(Model, "Wet NH4 deposition scale", Dimensionless);
	auto SO4WetDepositionScale   = RegisterInput(Model, "Wet SO4 deposition scale", Dimensionless);
	auto ClWetDepositionScale    = RegisterInput(Model, "Wet Cl deposition scale", Dimensionless);
	auto NO3WetDepositionScale   = RegisterInput(Model, "Wet NO3 deposition scale", Dimensionless);
	auto FWetDepositionScale     = RegisterInput(Model, "Wet F deposition scale", Dimensionless);
	auto PO4WetDepositionScale   = RegisterInput(Model, "Wet PO4 deposition scale", Dimensionless);
	
	
	auto FractionOfYear          = RegisterEquation(Model, "Fraction of year", YearPerTs);
	
	auto ForestDryDepositionModifier = RegisterEquation(Model, "Forest cover modifier on dry deposition", Dimensionless);
	
	auto CaWetDeposition         = RegisterEquation(Model, "Ca wet deposition", MEqPerM2PerTs);
	auto MgWetDeposition         = RegisterEquation(Model, "Mg wet deposition", MEqPerM2PerTs);
	auto NaWetDeposition         = RegisterEquation(Model, "Na wet deposition", MEqPerM2PerTs);
	auto KWetDeposition          = RegisterEquation(Model, "K wet deposition", MEqPerM2PerTs);
	auto NH4WetDeposition        = RegisterEquation(Model, "NH4 wet deposition", MEqPerM2PerTs);
	auto SO4WetDeposition        = RegisterEquation(Model, "SO4 wet deposition", MEqPerM2PerTs);
	auto ClWetDeposition         = RegisterEquation(Model, "Cl wet deposition", MEqPerM2PerTs);
	auto NO3WetDeposition        = RegisterEquation(Model, "NO3 wet deposition", MEqPerM2PerTs);
	auto FWetDeposition          = RegisterEquation(Model, "F wet deposition", MEqPerM2PerTs);
	auto PO4WetDeposition        = RegisterEquation(Model, "PO4 wet deposition", MEqPerM2PerTs);
	
	
	auto CaDeposition            = RegisterEquation(Model, "Ca total deposition", MEqPerM2PerTs);
	auto MgDeposition            = RegisterEquation(Model, "Mg total deposition", MEqPerM2PerTs);
	auto NaDeposition            = RegisterEquation(Model, "Na total deposition", MEqPerM2PerTs);
	auto KDeposition             = RegisterEquation(Model, "K total deposition", MEqPerM2PerTs);
	auto NH4Deposition           = RegisterEquation(Model, "NH4 total deposition", MEqPerM2PerTs);
	auto SO4Deposition           = RegisterEquation(Model, "SO4 total deposition", MEqPerM2PerTs);
	auto ClDeposition            = RegisterEquation(Model, "Cl total deposition", MEqPerM2PerTs);
	auto NO3Deposition           = RegisterEquation(Model, "NO3 total deposition", MEqPerM2PerTs);
	auto FDeposition             = RegisterEquation(Model, "F total deposition", MEqPerM2PerTs);
	auto PO4Deposition           = RegisterEquation(Model, "PO4 total deposition", MEqPerM2PerTs);
	
	
	
	auto CompartmentSolver = GetSolverHandle(Model, "Compartment solver");
	
	auto CaSedimentationEq       = RegisterEquation(Model, "Ca sedimentation", MEqPerM2PerTs, CompartmentSolver);
	auto MgSedimentationEq       = RegisterEquation(Model, "Mg sedimentation", MEqPerM2PerTs, CompartmentSolver);
	auto NaSedimentationEq       = RegisterEquation(Model, "Na sedimentation", MEqPerM2PerTs, CompartmentSolver);
	auto KSedimentationEq        = RegisterEquation(Model, "K sedimentation", MEqPerM2PerTs, CompartmentSolver);
	auto NH4SedimentationEq      = RegisterEquation(Model, "NH4 sedimentation", MEqPerM2PerTs, CompartmentSolver);
	auto SO4SedimentationEq      = RegisterEquation(Model, "SO4 sedimentation", MEqPerM2PerTs, CompartmentSolver);
	auto ClSedimentationEq       = RegisterEquation(Model, "Cl sedimentation", MEqPerM2PerTs, CompartmentSolver);
	auto NO3SedimentationEq      = RegisterEquation(Model, "NO3 sedimentation", MEqPerM2PerTs, CompartmentSolver);
	auto FSedimentationEq        = RegisterEquation(Model, "F sedimentation", MEqPerM2PerTs, CompartmentSolver);
	auto PO4SedimentationEq      = RegisterEquation(Model, "PO4 sedimentation", MEqPerM2PerTs, CompartmentSolver);
	
	SetInitialValue(Model, CaSedimentationEq, 0.0);
	SetInitialValue(Model, MgSedimentationEq, 0.0);
	SetInitialValue(Model, NaSedimentationEq, 0.0);
	SetInitialValue(Model, KSedimentationEq, 0.0);
	SetInitialValue(Model, NH4SedimentationEq, 0.0);
	SetInitialValue(Model, SO4SedimentationEq, 0.0);
	SetInitialValue(Model, ClSedimentationEq, 0.0);
	SetInitialValue(Model, NO3SedimentationEq, 0.0);
	SetInitialValue(Model, FSedimentationEq, 0.0);
	SetInitialValue(Model, PO4SedimentationEq, 0.0);
	
	
	
	// These are required by the MAGIC Core:
	auto Discharge               = RegisterEquation(Model, "Discharge", MPerTs);
	auto Temperature             = RegisterEquation(Model, "Temperature", DegreesCelsius);
	auto PartialPressureCO2      = RegisterEquation(Model, "CO2 partial pressure", Percent);
	auto OAConcentration         = RegisterEquation(Model, "Organic acid concentration", MMolPerM3);
	
	SetInitialValue(Model, OAConcentration, OAConcentrationPar);
	
	auto CaExternalFlux          = RegisterEquation(Model, "Sum of Ca fluxes not related to discharge", MEqPerM2PerTs, CompartmentSolver);
	auto MgExternalFlux          = RegisterEquation(Model, "Sum of Mg fluxes not related to discharge", MEqPerM2PerTs, CompartmentSolver);
	auto NaExternalFlux          = RegisterEquation(Model, "Sum of Na fluxes not related to discharge", MEqPerM2PerTs, CompartmentSolver);
	auto KExternalFlux           = RegisterEquation(Model, "Sum of K fluxes not related to discharge", MEqPerM2PerTs, CompartmentSolver);
	auto NH4ExternalFlux         = RegisterEquation(Model, "Sum of NH4 fluxes not related to discharge", MEqPerM2PerTs, CompartmentSolver);
	auto SO4ExternalFlux         = RegisterEquation(Model, "Sum of SO4 fluxes not related to discharge", MEqPerM2PerTs, CompartmentSolver);
	auto ClExternalFlux          = RegisterEquation(Model, "Sum of Cl fluxes not related to discharge", MEqPerM2PerTs, CompartmentSolver);
	auto NO3ExternalFlux         = RegisterEquation(Model, "Sum of NO3 fluxes not related to discharge", MEqPerM2PerTs, CompartmentSolver);
	auto FExternalFlux           = RegisterEquation(Model, "Sum of F fluxes not related to discharge", MEqPerM2PerTs, CompartmentSolver);
	auto PO4ExternalFlux         = RegisterEquation(Model, "Sum of PO4 fluxes not related to discharge", MEqPerM2PerTs, CompartmentSolver);
	
	
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
	
	// The following are re-registered and defined in the decomp-uptake module:
	auto TotalTreeDecompCaSource = RegisterEquation(Model, "Total Ca source from litter decomposition", MEqPerM2PerTs);
	auto TotalTreeDecompMgSource = RegisterEquation(Model, "Total Mg source from litter decomposition", MEqPerM2PerTs);
	auto TotalTreeDecompNaSource = RegisterEquation(Model, "Total Na source from litter decomposition", MEqPerM2PerTs);
	auto TotalTreeDecompKSource  = RegisterEquation(Model, "Total K source from litter decomposition", MEqPerM2PerTs);
	auto TotalTreeCaUptake       = RegisterEquation(Model, "Total tree Ca uptake", MEqPerM2PerTs);
	auto TotalTreeMgUptake       = RegisterEquation(Model, "Total tree Mg uptake", MEqPerM2PerTs);
	auto TotalTreeNaUptake       = RegisterEquation(Model, "Total tree Na uptake", MEqPerM2PerTs);
	auto TotalTreeKUptake        = RegisterEquation(Model, "Total tree K uptake", MEqPerM2PerTs);
	auto ForestCoverAvg          = RegisterEquation(Model, "Forest cover averaged over patches", Dimensionless);
	
	// From ABCD:
	auto Runoff                  = GetEquationHandle(Model, "Runoff");
	
	// From the core wrapper:
	auto IsSoil                  = GetParameterBoolHandle(Model, "This is a soil compartment");
	auto IsTop                   = GetParameterBoolHandle(Model, "This is a top compartment");
	
	auto ConcCa                  = GetEquationHandle(Model, "Ca(2+) ionic concentration");
	auto ConcMg                  = GetEquationHandle(Model, "Mg(2+) ionic concentration");
	auto ConcNa                  = GetEquationHandle(Model, "Na(+) ionic concentration");
	auto ConcK                   = GetEquationHandle(Model, "K(+) ionic concentration");
	auto ConcNH4                 = GetEquationHandle(Model, "NH4(+) ionic concentration");
	auto ConcSO4                 = GetEquationHandle(Model, "SO4(2-) ionic concentration");
	auto ConcCl                  = GetEquationHandle(Model, "Cl(-) ionic concentration");
	auto ConcNO3                 = GetEquationHandle(Model, "NO3(-) ionic concentration");
	auto ConcF                   = GetEquationHandle(Model, "F(-) ionic concentration");
	auto ConcPO4                 = GetEquationHandle(Model, "PO4(3-) ionic concentration");

	
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
	
	
	EQUATION(Model, ForestDryDepositionModifier,
		double forest = RESULT(ForestCoverAvg);
		if(!PARAMETER(IsTop) || !PARAMETER(IsSoil)) forest = 0.0;
		return forest;
	)
	
	
#define WET_DEPOSITION(Elem) \
	EQUATION(Model, Elem##WetDeposition, \
		double precip = INPUT(Precipitation) * 1e-3; \
		double conc = PARAMETER(Elem##DepConc); \
		double concin = INPUT(Elem##WetDepositionIn); \
		double scale = INPUT(Elem##WetDepositionScale); \
		if(!PARAMETER(IsTop)) return 0.0; \
		if(!INPUT_WAS_PROVIDED(Elem##WetDepositionScale) || !std::isfinite(scale)) \
			scale = 1.0; \
		if(INPUT_WAS_PROVIDED(Elem##WetDepositionIn) && std::isfinite(concin)) \
			conc = concin; \
		return conc * precip * scale; \
	)
	
	WET_DEPOSITION(Ca)
	WET_DEPOSITION(Mg)
	WET_DEPOSITION(Na)
	WET_DEPOSITION(K)
	WET_DEPOSITION(NH4)
	WET_DEPOSITION(SO4)
	WET_DEPOSITION(Cl)
	WET_DEPOSITION(NO3)
	WET_DEPOSITION(F)
	WET_DEPOSITION(PO4)
	
	
	#undef WET_DEPOSITION
	
#define TOTAL_DEPOSITION(Elem) \
	EQUATION(Model, Elem##Deposition, \
		double totalin = INPUT(Elem##DepositionTot); \
		double wet = RESULT(Elem##WetDeposition); \
		double ddf_max = PARAMETER(Elem##DryDepFactor); \
		double forest_cover_mod = RESULT(ForestDryDepositionModifier); \
		double ddf = 1.0 + (ddf_max - 1.0)*forest_cover_mod; \
		if(!PARAMETER(IsTop)) return 0.0; \
		if(INPUT_WAS_PROVIDED(Elem##DepositionTot) && std::isfinite(totalin)) \
			return totalin; \
		return wet * ddf; \
	)
	
	TOTAL_DEPOSITION(Ca)
	TOTAL_DEPOSITION(Mg)
	TOTAL_DEPOSITION(Na)
	TOTAL_DEPOSITION(K)
	TOTAL_DEPOSITION(NH4)
	TOTAL_DEPOSITION(SO4)
	TOTAL_DEPOSITION(Cl)
	TOTAL_DEPOSITION(NO3)
	TOTAL_DEPOSITION(F)
	TOTAL_DEPOSITION(PO4)
	
	#undef TOTAL_DEPOSITION
	
#define SEDIMENTATION(Elem) \
	EQUATION(Model, Elem##SedimentationEq, \
		double sed = RESULT(Conc##Elem)*PARAMETER(Elem##Sedimentation)*RESULT(FractionOfYear); \
		if(PARAMETER(IsSoil)) sed = 0.0; \
		return sed; \
	)
	
	SEDIMENTATION(Ca)
	SEDIMENTATION(Mg)
	SEDIMENTATION(Na)
	SEDIMENTATION(K)
	SEDIMENTATION(NH4)
	SEDIMENTATION(SO4)
	SEDIMENTATION(Cl)
	SEDIMENTATION(NO3)
	SEDIMENTATION(F)
	SEDIMENTATION(PO4)
	
#undef SEDIMENTATION
	
	
	
	
	EQUATION(Model, NO3BasicInputs,
		double deposition = RESULT(NO3Deposition);
		double weathering = PARAMETER(NO3Weathering)*RESULT(FractionOfYear);
		if(!PARAMETER(ThisIsATopCompartment)) deposition = 0.0;
		return deposition + weathering;
	)
	
	EQUATION(Model, NH4BasicInputs,
		double deposition = RESULT(NH4Deposition);
		double weathering = PARAMETER(NH4Weathering)*RESULT(FractionOfYear);
		if(!PARAMETER(ThisIsATopCompartment)) deposition = 0.0;
		return deposition + weathering;
	)
	
	EQUATION(Model, PO4BasicInputs,
		double deposition = RESULT(PO4Deposition);
		double weathering = PARAMETER(PO4Weathering)*RESULT(FractionOfYear);
		if(!PARAMETER(ThisIsATopCompartment)) deposition = 0.0;
		return deposition + weathering;
	)
	
	
	
	
	
	//TODO: The uptake should maybe be limited so that the mass never goes in the negative, but it may not be a problem
	EQUATION(Model, CaExternalFlux,
		double deposition = RESULT(CaDeposition);
		double weathering = RESULT(FractionOfYear)*PARAMETER(CaWeathering);
		double forest     = RESULT(TotalTreeDecompCaSource) - RESULT(TotalTreeCaUptake);
		double sed        = RESULT(CaSedimentationEq);
		if(!PARAMETER(ThisIsATopCompartment)) { deposition = 0.0; forest = 0.0; }
		if(!PARAMETER(IsSoil)) forest = 0.0;
		return deposition + weathering + forest - sed;
	)
	
	EQUATION(Model, MgExternalFlux,
		double deposition = RESULT(MgDeposition);
		double weathering = RESULT(FractionOfYear)*PARAMETER(MgWeathering);
		double forest     = RESULT(TotalTreeDecompMgSource) - RESULT(TotalTreeMgUptake);
		double sed        = RESULT(MgSedimentationEq);
		if(!PARAMETER(ThisIsATopCompartment)) { deposition = 0.0; forest = 0.0; }
		if(!PARAMETER(IsSoil)) forest = 0.0;
		return deposition + weathering + forest - sed;
	)
	
	EQUATION(Model, NaExternalFlux,
		double deposition = RESULT(NaDeposition);
		double weathering = RESULT(FractionOfYear)*PARAMETER(NaWeathering);
		double forest     = RESULT(TotalTreeDecompNaSource) - RESULT(TotalTreeNaUptake);
		double sed        = RESULT(NaSedimentationEq);
		if(!PARAMETER(ThisIsATopCompartment)) { deposition = 0.0; forest = 0.0; }
		if(!PARAMETER(IsSoil)) forest = 0.0;
		return deposition + weathering + forest - sed;
	)
	
	EQUATION(Model, KExternalFlux,
		double deposition = RESULT(KDeposition);
		double weathering = RESULT(FractionOfYear)*PARAMETER(KWeathering);
		double forest     = RESULT(TotalTreeDecompKSource) - RESULT(TotalTreeKUptake);
		double sed        = RESULT(KSedimentationEq);
		if(!PARAMETER(ThisIsATopCompartment)) { deposition = 0.0; forest = 0.0; }
		if(!PARAMETER(IsSoil)) forest = 0.0;
		return deposition + weathering + forest - sed;
	)
	
	EQUATION(Model, NH4ExternalFlux,
		return RESULT(NH4Inputs) - RESULT(NH4ProcessesLoss) - RESULT(NH4SedimentationEq);
	)
	
	EQUATION(Model, SO4ExternalFlux,
		double deposition = RESULT(SO4Deposition);
		double weathering = RESULT(FractionOfYear)*PARAMETER(SO4Weathering);
		double sed        = RESULT(SO4SedimentationEq);
		if(!PARAMETER(ThisIsATopCompartment)) deposition = 0.0;
		return deposition + weathering - sed;
	)
	
	EQUATION(Model, NO3ExternalFlux,
		return RESULT(NO3Inputs) - RESULT(NO3ProcessesLoss) - RESULT(NO3SedimentationEq);
	)
	
	EQUATION(Model, ClExternalFlux,
		double deposition = RESULT(ClDeposition);
		double weathering = RESULT(FractionOfYear)*PARAMETER(ClWeathering);
		double sed        = RESULT(ClSedimentationEq);
		if(!PARAMETER(ThisIsATopCompartment)) deposition = 0.0;
		return deposition + weathering - sed;
	)
	
	EQUATION(Model, FExternalFlux,
		double deposition = RESULT(FDeposition);
		double weathering = RESULT(FractionOfYear)*PARAMETER(FWeathering);
		double sed        = RESULT(FSedimentationEq);
		if(!PARAMETER(ThisIsATopCompartment)) deposition = 0.0;
		return deposition + weathering - sed;
	)
	
	EQUATION(Model, PO4ExternalFlux,
		return RESULT(PO4Inputs) - RESULT(PO4ProcessesLoss) - RESULT(PO4SedimentationEq);
	)
	
	
	EndModule(Model);
}