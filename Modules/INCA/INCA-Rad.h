


//#include "../boost_solvers.h"

static void
AddIncaRadModule(mobius_model *Model)
{
	BeginModule(Model, "INCA-Rad", "0.1");
	
	SetModuleDescription(Model, R""""(
Inca-Rad, is a small modification of INCA-Tox (INCA-Contaminants) that makes it suitable to describe radionuclides.

[Nizzetto, L., Butterfield, D., Futter, M., Lin, Y., Allan, I., Larssen, T., 2016. Assessment of contaminant fate in catchments using a novel integrated hydrobiogeochemical-multimedia fate model. Science of the Total Environment 544, 553-563.](https://doi.org/10.1016/j.scitotenv.2015.11.087)

[INCA-Contaminants home page](https://www.niva.no/en/publications/inca-contaminants)
)"""");


	auto Dimensionless    = RegisterUnit(Model);
	auto Bq               = RegisterUnit(Model, "Bq");
	auto M                = RegisterUnit(Model, "m");
	auto M3               = RegisterUnit(Model, "m3");
	auto M3PerM2          = RegisterUnit(Model, "m3/m2");
	auto BqPerKm2         = RegisterUnit(Model, "Bq/km2");
	auto BqPerKm2PerDay   = RegisterUnit(Model, "Bq/km2/day");
	auto BqPerDay         = RegisterUnit(Model, "Bq/day");
	auto BqPerM2PerDay    = RegisterUnit(Model, "Bq/m2/day");
	auto PerM2PerDay      = RegisterUnit(Model, "1/m2/day");
	auto BqPerM2          = RegisterUnit(Model, "Bq/m2");
	auto BqPerM3          = RegisterUnit(Model, "Bq/m3");
	auto BqPerKg          = RegisterUnit(Model, "Bq/kg");
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
	auto Years            = RegisterUnit(Model, "years");
	auto PerDay           = RegisterUnit(Model, "1/day");
	auto GPerMol          = RegisterUnit(Model, "g/mol");
	auto Cm3PerMol        = RegisterUnit(Model, "cm3/mol");
	auto GPerCmPerSPerHundred = RegisterUnit(Model, "10^-2 g/cm/s");
	
	auto DepositionToLand         = RegisterInput(Model, "Contaminant deposition to land", BqPerM2PerDay);
	auto DepositionToReach        = RegisterInput(Model, "Contaminant deposition to reach", BqPerDay);
	auto WindSpeed                = RegisterInput(Model, "Wind speed", MPerS);
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto Soils          = GetIndexSetHandle(Model, "Soils");
	auto Reach          = GetIndexSetHandle(Model, "Reaches");
	auto Class          = GetIndexSetHandle(Model, "Sediment size class");
	auto DirectRunoff = RequireIndex(Model, Soils, "Direct runoff");
	auto Soilwater    = RequireIndex(Model, Soils, "Soil water");
	auto Groundwater  = RequireIndex(Model, Soils, "Groundwater");
	
	auto Radionuclide = RegisterIndexSetBranched(Model, "Radionuclide");
	
	auto Chemistry = RegisterParameterGroup(Model, "Physio-chemistry", Radionuclide);
	auto Land      = RegisterParameterGroup(Model, "Contaminants by land class", LandscapeUnits, Radionuclide);

	//TODO: As always, find better default, min, max values for parameters!
	
	auto ContaminantMolarMass                  = RegisterParameterDouble(Model, Chemistry, "Contaminant molar mass", GPerMol, 50.0, 0.0, 1000.0);
	auto ContaminantMolecularVolume            = RegisterParameterDouble(Model, Chemistry, "Contaminant molecular volume at surface pressure", Cm3PerMol, 20.0, 0.0, 1000.0);
	auto AirWaterPhaseTransferEnthalpy         = RegisterParameterDouble(Model, Chemistry, "Enthalpy of phase transfer between air and water", KiloJoulesPerMol, 0.0);
	//auto OctanolWaterPhaseTransferEntalphy     = RegisterParameterDouble(Model, Chemistry, "Enthalpy of phase tranfer between octanol and water", KiloJoulesPerMol, 0.0);
	auto OCWaterPhaseTransferEntalphy          = RegisterParameterDouble(Model, Chemistry, "Enthalpy of phase tranfer between organic carbon and water", KiloJoulesPerMol, 0.0);
	auto HenrysConstant25                      = RegisterParameterDouble(Model, Chemistry, "Henry's constant at 25°C", PascalM3PerMol, 0.0);
	//auto OctanolWaterPartitioningCoefficient25 = RegisterParameterDouble(Model, Chemistry, "Octanol-water partitioning coefficient at 25°C", Dimensionless, 0.0);
	auto OCWaterPartitioningCoefficient25      = RegisterParameterDouble(Model, Chemistry, "Organic carbon-water partitioning coefficient at 25°C", M3PerKg, 0.0);
	auto WaterDOCPartitioningCoefficient       = RegisterParameterDouble(Model, Chemistry, "DOC-water partitioning coefficient", M3PerKg, 0.0);
	
	auto AtmosphericContaminantConcentration   = RegisterParameterDouble(Model, Chemistry, "Atmospheric contaminant concentration", BqPerM3, 0.0, 0.0, 100.0);
	auto HalfLife                              = RegisterParameterDouble(Model, Chemistry, "Element half life", Years, 1.0, 7.29e-31, 2.2e24);
	
	auto AirSoilOverallMassTransferCoefficient = RegisterParameterDouble(Model, Land, "Overall air-soil mass transfer coefficient", MPerDay, 0.0, 0.0, 100.0);
	auto TransferCoefficientBetweenEasilyAndPotentiallyAccessible = RegisterParameterDouble(Model, Land, "Transfer coefficient between easily and potentially accessible fractions", PerM2PerDay, 0.0, 0.0, 100.0);

	
	auto InitialSoilWaterContaminantConcentration = RegisterParameterDouble(Model, Land, "Initial soil water contaminant concentration", BqPerM3, 0.0, 0.0, 1e5);
	auto InitialSoilSOCContaminantConcentration = RegisterParameterDouble(Model, Land, "Initial soil SOC contaminant concentration", BqPerKg, 0.0, 0.0, 1e5);

	auto ContaminantReach = RegisterParameterGroup(Model, "Contaminants by reach", Reach, Radionuclide);
	auto OtherReachParams = RegisterParameterGroup(Model, "Reach parameters", Reach);

	auto InitialContaminantMassInGroundwater = RegisterParameterDouble(Model, ContaminantReach, "Initial contaminant mass in groundwater", BqPerKm2, 0.0, 0.0, 1e3);
	auto InitialContaminantMassInReach = RegisterParameterDouble(Model, ContaminantReach, "Initial contaminant mass in reach", Bq, 0.0, 0.0, 1e3);
	
	auto HeightOfLargeStones = RegisterParameterDouble(Model, OtherReachParams, "Average height of large stones in the stream bed", M, 0.0, 0.0, 0.5);
	auto AverageBedGrainDiameter = RegisterParameterDouble(Model, OtherReachParams, "Average bed grain diameter", M, 0.0001, 0.0, 0.1);
	//auto SedimentDryDensity = RegisterParameterDouble(Model, OtherReachParams, "Sediment dry density", KgPerM3, 2000.0, 0.0, 10000.0);
	auto SedimentPorosity   = RegisterParameterDouble(Model, OtherReachParams, "Sediment porosity", Dimensionless, 0.1, 0.0, 0.99);
	
	
	 //SoilTemperature.h
	auto SoilTemperature = GetEquationHandle(Model, "Soil temperature");
	
	 //WaterTemperature.h
	auto WaterTemperature = GetEquationHandle(Model, "Water temperature");
	
	 //PERSiST.h
	auto WaterDepth      = GetEquationHandle(Model, "Water depth");
	auto RunoffToReach   = GetEquationHandle(Model, "Runoff to reach");
	auto PercolationInput= GetEquationHandle(Model, "Percolation input");
	auto ReachFlow       = GetEquationHandle(Model, "Reach flow");
	auto ReachVolume     = GetEquationHandle(Model, "Reach volume");
	auto ReachVelocity   = GetEquationHandle(Model, "Reach velocity");
	auto ReachDepth      = GetEquationHandle(Model, "Reach depth");
	
	//INCA-Sed.h
	auto TotalMassOfBedGrainPerUnitArea = GetEquationHandle(Model, "Total mass of bed sediment per unit area");
	auto ReachShearVelocity             = GetEquationHandle(Model, "Reach shear velocity");
	
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
	
	//constexpr double Avogadro   = 6.02214076e23;
	
	auto SoilTemperatureKelvin = RegisterEquation(Model, "Soil temperature in Kelvin", K);
	
	EQUATION(Model, SoilTemperatureKelvin,
		return 273.15 + RESULT(SoilTemperature);
	)
	
	
	auto SoilSolver = RegisterSolver(Model, "Soil Solver", 0.1, IncaDascru);
	
	
	auto HenrysConstant = RegisterEquation(Model, "Henry's constant", PascalM3PerMol);
	
	auto AirWaterPartitioningCoefficient     = RegisterEquation(Model, "Air-water partitioning coefficient", Dimensionless);
	auto WaterSOCPartitioningCoefficient     = RegisterEquation(Model, "Water-SOC partitioning coefficient", M3PerKg);
	
	auto ContaminantDeliveryToReachByErosion = RegisterEquation(Model, "Contaminant delivery to reach by erosion", BqPerDay, SoilSolver);
	
	auto TotalContaminantDeliveryToReach     = RegisterEquationCumulative(Model, "Contaminant delivery to reach by erosion summed over land classes", ContaminantDeliveryToReachByErosion, LandscapeUnits);
	
	auto SoilWaterContaminantConcentration = RegisterEquation(Model, "Soil water contaminant concentration", BqPerM3, SoilSolver);
	auto SoilSOCContaminantConcentration   = RegisterEquation(Model, "Soil SOC contaminant concentration (easily accessible fraction)", BqPerKg, SoilSolver);
	auto SoilSOCContaminantConcentrationPotentiallyAccessible = RegisterEquation(Model, "Soil SOC contaminant concentration (potentially accessible fraction)", BqPerKg, SoilSolver);
	auto SoilDOCContaminantConcentration   = RegisterEquation(Model, "Soil DOC contaminant concentration", BqPerKg, SoilSolver);
	auto SoilAirContaminantConcentration   = RegisterEquation(Model, "Soil Air contaminant concentration", BqPerM3, SoilSolver);
	auto DiffusiveAirSoilExchangeFlux      = RegisterEquation(Model, "Diffusive exchange of contaminants between soil and atmosphere", BqPerKm2PerDay, SoilSolver);
	auto SoilContaminantDegradation        = RegisterEquation(Model, "Soil contaminant degradation (easily accessible fraction)", BqPerKm2PerDay, SoilSolver);
	auto SoilContaminantDegradationPotentiallyAccessible = RegisterEquation(Model, "Soil contaminant degradation (potentially accessible fraction)", BqPerKm2PerDay, SoilSolver);
	
	auto SoilContaminantFormation          = RegisterEquation(Model, "Soil contaminant formation (easily accessible fraction)", BqPerKm2PerDay);
	auto SoilContaminantFormationPotentiallyAccessible = RegisterEquation(Model, "Soil contaminant formation (potentially accessible fraction)", BqPerKm2PerDay);
	
	auto SoilWaterVolume                   = RegisterEquation(Model, "Soil water volume", M3PerKm2);
	auto SoilAirVolume                     = RegisterEquation(Model, "Soil air volume", M3PerKm2);
	
	
	auto ContaminantInputsToSoil           = RegisterEquation(Model, "Contaminant inputs to soil", BqPerKm2PerDay);
	
	auto InitialContaminantMassInSoil  = RegisterEquationInitialValue(Model, "Initial contaminant mass in soil (easily accessible fraction)", BqPerKm2);
	auto InitialContaminantMassInPotentiallyAccessibleFraction = RegisterEquationInitialValue(Model, "Initial contaminant mass in soil (potentially accessible fraction)", BqPerKm2);
	
	auto ContaminantMassInSoil             = RegisterEquationODE(Model, "Contaminant mass in soil (easily accessible fraction)", BqPerKm2, SoilSolver);
	SetInitialValue(Model, ContaminantMassInSoil, InitialContaminantMassInSoil);
	
	auto ContaminantMassInPotentiallyAccessibleFraction = RegisterEquationODE(Model, "Contaminant mass in soil (potentially accessible fraction)", BqPerKm2, SoilSolver);
	SetInitialValue(Model, ContaminantMassInPotentiallyAccessibleFraction, InitialContaminantMassInPotentiallyAccessibleFraction);
	auto TransferFromPotentiallyToEasilyAccessible = RegisterEquation(Model, "Contaminant transfer from potentially to easily accessible fraction", BqPerKm2PerDay, SoilSolver);
	
	
	auto SoilContaminantFluxToReach        = RegisterEquation(Model, "Soil contaminant flux to reach", BqPerKm2PerDay, SoilSolver);
	auto SoilContaminantFluxToGroundwater  = RegisterEquation(Model, "Soil contaminant flux to groundwater", BqPerKm2PerDay, SoilSolver);
	
	auto GroundwaterContaminantDegradation = RegisterEquation(Model, "Groundwater contaminant degradation", BqPerKm2PerDay, SoilSolver);
	auto GroundwaterContaminantFluxToReach = RegisterEquation(Model, "Groundwater contaminant flux to reach", BqPerKm2PerDay, SoilSolver);
	auto ContaminantMassInGroundwater      = RegisterEquationODE(Model, "Contaminant mass in groundwater", BqPerKm2, SoilSolver);
	SetInitialValue(Model, ContaminantMassInGroundwater, InitialContaminantMassInGroundwater);
	
	auto GroundwaterContaminantFormation   = RegisterEquation(Model, "Groundwater contaminant formation", BqPerKm2PerDay);
	
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
	
	
	EQUATION(Model, HenrysConstant,
		double LogH25 = std::log10(PARAMETER(HenrysConstant25));
		double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
		double tdiff = (1.0 / RESULT(SoilTemperatureKelvin) - 1.0/(273.15 + 25.0));
		double LogHT = LogH25 - ((1e3 * PARAMETER(AirWaterPhaseTransferEnthalpy) + R*(273.15 + 25.0)) / (std::log(10.0)*R) )  * tdiff;
		return std::pow(10.0, LogHT);
	)
	
	EQUATION(Model, AirWaterPartitioningCoefficient,
		double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
		return RESULT(HenrysConstant) / (R * RESULT(SoilTemperatureKelvin));
	)
	
	EQUATION(Model, WaterSOCPartitioningCoefficient,
		double LogKOC25 = std::log10(PARAMETER(OCWaterPartitioningCoefficient25));
		double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
		double tdiff = (1.0 / RESULT(SoilTemperatureKelvin) - 1.0/(273.15 + 25.0));
		double LogKOCT = LogKOC25 - (1e3*PARAMETER(OCWaterPhaseTransferEntalphy) / (std::log(10.0)*R))   * tdiff;
		return std::pow(10.0, LogKOCT);
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
			+ PARAMETER(WaterDOCPartitioningCoefficient) * RESULT(SoilDOCMass)
			+ RESULT(AirWaterPartitioningCoefficient) * RESULT(SoilAirVolume)
			+ RESULT(SoilWaterVolume)
			);
	)
	
	EQUATION(Model, SoilSOCContaminantConcentration,
		return RESULT(SoilWaterContaminantConcentration) * RESULT(WaterSOCPartitioningCoefficient);
	)
	
	EQUATION(Model, SoilDOCContaminantConcentration,
		return RESULT(SoilWaterContaminantConcentration) * PARAMETER(WaterDOCPartitioningCoefficient);
	)
	
	EQUATION(Model, SoilAirContaminantConcentration,
		return RESULT(SoilWaterContaminantConcentration) * RESULT(AirWaterPartitioningCoefficient);
	)
	
	EQUATION(Model, ContaminantMassInPotentiallyAccessibleFraction,
		return
			  - RESULT(TransferFromPotentiallyToEasilyAccessible)
			  - RESULT(SoilContaminantDegradationPotentiallyAccessible)
			  + RESULT(SoilContaminantFormationPotentiallyAccessible);
	)
	
	EQUATION(Model, TransferFromPotentiallyToEasilyAccessible,
		//NOTE: conc is in bq(contaminant) / kg(SOC)
		//We give a transfer coefficient that is independent of the SOC mass so that the behaviour is not too correlated to the SOC mass.
		return 1e6 * PARAMETER(TransferCoefficientBetweenEasilyAndPotentiallyAccessible) * PARAMETER(SoilSOCMass) * (RESULT(SoilSOCContaminantConcentrationPotentiallyAccessible) - RESULT(SoilSOCContaminantConcentration));
	)
	
	EQUATION(Model, SoilSOCContaminantConcentrationPotentiallyAccessible,
		double SOCmass = PARAMETER(SoilSOCMass)*(1.0 - PARAMETER(SizeOfEasilyAccessibleFraction))*1e6;
		return SafeDivide(RESULT(ContaminantMassInPotentiallyAccessibleFraction), SOCmass);
	)
	
	EQUATION(Model, SoilContaminantDegradationPotentiallyAccessible,
		return HalfLifeToRate(365.0 * PARAMETER(HalfLife)) * RESULT(ContaminantMassInPotentiallyAccessibleFraction);
	)
	
	EQUATION(Model, SoilContaminantFormationPotentiallyAccessible,
		double formation = 0.0;
		
		double halflife = PARAMETER(HalfLife);
		double molarmass = PARAMETER(ContaminantMolarMass);
		
		for(index_t Input : BRANCH_INPUTS(Radionuclide))
		{
			double degradation_other_bq = RESULT(SoilContaminantDegradationPotentiallyAccessible, Input);
			double degradation_other_mass = degradation_other_bq*PARAMETER(ContaminantMolarMass, Input)*PARAMETER(HalfLife, Input);
			formation += degradation_other_mass / (halflife*molarmass);
		}
		
		return formation;
	)
	
	EQUATION(Model, DiffusiveAirSoilExchangeFlux,
		return 1e6 * PARAMETER(AirSoilOverallMassTransferCoefficient) * (1e-3*PARAMETER(AtmosphericContaminantConcentration) - RESULT(SoilAirContaminantConcentration));
	)
	
	EQUATION(Model, ContaminantInputsToSoil,
		//TODO: Maybe let the user parametrize this in case they don't have detailed timeseries
		//NOTE: convert ng/m2 -> ng/km2
		return INPUT(DepositionToLand) * 1e6;
	)
	
	EQUATION(Model, ContaminantDeliveryToReachByErosion,
		return RESULT(SoilSOCContaminantConcentration) * RESULT(SOCDeliveryToReachPerLU);
	)
	
	EQUATION(Model, SoilContaminantFluxToReach,
		double water_runoff = RESULT(RunoffToReach, Soilwater) + RESULT(RunoffToReach, DirectRunoff);
		return
			water_runoff * RESULT(SoilWaterContaminantConcentration) * 1000.0 // Convert km^2 mm/day * Bq/m^3 to Bq/day
		  + RESULT(SoilDOCFluxToReach) * RESULT(SoilDOCContaminantConcentration);
	)
	
	EQUATION(Model, SoilContaminantFluxToGroundwater,
		return
			RESULT(PercolationInput, Groundwater) * RESULT(SoilWaterContaminantConcentration) * 1000.0 // Convert km^2 mm/day * Bq/m^3 to Bq/day
		  + RESULT(SoilDOCFluxToGroundwater) * RESULT(SoilDOCContaminantConcentration);
	)
	
	EQUATION(Model, SoilContaminantDegradation,
		double degradablemass =
		  RESULT(SoilWaterVolume) * RESULT(SoilWaterContaminantConcentration)
		+ RESULT(SoilDOCMass)     * RESULT(SoilDOCContaminantConcentration)
		+ PARAMETER(SoilSOCMass)*PARAMETER(SizeOfEasilyAccessibleFraction)*1e6  * RESULT(SoilSOCContaminantConcentration);
		
		return HalfLifeToRate(365.0 * PARAMETER(HalfLife)) * degradablemass;
	)
	
	EQUATION(Model, SoilContaminantFormation,
		double formation = 0.0;
		
		double halflife = PARAMETER(HalfLife);
		double molarmass = PARAMETER(ContaminantMolarMass);
		
		for(index_t Input : BRANCH_INPUTS(Radionuclide))
		{
			double degradation_other_bq = RESULT(SoilContaminantDegradation, Input);
			double degradation_other_mass = degradation_other_bq*PARAMETER(ContaminantMolarMass, Input)*PARAMETER(HalfLife, Input);
			formation += degradation_other_mass / (halflife*molarmass);
		}
		
		return formation;
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
		+ RESULT(SoilContaminantFormation);
	)
	
	EQUATION(Model, GroundwaterContaminantFluxToReach,
		//TODO: Do we need to care about partitioning here? Probably only if we ever need to track what enters the reach through DOC and water separately. But not in this initial simple version.
		return RESULT(RunoffToReach, Groundwater) * SafeDivide(RESULT(ContaminantMassInGroundwater), RESULT(WaterDepth, Groundwater));
	)
	
	EQUATION(Model, GroundwaterContaminantDegradation,
		double degradablemass = RESULT(ContaminantMassInGroundwater);
		return HalfLifeToRate(365.0 * PARAMETER(HalfLife)) * degradablemass;
	)
	
	EQUATION(Model, GroundwaterContaminantFormation,
		double formation = 0.0;
		
		double halflife = PARAMETER(HalfLife);
		double molarmass = PARAMETER(ContaminantMolarMass);
		
		for(index_t Input : BRANCH_INPUTS(Radionuclide))
		{
			double degradation_other_bq = RESULT(GroundwaterContaminantDegradation, Input);
			double degradation_other_mass = degradation_other_bq*PARAMETER(ContaminantMolarMass, Input)*PARAMETER(HalfLife, Input);
			formation += degradation_other_mass / (halflife*molarmass);
		}
		
		return formation;
	)
	
	EQUATION(Model, ContaminantMassInGroundwater,
		return
		  RESULT(SoilContaminantFluxToGroundwater)
		- RESULT(GroundwaterContaminantFluxToReach)
		- RESULT(GroundwaterContaminantDegradation)
		+ RESULT(GroundwaterContaminantFormation);
	)

	//auto ReachSolver = RegisterSolver(Model, "Reach contaminant solver", 0.1, IncaDascru); //NOTE: We can't use the reach solver from PERSiST, because we depend on the grain solver that again depends on the PERSiST reach solver.
	auto ReachSolver = RegisterSolver(Model, "Reach contaminant solver", 0.1, BoostRosenbrock4, 1e-3, 1e-3);
	
	auto WaterTemperatureKelvin        = RegisterEquation(Model, "Water temperature in Kelvin", K);
	
	auto DiffuseContaminantOutput      = RegisterEquation(Model, "Diffuse contaminant output", BqPerDay);
	auto TotalDiffuseContaminantOutput = RegisterEquationCumulative(Model, "Total diffuse contaminant output", DiffuseContaminantOutput, LandscapeUnits);
	auto ReachContaminantInput         = RegisterEquation(Model, "Reach contaminant input", BqPerDay);
	auto ContaminantMassInReach        = RegisterEquationODE(Model, "Contaminant mass in reach", Bq, ReachSolver);
	SetInitialValue(Model, ContaminantMassInReach, InitialContaminantMassInReach);
	auto ReachContaminantDissolvedFlux = RegisterEquation(Model, "Reach flux of dissolved contaminants", BqPerDay, ReachSolver);
	auto ReachContaminantSolidFlux     = RegisterEquation(Model, "Reach flux of solid-bound contaminants", BqPerDay, ReachSolver);
	auto ReachContaminantFlux          = RegisterEquation(Model, "Reach contaminant flux", BqPerDay, ReachSolver);
	auto ReachContaminantDegradation   = RegisterEquation(Model, "Reach contaminant degradation", BqPerDay, ReachSolver);
	
	auto ReachContaminantFormation     = RegisterEquation(Model, "Reach contaminant formation", BqPerDay);
	
	auto ReachHenrysConstant                      = RegisterEquation(Model, "Reach Henry's constant", PascalM3PerMol);
	auto ReachAirWaterPartitioningCoefficient     = RegisterEquation(Model, "Reach air-water partitioning coefficient", Dimensionless);
	auto ReachWaterSOCPartitioningCoefficient     = RegisterEquation(Model, "Reach water-SOC partitioning coefficient", M3PerKg);
	
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
	auto DiffusiveAirReachExchangeFlux            = RegisterEquation(Model, "Diffusive air reach exchange flux", BqPerDay, ReachSolver);
	
	auto ReachWaterContaminantConcentration = RegisterEquation(Model, "Reach water contaminant concentration", BqPerM3, ReachSolver);
	auto ReachDOCContaminantConcentration   = RegisterEquation(Model, "Reach DOC contaminant concentration", BqPerKg, ReachSolver);
	auto ReachSOCContaminantConcentration = RegisterEquation(Model, "Reach SOC contaminant concentration", BqPerKg, ReachSolver);
	
	auto ReachContaminantDeposition = RegisterEquation(Model, "Reach contaminant deposition", BqPerM2PerDay, ReachSolver);
	auto ReachContaminantEntrainment = RegisterEquation(Model, "Reach contaminant entrainment", BqPerM2PerDay, ReachSolver);
	
	auto PoreWaterVolume = RegisterEquation(Model, "Pore water volume", M3PerM2);
	
	auto BedContaminantDegradation        = RegisterEquation(Model, "Stream bed contaminant degradation", BqPerM2PerDay, ReachSolver);
	auto BedContaminantMass               = RegisterEquationODE(Model, "Stream bed contaminant mass", BqPerM2, ReachSolver);
	//SetInitialValue

	auto BedWaterContaminantConcentration = RegisterEquation(Model, "Stream bed pore water contaminant concentration", BqPerM3, ReachSolver);
	auto BedSOCContaminantConcentration   = RegisterEquation(Model, "Stream bed SOC contaminant concentration", BqPerKg, ReachSolver);
	
	auto DiffusiveSedimentReachExchangeFlux = RegisterEquation(Model, "Diffusive sediment reach water exchange flux", BqPerDay, ReachSolver);
	
	auto BedContaminantFormation          = RegisterEquation(Model, "Stream bed contaminant formation", BqPerM2PerDay);
	
	
	EQUATION(Model, DiffuseContaminantOutput,
		return
		( RESULT(SoilContaminantFluxToReach)
		+ RESULT(GroundwaterContaminantFluxToReach)
		)
		* PARAMETER(CatchmentArea) * PARAMETER(Percent)*0.01;
	)
	
	EQUATION(Model, ReachContaminantInput,
		double upstreamflux = 0.0;
		for(index_t Input : BRANCH_INPUTS(Reach))
			upstreamflux += RESULT(ReachContaminantFlux, Input);
	
		return
			upstreamflux
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
		return RESULT(ContaminantMassInReach) * HalfLifeToRate(365.0 * PARAMETER(HalfLife));
	)
	
	EQUATION(Model, ReachContaminantFormation,
		double formation = 0.0;
		
		double halflife = PARAMETER(HalfLife);
		double molarmass = PARAMETER(ContaminantMolarMass);
		
		for(index_t Input : BRANCH_INPUTS(Radionuclide))
		{
			double degradation_other_bq = RESULT(ReachContaminantDegradation, Input);
			double degradation_other_mass = degradation_other_bq*PARAMETER(ContaminantMolarMass, Input)*PARAMETER(HalfLife, Input);
			formation += degradation_other_mass / (halflife*molarmass);
		}
		
		return formation;
	)
	
	
	EQUATION(Model, ContaminantMassInReach,
		return
			  RESULT(ReachContaminantInput)
			- RESULT(ReachContaminantFlux)
			- RESULT(ReachContaminantDegradation)
			- RESULT(DiffusiveAirReachExchangeFlux)
			+ (RESULT(ReachContaminantEntrainment) - RESULT(ReachContaminantDeposition)) * PARAMETER(ReachLength)*PARAMETER(ReachWidth);
			- RESULT(DiffusiveSedimentReachExchangeFlux)
			+ RESULT(ReachContaminantFormation);
	)
	
	
	EQUATION(Model, WaterTemperatureKelvin,
		return RESULT(WaterTemperature) + 273.15;
	)
	
	EQUATION(Model, ReachHenrysConstant,
		double LogH25 = std::log10(PARAMETER(HenrysConstant25));
		double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
		double tdiff = (1.0 / RESULT(WaterTemperatureKelvin) - 1.0/(273.15 + 25.0));
		double LogHT = LogH25 - ((1e3 * PARAMETER(AirWaterPhaseTransferEnthalpy) + R*(273.15 + 25.0)) / (std::log(10.0)*R) )  * tdiff;
		return std::pow(10.0, LogHT);
	)
	
	EQUATION(Model, ReachAirWaterPartitioningCoefficient,
		double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
		return RESULT(ReachHenrysConstant) / (R * RESULT(WaterTemperatureKelvin));
	)
	
	EQUATION(Model, ReachWaterSOCPartitioningCoefficient,
		double LogKOC25 = std::log10(PARAMETER(OCWaterPartitioningCoefficient25));
		double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
		double tdiff = (1.0 / RESULT(WaterTemperatureKelvin) - 1.0/(273.15 + 25.0));
		double LogKOCT = LogKOC25 - (1e3*PARAMETER(OCWaterPhaseTransferEntalphy) / (std::log(10.0)*R))   * tdiff;
		return std::pow(10.0, LogKOCT);
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
		
		double depth = RESULT(ReachDepth);
		double velocity = RESULT(ReachVelocity);
		double shearvelocity = RESULT(ReachShearVelocity);
		double stonedepth = PARAMETER(HeightOfLargeStones);
		
		double schmidtnumber = RESULT(SchmidtNumber);
		double froudenumber  = RESULT(ElementFroudeNumber);
		double roughness     = RESULT(NonDimensionalRoughnessParameter);
		
		double kinematicviscosity = RESULT(ReachKinematicViscosity);
		double moleculardiffusitivity = RESULT(MolecularDiffusivityOfCompoundInWater);
		
		double criticalvelocity = (windspeed < 5.0) ? 0.2*std::cbrt(depth) : 3.0*std::cbrt(depth);	
		
		if(depth < stonedepth) Case = 5;
		else if(velocity < criticalvelocity) Case = 1;
		else if(roughness < 136.0) Case = 2;
		else if(froudenumber < 1.4) Case = 3;
		else Case = 4;
	
		
		if(Case == 1)
		{
			//TODO: Separate equations for these for easier debugging?
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
		return RESULT(ReachOverallAirWaterContaminantTransferVelocity) * (RESULT(ReachWaterContaminantConcentration) - SafeDivide(PARAMETER(AtmosphericContaminantConcentration), RESULT(ReachAirWaterPartitioningCoefficient))) * PARAMETER(ReachLength) * PARAMETER(ReachWidth);
	)
	
	
	EQUATION(Model, ReachWaterContaminantConcentration,
		return
			RESULT(ContaminantMassInReach) /
			  (PARAMETER(WaterDOCPartitioningCoefficient)*RESULT(ReachDOCMass) + RESULT(ReachWaterSOCPartitioningCoefficient)*RESULT(TotalSuspendedSOCMass) + RESULT(ReachVolume));
	)
	
	EQUATION(Model, ReachDOCContaminantConcentration,
		return RESULT(ReachWaterContaminantConcentration) * PARAMETER(WaterDOCPartitioningCoefficient);
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
		return RESULT(BedContaminantMass) * HalfLifeToRate(365.0 * PARAMETER(HalfLife));
	)
	
	EQUATION(Model, BedContaminantFormation,
		double formation = 0.0;
		
		double halflife = PARAMETER(HalfLife);
		double molarmass = PARAMETER(ContaminantMolarMass);
		
		for(index_t Input : BRANCH_INPUTS(Radionuclide))
		{
			double degradation_other_bq = RESULT(BedContaminantDegradation, Input);
			double degradation_other_mass = degradation_other_bq*PARAMETER(ContaminantMolarMass, Input)*PARAMETER(HalfLife, Input);
			formation += degradation_other_mass / (halflife*molarmass);
		}
		
		return formation;
	)
	
	EQUATION(Model, BedContaminantMass,
		return
		  RESULT(ReachContaminantDeposition)
		- RESULT(ReachContaminantEntrainment)
		- RESULT(BedContaminantDegradation)
		+ RESULT(DiffusiveSedimentReachExchangeFlux) / (PARAMETER(ReachLength)*PARAMETER(ReachWidth))
		+ RESULT(BedContaminantFormation);
	)
	
	EQUATION(Model, PoreWaterVolume,
		double sedimentdrydensity = 2650.0;
		//TODO: Also, this should maybe be an output of INCA-Sed instead of being computed by the contaminants module.
		return (RESULT(TotalMassOfBedGrainPerUnitArea) / sedimentdrydensity) * std::pow(PARAMETER(SedimentPorosity), 2.0/3.0);
	)
	
	EQUATION(Model, BedWaterContaminantConcentration,
		double beddocmass = RESULT(ReachDOCMass) * RESULT(PoreWaterVolume) / RESULT(ReachVolume); //Assuming same concentration as in the reach
		
		return RESULT(BedContaminantMass) /
			(RESULT(ReachWaterSOCPartitioningCoefficient)*RESULT(TotalBedSOCMass) + PARAMETER(WaterDOCPartitioningCoefficient)*beddocmass + RESULT(PoreWaterVolume));
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
	auto ReachSPMContaminantConcentration = RegisterEquation(Model, "Reach suspended particulate matter contaminant concentration", BqPerKg);
	
	EQUATION(Model, ReachSPMContaminantConcentration,
		auto conc = RESULT(ReachSOCContaminantConcentration);  //Bq(contaminant) / kg(SOC)
		auto dens = PARAMETER(GrainSOCDensity);                //kg(SOC) / kg(suspended_solid)
		return conc * dens;
	)

	EndModule(Model);
}