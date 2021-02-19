

static void
AddMagicForestModule(mobius_model *Model)
{
	BeginModule(Model, "MAGIC Forest", "_dev");
	
	SetModuleDescription(Model, R""""(
Forest growth driver module developed as part of the CatchCAN project.
)"""");
	
	auto Dimensionless  = RegisterUnit(Model);
	auto DegreesCelsius	= RegisterUnit(Model, "°C");
	auto MgPerL         = RegisterUnit(Model, "mg/l");
	auto MmPerTs        = RegisterUnit(Model, "mm/month");
	auto MEqPerM3       = RegisterUnit(Model, "meq/m3");
	auto MEqPerM2PerTs  = RegisterUnit(Model, "meq/m2/month");
	auto MMolPerM2PerTs = RegisterUnit(Model, "mmol/m2/month");
	
	//auto Compartment             = GetIndexSetHandle(Model, "Compartment");
	auto Compartment             = RegisterIndexSet(Model, "Compartment");
	
	auto CompartmentTemp         = RegisterParameterGroup(Model, "Compartment temperature", Compartment);
	
	auto MinCompartmentTemp      = RegisterParameterDouble(Model, CompartmentTemp, "Minimal compartment temperature", DegreesCelsius, 0.0, -10.0, 10.0);
	
	
	auto Deposition              = RegisterParameterGroup(Model, "Deposition");
	
	auto SeaSaltFactor           = RegisterParameterDouble(Model, Deposition, "Sea salt factor", Dimensionless, 1.0, 0.001, 5.0, "Cl deposition is multiplied with this value");
	auto SO4Factor               = RegisterParameterDouble(Model, Deposition, "SO4* factor", Dimensionless, 1.0, 0.001, 5.0, "SO4 deposition is multiplied with this to get SO4 excess deposition");
	auto CaFactor                = RegisterParameterDouble(Model, Deposition, "Ca* factor", Dimensionless, 1.0, 0.001, 5.0, "SO4 excess deposition is multiplied with this to get Ca excess deposition");
	auto NFactor                 = RegisterParameterDouble(Model, Deposition, "N factor", Dimensionless, 1.0, 0.001, 5.0, "NH4 and NO3 deposition are multiplied with this value");
	
	
	auto CAndN                   = RegisterParameterGroup(Model, "Carbon and Nitrogen", Compartment);
	
	auto DecompR0                = RegisterParameterDouble(Model, CAndN, "C decomposition at 0°C", MMolPerM2PerTs, 0.0, 0.0, 1000.0);
	auto DecompQ10               = RegisterParameterDouble(Model, CAndN, "C decomposition Q10", Dimensionless, 1.0, 1.0, 5.0);
	auto UptakeR0                = RegisterParameterDouble(Model, CAndN, "N uptake at 0°C", MMolPerM2PerTs, 0.0, 0.0, 1000.0);
	auto UptakeQ10               = RegisterParameterDouble(Model, CAndN, "N uptake Q10", Dimensionless, 1.0, 1.0, 5.0);
	auto NO3UptakeScale          = RegisterParameterDouble(Model, CAndN, "NO3 uptake scale", Dimensionless, 1.0, 0.1, 10.0);
	auto NH4UptakeScale          = RegisterParameterDouble(Model, CAndN, "NH4 uptake scale", Dimensionless, 1.0, 0.1, 10.0);
	
	auto LitterCN                = RegisterParameterDouble(Model, CAndN, "Litter C/N", Dimensionless, 50.0, 0.1, 200.0);
	
	
	auto AirTemperature          = RegisterInput(Model, "Air temperature", DegreesCelsius);
	auto Precipitation           = RegisterInput(Model, "Precipitation", MmPerTs);
	
	auto CaPrecipConc            = RegisterInput(Model, "Ca conc in precip", MEqPerM3);
	auto MgPrecipConc            = RegisterInput(Model, "Mg conc in precip", MEqPerM3);
	auto NaPrecipConc            = RegisterInput(Model, "Na conc in precip", MEqPerM3);
	auto KPrecipConc             = RegisterInput(Model, "K conc in precip", MEqPerM3);
	auto NH4PrecipConc           = RegisterInput(Model, "NH4 conc in precip", MEqPerM3);
	auto SO4PrecipConc           = RegisterInput(Model, "SO4 conc in precip", MEqPerM3);
	auto ClPrecipConc            = RegisterInput(Model, "Cl conc in precip", MEqPerM3);
	auto NO3PrecipConc           = RegisterInput(Model, "NO3 conc in precip", MEqPerM3);
	auto FPrecipConc             = RegisterInput(Model, "F conc in precip", MEqPerM3);
	
	
	
	auto Temperature             = RegisterEquation(Model, "Temperature", DegreesCelsius);
	auto Decomposition           = RegisterEquation(Model, "Organic C decomposition", MMolPerM2PerTs);
	auto UptakeBaseline          = RegisterEquation(Model, "N uptake baseline", MMolPerM2PerTs);
	auto DesiredNO3Uptake        = RegisterEquation(Model, "Desired NO3 uptake", MMolPerM2PerTs);
	auto DesiredNH4Uptake        = RegisterEquation(Model, "Desired NH4 uptake", MMolPerM2PerTs);
	auto LitterC                 = RegisterEquation(Model, "Organic C litter", MMolPerM2PerTs);
	
	
	auto MgDeposition            = RegisterEquation(Model, "Mg deposition", MEqPerM2PerTs);
	auto NH4Deposition           = RegisterEquation(Model, "NH4 deposition", MEqPerM2PerTs);
	auto NO3Deposition           = RegisterEquation(Model, "NO3 deposition", MEqPerM2PerTs);
	auto ClDeposition            = RegisterEquation(Model, "Cl deposition", MEqPerM2PerTs);
	auto NaDeposition            = RegisterEquation(Model, "Na deposition", MEqPerM2PerTs);
	auto KDeposition             = RegisterEquation(Model, "K deposition", MEqPerM2PerTs);
	auto SO4Deposition           = RegisterEquation(Model, "SO4 deposition", MEqPerM2PerTs);
	auto CaDeposition            = RegisterEquation(Model, "Ca deposition", MEqPerM2PerTs);
	auto FDeposition             = RegisterEquation(Model, "F deposition", MEqPerM2PerTs);
	
	EQUATION(Model, Temperature,
		return Max(INPUT(AirTemperature), PARAMETER(MinCompartmentTemp));
	)
	
	EQUATION(Model, Decomposition,
		return PARAMETER(DecompR0) * std::pow(PARAMETER(DecompQ10), RESULT(Temperature) * 0.1);
	)
	
	EQUATION(Model, UptakeBaseline,
		return PARAMETER(UptakeR0) * std::pow(PARAMETER(UptakeQ10), RESULT(Temperature) * 0.1);
	)
	
	EQUATION(Model, DesiredNO3Uptake,
		return RESULT(UptakeBaseline) * PARAMETER(NO3UptakeScale);
	)
	
	EQUATION(Model, DesiredNH4Uptake,
		return RESULT(UptakeBaseline) * PARAMETER(NH4UptakeScale);
	)
	
	EQUATION(Model, LitterC,
		return (RESULT(DesiredNO3Uptake) + RESULT(DesiredNH4Uptake))*PARAMETER(LitterCN);
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
	
	
	EndModule(Model);
}