

#include <armadillo>


inline double
logKxxT(double Log10RefCoef, double RefTempDegC, double TempDegC, double InternalEnergyChange)
// (log10) temperature corrected partitioning coefficent
{
	const double R = 8.314462618; //Gas constant [J/(mol K)]
	const double Ln10 = 2.30258509299;
	return Log10RefCoef - (InternalEnergyChange/(1e-3*R*Ln10))*(1.0/(TempDegC+273.15) - 1.0/(RefTempDegC+273.15));
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
	auto M3PerS        = RegisterUnit(Model, "m3/s");
	auto MgPerM3       = RegisterUnit(Model, "mg/m3");
	auto LPerKgOC      = RegisterUnit(Model, "log10(L/kg(OC))");
	auto KPaM3PerMol   = RegisterUnit(Model, "-log10(kPa m3/mol)");
	auto MolM3PerPa    = RegisterUnit(Model, "mol m3/Pa");
	auto MolPerPaM3    = RegisterUnit(Model, "mol/(m3 Pa)");
	auto GPerMol       = RegisterUnit(Model, "g/mol");
	auto KJPerMol      = RegisterUnit(Model, "kJ/mol");
	auto MolPerDayPa   = RegisterUnit(Model, "mol/(day Pa)");
	auto Pa            = RegisterUnit(Model, "Pa");
	auto GPerM3        = RegisterUnit(Model, "g/m3");
	auto MolPerDay     = RegisterUnit(Model, "mol/day");
	auto GPerDay       = RegisterUnit(Model, "g/day");
	
	
	
	//TODO: Some of these could be parametrized
	//constexpr double R = 8.314462618;   // Ideal gas constant [J/(mol K)]
	constexpr double R = 8.314;  //NOTE: Same as in Matlab model.
	constexpr double RefTemp = 25.0;    // Reference temperature for various constants (deg C)
	constexpr double Ea = 30000.0;      // Activation energy (J/mol)
	constexpr double ln2 = 0.69314718056;
	
	constexpr double rho_oc = 1.0;      //Density of organic carbon [kg/L]
	constexpr double rho_bc = 1.0;      //Density of black carbon (soot) [kg/L] 
	
	auto Compartment         = RegisterIndexSet(Model, "Compartment");
	auto BoundaryCompartment = RegisterIndexSet(Model, "Boundary compartment");
	auto Chemical            = RegisterIndexSet(Model, "Chemical");
	
	
	auto AirPars   = RegisterParameterGroup(Model, "Atmosphere");
	auto WaterPars = RegisterParameterGroup(Model, "Water compartment", Compartment);
	auto SedPars   = RegisterParameterGroup(Model, "Sediment compartment", Compartment);
	auto ChemPars  = RegisterParameterGroup(Model, "Chemicals", Chemical);
	auto AppxPars  = RegisterParameterGroup(Model, "KOC approximation");
	auto InitAirConc = RegisterParameterGroup(Model, "Atmospheric background", Chemical);
	auto InitBoundConc = RegisterParameterGroup(Model, "Boundary background", Chemical, BoundaryCompartment);
	auto InitCompConc = RegisterParameterGroup(Model, "Initial concentrations", Chemical, Compartment);
	
	auto FlowPars  = RegisterParameterGroup(Model, "Flow rates", Compartment, Compartment);
	auto BoundFlow = RegisterParameterGroup(Model, "Boundary flows", Compartment, BoundaryCompartment);
	
	
	auto AirTemperature      = RegisterInput(Model, "Air temperature", DegC);
	auto WaterTemperature    = RegisterInput(Model, "Water temperature", DegC);
	auto SedimentTemperature = RegisterInput(Model, "Sediment temperature", DegC);
	auto Precipitation       = RegisterInput(Model, "Precipitation", MPerDay);
	auto EmissionToAir       = RegisterInput(Model, "Emission to air", GPerDay);
	auto EmissionToWater     = RegisterInput(Model, "Emission to water", GPerDay);
	
	
	
	auto AirSideMassTransferCoeff   = RegisterParameterDouble(Model, AirPars, "Air-side mass transfer coefficient", MPerDay, 100.0, 0.0, 1000.0);	// k_VA
	auto WaterSideMassTransferCoeff = RegisterParameterDouble(Model, AirPars, "Water-side mass transfer coefficent", MPerDay, 1.0, 0.0, 10.0); // k_VW
	auto ScavengingRatio            = RegisterParameterDouble(Model, AirPars, "Scavenging ratio", Dimensionless, 200000.0, 0.0, 2000000.0, "The ratio between a raindrop's volume to the volume of air it sweeps through when falling"); // Q
	auto VolumeFractionAerosols     = RegisterParameterDouble(Model, AirPars, "Volume fraction of aerosols", Dimensionless, 1e-12, 0.0, 1e-10);  // v_Q or F_Q
	auto DryDepositionRate          = RegisterParameterDouble(Model, AirPars, "Particle dry deposition rate", MPerDay, 25.0, 0.0, 200.0); // U_Q
	auto AtmosphericResidenceTime   = RegisterParameterDouble(Model, AirPars, "Residence time of the atmospheric compartment", Days, 0.5, 0.1, 10.0); // tau
	auto AtmosphericVolume          = RegisterParameterDouble(Model, AirPars, "Volume of the atmospheric compartment", M3, 1.2e10, 1e5, 10e10); // V
	
	
	auto WaterSurfaceArea           = RegisterParameterDouble(Model, WaterPars, "Water surface area", M2, 5e6, 0.0, 361.9e12, "Surface area covered by water compartment");
	auto WaterHeight                = RegisterParameterDouble(Model, WaterPars, "Water effective height", M, 20.0, 0.0, 10984, "Vertical thickness of water compartment");
	auto ConcPOC                    = RegisterParameterDouble(Model, WaterPars, "POC concentration", MgPerM3, 100.0, 0.0, 1000.0, "Particulate organic carbon concentration");  // C_POC
	auto ConcDOC                    = RegisterParameterDouble(Model, WaterPars, "DOC concentration", MgPerM3, 3000.0, 0.0, 10000.0, "Dissolved organic carbon concentration");  // C_DOC
	auto ConcBC                     = RegisterParameterDouble(Model, WaterPars, "BC concentration", MgPerM3, 10.0, 0.0, 100.0, "Black carbon (soot) concentration");  // C_BC
	auto POCSettlingVelocity        = RegisterParameterDouble(Model, WaterPars, "POC settling velocity", MPerDay, 1.0, 0.0, 20.0, "Also applies to BC");  // U_POC
	auto HasWaterDegrad             = RegisterParameterBool(Model, WaterPars, "Degradation in water", true, "Whether or not chemicals can react/degrade in this water compartment");
	auto IsBelow                    = RegisterParameterUInt(Model, WaterPars, "Is below", Dimensionless, 10000, 0, 9999, "The numerical index of the other water compartment that this water compartment is below. If this is a surface layer, set the number to 10000");
	
	auto SedSurfaceArea             = RegisterParameterDouble(Model, SedPars, "Sediment surface area", M2, 5e6, 0.0, 361.9e12, "Surface area covered by the sediments of this compartment");
	auto SedHeight                  = RegisterParameterDouble(Model, SedPars, "Sediment effective height", M, 0.5, 0.0, 10.0, "Thickness of the sediment layer");
	auto Porosity                   = RegisterParameterDouble(Model, SedPars, "Sediment porosity", Dimensionless, 0.86, 0.0, 1.0);
	auto SedPOCVolumeFraction       = RegisterParameterDouble(Model, SedPars, "Sediment POC volume fraction", Dimensionless, 0.0825, 0.0, 0.2, "Fraction of solid sediments that is particulate organic carbon"); // f_POC
	auto SedBCVolumeFraction        = RegisterParameterDouble(Model, SedPars, "Sediment BC volume fraction", Dimensionless, 0.01, 0.0, 0.1, "Fraction of solid sediments that is black carbon (soot)"); // f_BC
	auto SedConcDOC                 = RegisterParameterDouble(Model, SedPars, "Sediment DOC concentration", MgPerM3, 78000.0, 0.0, 1e6, "Concentration in pore water");
	auto POCMineralizationHL        = RegisterParameterDouble(Model, SedPars, "POC mineralization half-life", Days, 32613.0, 0.0, 100000.0);
	auto BurialVelocity             = RegisterParameterDouble(Model, SedPars, "Burial velocity", MPerDay, 5e-7, 0.0, 5e-6);
	auto ResuspensionVelocity       = RegisterParameterDouble(Model, SedPars, "Resuspension velocity", MPerDay, 3e-6, 0.0, 3e-5);
	auto SedWaterMassTransferCoeff  = RegisterParameterDouble(Model, SedPars, "Sediment-water mass transfer coeffcient", MPerDay, 2.4e-3, 0.0, 5e-2, "Also applies to DOC");
	auto HasSedDegrad               = RegisterParameterBool(Model, SedPars, "Degradation in sediment", true, "Whether or not chemicals can react/degrade in this sediment compartment");
	
	auto LogKOW25                   = RegisterParameterDouble(Model, ChemPars, "(log10) Octanol-water partitioning coefficient", Dimensionless, 6.0, -3.0, 10.0, "Reference value at 25°C");
	auto LogKOA25                   = RegisterParameterDouble(Model, ChemPars, "(log10) Octanol-air partitioning coefficient", Dimensionless, 12.0, -3.0, 14.0, "Reference value at 25°C");
	auto MinusLogH25                = RegisterParameterDouble(Model, ChemPars, "(-log10) Henry's law constant", KPaM3PerMol, 3.0, -5.0, 10.0, "Reference value at 25°C");
	auto LogKOCObsWater             = RegisterParameterDouble(Model, ChemPars, "(log10) POC-water partitioning coefficient in water", LPerKgOC, 8.0, -3.0, 12.0);
	auto LogKOCObsSed               = RegisterParameterDouble(Model, ChemPars, "(log10) POC-water partitioning coefficent in sediments", LPerKgOC, 8.0, -3.0, 12.0);
	auto EstLogKOCWater             = RegisterParameterBool(Model, ChemPars, "Estimate the POC-water partitioning coefficent in water", false, "If true, ignore the above value, and estimate K_POC = b*K_OW^a, where a and b are given in the KOC approximation parameter group");
	auto EstLogKOCSed               = RegisterParameterBool(Model, ChemPars, "Estimate the POC-water partitioning coefficent in sediments", false, "If true, ignore the above value, and estimate K_POC = b*K_OW^a, where a and b are given in the KOC approximation parameter group");
	auto MolecularWeight            = RegisterParameterDouble(Model, ChemPars, "Molecular weight", GPerMol, 400.0, 0.0, 10000.0);
	auto DegradHLWater25            = RegisterParameterDouble(Model, ChemPars, "Degradation half-life in water", Days, 1e3, 1.0, 1e5, "Reference value at 25°C");
	auto DegradHLSed25              = RegisterParameterDouble(Model, ChemPars, "Degradation half-life in sediments", Days, 4.17e8, 1.0, 1e10, "Reference value at 25°C");
	auto InternalEnergyChangeOA     = RegisterParameterDouble(Model, ChemPars,"Internal energy change of the OA phase", KJPerMol, -100.0, -300.0, 300.0, "Often equivalent to enthalpy of phase change");
	auto InternalEnergyChangeOW     = RegisterParameterDouble(Model, ChemPars, "Internal energy change of the OW phase", KJPerMol, -100.0, -300.0, 300.0, "Often equivalent to enthalpy of phase change");
	auto InternalEnergyChangeAW     = RegisterParameterDouble(Model, ChemPars, "Internal energy change of the AW phase", KJPerMol, -100.0, -300.0, 300.0, "Often equivalent to enthalpy of phase change");
	auto ToxicEquivalentFactor      = RegisterParameterDouble(Model, ChemPars, "Toxic equivalent factor", Dimensionless, 1.0, 0.0, 50.0);
	
	
	auto POCParA                    = RegisterParameterDouble(Model, AppxPars, "K_POC a parameter", Dimensionless, 1.0, 0.1, 10.0, "From the approximation K_POC = b*K_OW^a if used");
	auto POCParB                    = RegisterParameterDouble(Model, AppxPars, "K_POC b parameter", Dimensionless, 0.35, 0.0, 10.0, "From the approximation K_POC = b*K_OW^a if used");
	auto DOCParA                    = RegisterParameterDouble(Model, AppxPars, "K_DOC a parameter", Dimensionless, 1.0, 0.1, 10.0, "From the approximation K_DOC = b*K_OW^a");
	auto DOCParB                    = RegisterParameterDouble(Model, AppxPars, "K_DOC b parameter", Dimensionless, 0.08, 0.0, 10.0, "From the approximation K_DOC = b*K_OW^a");
	auto BCParA                     = RegisterParameterDouble(Model,
	AppxPars, "K_BC a parameter", Dimensionless, 1.6, 0.1, 10.0, "From the approximation K_BC = b*K_OW^a");
	auto BCParB                     = RegisterParameterDouble(Model, AppxPars, "K_BC b parameter", Dimensionless, 0.0398, 0.0, 10.0, "From the approximation K_BC = b*K_OW^a");
	auto AerosolParA                = RegisterParameterDouble(Model, AppxPars, "K_aerosol a parameter", Dimensionless, 1.0, 0.1, 10.0, "From the approximation K_aerosol = b*K_OA^a");
	auto AerosolParB                = RegisterParameterDouble(Model, AppxPars, "K_aerosol b parameter", Dimensionless, 03.8, 0.0, 10.0, "From the approximation K_aerosol = b*K_OA^a");
	
	auto FlowRate                   = RegisterParameterDouble(Model, FlowPars, "Flow rate", M3PerS, 0.0, 0.0, 10000.0);
	
	auto FlowOut                    = RegisterParameterDouble(Model, BoundFlow, "Flow to boundary", M3PerS, 0.0, 0.0, 10000.0);
	auto FlowIn                     = RegisterParameterDouble(Model, BoundFlow, "Flow from boundary", M3PerS, 0.0, 0.0, 10000.0);
	
	
	auto BackgroundAirConc          = RegisterParameterDouble(Model, InitAirConc, "Background atmospheric concentration", GPerM3, 0.0, 0.0, 1e8);
	auto InitialWaterConc           = RegisterParameterDouble(Model, InitCompConc, "Initial concentration in water", GPerM3, 0.0, 0.0, 1e8);
	auto InitialSedConc             = RegisterParameterDouble(Model, InitCompConc, "Initial concentration in sediments", GPerM3, 0.0, 0.0, 1e8);
	auto BoundaryBackgroundConc     = RegisterParameterDouble(Model, InitBoundConc, "Background concentration in boundary", GPerM3, 0.0, 0.0, 1e8);
	
	
	
	auto LogKOWWater              = RegisterEquation(Model, "(log10) Octanol-water partitioning coefficient in water (temperature adjusted)", Dimensionless);
	auto LogKOWSed                = RegisterEquation(Model, "(log10) Octanol-water partitioning coefficient in sediments (temperature adjusted)", Dimensionless);
	auto LogKOA                   = RegisterEquation(Model, "(log10) Octanol-air partitioning coefficient (temperature adjusted)", Dimensionless);
	auto MinusLogHAir             = RegisterEquation(Model, "(-log10) Henry's law constant in atmoshpere (temperature adjusted)", KPaM3PerMol);
	auto MinusLogHWater           = RegisterEquation(Model, "(-log10) Henry's law constant in water (temperature adjusted)", KPaM3PerMol);
	auto MinusLogHSed             = RegisterEquation(Model, "(-log10) Henry's law constant in sediments (temperature adjusted)", KPaM3PerMol);
	
	auto WaterFCAir               = RegisterEquation(Model, "Water FC in atmosphere", MolPerPaM3);
	auto WaterFCWater             = RegisterEquation(Model, "Water FC in water", MolPerPaM3);
	auto WaterFCSed               = RegisterEquation(Model, "Water FC in sediments", MolPerPaM3);
	auto AirFC                    = RegisterEquation(Model, "Air FC", MolPerPaM3);
	auto AerosolFC                = RegisterEquation(Model, "Aerosol FC", MolPerPaM3);
	auto TotalFCAir               = RegisterEquation(Model, "Total FC in atmosphere", MolPerPaM3);
	auto POCFCWater               = RegisterEquation(Model, "POC FC in water", MolPerPaM3);
	auto POCFCSed                 = RegisterEquation(Model, "POC FC in sediments", MolPerPaM3);
	auto DOCFCWater               = RegisterEquation(Model, "DOC FC in water", MolPerPaM3);
	auto DOCFCSed                 = RegisterEquation(Model, "DOC FC in sediments", MolPerPaM3);
	auto BCFCWater                = RegisterEquation(Model, "BC FC in water", MolPerPaM3);
	auto BCFCSed                  = RegisterEquation(Model, "BC FC in sediments", MolPerPaM3);
	
	auto SolidFCWater             = RegisterEquation(Model, "Solids FC in water", MolPerPaM3);
	auto WaterAndDOCFCWater       = RegisterEquation(Model, "Water+DOC FC in water", MolPerPaM3);
	auto TotalFCWater             = RegisterEquation(Model, "Total FC in water", MolPerPaM3);
	auto PorewaterFCSed           = RegisterEquation(Model, "Water+DOC FC in sediments", MolPerPaM3);
	auto SolidFCSedExclSoot       = RegisterEquation(Model, "Solids FC in sediments excluding soot", MolPerPaM3);
	auto SolidFCSed               = RegisterEquation(Model, "Solids FC in sediments", MolPerPaM3);
	auto TotalFCSed               = RegisterEquation(Model, "Total FC in sediments", MolPerPaM3);
	
	auto ReactionRateWater        = RegisterEquation(Model, "Reaction rate in water", PerDay);
	auto ReactionRateSed          = RegisterEquation(Model, "Reaction rate in sediments", PerDay);
	
	auto FlowTCOut                = RegisterEquation(Model, "Flow to boundary TC", MolPerDayPa);
	auto PotentialSettlingTCOut   = RegisterEquation(Model, "Potential downward settling TC", MolPerDayPa);
	auto ReactionTCWater          = RegisterEquation(Model, "Reaction (degradation) TC in water", MolPerDayPa);
	auto ReactionTCSed            = RegisterEquation(Model, "Reaction (degradation) TC in sediments", MolM3PerPa);
	auto VolVaporTC               = RegisterEquation(Model, "Volatilisation to and vapor adsorption to water surface TC", MolPerDayPa);
	auto RainDissTC               = RegisterEquation(Model, "Rain dissolution to water surface TC", MolPerDayPa);
	auto WetDepositionTC          = RegisterEquation(Model, "Wet particle deposition TC", MolPerDayPa);
	auto DryDepositionTC          = RegisterEquation(Model, "Dry particle deposition TC", MolPerDayPa);
	auto TotalAirWaterTC          = RegisterEquation(Model, "Total air-water TC", MolPerDayPa);
	auto UpDiffTC                 = RegisterEquation(Model, "Upward water-sediment diffusion TC", MolPerDayPa);
	auto DownDiffTC               = RegisterEquation(Model, "Downward water-sediment diffusion TC", MolPerDayPa);
	auto ResuspensionTC           = RegisterEquation(Model, "Re-suspension TC", MolPerDayPa);
	auto BurialTC                 = RegisterEquation(Model, "BurialTC", MolPerDayPa);
	auto MineralizationTC         = RegisterEquation(Model, "POC mineralization TC", MolPerDayPa);
	auto GrossSedimentationTC     = RegisterEquation(Model, "Gross sedimentation TC", MolPerDayPa);
	auto IntoSedimentsTC          = RegisterEquation(Model, "Total water-sediment downward TC", MolPerDayPa);
	auto OutofSedimentsTC         = RegisterEquation(Model, "Total sediment-water upward TC", MolPerDayPa);

	auto FugacityInAir            = RegisterEquation(Model, "Fugacity in air", Pa);
	auto FugacityInWater          = RegisterEquation(Model, "Fugacity in water compartment", Pa);
	auto FugacityInSed            = RegisterEquation(Model, "Fugacity in sediment compartment", Pa);
	
	auto EmissionToWaterEq        = RegisterEquation(Model, "Emission to water", GPerDay);
	auto AdvectionFromBoundary    = RegisterEquation(Model, "Advection from boundary", MolPerDay);
	
	auto InitialFugacityInWater   = RegisterEquationInitialValue(Model, "Initial fugacity in water", Pa);
	auto InitialFugacityInSed     = RegisterEquationInitialValue(Model, "Initial fugacity in sediments", Pa);
	SetInitialValue(Model, FugacityInWater, InitialFugacityInWater);
	SetInitialValue(Model, FugacityInSed, InitialFugacityInSed);
	
	auto AirConc                  = RegisterEquation(Model, "Concentration in atmoshpere", GPerM3);
	auto WaterConc                = RegisterEquation(Model, "Total concentration in water", GPerM3);
	auto SedConc                  = RegisterEquation(Model, "Total concentration in sediments", GPerM3);
	auto WaterConcParticulate     = RegisterEquation(Model, "Particulate concentration in water", GPerM3);
	auto SedConcParticulate       = RegisterEquation(Model, "Particulate concentration in sediments (dry only)", GPerM3);
	auto ApparentDissolvedConcWater = RegisterEquation(Model, "Apparent dissolved concentration in water", GPerM3);
	auto ApparentDissolvedConcSed = RegisterEquation(Model, "Apparent dissolved concentration in sediments (water only)", GPerM3);
	
	
	auto SolveSedFlex = RegisterEquation(Model, "SedFlex solver code", Dimensionless);
	
	
	EQUATION(Model, InitialFugacityInWater,
		return PARAMETER(InitialWaterConc)/(PARAMETER(MolecularWeight)*RESULT(TotalFCWater));
	)
	
	EQUATION(Model, InitialFugacityInSed,
		return PARAMETER(InitialSedConc)/(PARAMETER(MolecularWeight)*RESULT(TotalFCSed));
	)
	
	
	
	EQUATION(Model, LogKOWWater,
		return logKxxT(PARAMETER(LogKOW25), RefTemp, INPUT(WaterTemperature), PARAMETER(InternalEnergyChangeOW));
	)
	
	EQUATION(Model, LogKOWSed,
		return logKxxT(PARAMETER(LogKOW25), RefTemp, INPUT(SedimentTemperature), PARAMETER(InternalEnergyChangeOW));
	)
	
	EQUATION(Model, LogKOA,
		return logKxxT(PARAMETER(LogKOA25), RefTemp, INPUT(AirTemperature), PARAMETER(InternalEnergyChangeOA));
	)
	
	constexpr double Offset = -std::log10(1e-3*R*(RefTemp+273.15));
	
	EQUATION(Model, MinusLogHAir,
		double logKAW_ref = -PARAMETER(MinusLogH25) + Offset;
		double logKAW_air = logKxxT(logKAW_ref, RefTemp, INPUT(AirTemperature), PARAMETER(InternalEnergyChangeAW));
		return -logKAW_air + Offset;
	)
	
	EQUATION(Model, MinusLogHWater,
		double logKAW_ref = -PARAMETER(MinusLogH25) + Offset;
		double logKAW_water = logKxxT(logKAW_ref, RefTemp, INPUT(WaterTemperature), PARAMETER(InternalEnergyChangeAW));
		//if(CURRENT_INDEX(Compartment) == FIRST_INDEX(Compartment) && CURRENT_INDEX(Chemical) == FIRST_INDEX(Chemical))
		//	WarningPrint("LogKAW_water ", logKAW_water, "\n", "LogKAW_ref ", logKAW_ref, "\n", "Offset ", Offset, "\n");
		return -logKAW_water + Offset;
	)
	
	EQUATION(Model, MinusLogHSed,
		double logKAW_ref = -PARAMETER(MinusLogH25) + Offset;
		double logKAW_sed = logKxxT(logKAW_ref, RefTemp, INPUT(SedimentTemperature), PARAMETER(InternalEnergyChangeAW));
		return -logKAW_sed + Offset;
	)
	
	
	EQUATION(Model, WaterFCAir,
		//Factor of 1e3 for converting kPa -> Pa
		return 1.0/(1e3*std::pow(10.0, -RESULT(MinusLogHAir)));
	)
	
	EQUATION(Model, WaterFCWater,
		//Factor of 1e3 for converting kPa -> Pa
		return 1.0/(1e3*std::pow(10.0, -RESULT(MinusLogHWater)));
	)
	
	EQUATION(Model, WaterFCSed,
		//Factor of 1e3 for converting kPa -> Pa
		return 1.0/(1e3*std::pow(10.0, -RESULT(MinusLogHSed)));
	)
	
	EQUATION(Model, AirFC,
		return 1.0/(R*(INPUT(AirTemperature) + 273.15));
	)
	
	EQUATION(Model, AerosolFC,
		double a = PARAMETER(AerosolParA);
		double b = PARAMETER(AerosolParB);
		double K_aerosol = b*std::pow(10.0, RESULT(LogKOA)*a);
		return RESULT(AirFC)*K_aerosol;
	)
	
	EQUATION(Model, TotalFCAir,
		return RESULT(AirFC) + PARAMETER(VolumeFractionAerosols)*RESULT(AerosolFC);
	)
	
	EQUATION(Model, POCFCWater,
		double Zw_wat = RESULT(WaterFCWater);
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
	
	EQUATION(Model, POCFCSed,
		double Zw_sed = RESULT(WaterFCSed);
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
	
	EQUATION(Model, BCFCWater,
		double Zw_wat = RESULT(WaterFCWater);
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
	
	EQUATION(Model, BCFCSed,
		double Zw_sed = RESULT(WaterFCSed);
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
	
	EQUATION(Model, DOCFCWater,
		double Zw_wat = RESULT(WaterFCWater);
		double logKOW = RESULT(LogKOWWater);
		double a      = PARAMETER(DOCParA);
		double b      = PARAMETER(DOCParB);
		double K_DOC  = b*std::pow(10.0, logKOW*a);
		return Zw_wat*K_DOC*rho_oc;
	)
	
	EQUATION(Model, DOCFCSed,
		double Zw_sed = RESULT(WaterFCSed);
		double logKOW = RESULT(LogKOWSed);
		double a      = PARAMETER(DOCParA);
		double b      = PARAMETER(DOCParB);
		double K_DOC  = b*std::pow(10.0, logKOW*a);
		return Zw_sed*K_DOC*rho_oc;
	)
	
	EQUATION(Model, SolidFCWater,
		//factor 1e-9/rho_oc for mg/m3 to mass fraction
		return (1e-9/rho_oc)*(PARAMETER(ConcPOC)*RESULT(POCFCWater)) + (1e-9/rho_bc)*(PARAMETER(ConcBC)*RESULT(BCFCWater));
	)
	
	EQUATION(Model, WaterAndDOCFCWater,
		return RESULT(WaterFCWater) + (1e-9/rho_oc)*(PARAMETER(ConcDOC)*RESULT(DOCFCWater));
	)
	
	EQUATION(Model, TotalFCWater,
		return RESULT(SolidFCWater) + RESULT(WaterAndDOCFCWater);
	)
	
	EQUATION(Model, PorewaterFCSed,
		// pore water = water + DOC
		return RESULT(WaterFCSed) + (1e-9/rho_oc)*PARAMETER(SedConcDOC)*RESULT(DOCFCSed);
	)
	
	EQUATION(Model, SolidFCSedExclSoot,
		return PARAMETER(SedPOCVolumeFraction)*RESULT(POCFCSed);
	)
	
	EQUATION(Model, SolidFCSed,
		return RESULT(SolidFCSedExclSoot) + PARAMETER(SedBCVolumeFraction)*RESULT(BCFCSed);
	)
	
	EQUATION(Model, TotalFCSed,
		double phi = PARAMETER(Porosity);
		return phi*RESULT(PorewaterFCSed) + (1.0-phi)*RESULT(SolidFCSed);
	)
	
	
	EQUATION(Model, ReactionRateWater,
		double Kreac_wat_ref = ln2/PARAMETER(DegradHLWater25);
		return Kreac_wat_ref*std::exp((Ea/R)*(1.0/(RefTemp+273.15)-1.0/(INPUT(WaterTemperature)+273.15)));
	)
	
	EQUATION(Model, ReactionRateSed,
		double Kreac_sed_ref = ln2/PARAMETER(DegradHLSed25);
		return Kreac_sed_ref*std::exp((Ea/R)*(1.0/(RefTemp+273.15)-1.0/(INPUT(SedimentTemperature)+273.15)));
	)
	
	
	EQUATION(Model, FlowTCOut,
		double SumFlowOut = 0.0;
		for(index_t Bound = FIRST_INDEX(BoundaryCompartment); Bound < INDEX_COUNT(BoundaryCompartment); ++Bound)
		{
			SumFlowOut += PARAMETER(FlowOut, Bound);
		}
		return 86400.0*SumFlowOut*RESULT(TotalFCWater);
	)
	
	EQUATION(Model, PotentialSettlingTCOut,
		// D value for poc and bc settling: (mol/(m3 oc*Pa))*(g oc/m3 w)*(m/d)*m2*(m3 oc/g oc) = mol/(Pa d)
		
		/*
		return
			RESULT(POCFCWater)*1e-3*PARAMETER(ConcPOC)*PARAMETER(POCSettlingVelocity)*PARAMETER(WaterSurfaceArea)/(1e6*rho_oc)
		  + RESULT(BCFCWater)*1e-3*PARAMETER(ConcBC)*PARAMETER(POCSettlingVelocity)*PARAMETER(WaterSurfaceArea)/(1e6*rho_bc);
		*/
		
		//TODO: multiply 1/m2  to unit of this one
		
		return
		    RESULT(POCFCWater)*1e-3*PARAMETER(ConcPOC)*PARAMETER(POCSettlingVelocity)/(1e6*rho_oc)
		  + RESULT(BCFCWater)*1e-3*PARAMETER(ConcBC)*PARAMETER(POCSettlingVelocity)/(1e6*rho_bc); 
	)
	
	EQUATION(Model, ReactionTCWater,
		return RESULT(ReactionRateWater)*RESULT(WaterFCWater)*((double)PARAMETER(HasWaterDegrad))*PARAMETER(WaterHeight)*PARAMETER(WaterSurfaceArea);
	)
	
	EQUATION(Model, ReactionTCSed,
		return RESULT(ReactionRateSed)*RESULT(WaterFCSed)*((double)PARAMETER(HasSedDegrad))*PARAMETER(SedHeight)*PARAMETER(SedSurfaceArea);
	)
	
	EQUATION(Model, VolVaporTC,
		double Value =
			1.0/(1.0/(PARAMETER(AirSideMassTransferCoeff)*RESULT(AirFC)*PARAMETER(WaterSurfaceArea)) + 1.0/(PARAMETER(WaterSideMassTransferCoeff)*RESULT(WaterFCWater)*PARAMETER(WaterSurfaceArea)));
		
		// Exchange with air only occurs in surface compartments
		if(PARAMETER(IsBelow)!=10000) return 0.0;
		return Value;
	)
	
	EQUATION(Model, RainDissTC,
		double Value = INPUT(Precipitation)*RESULT(WaterFCAir)*PARAMETER(WaterSurfaceArea);
		
		// Exchange with air only occurs in surface compartments
		if(PARAMETER(IsBelow)!=10000) return 0.0;
		return Value;
	)
	
	EQUATION(Model, WetDepositionTC,
		double Value = INPUT(Precipitation)*PARAMETER(VolumeFractionAerosols)*PARAMETER(ScavengingRatio)*RESULT(AerosolFC)*PARAMETER(WaterSurfaceArea);
		
		// Exchange with air only occurs in surface compartments
		if(PARAMETER(IsBelow)!=10000) return 0.0;
		return Value;
	)
	
	EQUATION(Model, DryDepositionTC,
		double Value = PARAMETER(DryDepositionRate)*PARAMETER(VolumeFractionAerosols)*RESULT(AerosolFC)*PARAMETER(WaterSurfaceArea);
		
		// Exchange with air only occurs in surface compartments
		if(PARAMETER(IsBelow)!=10000) return 0.0;
		return Value;
	)
	
	EQUATION(Model, TotalAirWaterTC,
		double Value = RESULT(VolVaporTC) + RESULT(RainDissTC) + RESULT(WetDepositionTC) + RESULT(DryDepositionTC);
		
		// Exchange with air only occurs in surface compartments
		if(PARAMETER(IsBelow)!=10000) return 0.0;
		return Value;
	)
	
	
	EQUATION(Model, UpDiffTC,
		return RESULT(PorewaterFCSed)*PARAMETER(SedWaterMassTransferCoeff)*PARAMETER(SedSurfaceArea);
	)
	
	EQUATION(Model, DownDiffTC,
		return RESULT(WaterAndDOCFCWater)*PARAMETER(SedWaterMassTransferCoeff)*PARAMETER(SedSurfaceArea);
	)
	
	EQUATION(Model, ResuspensionTC,
		return PARAMETER(ResuspensionVelocity)*PARAMETER(SedSurfaceArea)*RESULT(SolidFCSed);
	)
	
	EQUATION(Model, BurialTC,
		return PARAMETER(BurialVelocity)*PARAMETER(SedSurfaceArea)*RESULT(SolidFCSed);
	)
	
	EQUATION(Model, MineralizationTC,
		return (ln2*PARAMETER(SedHeight)/PARAMETER(POCMineralizationHL))*PARAMETER(SedSurfaceArea)*RESULT(SolidFCSedExclSoot);
	)
	
	EQUATION(Model, GrossSedimentationTC,
		return RESULT(ResuspensionTC) + RESULT(BurialTC) + RESULT(MineralizationTC);
	)
	
	EQUATION(Model, IntoSedimentsTC,
		return RESULT(GrossSedimentationTC) + RESULT(DownDiffTC);
	)
	
	EQUATION(Model, OutofSedimentsTC,
		return RESULT(ResuspensionTC) + RESULT(UpDiffTC);
	)
	
	
	EQUATION(Model, AirConc,
		return PARAMETER(BackgroundAirConc) + INPUT(EmissionToAir)*PARAMETER(AtmosphericResidenceTime)/PARAMETER(AtmosphericVolume);
	)
	
	EQUATION(Model, FugacityInAir,
		return RESULT(AirConc) / (PARAMETER(MolecularWeight)*RESULT(TotalFCAir));
	)
	
	EQUATION(Model, EmissionToWaterEq,
		return INPUT(EmissionToWater);
	)
	
	EQUATION(Model, AdvectionFromBoundary,
		double adv = 0.0;
		for(index_t Comp = FIRST_INDEX(BoundaryCompartment); Comp < INDEX_COUNT(BoundaryCompartment); ++Comp)
		{
			adv += PARAMETER(FlowIn, Comp)*PARAMETER(BoundaryBackgroundConc, Comp);
		}
		return adv*86400.0/PARAMETER(MolecularWeight); //((24*3600 s/day) * (mol/g) * (g/m3) * (m3/s) = mol/d)
	)
	
	EQUATION(Model, SolveSedFlex,
		u32 N = INDEX_COUNT(Compartment).Index;
		u32 N2 = N*2;
		
		
		arma::vec x0(N2, arma::fill::zeros);       // Fugacities
		arma::mat A(N2, N2, arma::fill::zeros);    // Transport capacities
		arma::vec b(N2, arma::fill::zeros);        // Sources
		
		
		//NOTE: This is a stopgap to get those in the right batch to not screw up initial values. Eventually we should fix the initial value system.
		RESULT(FugacityInWater, FIRST_INDEX(Compartment));
		RESULT(FugacityInSed,   FIRST_INDEX(Compartment));
		
		double MW = PARAMETER(MolecularWeight);
		
		for(index_t Comp = FIRST_INDEX(Compartment); Comp < INDEX_COUNT(Compartment); ++Comp)
		{
			u32 Wat = Comp.Index;
			u32 Sed = Wat + N;
			
			x0(Wat) = LAST_RESULT(FugacityInWater, Comp);
			x0(Sed) = LAST_RESULT(FugacityInSed, Comp);
			
			double Area = PARAMETER(WaterSurfaceArea, Comp);
			
			// Settling between water compartments
			u64 Below = PARAMETER(IsBelow, Comp);
			if(Below != 10000)
			{
				index_t BelowThat = INDEX_NUMBER(Compartment, (u32)Below);
				u32 FromWat = BelowThat.Index;
				
				double Settling = RESULT(PotentialSettlingTCOut, BelowThat) * Area;
				
				A(Wat, FromWat) += Settling;
				A(FromWat, FromWat) -= Settling;
			}
			
			// Sinks
			double SinkWat = RESULT(FlowTCOut, Comp) + RESULT(ReactionTCWater, Comp) + RESULT(VolVaporTC, Comp);
			double SinkSed = RESULT(ReactionTCSed, Comp) + RESULT(BurialTC, Comp);
			
			A(Wat, Wat) -= SinkWat;
			A(Sed, Sed) -= SinkSed;
			
			// Sediment-water exchange
			
			double IntoSed  = RESULT(IntoSedimentsTC, Comp);
			double OutofSed = RESULT(OutofSedimentsTC, Comp);
			
			A(Sed, Wat) += IntoSed;
			A(Wat, Wat) -= IntoSed;
			A(Wat, Sed) += OutofSed;
			A(Sed, Sed) -= OutofSed;

			double TotFCWat = RESULT(TotalFCWater, Comp);
			
			// Flow between water compartments
			for(index_t ToComp = FIRST_INDEX(Compartment); ToComp < INDEX_COUNT(Compartment); ++ToComp)
			{
				u32 ToWat = ToComp.Index;
				
				double Flow = 86400.0*PARAMETER(FlowRate, Comp, ToComp)*TotFCWat;
				A(ToWat, Wat) += Flow;
				A(Wat, Wat) -= Flow;
			}
		}
		
		double fAir = RESULT(FugacityInAir);
		
		// Adjust units of A from mol/(day Pa) to 1/day
		//NOTE: There should not be any assignments to A below this!
		// For that reason, this loop can also not be combined with the above one since it adjusts things in a different order
		for(index_t Comp = FIRST_INDEX(Compartment); Comp < INDEX_COUNT(Compartment); ++Comp)
		{
			u32 Wat = Comp.Index;
			u32 Sed = Wat + N;
			
			double WatCorr = RESULT(TotalFCWater, Comp)*PARAMETER(WaterSurfaceArea, Comp)*PARAMETER(WaterHeight, Comp);
			double SedCorr = RESULT(TotalFCSed, Comp)*PARAMETER(SedSurfaceArea, Comp)*PARAMETER(SedHeight, Comp);
			
			for(int J = 0; J < N2; ++J)
			{
				//mol/(day Pa) -> 1/day
				A(Wat, J) /= WatCorr;
				A(Sed, J) /= SedCorr;
			}
			
			// Filling in sources
			
			//NOTE: This means that EmissionToWater HAS to have an index set dependency (Chemical, Compartment)
			// We could do the division by MW inside EmissionToWaterEq though
			b(Wat) += RESULT(EmissionToWaterEq, Comp) / MW;
			b(Wat) += RESULT(TotalAirWaterTC, Comp)*fAir; //NOTE that TotalAirWaterTC is already 0 for non-surface compartments
			b(Wat) += RESULT(AdvectionFromBoundary, Comp);
			
			b(Wat) /= WatCorr;  // mol/day -> Pa/day
		}
		
		/*
		NOTE: the linear system
			dx/dt = A*x + b
		has solution
			x(t) = exp(A*t)x(0) + A^-1(exp(A*t) - I)b
		*/
		
		//if(CURRENT_INDEX(Chemical) == FIRST_INDEX(Chemical) && CURRENT_TIMESTEP() == 0)
		//	WarningPrint("\nA = \n", A, "\n\n\n");
		
		double dt = (double)CURRENT_TIME().StepLengthInSeconds / 86400.0;  // Step length in days

		arma::mat expAdt(N2, N2, arma::fill::none);
		bool Success = arma::expmat(expAdt, dt*A);
		if(!Success) FatalError("ERROR(SedFlex): Failed to exponentiate matrix");
		arma::vec x(N2, arma::fill::none);
		Success  = arma::solve(x, A, expAdt*b - b);
		if(!Success) FatalError("ERROR(SedFlex): Failed to solve linear system");
		x += expAdt*x0;
		
		//arma::mat expAdt = arma::expmat(dt*A);
		//arma::vec x  = expAdt*x0 + arma::solve(A, expAdt*b - b);
		//NOTE: In exact math, The image (colspace) of exp(A*t)-I will always be in the image of A, so the result is well defined, 
		// but there could be numerical errors in the computation of expAdt, or it could be ill-conditioned?
		// Is there a better way to do this that computes A^-1(exp(At)-I) directly, and which doesn't involve using ODE solvers (which is slow) ?
		
		// Write the results back out
		for(index_t Comp = FIRST_INDEX(Compartment); Comp < INDEX_COUNT(Compartment); ++Comp)
		{
			u32 Wat = Comp.Index;
			u32 Sed = Wat + N;
			
			SET_RESULT(FugacityInWater, x(Wat), Comp);
			SET_RESULT(FugacityInSed,   x(Sed), Comp);
		}
		
		return 0.0; //The return value of this code piece is not meaningful on its own.
	)
	
	
	
	EquationIsComputedBy(Model, FugacityInWater, SolveSedFlex, Chemical, Compartment);
	EquationIsComputedBy(Model, FugacityInSed,   SolveSedFlex, Chemical, Compartment);
	
	
	
	EQUATION(Model, WaterConc,
		return RESULT(FugacityInWater)*RESULT(TotalFCWater)*PARAMETER(MolecularWeight);
	)
	
	EQUATION(Model, SedConc,
		return RESULT(FugacityInSed)*RESULT(TotalFCSed)*PARAMETER(MolecularWeight);
	)
	
	EQUATION(Model, WaterConcParticulate,
		return RESULT(FugacityInWater)*RESULT(SolidFCWater)*PARAMETER(MolecularWeight);
	)
	
	EQUATION(Model, SedConcParticulate,
		return RESULT(FugacityInSed)*RESULT(SolidFCSed)*PARAMETER(MolecularWeight);
	)
	
	EQUATION(Model, ApparentDissolvedConcWater,
		return RESULT(FugacityInWater)*RESULT(WaterAndDOCFCWater)*PARAMETER(MolecularWeight);
	)
	
	EQUATION(Model, ApparentDissolvedConcSed,
		return RESULT(FugacityInSed)*RESULT(PorewaterFCSed)*PARAMETER(MolecularWeight);
	)

	
	EndModule(Model);
}



