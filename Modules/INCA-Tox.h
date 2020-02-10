


//#include "../boost_solvers.h"

//NOTE: THIS MODULE IS IN DEVELOPMENT!


static void
AddIncaToxModule(mobius_model *Model)
{
	BeginModule(Model, "INCA-Tox", "0.00.0.0.23");
	
	auto Dimensionless    = RegisterUnit(Model);
	auto Ng               = RegisterUnit(Model, "ng");
	auto M                = RegisterUnit(Model, "m");
	auto M3               = RegisterUnit(Model, "m3");
	auto M3PerM2          = RegisterUnit(Model, "m3/m2");
	auto NgPerKm2         = RegisterUnit(Model, "ng/km2");
	auto NgPerKm2PerDay   = RegisterUnit(Model, "ng/km2/day");
	auto NgPerDay         = RegisterUnit(Model, "ng/day");
	auto NgPerM2PerDay    = RegisterUnit(Model, "ng/m2/day");
	auto NgPerM2          = RegisterUnit(Model, "ng/m2");
	auto NgPerM3          = RegisterUnit(Model, "ng/m3");
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
	auto PerDay           = RegisterUnit(Model, "1/day");
	auto GPerMol          = RegisterUnit(Model, "g/mol");
	auto Cm3PerMol        = RegisterUnit(Model, "cm3/mol");
	auto GPerCmPerSPerHundred = RegisterUnit(Model, "10^-2 g/cm/s");
	
	auto AtmosphericDryDeposition = RegisterInput(Model, "Atmospheric dry contaminant deposition", NgPerM2PerDay);
	auto AtmosphericWetDeposition = RegisterInput(Model, "Atmospheric wet contaminant deposition", NgPerM2PerDay);
	auto LandManagementDeposition = RegisterInput(Model, "Land management conatminant deposition", NgPerM2PerDay);
	auto LitterFallDeposition     = RegisterInput(Model, "Litter fall contaminant deposition",     NgPerM2PerDay);
	
	
	auto WindSpeed = RegisterInput(Model, "Wind speed", MPerS);
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto Soils          = GetIndexSetHandle(Model, "Soils");
	auto Reach          = GetIndexSetHandle(Model, "Reaches");
	auto Class          = GetIndexSetHandle(Model, "Sediment size class");
	auto DirectRunoff = RequireIndex(Model, Soils, "Direct runoff");
	auto Soilwater    = RequireIndex(Model, Soils, "Soil water");
	auto Groundwater  = RequireIndex(Model, Soils, "Groundwater");
	
	
	
	auto Chemistry = RegisterParameterGroup(Model, "Chemistry");
	auto Land      = RegisterParameterGroup(Model, "Contaminants by land class", LandscapeUnits);

	//TODO: As always, find better default, min, max values for parameters!
	
	auto ContaminantMolarMass                  = RegisterParameterDouble(Model, Chemistry, "Contaminant molar mass", GPerMol, 50.0, 0.0, 1000.0);
	auto ContaminantMolecularVolume            = RegisterParameterDouble(Model, Chemistry, "Contaminant molecular volume at surface pressure", Cm3PerMol, 20.0, 0.0, 1000.0);
	auto AirWaterPhaseTransferEnthalpy         = RegisterParameterDouble(Model, Chemistry, "Enthalpy of phase transfer between air and water", KiloJoulesPerMol, 0.0);
	auto OctanolWaterPhaseTransferEntalphy     = RegisterParameterDouble(Model, Chemistry, "Enthalpy of phase tranfer between octanol and water", KiloJoulesPerMol, 0.0);
	auto HenrysConstant25                      = RegisterParameterDouble(Model, Chemistry, "Henry's constant at 25°C", PascalM3PerMol, 0.0);
	auto OctanolWaterPartitioningCoefficient25 = RegisterParameterDouble(Model, Chemistry, "Octanol-water partitioning coefficient at 25°C", Dimensionless, 0.0);
	
	auto AtmosphericContaminantConcentration   = RegisterParameterDouble(Model, Chemistry, "Atmospheric contaminant concentration", NgPerM3, 0.0, 0.0, 100.0);
	auto SoilContaminantDegradationRateConstant    = RegisterParameterDouble(Model, Chemistry, "Contaminant degradation rate constant in soil", PerDay, 0.0, 0.0, 1.0);
	auto GroundwaterContaminantDegradationRateConstant    = RegisterParameterDouble(Model, Chemistry, "Contaminant degradation rate constant in groundwater", PerDay, 0.0, 0.0, 1.0);
	auto ReachContaminantDegradationRateConstant    = RegisterParameterDouble(Model, Chemistry, "Contaminant degradation rate constant in the stream", PerDay, 0.0, 0.0, 1.0);
	auto BedContaminantDegradationRateConstant      = RegisterParameterDouble(Model, Chemistry, "Contaminant degradation rate constant in the stream bed", PerDay, 0.0, 0.0, 1.0);
	
	auto AirSoilOverallMassTransferCoefficient = RegisterParameterDouble(Model, Land, "Overall air-soil mass transfer coefficient", MPerDay, 0.0, 0.0, 100.0);
	
	auto ContaminantsGrain = RegisterParameterGroup(Model, "Contaminants by grain class", Class);
	auto ContaminantSOCScalingFactor = RegisterParameterDouble(Model, ContaminantsGrain, "Contaminant SOC scaling factor", Dimensionless, 1.0, 0.0, 1.0);
	
	
	auto ContaminantReach = RegisterParameterGroup(Model, "Contaminants by reach", Reach);
	
	auto InitialContaminantMassInSoil  = RegisterParameterDouble(Model, Land, "Initial contaminant mass in soil", NgPerKm2, 0.0, 0.0, 1e3);
	auto InitialContaminantMassInGroundwater = RegisterParameterDouble(Model, ContaminantReach, "Initial contaminant mass in groundwater", NgPerKm2, 0.0, 0.0, 1e3);
	auto InitialContaminantMassInReach = RegisterParameterDouble(Model, ContaminantReach, "Initial contaminant mass in reach", Ng, 0.0, 0.0, 1e3);
	
	//Seems a little weird to put this in the contaminants "folder", but there is no reason to put it anywhere else.
	auto HeightOfLargeStones = RegisterParameterDouble(Model, ContaminantReach, "Average height of large stones in the stream bed", M, 0.0, 0.0, 0.5);
	auto AverageBedGrainDiameter = RegisterParameterDouble(Model, ContaminantReach, "Average bed grain diameter", M, 0.0001, 0.0, 0.1);
	auto SedimentDryDensity = RegisterParameterDouble(Model, ContaminantReach, "Sediment dry density", KgPerM3, 2000.0, 0.0, 10000.0);
	auto SedimentPorosity   = RegisterParameterDouble(Model, ContaminantReach, "Sediment porosity", Dimensionless, 0.1, 0.0, 0.99);
	
	auto SoilTemperature = GetEquationHandle(Model, "Soil temperature"); //SoilTemperature.h
	auto WaterDepth      = GetEquationHandle(Model, "Water depth");      //PERSiST.h
	auto RunoffToReach   = GetEquationHandle(Model, "Runoff to reach");  //PERSiST.h
	auto PercolationInput= GetEquationHandle(Model, "Percolation input");//PERSiST.h
	auto SoilDOCMass     = GetEquationHandle(Model, "Soil DOC mass");    //INCA-Tox-C.h
	auto SoilDOCFluxToReach = GetEquationHandle(Model, "Soil DOC flux to reach"); //INCA-Tox-C
	auto SoilDOCFluxToGroundwater = GetEquationHandle(Model, "Soil DOC flux to groundwater"); //INCA-Tox-C
	auto WaterTemperature = GetEquationHandle(Model, "Water temperature"); //WaterTemperature.h
	auto ReachFlow       = GetEquationHandle(Model, "Reach flow"); //PERSiST.h
	auto ReachVolume     = GetEquationHandle(Model, "Reach volume"); //PERSiST.h
	auto ReachVelocity   = GetEquationHandle(Model, "Reach velocity"); //PERSiST.h
	auto ReachDepth      = GetEquationHandle(Model, "Reach depth");    //PERSiST.h
	auto ReachDOCOutput  = GetEquationHandle(Model, "Reach DOC output"); //INCA-Tox-C.h
	auto ReachDOCMass    = GetEquationHandle(Model, "Reach DOC mass");   //INCA-Tox-C.h
	auto SOCDeliveryToReach = GetEquationHandle(Model, "SOC delivery to reach by erosion"); //INCA-Tox-C.h
	auto TotalMassOfBedGrainPerUnitArea = GetEquationHandle(Model, "Total mass of bed sediment per unit area"); //INCA-Sed.h
	auto ReachShearVelocity = GetEquationHandle(Model, "Reach shear velocity"); //INCA-Sed.h
	auto ReachSOCDeposition = GetEquationHandle(Model, "Reach SOC deposition"); //INCA-Tox-C.h
	auto ReachSOCEntrainment = GetEquationHandle(Model, "Reach SOC entrainment"); //INCA-Tox-C.h
	auto ReachSuspendedSOCMass = GetEquationHandle(Model, "Reach suspended SOC mass"); //INCA-Tox-C.h
	auto BedSOCMass       = GetEquationHandle(Model, "Stream bed SOC mass"); //INCA-Tox-C.h
	
	auto CatchmentArea   = GetParameterDoubleHandle(Model, "Terrestrial catchment area"); //PERSiST.h
	auto Percent         = GetParameterDoubleHandle(Model, "%");                //PERSiST.h
	auto MaximumCapacity = GetParameterDoubleHandle(Model, "Maximum capacity"); //PERSiST.h
	auto SoilSOCMass     = GetParameterDoubleHandle(Model, "Soil SOC mass");      //INCA-Tox-C.h
	auto ReachLength     = GetParameterDoubleHandle(Model, "Reach length"); //PERSiST.h
	auto ReachWidth      = GetParameterDoubleHandle(Model, "Reach width"); //PERSiST.h
	
	
	
	auto SoilTemperatureKelvin = RegisterEquation(Model, "Soil temperature in Kelvin", K);
	
	EQUATION(Model, SoilTemperatureKelvin,
		return 273.15 + RESULT(SoilTemperature);
	)
	
	
	auto SoilSolver = RegisterSolver(Model, "Soil Solver", 0.1, IncaDascru);
	
	
	auto HenrysConstant = RegisterEquation(Model, "Henry's constant", PascalM3PerMol);
	
	auto AirWaterPartitioningCoefficient     = RegisterEquation(Model, "Air-water partitioning coefficient", Dimensionless);
	auto OctanolWaterPartitioningCoefficient = RegisterEquation(Model, "Octanol-water partitioning coefficient", Dimensionless);
	auto WaterSOCPartitioningCoefficient     = RegisterEquation(Model, "Water-SOC partitioning coefficient", M3PerKg);
	auto WaterDOCPartitioningCoefficient     = RegisterEquation(Model, "Water-DOC partitioning coefficient", M3PerKg);
	
	auto ContaminantDeliveryToReachByErosion = RegisterEquation(Model, "Contaminant delivery to reach by erosion", NgPerDay);
	auto ContaminantDeliveryToReachByAllErosion = RegisterEquationCumulative(Model, "Contaminant delivery to reach by erosion summed over grain classes", ContaminantDeliveryToReachByErosion, Class);
	auto TotalContaminantDeliveryToReachByAllErosion = RegisterEquationCumulative(Model, "Contaminant delivery to reach by erosion summed over grain classes and land classes", ContaminantDeliveryToReachByAllErosion, LandscapeUnits);
	
	auto SoilWaterContaminantConcentration = RegisterEquation(Model, "Soil water contaminant concentration", NgPerM3);
	SetSolver(Model, SoilWaterContaminantConcentration, SoilSolver);
	auto SoilSOCContaminantConcentration   = RegisterEquation(Model, "Soil SOC contaminant concentration", NgPerKg);
	SetSolver(Model, SoilSOCContaminantConcentration, SoilSolver);
	auto SoilDOCContaminantConcentration   = RegisterEquation(Model, "Soil DOC contaminant concentration", NgPerKg);
	SetSolver(Model, SoilDOCContaminantConcentration, SoilSolver);
	auto SoilAirContaminantConcentration   = RegisterEquation(Model, "Soil Air contaminant concentration", NgPerM3);
	SetSolver(Model, SoilAirContaminantConcentration, SoilSolver);
	auto DiffusiveAirSoilExchangeFlux      = RegisterEquation(Model, "Diffusive exchange of contaminants between soil and atmosphere", NgPerKm2PerDay);
	SetSolver(Model, DiffusiveAirSoilExchangeFlux, SoilSolver);
	auto SoilContaminantDegradation        = RegisterEquation(Model, "Soil contaminant degradation", NgPerKm2PerDay);
	SetSolver(Model, SoilContaminantDegradation, SoilSolver);
	
	auto SoilWaterVolume                   = RegisterEquation(Model, "Soil water volume", M3PerKm2);
	auto SoilAirVolume                     = RegisterEquation(Model, "Soil air volume", M3PerKm2);
	
	auto ContaminantInputsToSoil           = RegisterEquation(Model, "Contaminant inputs to soil", NgPerKm2PerDay);
	auto ContaminantMassInSoil             = RegisterEquationODE(Model, "Contaminant mass in soil", NgPerKm2);
	SetSolver(Model, ContaminantMassInSoil, SoilSolver);
	SetInitialValue(Model, ContaminantMassInSoil, InitialContaminantMassInSoil);
	auto SoilContaminantFluxToReach        = RegisterEquation(Model, "Soil contaminant flux to reach", NgPerKm2PerDay);
	SetSolver(Model, SoilContaminantFluxToReach, SoilSolver);
	auto SoilContaminantFluxToGroundwater  = RegisterEquation(Model, "Soil contaminant flux to groundwater", NgPerKm2PerDay);
	SetSolver(Model, SoilContaminantFluxToGroundwater, SoilSolver);
	auto SoilContaminantFluxToDirectRunoff = RegisterEquation(Model, "Soil contaminant flux to direct runoff", NgPerKm2PerDay);
	SetSolver(Model, SoilContaminantFluxToDirectRunoff, SoilSolver);
	
	auto GroundwaterContaminantDegradation = RegisterEquation(Model, "Groundwater contaminant degradation", NgPerKm2PerDay);
	SetSolver(Model, GroundwaterContaminantDegradation, SoilSolver);
	auto GroundwaterContaminantFluxToReach = RegisterEquation(Model, "Groundwater contaminant flux to reach", NgPerKm2PerDay);
	SetSolver(Model, GroundwaterContaminantFluxToReach, SoilSolver);
	auto ContaminantMassInGroundwater      = RegisterEquationODE(Model, "Contaminant mass in groundwater", NgPerKm2);
	SetSolver(Model, ContaminantMassInGroundwater, SoilSolver);
	SetInitialValue(Model, ContaminantMassInGroundwater, InitialContaminantMassInGroundwater);
	
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
	
	EQUATION(Model, OctanolWaterPartitioningCoefficient,
		double LogKOW25 = std::log10(PARAMETER(OctanolWaterPartitioningCoefficient25));
		double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
		double tdiff = (1.0 / RESULT(SoilTemperatureKelvin) - 1.0/(273.15 + 25.0));
		double LogKOWT = LogKOW25 - (1e3*PARAMETER(OctanolWaterPhaseTransferEntalphy) / (std::log(10.0)*R))   * tdiff;
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
			  RESULT(WaterSOCPartitioningCoefficient) * PARAMETER(SoilSOCMass)
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
	
	EQUATION(Model, DiffusiveAirSoilExchangeFlux,
		return PARAMETER(AirSoilOverallMassTransferCoefficient) * (1e-3*PARAMETER(AtmosphericContaminantConcentration) - RESULT(SoilAirContaminantConcentration));
	)
	
	EQUATION(Model, ContaminantInputsToSoil,
		//TODO: dry and wet inputs should depend on precipitation
		//TODO: maybe also let the user parametrize these in case they don't have detailed timeseries
		//NOTE: convert ng/m2 -> ng/km2
		return (
			INPUT(AtmosphericDryDeposition) + INPUT(AtmosphericWetDeposition) + INPUT(LandManagementDeposition) + INPUT(LitterFallDeposition)
			) * 1e6 
			;
	)
	
	EQUATION(Model, ContaminantDeliveryToReachByErosion,
		return LAST_RESULT(SoilSOCContaminantConcentration) * RESULT(SOCDeliveryToReach) * PARAMETER(ContaminantSOCScalingFactor); //NOTE: LAST_RESULT because otherwise we would get a circular dependency with the solver, and we can't do that since this equation indexes over Grain class.
	)
	
	EQUATION(Model, SoilContaminantFluxToReach,
		return
			RESULT(RunoffToReach, Soilwater) * RESULT(SoilWaterContaminantConcentration) * 1000.0 // Convert km^2 mm/day * ng/m^3 to ng/day
		  + RESULT(SoilDOCFluxToReach) * RESULT(SoilDOCContaminantConcentration);
	)
	
	EQUATION(Model, SoilContaminantFluxToGroundwater,
		return
			RESULT(PercolationInput, Groundwater) * RESULT(SoilWaterContaminantConcentration) * 1000.0 // Convert km^2 mm/day * ng/m^3 to ng/day
		  + RESULT(SoilDOCFluxToGroundwater) * RESULT(SoilDOCContaminantConcentration);
	)
	
	EQUATION(Model, SoilContaminantFluxToDirectRunoff,
		return 0.0; //TODO
	)
	
	EQUATION(Model, SoilContaminantDegradation,
		double degradablemass =
		  RESULT(SoilWaterVolume) * RESULT(SoilWaterContaminantConcentration)
		+ RESULT(SoilDOCMass)     * RESULT(SoilDOCContaminantConcentration)
		+ PARAMETER(SoilSOCMass)  * RESULT(SoilSOCContaminantConcentration);
		
		return PARAMETER(SoilContaminantDegradationRateConstant) * degradablemass;
	)
	
	EQUATION(Model, ContaminantMassInSoil,
		return
		  RESULT(ContaminantInputsToSoil)
		- RESULT(SoilContaminantFluxToReach)
		- RESULT(SoilContaminantFluxToGroundwater)
		- RESULT(SoilContaminantFluxToDirectRunoff)
		+ RESULT(DiffusiveAirSoilExchangeFlux)
		- RESULT(SoilContaminantDegradation)
		- RESULT(ContaminantDeliveryToReachByAllErosion);
	)
	
	EQUATION(Model, GroundwaterContaminantFluxToReach,
		//TODO: Do we need to care about partitioning here? Probably only if we ever need to track what enters the reach through DOC and water separately. But not in this initial simple version.
		return RESULT(RunoffToReach, Groundwater) * SafeDivide(RESULT(ContaminantMassInGroundwater), RESULT(WaterDepth, Groundwater));
	)
	
	EQUATION(Model, GroundwaterContaminantDegradation,
		double degradablemass = RESULT(ContaminantMassInGroundwater);
		return PARAMETER(GroundwaterContaminantDegradationRateConstant) * degradablemass;
	)
	
	EQUATION(Model, ContaminantMassInGroundwater,
		return
		  RESULT(SoilContaminantFluxToGroundwater)
		- RESULT(GroundwaterContaminantFluxToReach)
		- RESULT(GroundwaterContaminantDegradation);
	)

	
	auto ReachSolver = RegisterSolver(Model, "Reach contaminant solver", 0.1, IncaDascru); //NOTE: We can't use the reach solver from PERSiST, because we depend on the grain solver that again depends on the PERSiST reach solver.
	//auto ReachSolver = RegisterSolver(Model, "Reach contaminant solver", 0.1, BoostRosenbrock4, 1e-3, 1e-3);
	
	auto WaterTemperatureKelvin        = RegisterEquation(Model, "Water temperature in Kelvin", K);
	
	auto DiffuseContaminantOutput      = RegisterEquation(Model, "Diffuse contaminant output", NgPerDay);
	auto TotalDiffuseContaminantOutput = RegisterEquationCumulative(Model, "Total diffuse contaminant output", DiffuseContaminantOutput, LandscapeUnits);
	auto ReachContaminantInput         = RegisterEquation(Model, "Reach contaminant input", NgPerDay);
	auto ContaminantMassInReach        = RegisterEquationODE(Model, "Contaminant mass in reach", Ng);
	SetSolver(Model, ContaminantMassInReach, ReachSolver);
	SetInitialValue(Model, ContaminantMassInReach, InitialContaminantMassInReach);
	auto ReachContaminantFlux          = RegisterEquation(Model, "Reach contaminant flux", NgPerDay);
	SetSolver(Model, ReachContaminantFlux, ReachSolver);
	auto ReachContaminantDegradation   = RegisterEquation(Model, "Reach contaminant degradation", NgPerDay);
	SetSolver(Model, ReachContaminantDegradation, ReachSolver);
	
	auto ReachHenrysConstant                      = RegisterEquation(Model, "Reach Henry's constant", PascalM3PerMol);
	auto ReachAirWaterPartitioningCoefficient     = RegisterEquation(Model, "Reach air-water partitioning coefficient", Dimensionless);
	auto ReachOctanolWaterPartitioningCoefficient = RegisterEquation(Model, "Reach octanol-water partitioning coefficient", Dimensionless);
	auto ReachWaterSOCPartitioningCoefficient     = RegisterEquation(Model, "Reach water-SOC partitioning coefficient", M3PerKg);
	auto ReachWaterDOCPartitioningCoefficient     = RegisterEquation(Model, "Reach water-DOC partitioning coefficient", M3PerKg);
	
	auto MolecularDiffusivityOfCompoundInAir      = RegisterEquation(Model, "Molecular diffusivity of compound in air", M2PerS);
	auto MolecularDiffusivityOfWaterVapourInAir   = RegisterEquation(Model, "Molecular diffusivity of water vapour in air", M2PerS);
	auto MolecularDiffusivityOfCompoundInWater    = RegisterEquation(Model, "Molecular diffusivity of compound in water", M2PerS);
	auto ReachKinematicViscosity                  = RegisterEquation(Model, "Reach kinematic viscosity", M2PerS);
	auto ReachDynamicViscosity                    = RegisterEquation(Model, "Reach dynamic viscosity", GPerCmPerSPerHundred);
	auto SchmidtNumber                            = RegisterEquation(Model, "Schmidt number", Dimensionless);
	auto NonDimensionalRoughnessParameter         = RegisterEquation(Model, "Non-dimensional roughness parameter", Dimensionless);
	auto ElementFroudeNumber                      = RegisterEquation(Model, "Element Froude number", Dimensionless);
	
	
	auto ReachAirContaminantTransferVelocity      = RegisterEquation(Model, "Reach air contamninant transfer velocity", MPerDay);
	auto ReachWaterContaminantTransferVelocity    = RegisterEquation(Model, "Reach water contaminant transfer velocity", MPerDay);
	auto ReachOverallAirWaterContaminantTransferVelocity = RegisterEquation(Model, "Reach overall air-water transfer velocity", MPerDay);
	auto DiffusiveAirReachExchangeFlux            = RegisterEquation(Model, "Diffusive air reach exchange flux", NgPerDay);
	SetSolver(Model, DiffusiveAirReachExchangeFlux, ReachSolver);
	
	auto ReachWaterContaminantConcentration = RegisterEquation(Model, "Reach water contaminant concentration", NgPerM3);
	SetSolver(Model, ReachWaterContaminantConcentration, ReachSolver);
	auto ReachDOCContaminantConcentration   = RegisterEquation(Model, "Reach DOC contaminant concentration", NgPerKg);
	SetSolver(Model, ReachDOCContaminantConcentration, ReachSolver);
	auto ReachSOCContaminantConcentration = RegisterEquation(Model, "Reach SOC contaminant concentration", NgPerKg);
	//SetSolver(Model, ReachSOCContaminantConcentration, ReachSolver);
	
	auto ReachSedimentContaminantFactor = RegisterEquation(Model, "Reach sediment SOC contaminant factor", M3);
	auto TotalReachSedimentContaminantFactor = RegisterEquationCumulative(Model, "Total reach sediment SOC contaminant factor", ReachSedimentContaminantFactor, Class);
	
	auto ReachContaminantDeposition = RegisterEquation(Model, "Reach contaminant deposition", NgPerM2PerDay);
	auto ReachContaminantEntrainment = RegisterEquation(Model, "Reach contaminant entrainment", NgPerM2PerDay);
	
	auto TotalReachContaminantDeposition = RegisterEquationCumulative(Model, "Total reach contaminant deposition", ReachContaminantDeposition, Class);
	auto TotalReachContaminantEntrainment = RegisterEquationCumulative(Model, "Total reach contaminant entrainment", ReachContaminantEntrainment, Class);
	
	auto PoreWaterVolume = RegisterEquation(Model, "Pore water volume", M3PerM2);
	
	auto BedContaminantDegradation        = RegisterEquation(Model, "Stream bed contaminant degradation", NgPerM2PerDay);
	SetSolver(Model, BedContaminantDegradation, ReachSolver);
	auto BedContaminantMass               = RegisterEquationODE(Model, "Stream bed contaminant mass", NgPerM2);
	SetSolver(Model, BedContaminantMass, ReachSolver);
	//SetInitialValue
	
	auto BedSedimentContaminantFactor = RegisterEquation(Model, "Stream bed SOC contaminant factor", M3PerM2);
	auto TotalBedSedimentContaminantFactor = RegisterEquationCumulative(Model, "Total stream bed SOC contaminant factor", BedSedimentContaminantFactor, Class);
	auto BedWaterContaminantConcentration = RegisterEquation(Model, "Stream bed pore water contaminant concentration", NgPerM3);
	auto BedSOCContaminantConcentration   = RegisterEquation(Model, "Stream bed SOC contaminant concentration", NgPerKg);
	
	
	
	EQUATION(Model, DiffuseContaminantOutput,
		return
		( RESULT(SoilContaminantFluxToReach)
		+ RESULT(GroundwaterContaminantFluxToReach)
		)
		* PARAMETER(CatchmentArea) * PARAMETER(Percent)*0.01;
	)
	
	EQUATION(Model, ReachContaminantInput,
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(ReachContaminantFlux, *Input);
		)
	
		return
			upstreamflux
			+ RESULT(TotalDiffuseContaminantOutput)
			+ RESULT(TotalContaminantDeliveryToReachByAllErosion)
			//+ deposition
			;
	)
	
	
	EQUATION(Model, ReachContaminantFlux,
		return
		  RESULT(ReachFlow) * RESULT(ReachWaterContaminantConcentration)
		+ RESULT(ReachDOCOutput) * RESULT(ReachDOCContaminantConcentration);
		// + suspended sediment flux
	)
	
	EQUATION(Model, ReachContaminantDegradation,
		return RESULT(ContaminantMassInReach) * PARAMETER(ReachContaminantDegradationRateConstant);
	)
	
	
	EQUATION(Model, ContaminantMassInReach,
		return
			  RESULT(ReachContaminantInput)
			- RESULT(ReachContaminantFlux)
			- RESULT(ReachContaminantDegradation)
			- RESULT(DiffusiveAirReachExchangeFlux)
			+ (RESULT(TotalReachContaminantEntrainment) - RESULT(TotalReachContaminantDeposition)) * PARAMETER(ReachLength)*PARAMETER(ReachWidth);
			// exchange with stream bed
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
	
	EQUATION(Model, ReachOctanolWaterPartitioningCoefficient,
		double LogKOW25 = std::log10(PARAMETER(OctanolWaterPartitioningCoefficient25));
		double R = 8.314; //Ideal gas constant (J K^-1 mol^-1)
		double tdiff = (1.0 / RESULT(WaterTemperatureKelvin) - 1.0/(273.15 + 25.0));
		double LogKOWT = LogKOW25 - (1e3*PARAMETER(OctanolWaterPhaseTransferEntalphy) / (std::log(10.0)*R))   * tdiff;
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
		return RESULT(ReachOverallAirWaterContaminantTransferVelocity) * (RESULT(ReachWaterContaminantConcentration) - 1e-3*PARAMETER(AtmosphericContaminantConcentration)/RESULT(ReachAirWaterPartitioningCoefficient)) * PARAMETER(ReachLength) * PARAMETER(ReachWidth);
	)
	
	
	EQUATION(Model, ReachWaterContaminantConcentration,
		return
			RESULT(ContaminantMassInReach) /
			  (RESULT(ReachWaterDOCPartitioningCoefficient)*RESULT(ReachDOCMass) + RESULT(TotalReachSedimentContaminantFactor) + RESULT(ReachVolume));
	)
	
	EQUATION(Model, ReachDOCContaminantConcentration,
		return RESULT(ReachWaterContaminantConcentration) * RESULT(ReachWaterDOCPartitioningCoefficient);
	)
	
	EQUATION(Model, ReachSedimentContaminantFactor,
		return RESULT(ReachWaterSOCPartitioningCoefficient) * RESULT(ReachSuspendedSOCMass) * PARAMETER(ContaminantSOCScalingFactor);
	)
	
	EQUATION(Model, ReachSOCContaminantConcentration,
		return RESULT(ReachWaterContaminantConcentration) * RESULT(ReachWaterSOCPartitioningCoefficient) * PARAMETER(ContaminantSOCScalingFactor);
	)
	
	EQUATION(Model, ReachContaminantDeposition,
		return RESULT(ReachSOCDeposition) * LAST_RESULT(ReachSOCContaminantConcentration);    //TODO: Unless we use LAST_RESULT we get a circular dependency among equations.. How to fix this??? Alternatively use LAST_RESULT(ReachWaterContaminantConcentration) in ReachSOCContaminantConcentration, but that is not much better...
	)
	
	EQUATION(Model, ReachContaminantEntrainment,
		return RESULT(ReachSOCEntrainment) * LAST_RESULT(BedSOCContaminantConcentration);     //TODO: Same as above
	)
	
	EQUATION(Model, BedContaminantDegradation,
		return RESULT(BedContaminantMass) * PARAMETER(BedContaminantDegradationRateConstant);
	)
	
	EQUATION(Model, BedContaminantMass,
		return
		  RESULT(TotalReachContaminantDeposition)
		- RESULT(TotalReachContaminantEntrainment)
		- RESULT(BedContaminantDegradation);
		// TODO: diffusive exchange with stream
	)
	
	EQUATION(Model, PoreWaterVolume,
		//TODO: We could compute this based on the actual density of sediment classes (INCA-MP parameter) instead of having a separate parameter for it.
		//TODO: Also, this should maybe be an output of INCA-MP instead of being computed by the contaminants module.
		return (RESULT(TotalMassOfBedGrainPerUnitArea) / PARAMETER(SedimentDryDensity)) * std::pow(PARAMETER(SedimentPorosity), 2.0/3.0);
	)
	
	EQUATION(Model, BedSedimentContaminantFactor,
		return RESULT(ReachWaterSOCPartitioningCoefficient) * LAST_RESULT(BedSOCMass) * PARAMETER(ContaminantSOCScalingFactor);
	)
	
	EQUATION(Model, BedWaterContaminantConcentration,
		return RESULT(BedContaminantMass) /
			(RESULT(TotalBedSedimentContaminantFactor) + RESULT(PoreWaterVolume)); //TODO: DOC in pore water, but is it necessary?
	)	
	
	EQUATION(Model, BedSOCContaminantConcentration,
		return RESULT(BedWaterContaminantConcentration) * RESULT(ReachWaterSOCPartitioningCoefficient) * PARAMETER(ContaminantSOCScalingFactor);
	)
	
	/*
	EQUATION(Model, WaterSedimentApparentViscosity,
		double por = (1.0 - PARAMETER(SedimentPorosity)) / PARAMETER(SedimentPorosity);
		return (RESULT(ReachKinematicViscosity) / (32.0 * 5.6e-3)) * 0.1 * por * por;
	)
	
	EQUATION(Model, TurbulentReynoldsNumber,
		double periodOfVelocityPulse = 1.0;
		return RESULT(ReachShearVelocity) * RESULT(ReachShearVelocity) * periodOfVelocityPulse / RESULT(WaterSedimentApparentViscosity);
	)
	
	EQUATION(Model, ReachBedWaterContaminantDiffusiveExchange,
		return 
	)
	*/
	EndModule(Model);
}