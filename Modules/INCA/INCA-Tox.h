


//#include "../boost_solvers.h"

static double
TemperatureAdjustLogKH(double LogH25, double T, double dAW)
{
	constexpr double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
	double tdiff = (1.0 / T - 1.0/(273.15 + 25.0));
	double LogHT = LogH25 - ((1e3*dAW + R*(273.15 + 25.0)) / (std::log(10.0)*R) )  * tdiff;
	return LogHT;
}

static double
TemperatureAdjustLogKow(double LogKOW25, double T, double dOW)
{
	constexpr double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
	double tdiff = (1.0 / T - 1.0/(273.15 + 25.0));
	double LogKOWT = LogKOW25 - (1e3*dOW / (std::log(10.0)*R)) * tdiff;
	return LogKOWT;
}

static void
AddIncaToxModule(mobius_model *Model)
{
	BeginModule(Model, "INCA-Tox", "0.3.1");
	
	SetModuleDescription(Model, R""""(
Inca-Tox, also called INCA-Contaminants or INCA-POP is described in

[Nizzetto, L., Butterfield, D., Futter, M., Lin, Y., Allan, I., Larssen, T., 2016. Assessment of contaminant fate in catchments using a novel integrated hydrobiogeochemical-multimedia fate model. Science of the Total Environment 544, 553-563.](https://doi.org/10.1016/j.scitotenv.2015.11.087)

[Model home page](https://www.niva.no/en/publications/inca-contaminants)

New to the Mobius version is some simplification in process implementations.

New to V0.3: Multiple contaminants at a time.
)"""");
	
	auto Dimensionless    = RegisterUnit(Model);
	auto Ng               = RegisterUnit(Model, "ng");
	auto M                = RegisterUnit(Model, "m");
	auto M3               = RegisterUnit(Model, "m3");
	auto M3PerM2          = RegisterUnit(Model, "m3/m2");
	auto M3PerDay         = RegisterUnit(Model, "m3/day");
	auto NgPerKm2         = RegisterUnit(Model, "ng/km2");
	auto NgPerKm2PerDay   = RegisterUnit(Model, "ng/km2/day");
	auto NgPerDay         = RegisterUnit(Model, "ng/day");
	auto NgPerM2PerDay    = RegisterUnit(Model, "ng/m2/day");
	auto NgPerM2          = RegisterUnit(Model, "ng/m2");
	auto NgPerM3          = RegisterUnit(Model, "ng/m3");
	auto NgPerL           = RegisterUnit(Model, "ng/l");
	auto NgPerKg          = RegisterUnit(Model, "ng/kg");
	auto KgPerM3          = RegisterUnit(Model, "kg/m3");
	auto M3PerKg          = RegisterUnit(Model, "m3/kg");
	auto M3PerKm2         = RegisterUnit(Model, "m3/km2");
	auto KiloJoulesPerMol = RegisterUnit(Model, "kJ/mol");
	auto PascalM3PerMol   = RegisterUnit(Model, "Pa m3/mol");
	auto MPerDay          = RegisterUnit(Model, "m/day");
	auto MPerS            = RegisterUnit(Model, "m/s");
	auto M2PerS           = RegisterUnit(Model, "m2/s");
	auto PGPerM3          = RegisterUnit(Model, "pg/m3");
	auto K                = RegisterUnit(Model, "K");
	auto Days             = RegisterUnit(Model, "day");
	auto PerDay           = RegisterUnit(Model, "1/day");
	auto GPerMol          = RegisterUnit(Model, "g/mol");
	auto Cm3PerMol        = RegisterUnit(Model, "cm3/mol");
	auto GPerCmPerSPerHundred = RegisterUnit(Model, "10^-2 g/cm/s");
	auto DegreesCelsius   = RegisterUnit(Model, "°C");
	auto Log10            = RegisterUnit(Model, "log10");	
	
	auto DepositionToLand         = RegisterInput(Model, "Contaminant deposition to land", NgPerM2PerDay);
	auto DepositionToReach        = RegisterInput(Model, "Contaminant deposition to reach", NgPerDay);
	auto AtmosphericContaminantConcentrationIn = RegisterInput(Model, "Atmospheric contaminant concentration", NgPerM3);
	auto WindSpeed                = RegisterInput(Model, "Wind speed at 10m", MPerS);
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto Soils          = GetIndexSetHandle(Model, "Soils");
	auto Reach          = GetIndexSetHandle(Model, "Reaches");
	auto Class          = GetIndexSetHandle(Model, "Sediment size class");
	auto DirectRunoff = RequireIndex(Model, Soils, "Direct runoff");
	auto Soilwater    = RequireIndex(Model, Soils, "Soil water");
	auto Groundwater  = RequireIndex(Model, Soils, "Groundwater");
	
	auto Contaminant  = RegisterIndexSet(Model, "Contaminants");
	
	
	auto Chemistry = RegisterParameterGroup(Model, "Chemistry", Contaminant);
	auto Land      = RegisterParameterGroup(Model, "Contaminants by land class", LandscapeUnits, Contaminant);
	auto ContaminantReach = RegisterParameterGroup(Model, "Contaminants by reach", Reach, Contaminant);
	auto ReachChar = RegisterParameterGroup(Model, "Additional Reach characteristics", Reach);

	//TODO: As always, find better default, min, max values for parameters!
	
	auto ContaminantMolarMass                  = RegisterParameterDouble(Model, Chemistry, "Contaminant molar mass", GPerMol, 50.0, 0.0, 1000.0);
	auto ContaminantMolecularVolume            = RegisterParameterDouble(Model, Chemistry, "Contaminant molecular volume at surface pressure", Cm3PerMol, 20.0, 0.0, 1000.0);
	auto AirWaterPhaseTransferEnthalpy         = RegisterParameterDouble(Model, Chemistry, "Enthalpy of phase transfer between air and water", KiloJoulesPerMol, 0.0, -100.0, 100.0);
	auto OctanolWaterPhaseTransferEnthalpy     = RegisterParameterDouble(Model, Chemistry, "Enthalpy of phase transfer between octanol and water", KiloJoulesPerMol, 0.0, -100.0, 100.0);
	auto OctanolAirPhaseTransferEnthalpy       = RegisterParameterDouble(Model, Chemistry, "Enthalpy of phase transfer between octanol and air", KiloJoulesPerMol, 0.0, -100.0, 100.0, "Used if dry or wet deposition is computed by the model");
	auto HenrysConstant25                      = RegisterParameterDouble(Model, Chemistry, "Henry's constant at 25°C", PascalM3PerMol, 0.0);
	auto Log10OctanolWaterPartitioningCoefficient25 = RegisterParameterDouble(Model, Chemistry, "Log10 Octanol-water partitioning coefficient at 25°C", Dimensionless, 0.0);
	auto Log10OctanolAirPartitioningCoefficient25   = RegisterParameterDouble(Model, Chemistry, "Log10 Octanol-air partitioning coefficient at 25°C", Dimensionless, 0.0);
	
	auto AtmosphericContaminantConcentration   = RegisterParameterDouble(Model, Chemistry, "Atmospheric contaminant concentration", NgPerM3, 0.0, 0.0, 100.0);
	
	auto SoilContaminantHalfLife                   = RegisterParameterDouble(Model, Chemistry, "Soil contaminant half life (easily accessible fraction)", Days, 3650.0, 10.0, 365000.0);
	auto SoilContaminantHalfLifePotentially        = RegisterParameterDouble(Model, Chemistry, "Soil contaminant half life (potentially accessible fraction)", Days, 3650.0, 10.0, 365000.0);
	auto GroundwaterContaminantHalfLife            = RegisterParameterDouble(Model, Chemistry, "Groundwater contaminant half life", Days, 3650.0, 10.0, 365000.0);
	auto ReachContaminantHalfLife                  = RegisterParameterDouble(Model, Chemistry, "Reach contaminant half life", Days, 3650.0, 10.0, 365000.0);
	auto StreamBedContaminantHalfLife              = RegisterParameterDouble(Model, Chemistry, "Stream bed contaminant half life", Days, 3650.0, 10.0, 365000.0);
	
	
	auto DegradationResponseToTemperature           = RegisterParameterDouble(Model, Chemistry, "Degradation rate response to 10°C change in temperature", Dimensionless, 1.0, 0.5, 5.0);
	auto TemperatureAtWhichDegradationRatesAreMeasured = RegisterParameterDouble(Model, Chemistry, "Temperature at which degradation rates are measured", DegreesCelsius, 20.0, -20.0, 50.0);
	auto DepositionToLandPar                   = RegisterParameterDouble(Model, Chemistry, "Deposition to land", NgPerM2PerDay, 0.0, 0.0, 100.0, "Constant daily deposition. Alternative to time series of the same name");
	auto ComputeWetDeposition                  = RegisterParameterBool(Model, Chemistry, "Compute wet deposition", false, "If true, precipitation is assumed to have an equilibrium concentration of contaminants relative to the atmospheric concentration");
	auto ComputeDryDeposition                  = RegisterParameterBool(Model, Chemistry, "Compute dry deposition", false, "If true, dry deposition flux is given by atmospheric concentration multiplied with dry deposition velocity");
	auto DryDepositionVelocity                 = RegisterParameterDouble(Model, Chemistry, "Dry deposition velocity", MPerDay, 0.0, 0.0, 100.0, "Used if dry deposition is computed");
	auto AerosolVolumeFraction                 = RegisterParameterDouble(Model, Chemistry, "Volume fraction of aerosols", Dimensionless, 1e-10, 0.0, 1e-9, "Used if dry or wet deposition is computed");
	auto ScavengingRatio                       = RegisterParameterDouble(Model, Chemistry, "Scavenging ratio", Dimensionless, 2e5, 0.0, 2e6, "Used if wet deposition is computed. Ratio of the volume an air drop passes through to its own volume.");
	
	
	auto AirSoilOverallMassTransferCoefficient = RegisterParameterDouble(Model, Land, "Overall air-soil mass transfer coefficient", MPerDay, 0.0, 0.0, 100.0);
	auto TransferCoefficientBetweenEasilyAndPotentiallyAccessible = RegisterParameterDouble(Model, Land, "Transfer coefficient between easily and potentially accessible fractions", MPerDay, 0.0, 0.0, 100.0);
	//auto InitialContaminantMassInSoil  = RegisterParameterDouble(Model, Land, "Initial contaminant mass in soil (easily accessible fraction)", NgPerKm2, 0.0, 0.0, 1e15);
	//auto InitialContaminantMassInPotentiallyAccessibleFraction = RegisterParameterDouble(Model, Land, "Initial contaminant mass in soil (potentially accessible fraction)", NgPerKm2, 0.0, 0.0, 1e15);
	
	auto InitialSoilWaterContaminantConcentration = RegisterParameterDouble(Model, Land, "Initial soil water contaminant concentration", NgPerM3, 0.0, 0.0, 1e5);
	auto InitialSoilSOCContaminantConcentration = RegisterParameterDouble(Model, Land, "Initial soil SOC contaminant concentration", NgPerKg, 0.0, 0.0, 1e5);
	
	auto InitialContaminantMassInGroundwater = RegisterParameterDouble(Model, ContaminantReach, "Initial contaminant mass in groundwater", NgPerKm2, 0.0, 0.0, 1e3);
	auto InitialContaminantMassInReach = RegisterParameterDouble(Model, ContaminantReach, "Initial contaminant mass in reach", Ng, 0.0, 0.0, 1e3);
	
	//Seems a little weird to put this in the contaminants "folder", but there is no reason to put it anywhere else.
	auto HeightOfLargeStones = RegisterParameterDouble(Model, ReachChar, "Average height of large stones in the stream bed", M, 0.0, 0.0, 0.5);
	auto AverageBedGrainDiameter = RegisterParameterDouble(Model, ReachChar, "Average bed grain diameter", M, 0.0001, 0.0, 0.1);
	//auto SedimentDryDensity = RegisterParameterDouble(Model, ContaminantReach, "Sediment dry density", KgPerM3, 2000.0, 0.0, 10000.0);
	auto SedimentPorosity   = RegisterParameterDouble(Model, ReachChar, "Sediment porosity", Dimensionless, 0.1, 0.0, 0.99);
	
	
	 //SoilTemperature.h
	auto SoilTemperature  = GetEquationHandle(Model, "Soil temperature");
	
	 //WaterTemperature.h
	auto WaterTemperature = GetEquationHandle(Model, "Water temperature");
	
	 //PERSiST.h
	auto Precipitation    = GetInputHandle(Model, "Actual precipitation");
	auto AirTemperature   = GetInputHandle(Model, "Air temperature");
	auto WaterDepth       = GetEquationHandle(Model, "Water depth");
	auto RunoffToReach    = GetEquationHandle(Model, "Runoff to reach");
	auto PercolationInput = GetEquationHandle(Model, "Percolation input");
	auto ReachFlow        = GetEquationHandle(Model, "Reach flow");
	auto ReachVolume      = GetEquationHandle(Model, "Reach volume");
	auto ReachVelocity    = GetEquationHandle(Model, "Reach velocity");
	auto ReachDepth       = GetEquationHandle(Model, "Reach depth");
	auto ReachAbstraction = GetEquationHandle(Model, "Reach abstraction");
	
	//INCA-Sed.h
	auto TotalMassOfBedGrainPerUnitArea = GetEquationHandle(Model, "Total mass of bed sediment per unit area");
	auto ReachShearVelocity = GetEquationHandle(Model, "Reach shear velocity");
	
	 //INCA-Tox-C.h
	auto SoilDOCMass              = GetEquationHandle(Model, "Soil DOC mass");
	auto SoilDOCFluxToReach       = GetEquationHandle(Model, "Soil DOC flux to reach");
	auto SoilDOCFluxToGroundwater = GetEquationHandle(Model, "Soil DOC flux to groundwater");
	auto ReachDOCOutput           = GetEquationHandle(Model, "Reach DOC output");
	auto ReachDOCMass             = GetEquationHandle(Model, "Reach DOC mass");
	auto SOCDeliveryToReachPerLU  = GetEquationHandle(Model, "SOC delivery to reach per landscape unit");
	auto TotalSOCDeposition       = GetEquationHandle(Model, "Total reach SOC deposition");
	auto TotalSOCEntrainment      = GetEquationHandle(Model, "Total reach SOC entrainment");
	auto TotalSuspendedSOCMass    = GetEquationHandle(Model, "Total suspended SOC mass");
	auto TotalBedSOCMass          = GetEquationHandle(Model, "Total stream bed SOC mass");
	auto TotalReachSOCFlux        = GetEquationHandle(Model, "Total reach SOC flux");
	
	// Persist.h
	auto CatchmentArea   = GetParameterDoubleHandle(Model, "Terrestrial catchment area");
	auto Percent         = GetParameterDoubleHandle(Model, "%");
	auto MaximumCapacity = GetParameterDoubleHandle(Model, "Maximum capacity");
	auto ReachLength     = GetParameterDoubleHandle(Model, "Reach length");
	auto ReachWidth      = GetParameterDoubleHandle(Model, "Reach bottom width");
	
	//INCA-Tox-C.h
	auto SoilSOCMass     = GetParameterDoubleHandle(Model, "Soil SOC mass");
	auto SizeOfEasilyAccessibleFraction = GetParameterDoubleHandle(Model, "Size of easily accessible SOC fraction");
	
	auto SoilTemperatureKelvin = RegisterEquation(Model, "Soil temperature in Kelvin", K);
	auto SoilDegradationTemperatureModifier = RegisterEquation(Model, "Temperature modifier for degradation in soil", Dimensionless);
	auto ReachDegradationTemperatureModifier = RegisterEquation(Model, "Temperature modifier for degradation in reach", Dimensionless);
	
	EQUATION(Model, SoilTemperatureKelvin,
		return 273.15 + RESULT(SoilTemperature);
	)
	
	EQUATION(Model, SoilDegradationTemperatureModifier,
		return pow(PARAMETER(DegradationResponseToTemperature), 0.1*(RESULT(SoilTemperature) - PARAMETER(TemperatureAtWhichDegradationRatesAreMeasured)));
	)
	
	EQUATION(Model, ReachDegradationTemperatureModifier,
		return pow(PARAMETER(DegradationResponseToTemperature), 0.1*(RESULT(WaterTemperature) - PARAMETER(TemperatureAtWhichDegradationRatesAreMeasured)));
	)
	
	
	auto SoilSolver = RegisterSolver(Model, "Soil Solver", 0.1, IncaDascru);
	
	auto AtmosphereHenrysConstant          = RegisterEquation(Model, "Henry's constant (atmosphere)", PascalM3PerMol);
	auto AtmosphereAirWaterPartitioningCoefficient = RegisterEquation(Model, "Air-water partitioning coefficient (atmosphere)", Dimensionless);
	auto AerosolAirPartitioningCoefficient = RegisterEquation(Model, "Aerosol-air partitioning coefficient (atmosphere)", Dimensionless);
	auto AerosolContaminantConcentration   = RegisterEquation(Model, "Aerosol contaminant concentration", NgPerM3);
	auto ComputedPrecipitationContaminantConcentration = RegisterEquation(Model, "Computed contaminant concentration in precipitation", NgPerM3);
	auto ComputedWetDeposition             = RegisterEquation(Model, "Computed wet deposition", NgPerM2PerDay);
	auto ComputedDryDeposition             = RegisterEquation(Model, "Computed dry deposition", NgPerM2PerDay);
	auto ContaminantInputsToSoil           = RegisterEquation(Model, "Contaminant inputs to soil", NgPerKm2PerDay);
	
	
	auto HenrysConstant                      = RegisterEquation(Model, "Henry's constant (soil)", PascalM3PerMol);
	auto AirWaterPartitioningCoefficient     = RegisterEquation(Model, "Air-water partitioning coefficient (soil)", Dimensionless);
	auto OctanolWaterPartitioningCoefficient = RegisterEquation(Model, "Octanol-water partitioning coefficient (soil)", Dimensionless);
	auto WaterSOCPartitioningCoefficient     = RegisterEquation(Model, "Water-SOC partitioning coefficient (soil)", M3PerKg);
	auto WaterDOCPartitioningCoefficient     = RegisterEquation(Model, "Water-DOC partitioning coefficient (soil)", M3PerKg);
	
	auto ContaminantDeliveryToReachByErosion = RegisterEquation(Model, "Contaminant delivery to reach by erosion", NgPerDay, SoilSolver);
	
	auto TotalContaminantDeliveryToReach     = RegisterEquationCumulative(Model, "Contaminant delivery to reach by erosion summed over land classes", ContaminantDeliveryToReachByErosion, LandscapeUnits);
	
	auto SoilWaterContaminantConcentration = RegisterEquation(Model, "Soil water contaminant concentration", NgPerM3, SoilSolver);
	auto SoilSOCContaminantConcentration   = RegisterEquation(Model, "Soil SOC contaminant concentration (easily accessible fraction)", NgPerKg, SoilSolver);
	auto SoilSOCContaminantConcentrationPotentiallyAccessible = RegisterEquation(Model, "Soil SOC contaminant concentration (potentially accessible fraction)", NgPerKg, SoilSolver);
	auto SoilDOCContaminantConcentration   = RegisterEquation(Model, "Soil DOC contaminant concentration", NgPerKg, SoilSolver);
	auto SoilAirContaminantConcentration   = RegisterEquation(Model, "Soil Air contaminant concentration", NgPerM3, SoilSolver);
	auto DiffusiveAirSoilExchangeFlux      = RegisterEquation(Model, "Diffusive exchange of contaminants between soil and atmosphere", NgPerKm2PerDay, SoilSolver);
	auto SoilContaminantDegradation        = RegisterEquation(Model, "Soil contaminant degradation (easily accessible fraction)", NgPerKm2PerDay, SoilSolver);
	auto SoilContaminantDegradationPotentiallyAccessible = RegisterEquation(Model, "Soil contaminant degradation (potentially accessible fraction)", NgPerKm2PerDay, SoilSolver);
	
	auto SoilWaterVolume                   = RegisterEquation(Model, "Soil water volume", M3PerKm2);
	auto SoilAirVolume                     = RegisterEquation(Model, "Soil air volume", M3PerKm2);
	
	
	auto InitialContaminantMassInSoil  = RegisterEquationInitialValue(Model, "Initial contaminant mass in soil (easily accessible fraction)", NgPerKm2);
	auto InitialContaminantMassInPotentiallyAccessibleFraction = RegisterEquationInitialValue(Model, "Initial contaminant mass in soil (potentially accessible fraction)", NgPerKm2);
	
	auto ContaminantMassInSoil             = RegisterEquationODE(Model, "Contaminant mass in soil (easily accessible fraction)", NgPerKm2, SoilSolver);
	SetInitialValue(Model, ContaminantMassInSoil, InitialContaminantMassInSoil);
	
	auto ContaminantMassInPotentiallyAccessibleFraction = RegisterEquationODE(Model, "Contaminant mass in soil (potentially accessible fraction)", NgPerKm2, SoilSolver);
	SetInitialValue(Model, ContaminantMassInPotentiallyAccessibleFraction, InitialContaminantMassInPotentiallyAccessibleFraction);
	auto TransferFromPotentiallyToEasilyAccessible = RegisterEquation(Model, "Contaminant transfer from potentially to easily accessible fraction", NgPerKm2PerDay, SoilSolver);
	
	
	auto SoilContaminantFluxToReach        = RegisterEquation(Model, "Soil contaminant flux to reach", NgPerKm2PerDay, SoilSolver);
	auto SoilContaminantFluxToGroundwater  = RegisterEquation(Model, "Soil contaminant flux to groundwater", NgPerKm2PerDay, SoilSolver);
	
	auto GroundwaterContaminantDegradation = RegisterEquation(Model, "Groundwater contaminant degradation", NgPerKm2PerDay, SoilSolver);
	auto GroundwaterContaminantFluxToReach = RegisterEquation(Model, "Groundwater contaminant flux to reach", NgPerKm2PerDay, SoilSolver);
	auto ContaminantMassInGroundwater      = RegisterEquationODE(Model, "Contaminant mass in groundwater", NgPerKm2, SoilSolver);
	SetInitialValue(Model, ContaminantMassInGroundwater, InitialContaminantMassInGroundwater);
	auto GroundwaterContaminantConcentration = RegisterEquation(Model, "Groundwater contaminant concentration", NgPerM3);
	
	EQUATION(Model, InitialContaminantMassInSoil,
		double concinsoc = PARAMETER(InitialSoilSOCContaminantConcentration);
		double socmass   = PARAMETER(SoilSOCMass)*PARAMETER(SizeOfEasilyAccessibleFraction)*1e6; //kg/m2 -> kg/km2
		double concinwater = PARAMETER(InitialSoilWaterContaminantConcentration);
		double watervolume = RESULT(SoilWaterVolume);
		
		return concinsoc*socmass + concinwater*watervolume;
	)
	
	EQUATION(Model, InitialContaminantMassInPotentiallyAccessibleFraction,
		double concinsoc = PARAMETER(InitialSoilSOCContaminantConcentration);
		double socmass   = PARAMETER(SoilSOCMass)*(1.0 - PARAMETER(SizeOfEasilyAccessibleFraction))*1e6; //kg/m2 -> kg/km2
		return concinsoc * socmass;
	)
	
	EQUATION(Model, AtmosphereHenrysConstant,
		double LogHT = TemperatureAdjustLogKH(std::log10(PARAMETER(HenrysConstant25)), INPUT(AirTemperature) + 273.15, PARAMETER(AirWaterPhaseTransferEnthalpy));
		return std::pow(10.0, LogHT);
	)
	
	EQUATION(Model, AtmosphereAirWaterPartitioningCoefficient,
		double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
		return RESULT(AtmosphereHenrysConstant) / (R * (INPUT(AirTemperature) + 273.15));
	)
	
	EQUATION(Model, AerosolAirPartitioningCoefficient,
		double LogKOAT = TemperatureAdjustLogKow(PARAMETER(Log10OctanolAirPartitioningCoefficient25), INPUT(AirTemperature) + 273.15, PARAMETER(OctanolAirPhaseTransferEnthalpy));
		double LogKaerosol = LogKOAT + 0.56;     // 0.56=log10(3.8)   Source: SedFlex, but TODO check properly why this number is used and if it should be parametrized. See McLeod &al 2010 
		return std::pow(10.0, LogKaerosol);
	)
	
	EQUATION(Model, ComputedPrecipitationContaminantConcentration,
		double airwater_p = RESULT(AtmosphereAirWaterPartitioningCoefficient);
		double air_c = IF_INPUT_ELSE_PARAMETER(AtmosphericContaminantConcentrationIn, AtmosphericContaminantConcentration);
		return air_c/airwater_p; // ng/m3
	)
	
	EQUATION(Model, AerosolContaminantConcentration,
		double aerosol_air_p = RESULT(AerosolAirPartitioningCoefficient);
		double air_c = IF_INPUT_ELSE_PARAMETER(AtmosphericContaminantConcentrationIn, AtmosphericContaminantConcentration);
		return air_c*aerosol_air_p;
	)
	
	EQUATION(Model, ComputedWetDeposition,
		double water_in = INPUT(Precipitation) * 1e-3; // m/day
		double conc_precip = RESULT(ComputedPrecipitationContaminantConcentration); // ng/m3
		
		double scavenging = PARAMETER(ScavengingRatio)*PARAMETER(AerosolVolumeFraction)*RESULT(AerosolContaminantConcentration);
		
		if(PARAMETER(ComputeWetDeposition))
			return (conc_precip + scavenging) * water_in; // ng/m2/day
		return 0.0;
	)
	
	EQUATION(Model, ComputedDryDeposition,
		//double conc_air = IF_INPUT_ELSE_PARAMETER(AtmosphericContaminantConcentrationIn, AtmosphericContaminantConcentration);
		double conc_aerosol = RESULT(AerosolContaminantConcentration)*PARAMETER(AerosolVolumeFraction);
		
		double velocity = PARAMETER(DryDepositionVelocity);
		if(PARAMETER(ComputeDryDeposition))
			return conc_aerosol * velocity;
		return 0.0;
	)
	
	
	
	
	EQUATION(Model, HenrysConstant,
		double LogHT = TemperatureAdjustLogKH(std::log10(PARAMETER(HenrysConstant25)), RESULT(SoilTemperatureKelvin), PARAMETER(AirWaterPhaseTransferEnthalpy));
		return std::pow(10.0, LogHT);
	)
	
	EQUATION(Model, AirWaterPartitioningCoefficient,
		double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
		return RESULT(HenrysConstant) / (R * RESULT(SoilTemperatureKelvin));
	)
	
	EQUATION(Model, OctanolWaterPartitioningCoefficient,
		double LogKOWT = TemperatureAdjustLogKow(PARAMETER(Log10OctanolWaterPartitioningCoefficient25), RESULT(SoilTemperatureKelvin), PARAMETER(OctanolWaterPhaseTransferEnthalpy));
		return std::pow(10.0, LogKOWT);
	)
	
	EQUATION(Model, WaterSOCPartitioningCoefficient,
		double densityofSOC = 1900.0; // kg/m^3
		double rOC = 0.41;   // Empirical constant.
		return RESULT(OctanolWaterPartitioningCoefficient) * rOC / densityofSOC;
	)
	
	EQUATION(Model, WaterDOCPartitioningCoefficient,
		double densityofDOC = 1100.0; // kg/m^3
		return std::pow(10.0, 0.93*std::log10(RESULT(OctanolWaterPartitioningCoefficient)) - 0.45) / densityofDOC;
	)
	
	EQUATION(Model, SoilWaterVolume,
		return RESULT(WaterDepth, Soilwater) * 1e3;  //Convert mm*km2 / km2 to m3/km2
	)
	
	EQUATION(Model, SoilAirVolume,
		return (PARAMETER(MaximumCapacity, Soilwater, CURRENT_INDEX(LandscapeUnits)) - RESULT(WaterDepth, Soilwater)) * 1e3;  //Convert mm*km2 / km2 to m3/km2
	)
	
	EQUATION(Model, SoilWaterContaminantConcentration,
		return 
			RESULT(ContaminantMassInSoil) /
			(
			  RESULT(WaterSOCPartitioningCoefficient) * PARAMETER(SoilSOCMass)*PARAMETER(SizeOfEasilyAccessibleFraction)*1e6
			+ RESULT(WaterDOCPartitioningCoefficient) * RESULT(SoilDOCMass)
			+ RESULT(AirWaterPartitioningCoefficient) * RESULT(SoilAirVolume)
			+ RESULT(SoilWaterVolume)
			);
	)
	
	EQUATION(Model, SoilSOCContaminantConcentration,
		return RESULT(SoilWaterContaminantConcentration) * RESULT(WaterSOCPartitioningCoefficient);
	)
	
	EQUATION(Model, SoilDOCContaminantConcentration,
		return RESULT(SoilWaterContaminantConcentration) * RESULT(WaterDOCPartitioningCoefficient);
	)
	
	EQUATION(Model, SoilAirContaminantConcentration,
		return RESULT(SoilWaterContaminantConcentration) * RESULT(AirWaterPartitioningCoefficient);
	)
	
	EQUATION(Model, ContaminantMassInPotentiallyAccessibleFraction,
		return
			  -RESULT(TransferFromPotentiallyToEasilyAccessible)
			  -RESULT(SoilContaminantDegradationPotentiallyAccessible);
	)
	
	EQUATION(Model, TransferFromPotentiallyToEasilyAccessible,
		return 1e6 * PARAMETER(TransferCoefficientBetweenEasilyAndPotentiallyAccessible) * (RESULT(SoilSOCContaminantConcentrationPotentiallyAccessible) - RESULT(SoilSOCContaminantConcentration));
	)
	
	EQUATION(Model, SoilSOCContaminantConcentrationPotentiallyAccessible,
		double SOCmass = PARAMETER(SoilSOCMass)*(1.0 - PARAMETER(SizeOfEasilyAccessibleFraction))*1e6;
		return SafeDivide(RESULT(ContaminantMassInPotentiallyAccessibleFraction), SOCmass);
	)
	
	EQUATION(Model, SoilContaminantDegradationPotentiallyAccessible,
		return HalfLifeToRate(PARAMETER(SoilContaminantHalfLifePotentially)) * RESULT(SoilDegradationTemperatureModifier) * RESULT(ContaminantMassInPotentiallyAccessibleFraction);
	)
	
	
	EQUATION(Model, DiffusiveAirSoilExchangeFlux,
		double atmospheric = IF_INPUT_ELSE_PARAMETER(AtmosphericContaminantConcentrationIn, AtmosphericContaminantConcentration);
		return 1e6 * PARAMETER(AirSoilOverallMassTransferCoefficient) * (/*1e-3*/atmospheric - RESULT(SoilAirContaminantConcentration));
	)
	
	
	
	EQUATION(Model, ContaminantInputsToSoil,
		double input = IF_INPUT_ELSE_PARAMETER(DepositionToLand, DepositionToLandPar) + RESULT(ComputedWetDeposition) + RESULT(ComputedDryDeposition);
		return input * 1e6; //NOTE: convert ng/m2 -> ng/km2
	)
	
	EQUATION(Model, ContaminantDeliveryToReachByErosion,
		return RESULT(SoilSOCContaminantConcentration) * RESULT(SOCDeliveryToReachPerLU);
	)
	
	EQUATION(Model, SoilContaminantFluxToReach,
		double water_runoff = RESULT(RunoffToReach, Soilwater) + RESULT(RunoffToReach, DirectRunoff);
		return
			water_runoff * RESULT(SoilWaterContaminantConcentration) * 1000.0 // Convert km^2 mm/day * ng/m^3 to ng/day
		  + RESULT(SoilDOCFluxToReach) * RESULT(SoilDOCContaminantConcentration);
	)
	
	EQUATION(Model, SoilContaminantFluxToGroundwater,
		return
			RESULT(PercolationInput, Groundwater) * RESULT(SoilWaterContaminantConcentration) * 1000.0 // Convert km^2 mm/day * ng/m^3 to ng/day
		  + RESULT(SoilDOCFluxToGroundwater) * RESULT(SoilDOCContaminantConcentration);
	)
	
	EQUATION(Model, SoilContaminantDegradation,
		double degradablemass =
		  RESULT(SoilWaterVolume) * RESULT(SoilWaterContaminantConcentration)
		+ RESULT(SoilDOCMass)     * RESULT(SoilDOCContaminantConcentration)
		+ PARAMETER(SoilSOCMass)*PARAMETER(SizeOfEasilyAccessibleFraction)*1e6  * RESULT(SoilSOCContaminantConcentration);
		
		return HalfLifeToRate(PARAMETER(SoilContaminantHalfLife)) * RESULT(SoilDegradationTemperatureModifier) * degradablemass;
	)
	
	EQUATION(Model, ContaminantMassInSoil,
		return
		  RESULT(ContaminantInputsToSoil)
		- RESULT(SoilContaminantFluxToReach)
		- RESULT(SoilContaminantFluxToGroundwater)
		+ RESULT(DiffusiveAirSoilExchangeFlux)
		- RESULT(SoilContaminantDegradation)
		- RESULT(ContaminantDeliveryToReachByErosion)
		+ RESULT(TransferFromPotentiallyToEasilyAccessible);
	)
	
	EQUATION(Model, GroundwaterContaminantFluxToReach,
		//TODO: Do we need to care about partitioning here? Probably only if we ever need to track what enters the reach through DOC and water separately. But not in this initial simple version.
		return RESULT(RunoffToReach, Groundwater) * SafeDivide(RESULT(ContaminantMassInGroundwater), RESULT(WaterDepth, Groundwater));
	)
	
	EQUATION(Model, GroundwaterContaminantDegradation,
		double degradablemass = RESULT(ContaminantMassInGroundwater);
		double degr_temp = RESULT(SoilDegradationTemperatureModifier);
		//double degr_temp = 1.0;
		return HalfLifeToRate(PARAMETER(GroundwaterContaminantHalfLife)) * degr_temp * degradablemass;
	)
	
	EQUATION(Model, ContaminantMassInGroundwater,
		return
		  RESULT(SoilContaminantFluxToGroundwater)
		- RESULT(GroundwaterContaminantFluxToReach)
		- RESULT(GroundwaterContaminantDegradation);
	)
	
	EQUATION(Model, GroundwaterContaminantConcentration,
		return SafeDivide(RESULT(ContaminantMassInGroundwater), RESULT(WaterDepth, Groundwater))*1e-3;
	)

	
	//auto ReachSolver = RegisterSolver(Model, "Reach contaminant solver", 0.1, IncaDascru); //NOTE: We can't use the reach solver from PERSiST, because we depend on the grain solver that again depends on the PERSiST reach solver.
	auto ReachSolver = RegisterSolver(Model, "Reach contaminant solver", 0.1, BoostRosenbrock4, 1e-3, 1e-3);
	
	auto WaterTemperatureKelvin        = RegisterEquation(Model, "Water temperature in Kelvin", K);
	
	auto DiffuseContaminantOutput      = RegisterEquation(Model, "Diffuse contaminant output", NgPerDay);
	auto TotalDiffuseContaminantOutput = RegisterEquationCumulative(Model, "Total diffuse contaminant output", DiffuseContaminantOutput, LandscapeUnits);
	auto ReachContaminantInputFromUpstream = RegisterEquation(Model, "Reach contaminant input from upstream", NgPerDay);
	auto ReachContaminantInput         = RegisterEquation(Model, "Reach contaminant input", NgPerDay);
	auto ContaminantMassInReach        = RegisterEquationODE(Model, "Contaminant mass in reach", Ng, ReachSolver);
	SetInitialValue(Model, ContaminantMassInReach, InitialContaminantMassInReach);
	auto ReachContaminantDissolvedFlux = RegisterEquation(Model, "Reach flux of dissolved contaminants", NgPerDay, ReachSolver);
	auto ReachContaminantSolidFlux     = RegisterEquation(Model, "Reach flux of solid-bound contaminants", NgPerDay, ReachSolver);
	auto ReachContaminantFlux          = RegisterEquation(Model, "Total reach contaminant flux", NgPerDay, ReachSolver);
	auto ReachContaminantDegradation   = RegisterEquation(Model, "Reach contaminant degradation", NgPerDay, ReachSolver);
	auto ReachContaminantAbstraction   = RegisterEquation(Model, "Reach contaminant abstraction", NgPerDay, ReachSolver);
	
	auto ReachHenrysConstant                      = RegisterEquation(Model, "Henry's constant (reach)", PascalM3PerMol);
	auto ReachAirWaterPartitioningCoefficient     = RegisterEquation(Model, "Air-water partitioning coefficient (reach)", Dimensionless);
	auto ReachOctanolWaterPartitioningCoefficient = RegisterEquation(Model, "Octanol-water partitioning coefficient (reach)", Dimensionless);
	auto ReachWaterSOCPartitioningCoefficient     = RegisterEquation(Model, "Water-SOC partitioning coefficient (reach)", M3PerKg);
	auto ReachWaterDOCPartitioningCoefficient     = RegisterEquation(Model, "Water-DOC partitioning coefficient (reach)", M3PerKg);
	
	auto MolecularDiffusivityOfCompoundInAir      = RegisterEquation(Model, "Molecular diffusivity of compound in air", M2PerS);
	auto MolecularDiffusivityOfWaterVapourInAir   = RegisterEquation(Model, "Molecular diffusivity of water vapour in air", M2PerS);
	auto MolecularDiffusivityOfCompoundInWater    = RegisterEquation(Model, "Molecular diffusivity of compound in water", M2PerS);
	auto ReachKinematicViscosity                  = RegisterEquation(Model, "Reach kinematic viscosity", M2PerS);
	auto ReachDynamicViscosity                    = RegisterEquation(Model, "Reach dynamic viscosity", GPerCmPerSPerHundred);
	auto SchmidtNumber                            = RegisterEquation(Model, "Schmidt number", Dimensionless);
	auto NonDimensionalRoughnessParameter         = RegisterEquation(Model, "Non-dimensional roughness parameter", Dimensionless);
	auto ElementFroudeNumber                      = RegisterEquation(Model, "Element Froude number", Dimensionless);
	auto DiffusivityOfDissolvedCompoundInWater    = RegisterEquation(Model, "Diffusivity of dissolved compound in water", M2PerS);
	
	
	auto ReachAirContaminantTransferVelocity      = RegisterEquation(Model, "Reach air contamninant transfer velocity", MPerDay);
	auto ReachWaterContaminantTransferVelocity    = RegisterEquation(Model, "Reach water contaminant transfer velocity", MPerDay);
	auto ReachOverallAirWaterContaminantTransferVelocity = RegisterEquation(Model, "Reach overall air-water transfer velocity", MPerDay);
	auto DiffusiveAirReachExchangeFlux            = RegisterEquation(Model, "Diffusive air reach exchange flux", NgPerDay, ReachSolver);
	
	auto ReachWaterContaminantConcentration = RegisterEquation(Model, "Reach water contaminant concentration", NgPerM3, ReachSolver);
	auto ReachDOCContaminantConcentration   = RegisterEquation(Model, "Reach DOC contaminant concentration", NgPerKg, ReachSolver);
	auto ReachSOCContaminantConcentration = RegisterEquation(Model, "Reach SOC contaminant concentration", NgPerKg, ReachSolver);
	
	auto ReachContaminantDeposition = RegisterEquation(Model, "Reach contaminant deposition", NgPerM2PerDay, ReachSolver);
	auto ReachContaminantEntrainment = RegisterEquation(Model, "Reach contaminant entrainment", NgPerM2PerDay, ReachSolver);
	
	auto PoreWaterVolume = RegisterEquation(Model, "Pore water volume", M3PerM2);
	
	auto BedContaminantDegradation        = RegisterEquation(Model, "Stream bed contaminant degradation", NgPerM2PerDay, ReachSolver);
	auto BedContaminantMass               = RegisterEquationODE(Model, "Stream bed contaminant mass", NgPerM2, ReachSolver);
	//SetInitialValue

	auto BedWaterContaminantConcentration = RegisterEquation(Model, "Stream bed pore water contaminant concentration", NgPerM3, ReachSolver);
	auto BedSOCContaminantConcentration   = RegisterEquation(Model, "Stream bed SOC contaminant concentration", NgPerKg, ReachSolver);
	
	auto DiffusiveSedimentReachExchangeFlux = RegisterEquation(Model, "Diffusive sediment reach water exchange flux", NgPerDay, ReachSolver);
	
	
	
	EQUATION(Model, DiffuseContaminantOutput,
		return
		( RESULT(SoilContaminantFluxToReach)
		+ RESULT(GroundwaterContaminantFluxToReach)
		)
		* PARAMETER(CatchmentArea) * PARAMETER(Percent)*0.01;
	)
	
	EQUATION(Model, ReachContaminantInputFromUpstream,
		index_t Cont = CURRENT_INDEX(Contaminant);
		
		double upstreamflux = 0.0;
		for(index_t Input : BRANCH_INPUTS(Reach))
			upstreamflux += RESULT(ReachContaminantFlux, Input, Cont);
			
		return upstreamflux;
	)
	
	EQUATION(Model, ReachContaminantInput,

		return
			  RESULT(ReachContaminantInputFromUpstream)
			+ RESULT(TotalDiffuseContaminantOutput)
			+ RESULT(TotalContaminantDeliveryToReach)
			+ INPUT(DepositionToReach)
			;
	)
	
	EQUATION(Model, ReachContaminantDissolvedFlux,
		return
			  RESULT(ReachFlow) * RESULT(ReachWaterContaminantConcentration) * 86400.0
			+ RESULT(ReachDOCOutput) * RESULT(ReachDOCContaminantConcentration);
	)
	
	EQUATION(Model, ReachContaminantSolidFlux,
		return RESULT(TotalReachSOCFlux) * RESULT(ReachSOCContaminantConcentration);
	)
	
	EQUATION(Model, ReachContaminantFlux,
		return
		  RESULT(ReachContaminantDissolvedFlux) + RESULT(ReachContaminantSolidFlux);
	)
	
	EQUATION(Model, ReachContaminantDegradation,
		return HalfLifeToRate(PARAMETER(ReachContaminantHalfLife)) * RESULT(ContaminantMassInReach) * RESULT(ReachDegradationTemperatureModifier);
	)
	
	EQUATION(Model, ReachContaminantAbstraction,
		return SafeDivide(RESULT(ContaminantMassInReach), RESULT(ReachVolume)) * RESULT(ReachAbstraction)*86400.0;
	)
	
	EQUATION(Model, ContaminantMassInReach,
		return
			  RESULT(ReachContaminantInput)
			- RESULT(ReachContaminantFlux)
			- RESULT(ReachContaminantAbstraction)
			- RESULT(ReachContaminantDegradation)
			- RESULT(DiffusiveAirReachExchangeFlux)
			+ (RESULT(ReachContaminantEntrainment) - RESULT(ReachContaminantDeposition)) * PARAMETER(ReachLength)*PARAMETER(ReachWidth);
			- RESULT(DiffusiveSedimentReachExchangeFlux);
	)
	
	
	EQUATION(Model, WaterTemperatureKelvin,
		return RESULT(WaterTemperature) + 273.15;
	)
	
	EQUATION(Model, ReachHenrysConstant,
		double LogHT = TemperatureAdjustLogKH(std::log10(PARAMETER(HenrysConstant25)), RESULT(WaterTemperatureKelvin), PARAMETER(AirWaterPhaseTransferEnthalpy));
		return std::pow(10.0, LogHT);
	)
	
	EQUATION(Model, ReachAirWaterPartitioningCoefficient,
		double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
		return RESULT(ReachHenrysConstant) / (R * RESULT(WaterTemperatureKelvin));
	)
	
	EQUATION(Model, ReachOctanolWaterPartitioningCoefficient,
		double LogKOWT = TemperatureAdjustLogKow(PARAMETER(Log10OctanolWaterPartitioningCoefficient25), RESULT(WaterTemperatureKelvin), PARAMETER(OctanolWaterPhaseTransferEnthalpy));
		return std::pow(10.0, LogKOWT);
	)
	
	EQUATION(Model, ReachWaterSOCPartitioningCoefficient,
		double densityofSOC = 1900.0; // kg/m^3
		double rOC = 0.41;   // Empirical constant.
		return RESULT(ReachOctanolWaterPartitioningCoefficient) * rOC / densityofSOC;
	)
	
	EQUATION(Model, ReachWaterDOCPartitioningCoefficient,
		double densityofDOC = 1100.0; // kg/m^3
		return std::pow(10.0, 0.93*std::log10(RESULT(ReachOctanolWaterPartitioningCoefficient)) - 0.45) / densityofDOC;
	)
	
	
	EQUATION(Model, MolecularDiffusivityOfCompoundInAir,
		double C0 = std::cbrt(20.1) + std::cbrt(PARAMETER(ContaminantMolecularVolume));
		double Constant = 1e-7 * std::sqrt(1.0/28.97 + 1.0/PARAMETER(ContaminantMolarMass)) / (C0*C0);
		return Constant * std::pow(RESULT(WaterTemperatureKelvin), 1.75);
	)
	
	EQUATION(Model, MolecularDiffusivityOfWaterVapourInAir,
		double C0 = std::cbrt(20.1) + std::cbrt(18.0);
		double Constant = 1e-7 * std::sqrt(1.0/28.97 + 1.0/18.0) / (C0*C0);
		return Constant * std::pow(RESULT(WaterTemperatureKelvin), 1.75);
	)
	
	EQUATION(Model, ReachKinematicViscosity,
		return 0.00285 * std::exp(-0.027*RESULT(WaterTemperatureKelvin));
	)
	
	EQUATION(Model, ReachDynamicViscosity,
		return 2646.8 * std::exp(-0.0268*RESULT(WaterTemperatureKelvin));
	)
	
	EQUATION(Model, MolecularDiffusivityOfCompoundInWater,
		return 13.26e-9 / (std::pow(RESULT(ReachDynamicViscosity), 1.14) * std::pow(PARAMETER(ContaminantMolecularVolume), 0.589));
	)
	
	EQUATION(Model, ReachAirContaminantTransferVelocity,
		double vH2O = (0.2 * INPUT(WindSpeed) + 0.3)*864.0;
		return vH2O * std::pow( RESULT(MolecularDiffusivityOfCompoundInAir) / RESULT(MolecularDiffusivityOfWaterVapourInAir), 0.67);
	)
	
	EQUATION(Model, SchmidtNumber,
		return RESULT(ReachKinematicViscosity) / RESULT(MolecularDiffusivityOfCompoundInWater);
	)
	
	EQUATION(Model, NonDimensionalRoughnessParameter,
		return PARAMETER(AverageBedGrainDiameter) * RESULT(ReachShearVelocity) / RESULT(ReachKinematicViscosity);
	)
	
	EQUATION(Model, ElementFroudeNumber,
		double earthsurfacegravity = 9.81;
		double heightabovestones = RESULT(ReachDepth) - PARAMETER(HeightOfLargeStones);
		return RESULT(ReachDepth) * SafeDivide(RESULT(ReachVelocity), std::sqrt(earthsurfacegravity * heightabovestones * heightabovestones * heightabovestones));
	)
	
	EQUATION(Model, ReachWaterContaminantTransferVelocity,

		double windspeed = INPUT(WindSpeed);
		double asc = 0.5;
		double vCO2;
		
		int Case = 1;
		
		/*
		double depth = RESULT(ReachDepth);
		double velocity = RESULT(ReachVelocity);
		double shearvelocity = RESULT(ReachShearVelocity);
		double stonedepth = PARAMETER(HeightOfLargeStones);
		*/
		
		double schmidtnumber = RESULT(SchmidtNumber);
		//double froudenumber  = RESULT(ElementFroudeNumber);
		//double roughness     = RESULT(NonDimensionalRoughnessParameter);
		
		//double kinematicviscosity = RESULT(ReachKinematicViscosity);
		//double moleculardiffusitivity = RESULT(MolecularDiffusivityOfCompoundInWater);
		
		//double criticalvelocity = (windspeed < 5.0) ? 0.2*std::cbrt(depth) : 3.0*std::cbrt(depth);	
		
		/*
		if(depth < stonedepth) Case = 5;
		else if(velocity < criticalvelocity) Case = 1;
		else if(roughness < 136.0) Case = 2;
		else if(froudenumber < 1.4) Case = 3;
		else Case = 4;
		*/
		
		if(Case == 1)
		{
			if(windspeed <= 4.2)
			{
				asc = 0.67;
				vCO2 = 0.65e-3;
			}
			else if(windspeed <= 13.0)
				vCO2 = (0.79*windspeed - 2.68)*1e-3;
			else 
				vCO2 = (1.64*windspeed - 13.69)*1e-3;
			
			return std::pow(schmidtnumber/600.0, -asc) * vCO2 * 864.0;
		}
		/*
		else if(Case == 2)
		{
			return (0.161 / std::sqrt(schmidtnumber)) * std::pow(SafeDivide(kinematicviscosity*shearvelocity, depth), 0.25) * 86400.0;
		}
		else if(Case == 3)
		{
			return std::sqrt(SafeDivide(moleculardiffusitivity * velocity, depth)) * 86400.0;
		}
		else if(Case == 4)
		{
			return (shearvelocity / std::sqrt(schmidtnumber)) * (0.0071 + 0.023*froudenumber) * 86400.0;
		}
		else if(Case == 5)
		{
			//NOTE: In this case the exchange flux should be computed differently, not using the transfer velocity. //TODO!
			return 0.0;
		}
		*/
		return 0.0; //Control flow will never reach here..
	)
	
	EQUATION(Model, ReachOverallAirWaterContaminantTransferVelocity,
		return 
		1.0 / 
		(
			  1.0 / RESULT(ReachWaterContaminantTransferVelocity)
			+ 1.0 / (RESULT(ReachAirWaterPartitioningCoefficient) * RESULT(ReachAirContaminantTransferVelocity)) 
		);
	)
	
	EQUATION(Model, DiffusiveAirReachExchangeFlux,
		double atmospheric = IF_INPUT_ELSE_PARAMETER(AtmosphericContaminantConcentrationIn, AtmosphericContaminantConcentration);
		return RESULT(ReachOverallAirWaterContaminantTransferVelocity) * (RESULT(ReachWaterContaminantConcentration) - atmospheric/RESULT(ReachAirWaterPartitioningCoefficient)) * PARAMETER(ReachLength) * PARAMETER(ReachWidth);
	)
	
	
	EQUATION(Model, ReachWaterContaminantConcentration,
		return
			RESULT(ContaminantMassInReach) /
			  (RESULT(ReachWaterDOCPartitioningCoefficient)*RESULT(ReachDOCMass) + RESULT(ReachWaterSOCPartitioningCoefficient)*RESULT(TotalSuspendedSOCMass) + RESULT(ReachVolume));
	)
	
	EQUATION(Model, ReachDOCContaminantConcentration,
		return RESULT(ReachWaterContaminantConcentration) * RESULT(ReachWaterDOCPartitioningCoefficient);
	)
	
	
	EQUATION(Model, ReachSOCContaminantConcentration,
		return RESULT(ReachWaterContaminantConcentration) * RESULT(ReachWaterSOCPartitioningCoefficient);
	)
	
	EQUATION(Model, ReachContaminantDeposition,
		return RESULT(TotalSOCDeposition) * RESULT(ReachSOCContaminantConcentration);
	)
	
	EQUATION(Model, ReachContaminantEntrainment,
		return RESULT(TotalSOCEntrainment) * RESULT(BedSOCContaminantConcentration);
	)
	
	EQUATION(Model, BedContaminantDegradation,
		return HalfLifeToRate(PARAMETER(StreamBedContaminantHalfLife)) * RESULT(BedContaminantMass) * RESULT(ReachDegradationTemperatureModifier);
	)
	
	EQUATION(Model, BedContaminantMass,
		return
		  RESULT(ReachContaminantDeposition)
		- RESULT(ReachContaminantEntrainment)
		- RESULT(BedContaminantDegradation)
		+ RESULT(DiffusiveSedimentReachExchangeFlux) / (PARAMETER(ReachLength)*PARAMETER(ReachWidth));
	)
	
	EQUATION(Model, PoreWaterVolume,
		double sedimentdrydensity = 2650.0;
		//TODO: Also, this should maybe be an output of INCA-Sed instead of being computed by the contaminants module.
		return (RESULT(TotalMassOfBedGrainPerUnitArea) / sedimentdrydensity) * std::pow(PARAMETER(SedimentPorosity), 2.0/3.0);
	)
	
	EQUATION(Model, BedWaterContaminantConcentration,
		double beddocmass = RESULT(ReachDOCMass) * RESULT(PoreWaterVolume) / RESULT(ReachVolume); //Assuming same concentration as in the reach
		
		return SafeDivide(RESULT(BedContaminantMass),
			(RESULT(ReachWaterSOCPartitioningCoefficient)*RESULT(TotalBedSOCMass) + RESULT(ReachWaterDOCPartitioningCoefficient)*beddocmass + RESULT(PoreWaterVolume)));
	)	
	
	EQUATION(Model, BedSOCContaminantConcentration,
		return RESULT(BedWaterContaminantConcentration) * RESULT(ReachWaterSOCPartitioningCoefficient);
	)
	
	
	EQUATION(Model, DiffusivityOfDissolvedCompoundInWater,
		// Empirical formula from Grant, Stewardson, Marusic 2012,
		double constant = 3311311.21483; // Constant is 10^7.2*0.1^0.68. Here 0.1 is the depth of sediment participating in diffusive exchange.
		double sedimentpermeability = 1e-10;   // m^2
		return constant * pow(RESULT(ReachVelocity), 0.83) * pow(RESULT(ReachShearVelocity), 1.4) * pow(sedimentpermeability, 0.91) * pow(RESULT(ReachDepth), 0.68) * pow(PARAMETER(SedimentPorosity), 1.5);
	)
	
	EQUATION(Model, DiffusiveSedimentReachExchangeFlux,
		double z = 0.1; // Thickness of sediment layer that participates in diffusive exchange.
		return RESULT(DiffusivityOfDissolvedCompoundInWater) * (RESULT(ReachWaterContaminantConcentration) - RESULT(BedWaterContaminantConcentration)) * PARAMETER(ReachLength) * PARAMETER(ReachWidth) * 86400.0 / z;
	)
	
	auto GrainSOCDensity = GetParameterDoubleHandle(Model, "Grain SOC density");
	auto ReachDOCConcentration = GetEquationHandle(Model, "Reach DOC concentration");
	
	auto ReachWaterContaminantConcentrationNgL = RegisterEquation(Model, "Reach water contaminant concentration ng/l", NgPerL);
	auto ReachSPMContaminantConcentration = RegisterEquation(Model, "Reach suspended particulate matter contaminant concentration", NgPerKg);
	auto ApparentDissolvedConcentration   = RegisterEquation(Model, "Reach apparent dissolved contaminant concentration (water + DOC)", NgPerL);
	
	EQUATION(Model, ReachWaterContaminantConcentrationNgL,
		return 1e-3 * RESULT(ReachWaterContaminantConcentration);
	)
	
	EQUATION(Model, ReachSPMContaminantConcentration,
		auto conc = RESULT(ReachSOCContaminantConcentration);  //ng(contaminant) / kg(SOC)
		auto dens = PARAMETER(GrainSOCDensity);                //kg(SOC) / kg(suspended_solid)
		return conc * dens;
	)
	
	EQUATION(Model, ApparentDissolvedConcentration,
		return 
			  RESULT(ReachWaterContaminantConcentrationNgL) 
			+ RESULT(ReachDOCContaminantConcentration)*RESULT(ReachDOCConcentration)*1e-6; // kg(DOC)/L * ng(contaminant)/kg(DOC) -> ng(contaminant)/L
	)

	EndModule(Model);
}