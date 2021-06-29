

inline double
logKxxT(double Log10RefCoef, double RefTempDegC, double TempDegC, double InternalEnergyChange)
// (log10) temperature corrected partitioning coefficent
{
	const double R = 8.314462618; //Gas constant [J/(mol K)]
	const double Ln10 = 2.30258509299;
	return Log10RefCoef - (InternalEnergyChange/(R*Ln10))*(1.0/(TempDegC+273.15) + 1.0/(RefTempDegC+273.15));
}

static void
AddSedFlexModel(mobius_model *Model)
{
	BeginModule(Model, "SedFlex", "0.1");
	
	auto Dimensionless = RegisterUnit(Model);
	auto Days          = RegisterUnit(Model, "day");
	auto PerDay        = RegisterUnit(Model, "1/day");
	auto MPerDay       = RegisterUnit(Model, "m/day");
	auto DegC          = RegisterUnit(Model, "°C");
	auto M             = RegisterUnit(Model, "m");
	auto M2            = RegisterUnit(Model, "m2");
	auto M3            = RegisterUnit(Model, "m3");
	auto MgPerM3       = RegisterUnit(Model, "mg/m3");
	auto LPerKgOC      = RegisterUnit(Model, "log10(L/kg(OC))");
	auto KPaM3PerMol   = RegisterUnit(Model, "-log10(kPa m3/mol)");
	auto MolM3PerPa    = RegisterUnit(Model, "mol m3/Pa");
	auto GPerMol       = RegisterUnit(Model, "g/mol");
	auto KJPerMol      = RegisterUnit(Model, "kJ/mol");
	
	
	//TODO: Some of these could be parametrized
	constexpr double R = 8.314462618;   // Ideal gas constant [J/(mol K)]
	constexpr double RefTemp = 25.0;    // Reference temperature for various constants (deg C)
	constexpr double Ea = 30000.0;      // Activation energy (J/mol)
	constexpr double ln2 = 0.69314718056;
	
	constexpr double rho_OC = 1.0;      //Density of organic carbon [kg/L]
	constexpr double rho_BC = 1.0;      //Density of black carbon (soot) [kg/L] 
	
	constexpr double Offset = -std::log10(1e3*R*(RefTemp+273.15));
	
	auto Compartment = RegisterIndexSet(Model, "Compartment");
	auto Chemical    = RegisterIndexSet(Model, "Chemical");
	
	
	auto AirPars   = RegisterParameterGroup(Model, "Atmosphere");
	auto WaterPars = RegisterParameterGroup(Model, "Water compartment", Compartment);
	auto SedPars   = RegisterParameterGroup(Model, "Sediment compartment", Compartment);
	auto ChemPars  = RegisterParameterGroup(Model, "Chemistry");
	auto AppxPars  = RegisterParameterGroup(Model, "KOC approximation");
	
	auto AirTemperature      = RegisterInput(Model, "Air temperature", DegC);
	auto WaterTemperature    = RegisterInput(Model, "Water temperature", DegC);
	auto SedimentTemperature = RegisterInput(Model, "Sediment temperature", DegC);
	auto Precipitation  = RegisterInput(Model, "Precipitation", MPerDay);
	
	auto AirSideMassTransferCoeff   = RegisterParameterDouble(Model, AirPars, "Air-side mass transfer coefficient", MPerDay, 100.0, 0.0, 1000.0); // k_A
	auto WaterSideMassTransferCoeff = RegisterParameterDouble(Model, AirPars, "Water-side mass transfer coefficent", MPerDay, 1.0, 0.0, 10.0);
	auto ScavengingRatio            = RegisterParameterDouble(Model, AirPars, "Scavenging ratio", Dimensionless, 200000.0, 0.0, 2000000.0, "The ratio between a raindrop's volume to the volume of air it sweeps through when falling"); // Q
	auto VolumeFractionAerosols     = RegisterParameterDouble(Model, AirPars, "Volume fraction of aerosols", Dimensionless, 1e-12, 0.0, 1e-10);  // v_Q or F_Q
	auto DryDepositionRate          = RegisterParameterDouble(Model, AirPars, "Particle dry deposition rate", MPerDay, 25.0, 0.0, 200.0); // U_Q
	auto AtmosphericResidenceTime   = RegisterParameterDouble(Model, AirPars, "Residence time of the atmospheric compartment", Days, 0.5, 0.1, 10.0); // tau
	auto AtmosphericVolume          = RegisterParameterDouble(Model, AirPars, "Volume of the atmospheric compartment", M3, 1.2e10, 1e5, 10e10); // V
	
	
	auto WaterSurfaceArea           = RegisterParameterDouble(Model, WaterPars, "Water surface area", M2, 5e6, 0.0, 361.9e12, "Surface area covered by water compartment");
	auto WaterHeight                = RegisterParameterDouble(Model, WaterPars, "Water effective height", M, 20.0, 0.0, 10,984, "Vertical thickness of water compartment");
	auto ConcPOC                    = RegisterParameterDouble(Model, WaterPars, "POC concentration", MgPerM3, 100.0, 0.0, 1000.0, "Particulate organic carbon concentration");  // C_POC
	auto ConcDOC                    = RegisterParameterDouble(Model, WaterPars, "DOC concentration", MgPerM3, 3000.0, 0.0, 10000.0, "Dissolved organic carbon concentration");  // C_DOC
	auto ConcBC                     = RegisterParameterDouble(Model, WaterPars, "BC concentration", MgPerM3, 10.0, 0.0, 100.0, "Black carbon (soot) concentration");  // C_BC
	POCSettlingVelocity             = RegisterParameterDouble(Model, "POC settling velocity", MPerDay, 1.0, 0.0, 20.0, "Also applies to BC");  // U_POC
	auto HasWaterDegrad             = RegisterParameterBool(Model, WaterPars, "Degradation in water", true, "Wheter or not chemicals can degrade in this water compartment");
	//IsBelow: How to represent this?
	
	auto SedSurfaceArea             = RegisterParameterDouble(Model, SedPars, "Sediment surface area", M2, 5e6, 0.0, 361.9e12, "Surface area covered by the sediments of this compartment");
	auto SedHeight                  = RegisterParameterDouble(Model, SedPars, "Sediment effective height", M, 0.5, 0.0, 10.0, "Thickness of the sediment layer");
	auto Porosity                   = RegisterParameterDouble(Model, SedPars, "Sediment porosity", Dimensionless, 0.86, 0.0, 1.0);
	auto SedPOCVolumeFraction       = RegisterParameterDouble(Model, SedPars, "Sediment POC volume fraction", Dimensionless, 0.0825, 0.0, 0.2, "Fraction of solid sediments that is particulate organic carbon"); // f_POC
	auto SedimentBCVolumeFraction   = RegisterParameterDouble(Model, SedPars, "Sediment BC volume fraction", Dimensionless, 0.01, 0.0, 0.1, "Fraction of solid sediments that is black carbon (soot)"); // f_BC
	auto SedimentConcDOC            = RegisterParameterDouble(Model, SedPars, "Sediment DOC concentration", MgPerM3, 78000.0, 0.0, 1e6, "Concentration in pore water");
	auto POCMineralizationHL        = RegisterParameterDouble(Model, SedPars, "POC mineralization half-life", Days, 32613.0, 0.0, 100000.0);
	auto BurialVelocity             = RegisterParameterDouble(Model, SedPars, "Burial velocity", MPerDay, 5e-7, 0.0, 5e-6);
	auto ResuspensionVelocity       = RegisterParameterDouble(Model, SedPars, "Resuspension velocity", MPerDay, 3e-6, 0.0, 3e-5);
	auto SedWaterMassTransferCoeff  = RegisterParameterDouble(Model, SedPars, "Sediment-water mass transfer coeffcient", MPerDay, 2.4e-3, 0.0, 5e-2, "Also applies to DOC");
	
	auto LogKOW25                   = RegisterParameterDouble(Model, ChemPars, "(log10) Octanol-water partitioning coefficient", Dimensionless, 6.0, -3.0, 10.0, "Reference value at 25°C");
	auto LogKOA25                   = RegisterParameterDouble(Model, ChemPars, "(log10) Octanol-air partitioning coefficient", Dimensionless, 12.0, -3.0, 14.0, "Reference value at 25°C");
	auto MinusLogH25                = RegisterParameterDouble(Model, ChemPars, "(-log10) Henry's law constant", KPaM3PerMol, 3.0, -5.0, 10.0, "Reference value at 25°C");
	auto LogKOCObsWater             = RegisterParameterDouble(Model, ChemPars, "(log10) POC-water partitioning coefficient in open water", LPerKgOC, 8.0, -3.0, 12.0);
	auto LogKOCObsSed               = RegisterParameterDouble(Model, ChemPars, "(log10) POC-water partitioning coefficent in sediments"
	auto EstLogKOCWater             = RegisterParameterBool(Model, ChemPars, "Estimate the POC-water partitioning coefficent in open water", false, "If true, ignore the above value, and estimate K_POC = b*K_OW^a, where a and b are given in the KOC approximation parameter group");
	auto EstLogKOCSed               = RegisterParameterBool(Model, ChemPars, "Estimate the POC-water partitioning coefficent in sediments", false, "If true, ignore the above value, and estimate K_POC = b*K_OW^a, where a and b are given in the KOC approximation parameter group");
	auto MolecularWeight            = RegisterParameterDouble(Model, ChemPars, "Molecular weight", GPerMol, 400.0, 0.0, 10000.0);
	auto DegradHLWater25            = RegisterParameterDouble(Model, ChemPars, "Degradation half-life in open water", Days, 1e3, 1.0, 1e5, "Reference value at 25°C");
	auto DegradHLSed25              = RegisterParameterDouble(Model, ChemPars, "Degradation half-life in sediments", Days, 4.17e8, 1.0, 1e10, "Reference value at 25°C");
	auto InternalEnergyChangeOA     = RegisterParameterDouble(Model, "Internal energy change of the OA phase", KJPerMol, -100.0, -300.0, 300.0, "Often equivalent to enthalpy of phase change");
	auto InternalEnergyChangeOW     = RegisterParameterDouble(Model, "Internal energy change of the OW phase", KJPerMol, -100.0, -300.0, 300.0, "Often equivalent to enthalpy of phase change");
	auto InternalEnergyChangeAW     = RegisterParameterDouble(Model, "Internal energy change of the AW phase", KJPerMol, -100.0, -300.0, 300.0, "Often equivalent to enthalpy of phase change");
	
	
	auto POCParA                    = RegisterParameterDouble(Model, AppxPars, "K_POC a parameter", Dimensionless, 1.0, 0.1, 10.0, "From the approximation K_POC = b*K_OW^a if used");
	auto POCParB                    = RegisterParameterDouble(Model, AppxPars, "K_POC b parameter", Dimensionless, 0.35, 0.0, 10.0, "From the approximation K_POC = b*K_OW^a if used");
	auto DOCParA                    = RegisterParameterDouble(Model, AppxPars, "K_DOC a parameter", Dimensionless, 1.0, 0.1, 10.0, "From the approximation K_DOC = b*K_OW^a");
	auto DOCParB                    = RegisterParameterDouble(Model, AppxPars, "K_DOC b parameter", Dimensionless, 0.08, 0.0, 10.0, "From the approximation K_DOC = b*K_OW^a");
	auto BCParA                     = RegisterParameterDouble(Model,
	AppxPars, "K_BC a parameter", Dimensionless, 1.6, 0.1, 10.0, "From the approximation K_BC = b*K_OW^a");
	auto BCParB                     = RegisterParameterDouble(Model, AppxPars, "K_BC b parameter", Dimensionless, 0.0398, 1.0, 0.0, 10.0, "From the approximation K_BC = b*K_OW^a");
	auto AerosolParA                = RegisterParameterDouble(Model, AppxPars, "K_aerosol a parameter", Dimensionless, 1.0, 0.1, 10.0, "From the approximation K_aerosol = b*K_OA^a");
	auto AerosolParB                = RegisterParameterDouble(Model, AppxPars, "K_aerosol b parameter", Dimensionless, 03.8, 0.0, 10.0, "From the approximation K_aerosol = b*K_OA^a");
	
	
	auto LogKOWWater              = RegisterEquation(Model, "(log10) Octanol-water partitioning coefficient in open water (temperature adjusted)", Dimensionless);
	auto LogKOWSed                = RegisterEquation(Model, "(log10) Octanol-water partitioning coefficient in sediments (temperature adjusted)", Dimensionless);
	auto LogKOA                   = RegisterEquation(Model, "(log10) Octanol-air partitioning coefficient (temperature adjusted)", Dimensionless);
	auto MinusLogHAir             = RegisterEquation(Model, "(-log10) Henry's law constant in atmoshpere (temperature adjusted)", KPaM3PerMol);
	auto MinusLogHWater           = RegisterEquation(Model, "(-log10) Henry's law constant in open water (temperature adjusted)", KPaM3PerMol);
	auto MinusLogHSed             = RegisterEquation(Model, "(-log10) Henry's law constant in sediments (temperature adjusted)", KPaM3PerMol);
	
	auto WaterFugacityCapacityAir   = RegisterEquation(Model, "Water fugacity capacity in atmosphere", MolM3PerPa);
	auto WaterFugacityCapacityWater = RegisterEquation(Model, "Water fugacity capacity in open water", MolM3PerPa);
	auto WaterFugacityCapacitySed   = RegisterEquation(Model, "Water fugacity capacity in sediments", MolM3PerPa);
	auto AirFugacityCapacity        = RegisterEquation(Model, "Air fugacity capacity", MolM3PerPa);
	auto AerosolFugacityCapacity    = RegisterEquation(Model, "Aerosol fugacity capacity", MolM3PerPa);
	auto POCFugacityCapacityWater   = RegisterEquation(Model, "POC fugacity capacity in open water", MolM3PerPa);
	auto POCFugacityCapacitySed     = RegisterEquation(Model, "POC fugacity capacity in sediments", MolM3PerPa);
	auto DOCFugacityCapacityWater   = RegisterEquation(Model, "DOC fugacity capacity in open water", MolM3PerPa);
	auto DOCFugacityCapacitySed     = RegisterEquation(Model, "DOC fugacity capacity in sediments", MolM3PerPa);
	auto BCFugacityCapacityWater    = RegisterEquation(Model, "BC fugacity capacity in open water", MolM3PerPa);
	auto BCFugacityCapacitySed      = RegisterEquation(Model, "BC fugacity capacity in sediments", MolM3PerPa);
	
	auto DegradationRateWater       = RegisterEquation(Model, "Degradation rate in open water", PerDay);
	auto DegradationRateSed         = RegisterEquation(Model, "Degradation rate in sediments", PerDay);
	
	
	EQUATION(Model, LogKOWWater,
		return logKxxT(PARAMETER(LogKOW25), RefTemp, INPUT(WaterTemperature), PARAMETER(InternalEnergyChangeOW));
	)
	
	EQUATION(Model, LogKOWSed,
		return logKxxT(PARAMETER(LogKOW25), RefTemp, INPUT(SedimentTemperature), PARAMETER(InternalEnergyChangeOW));
	)
	
	EQUATION(Model, LogKOA,
		return logKxxT(PARAMETER(LogKOA25), RefTemp, INPUT(AirTemperature), PARAMETER(InternalEnergyChangeOA));
	)
	
	EQUATION(Model, MinusLogHAir,
		double logKAW_ref = -PARAMETER(MinusLogH25) + Offset;
		double logKAW_air = logKxxT(logKAW_ref, RefTemp, INPUT(AirTemperature), PARAMETERS(InternalEnergyChangeAW));
		return -logKAW_air + Offset;
	)
	
	EQUATION(Model, MinusLogHWater,
		double logKAW_ref = -PARAMETER(MinusLogH25) + Offset;
		double logKAW_water = logKxxT(logKAW_ref, RefTemp, INPUT(WaterTemperature), PARAMETERS(InternalEnergyChangeAW));
		return -logKAW_water + Offset;
	)
	
	EQUATION(Model, MinusLogHSed,
		double logKAW_ref = -PARAMETER(MinusLogH25) + Offset;
		double logKAW_sed = logKxxT(logKAW_ref, RefTemp, INPUT(SedimentTemperature), PARAMETERS(InternalEnergyChangeAW));
		return -logKAW_sed + Offset;
	)
	
	
	EQUATION(Model, WaterFugacityCapacityAir,
		//Factor of 1e3 for converting kPa -> Pa
		return 1.0/(1e3*std::pow(10.0, -PARAMETER(MinusLogHAir)));
	)
	
	EQUATION(Model, WaterFugacityCapacityWater,
		//Factor of 1e3 for converting kPa -> Pa
		return 1.0/(1e3*std::pow(10.0, -PARAMETER(MinusLogHWater)));
	)
	
	EQUATION(Model, WaterFugacityCapacitySed,
		//Factor of 1e3 for converting kPa -> Pa
		return 1.0/(1e3*std::pow(10.0, -PARAMETER(MinusLogHSed)));
	)
	
	EQUATION(Model, AirFugacityCapacity,
		return 1.0/(R*(INPUT(AirTemperature) + 273.15));
	)
	
	EQUATION(Model, AerosolFugacityCapacity,
		double a = PARAMETER(AerosolParA);
		double b = PARAMETER(AerosolParB);
		double K_aerosol = b*std::pow(10.0, RESULT(LogKOA)*a);
		return RESULT(AirFugacityCapacity)*K_aerosol;
	)
	
	EQUATION(Model, POCFugacityCapacityWater,
		double Zw_wat = RESULT(WaterFugacityCapacityWater);
		double logK_POC = PARAMETER(LogKOCObsWater);
		double logKOW   = RESULT(LogKOWWater);
		double a      = PARAMETER(POCParA);
		double b      = PARAMETER(POCParB);
		double K_POC;
		
		if(PARAMETER(EstLogKOCWater))
			K_POC = b*std::pow(10.0, logKOW*a);
		else
			K_POC = std::pow(10.0, logK_POC);
		
		return Zw_wat*K_POC*rho_oc;
	)
	
	EQUATION(Model, POCFugacityCapacityWater,
		double Zw_sed = RESULT(WaterFugacityCapacitySed);
		double logK_POC = PARAMETER(LogKOCObsSed);
		double logKOW   = RESULT(LogKOWSed);
		double a      = PARAMETER(POCParA);
		double b      = PARAMETER(POCParB);
		double K_POC;
		
		if(PARAMETER(EstLogKOCWater))
			K_POC = b*std::pow(10.0, logKOW*a);
		else
			K_POC = std::pow(10.0, logK_POC);
		
		return Zw_sed*K_POC*rho_oc;
	)
	
	EQUATION(Model, BCFugacityCapacityWater,
		double Zw_wat = RESULT(WaterFugacityCapacityWater);
		double logKOW = RESULT(LogKOWWater);
		double a      = PARAMETER(BCParA);
		double b      = PARAMETER(BCParB);
		double K_BC;
		
		if(PARAMETER(EstLogKOCWater))
			K_BC = b*std::pow(10.0, logKOW*a);
		else
			K_BC = 0.0; //if Kd is given, it is supposed to relate to organic carbon, and black carbon is neglected
		
		return Zw_wat*K_BC*rho_bc;
	)
	
	EQUATION(Model, BCFugacityCapacitySed,
		double Zw_sed = RESULT(WaterFugacityCapacitySed);
		double logKOW = RESULT(LogKOWSed);
		double a      = PARAMETER(BCParA);
		double b      = PARAMETER(BCParB);
		double K_BC;
		
		if(PARAMETER(EstLogKOCWater))
			K_BC = b*std::pow(10.0, logKOW*a);
		else
			K_BC = 0.0; //if Kd is given, it is supposed to relate to organic carbon, and black carbon is neglected
		
		return Zw_sed*K_BC*rho_bc;
	)
	
	EQUATION(Model, DOCFugacityCapacityWater,
		double Zw_wat = RESULT(WaterFugacityCapacityWater);
		double logKOW = RESULT(LogKOWWater);
		double a      = PARAMETER(DOCParA);
		double b      = PARAMETER(DOCParB);
		double K_DOC  = b*std::pow(10.0, logKOW*a);
		return Zw_wat*K_DOC*rho_oc;
	)
	
	EQUATION(Model, DOCFugacityCapacitySed,
		double Zw_sed = RESULT(WaterFugacityCapacitySed);
		double logKOW = RESULT(LogKOWSed);
		double a      = PARAMETER(DOCParA);
		double b      = PARAMETER(DOCParB);
		double K_DOC  = b*std::pow(10.0, logKOW*a);
		return Zw_sed*K_DOC*rho_oc;
	)
	
	EQUATION(Model, DegradationRateWater,
		double Kreac_wat_ref = ln2/PARAMETER(DegradHLWater25);
		Kreac_wat_ref*std::exp((Ea/R)*(1.0/(RefTemp+273.15)-1.0/(INPUT(WaterTemperature)+273.15)));
	)
	
	EQUATION(Model, DegradationRateSed,
		double Kreac_sed_ref = ln2/PARAMETER(DegradHLSed25);
		Kreac_sed_ref*std::exp((Ea/R)*(1.0/(RefTemp+273.15)-1.0/(INPUT(SedimentTemperature)+273.15)));
	)
}



