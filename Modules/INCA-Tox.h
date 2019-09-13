


//#include "../boost_solvers.h"

//NOTE: THIS MODULE IS IN DEVELOPMENT!


static void
AddIncaToxModule(mobius_model *Model)
{
	auto Dimensionless    = RegisterUnit(Model);
	auto NgPerKm2         = RegisterUnit(Model, "ng/km2");
	auto NgPerKm2PerDay   = RegisterUnit(Model, "ng/km2/day");
	auto NgPerM2PerDay    = RegisterUnit(Model, "ng/m2/day");
	auto NgPerM3          = RegisterUnit(Model, "ng/m3");
	auto NgPerKg          = RegisterUnit(Model, "ng/kg");
	auto M3PerKg          = RegisterUnit(Model, "m3/kg");
	auto M3PerKm2         = RegisterUnit(Model, "m3/km2");
	auto KiloJoulesPerMol = RegisterUnit(Model, "kJ/mol");
	auto PascalM3PerMol   = RegisterUnit(Model, "Pa m3/mol");
	auto MPerDay          = RegisterUnit(Model, "m/day");
	auto PGPerM3          = RegisterUnit(Model, "pg/m3");
	auto K                = RegisterUnit(Model, "K");
	auto PerDay           = RegisterUnit(Model, "1/day");
	
	auto AtmosphericDryDeposition = RegisterInput(Model, "Atmospheric dry contaminant deposition", NgPerM2PerDay);
	auto AtmosphericWetDeposition = RegisterInput(Model, "Atmospheric wet contaminant deposition", NgPerM2PerDay);
	auto LandManagementDeposition = RegisterInput(Model, "Land management conatminant deposition", NgPerM2PerDay);
	auto LitterFallDeposition     = RegisterInput(Model, "Litter fall contaminant deposition",     NgPerM2PerDay);
	
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto Soils          = GetIndexSetHandle(Model, "Soils");
	auto DirectRunoff = RequireIndex(Model, Soils, "Direct runoff");
	auto Soilwater    = RequireIndex(Model, Soils, "Soil water");
	auto Groundwater  = RequireIndex(Model, Soils, "Groundwater");
	
	
	auto Chemistry = RegisterParameterGroup(Model, "Chemistry");
	auto Land      = GetParameterGroupHandle(Model, "Landscape units");

	//TODO: As always, find better default, min, max values for parameters!
	
	auto AirWaterPhaseTransferEnthalpy         = RegisterParameterDouble(Model, Chemistry, "Enthalpy of phase transfer between air and water", KiloJoulesPerMol, 0.0);
	auto OctanolWaterPhaseTransferEntalphy     = RegisterParameterDouble(Model, Chemistry, "Enthalpy of phase tranfer between octanol and water", KiloJoulesPerMol, 0.0);
	auto HenrysConstant25                      = RegisterParameterDouble(Model, Chemistry, "Henry's constant at 25°C", PascalM3PerMol, 0.0);
	auto OctanolWaterPartitioningCoefficient25 = RegisterParameterDouble(Model, Chemistry, "Octanol-water partitioning coefficient at 25°C", Dimensionless, 0.0);
	
	auto AtmosphericContaminantConcentration   = RegisterParameterDouble(Model, Chemistry, "Atmospheric contaminant concentration", PGPerM3, 0.0, 0.0, 100.0);
	auto ContaminantDegradationRateConstant    = RegisterParameterDouble(Model, Chemistry, "Contaminant degradation rate constant", PerDay, 0.0, 0.0, 1.0);
	
	auto AirSoilOverallMassTransferCoefficient = RegisterParameterDouble(Model, Land, "Overall air-soil mass transfer coefficient", MPerDay, 0.0, 0.0, 100.0);
	
	
	auto SoilTemperature = GetEquationHandle(Model, "Soil temperature"); //SoilTemperature.h
	auto WaterDepth      = GetEquationHandle(Model, "Water depth");      //PERSiST.h
	auto RunoffToReach   = GetEquationHandle(Model, "Runoff to reach");  //PERSiST.h
	auto PercolationInput= GetEquationHandle(Model, "Percolation input");//PERSiST.h
	auto SoilDOCMass     = GetEquationHandle(Model, "Soil DOC mass");    //INCA-Tox-C.h
	auto SoilDOCFluxToReach = GetEquationHandle(Model, "Soil DOC flux to reach"); //INCA-Tox-C
	auto SoilDOCFluxToGroundwater = GetEquationHandle(Model, "Soil DOC flux to groundwater"); //INCA-Tox-C
	
	auto MaximumCapacity = GetParameterDoubleHandle(Model, "Maximum capacity"); //PERSiST.h
	auto SoilSOCMass   = GetParameterDoubleHandle(Model, "Soil SOC mass");      //INCA-Tox-C.h
	
	
	auto SoilTemperatureKelvin = RegisterEquation(Model, "Soil temperature in Kelvin", K);
	
	EQUATION(Model, SoilTemperatureKelvin,
		return 273.15 + RESULT(SoilTemperature);
	)
	
	
	auto SoilSolver = RegisterSolver(Model, "Soil Solver", 0.1, IncaDascru);
	//auto SoilSolver = RegisterSolver(Model, "Soil Solver", 0.1, BoostRosenbrock4, 1e-3, 1e-3);
	
	auto HenrysConstant = RegisterEquation(Model, "Henry's constant", PascalM3PerMol);
	
	auto AirWaterPartitioningCoefficient     = RegisterEquation(Model, "Air-water partitioning coefficient", Dimensionless);
	auto OctanolWaterPartitioningCoefficient = RegisterEquation(Model, "Octanol-water partitioning coefficient", Dimensionless);
	auto WaterSOCPartitioningCoefficient     = RegisterEquation(Model, "Water-SOC partitioning coefficient", M3PerKg);
	auto WaterDOCPartitioningCoefficient     = RegisterEquation(Model, "Water-DOC partitioning coefficient", M3PerKg);
	
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
	//SetInitialValue
	auto SoilContaminantFluxToReach        = RegisterEquation(Model, "Soil contaminant flux to reach", NgPerKm2PerDay);
	SetSolver(Model, SoilContaminantFluxToReach, SoilSolver);
	auto SoilContaminantFluxToGroundwater  = RegisterEquation(Model, "Soil contaminant flux to groundwater", NgPerKm2PerDay);
	SetSolver(Model, SoilContaminantFluxToGroundwater, SoilSolver);
	auto SoilContaminantFluxToDirectRunoff = RegisterEquation(Model, "Soil contaminant flux to direct runoff", NgPerKm2PerDay);
	SetSolver(Model, SoilContaminantFluxToDirectRunoff, SoilSolver);
	
	auto GroundwaterContaminantFluxToReach = RegisterEquation(Model, "Groundwater contaminant flux to reach", NgPerKm2PerDay);
	SetSolver(Model, GroundwaterContaminantFluxToReach, SoilSolver);
	auto ContaminantMassInGroundwater      = RegisterEquationODE(Model, "Contaminant mass in groundwater", NgPerKm2);
	SetSolver(Model, ContaminantMassInGroundwater, SoilSolver);
	//SetInitialValue
	
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
		
		return PARAMETER(ContaminantDegradationRateConstant) * degradablemass;
	)
	
	EQUATION(Model, ContaminantMassInSoil,
		return
		  RESULT(ContaminantInputsToSoil)
		- RESULT(SoilContaminantFluxToReach)
		- RESULT(SoilContaminantFluxToGroundwater)
		- RESULT(SoilContaminantFluxToDirectRunoff)
		+ RESULT(DiffusiveAirSoilExchangeFlux);
		- RESULT(SoilContaminantDegradation);
		//TODO: erosion (from microplastics module) should be able to transport SOC with contaminants in it.
	)
	
	EQUATION(Model, GroundwaterContaminantFluxToReach,
		//TODO: Do we need to care about partitioning here
		return RESULT(RunoffToReach, Groundwater) * SafeDivide(RESULT(ContaminantMassInGroundwater), RESULT(WaterDepth, Groundwater));
	)
	
	EQUATION(Model, ContaminantMassInGroundwater,
		return
		  RESULT(SoilContaminantFluxToGroundwater)
		- RESULT(GroundwaterContaminantFluxToReach);
		//TODO: degradation, probably
	)
	
	/*
	
	EQUATION(Model, DiffuseContaminantOutput,
	
	)
	
	EQUATION(Model, ReachContaminantInput,
		double upstreamflux = 0.0;
		FOREACH_INPUT(Reach,
			upstreamflux += RESULT(ReachContaminantFlux, *Input);
		)
	
		return
			upstreamflux
			+ RESULT(DiffuseContaminantOutput)
			//from erosion
			;
	)
	
	EQUATION(Model, ContaminantMassInReach,
		return
			  RESULT(ReachContaminantInput)
			- RESULT(ReachContaminantFlux);
	)
	*/
}