

//NOTE: Has to be used with PERSiST and INCA-Sed

static void
AddIncaToxDOCModule(mobius_model *Model)
{
	BeginModule(Model, "INCA-Tox Carbon", "0.1.1");
SetModuleDescription(Model, R""""(
This is a simple DOC model intended only for use with INCA-Tox. It is not suitable for studying complicated carbon transport dynamics in itself. It does for instance not try to keep track of the carbon balance or soil processes, it just provides a way to set up a simple empirical model of DOC transport in order for contaminants bound in the carbon to be transported correctly.
)"""");
	
	auto Dimensionless = RegisterUnit(Model);
	auto MgPerL   = RegisterUnit(Model, "mg/l");
	auto Kg       = RegisterUnit(Model, "kg");
	auto KgPerDay = RegisterUnit(Model, "kg/day");
	auto KgPerKm2 = RegisterUnit(Model, "kg/km2");
	auto KgPerM2  = RegisterUnit(Model, "kg/m2");
	auto KgPerKg  = RegisterUnit(Model, "kg/kg");
	auto KgPerKm2PerDay = RegisterUnit(Model, "kg/km2/day");
	auto KgPerM2PerDay  = RegisterUnit(Model, "kg/m2/day");
	auto PerDay   = RegisterUnit(Model, "1/day");
	auto M3PerKm2 = RegisterUnit(Model, "m3/km2");
	auto MgPerLPerC = RegisterUnit(Model, "mg/l/°C");
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto Reaches      = GetIndexSetHandle(Model, "Reaches");
	auto Soils        = GetIndexSetHandle(Model, "Soils");
	auto Class        = GetIndexSetHandle(Model, "Sediment size class");
	auto DirectRunoff = RequireIndex(Model, Soils, "Direct runoff");
	auto Soilwater    = RequireIndex(Model, Soils, "Soil water");
	auto Groundwater  = RequireIndex(Model, Soils, "Groundwater");
	
	
	auto Land = RegisterParameterGroup(Model, "Carbon by land class", LandscapeUnits);
	auto Reach = RegisterParameterGroup(Model, "Carbon by subcatchment", Reaches);
	
	auto SoilSOCMass                       = RegisterParameterDouble(Model, Land, "Soil SOC mass", KgPerM2, 0.0, 0.0, 1e7);
	auto SizeOfEasilyAccessibleFraction    = RegisterParameterDouble(Model, Land, "Size of easily accessible SOC fraction", Dimensionless, 0.1, 0.0, 1.0, "Fractional size of easily accessible SOC relative to total SOC");
	auto SoilWaterBaselineDOCConcentration = RegisterParameterDouble(Model, Land, "Soil water baseline DOC concentration", MgPerL, 0.0, 0.0, 20.0);
	auto SoilWaterDOCTemperatureResponse   = RegisterParameterDouble(Model, Land, "Soil water DOC temperature response", MgPerLPerC, 0.0, 0.0, 5.0);
	auto MineralLayerDOCConcentration      = RegisterParameterDouble(Model, Reach, "Mineral layer DOC concentration", MgPerL, 0.0, 0.0, 20.0);
	
	auto GrainClass = RegisterParameterGroup(Model, "Carbon by grain class", Class);
	
	auto GrainSOCDensity                   = RegisterParameterDouble(Model, GrainClass, "Grain SOC density", KgPerKg, 0.0, 0.0, 1.0);
	
	
	//PERSiST.h :
	auto WaterDepth            = GetEquationHandle(Model, "Water depth");
	auto RunoffToReach         = GetEquationHandle(Model, "Runoff to reach");   
	auto PercolationInput      = GetEquationHandle(Model, "Percolation input");
	auto ReachFlow             = GetEquationHandle(Model, "Reach flow");
	auto ReachVolume           = GetEquationHandle(Model, "Reach volume");
	
	//IncaSed.h :
	auto AreaScaledGrainDeliveryToReach = GetEquationHandle(Model, "Area scaled sediment delivery to reach");
	auto SuspendedGrainMass   = GetEquationHandle(Model, "Suspended sediment mass");
	auto GrainDeposition      = GetEquationHandle(Model, "Sediment deposition");
	auto GrainEntrainment     = GetEquationHandle(Model, "Sediment entrainment");
	auto MassOfBedGrainPerUnitArea = GetEquationHandle(Model, "Mass of bed sediment per unit area");
	auto PercentageOfSedimentInGrainSizeClass = GetParameterDoubleHandle(Model, "Percentage of sediment in grain size class");
	
	//SoilTemperature.h
	auto SoilTemperature      = GetEquationHandle(Model, "Soil temperature");
	
	auto Percent                  = GetParameterDoubleHandle(Model, "%");
	auto TerrestrialCatchmentArea = GetParameterDoubleHandle(Model, "Terrestrial catchment area");
	
	auto SoilWaterDOCConcentration = RegisterEquation(Model, "Soil water DOC concentration", MgPerL);
	auto SoilDOCMass               = RegisterEquation(Model, "Soil DOC mass", KgPerKm2);
	auto SoilDOCFluxToReach        = RegisterEquation(Model, "Soil DOC flux to reach", KgPerKm2PerDay);
	auto SoilDOCFluxToGroundwater  = RegisterEquation(Model, "Soil DOC flux to groundwater", KgPerKm2PerDay);
	auto GroundwaterDOCFluxToReach = RegisterEquation(Model, "Groundwater DOC flux to reach", KgPerKm2PerDay);
	
	auto DiffuseDOCOutput          = RegisterEquation(Model, "Diffuse DOC output", KgPerDay);
	auto TotalDiffuseDOCOutput     = RegisterEquationCumulative(Model, "Total diffuse DOC output", DiffuseDOCOutput, LandscapeUnits);
	
	auto SOCDeliveryToReach        = RegisterEquation(Model, "SOC delivery to reach by erosion per grain class and landscape unit", KgPerDay);
	auto SOCDeliveryToReachPerLU   = RegisterEquationCumulative(Model, "SOC delivery to reach per landscape unit", SOCDeliveryToReach, Class);
	auto TotalSOCDeliveryToReach   = RegisterEquationCumulative(Model, "Total SOC delivery to reach", SOCDeliveryToReachPerLU, LandscapeUnits);
	
	auto ReachSolver = GetSolverHandle(Model, "Reach solver");
	auto ReachDOCMass              = RegisterEquationODE(Model, "Reach DOC mass", Kg);
	SetSolver(Model, ReachDOCMass, ReachSolver);
	//SetInitialValue
	auto ReachDOCOutput            = RegisterEquation(Model, "Reach DOC output", KgPerDay);
	SetSolver(Model, ReachDOCOutput, ReachSolver);
	auto ReachDOCInput             = RegisterEquation(Model, "Reach DOC input", KgPerDay);
	auto ReachDOCConcentration     = RegisterEquation(Model, "Reach DOC concentration", MgPerL);
	auto ReachSuspendedSOCMass     = RegisterEquation(Model, "Reach suspended SOC mass", Kg);
	auto BedSOCMass                = RegisterEquation(Model, "Stream bed SOC mass", KgPerM2);
	auto ReachSOCDeposition        = RegisterEquation(Model, "Reach SOC deposition", KgPerM2PerDay);
	auto ReachSOCEntrainment       = RegisterEquation(Model, "Reach SOC entrainment", KgPerM2PerDay);
	auto ReachSuspendedSOCFlux     = RegisterEquation(Model, "Reach suspended SOC flux", KgPerDay);
	
	auto TotalSuspendedSOCMass     = RegisterEquationCumulative(Model, "Total suspended SOC mass", ReachSuspendedSOCMass, Class);
	auto TotalBedSOCMass           = RegisterEquationCumulative(Model, "Total stream bed SOC mass", BedSOCMass, Class);
	auto TotalSOCDeposition        = RegisterEquationCumulative(Model, "Total reach SOC deposition", ReachSOCDeposition, Class);
	auto TotalSOCEntrainment       = RegisterEquationCumulative(Model, "Total reach SOC entrainment", ReachSOCEntrainment, Class);
	auto TotalSOCFlux              = RegisterEquationCumulative(Model, "Total reach SOC flux", ReachSuspendedSOCFlux, Class);
	
	
	EQUATION(Model, SoilWaterDOCConcentration,
		return PARAMETER(SoilWaterBaselineDOCConcentration) + PARAMETER(SoilWaterDOCTemperatureResponse)*RESULT(SoilTemperature);
	)
	
	EQUATION(Model, SoilDOCMass,
		return RESULT(SoilWaterDOCConcentration) * RESULT(WaterDepth, Soilwater); //NOTE: convert mm * mg/l  to kg/km2 (factor is 1)
	)
	
	EQUATION(Model, SoilDOCFluxToReach,
		return (RESULT(RunoffToReach, Soilwater) + RESULT(RunoffToReach, DirectRunoff)) * RESULT(SoilWaterDOCConcentration); //NOTE: convert mm * mg/l  to kg/km2 (factor is 1)
	)
	
	//NOTE: this is currently not used to modify DOC concentration in the groundwater, it is just here for contaminant transport.
	EQUATION(Model, SoilDOCFluxToGroundwater,
		return RESULT(PercolationInput, Groundwater) * RESULT(SoilWaterDOCConcentration); //NOTE: convert mm * mg/l  to kg/km2 (factor is 1)
	)
	
	EQUATION(Model, GroundwaterDOCFluxToReach,
		return RESULT(RunoffToReach, Groundwater) * PARAMETER(MineralLayerDOCConcentration); //NOTE: convert mm * mg/l  to kg/km2 (factor is 1)
	)
	
	//TODO: Transport by direct runoff (infiltration excess)?
	
	EQUATION(Model, SOCDeliveryToReach,
		return RESULT(AreaScaledGrainDeliveryToReach) * PARAMETER(PercentageOfSedimentInGrainSizeClass) * 0.01 * PARAMETER(GrainSOCDensity);
	)
	
	EQUATION(Model, DiffuseDOCOutput,
		return PARAMETER(Percent) * 0.01 * PARAMETER(TerrestrialCatchmentArea) *
		(
		  RESULT(SoilDOCFluxToReach)
		+ RESULT(GroundwaterDOCFluxToReach)
		);
	)
	
	EQUATION(Model, ReachDOCOutput,
		return 86400.0 * RESULT(ReachFlow) * SafeDivide(RESULT(ReachDOCMass), RESULT(ReachVolume));
	)
	
	EQUATION(Model, ReachDOCInput,
		double upstreamdoc = 0.0;
		for(index_t Input : BRANCH_INPUTS(Reaches))
			upstreamdoc += RESULT(ReachDOCOutput, Input);
		
		double diffusedoc = RESULT(TotalDiffuseDOCOutput);
	
		return diffusedoc + upstreamdoc;
	)
	
	EQUATION(Model, ReachDOCMass,
		//TODO: effluent and abstraction
		return RESULT(ReachDOCInput) - RESULT(ReachDOCOutput);
	)
	
	EQUATION(Model, ReachDOCConcentration,
		return SafeDivide(RESULT(ReachDOCMass), RESULT(ReachVolume)) * 1000.0; // Convert kg/m3 -> mg/l
	)
	
	
	EQUATION(Model, ReachSuspendedSOCMass,
		return RESULT(SuspendedGrainMass) * PARAMETER(GrainSOCDensity);
	)
	
	EQUATION(Model, ReachSuspendedSOCFlux,
		return 86400.0 * RESULT(ReachFlow) * SafeDivide(RESULT(ReachSuspendedSOCMass), RESULT(ReachVolume));
	)
	
	EQUATION(Model, BedSOCMass,
		return RESULT(MassOfBedGrainPerUnitArea) * PARAMETER(GrainSOCDensity);
	)
	
	EQUATION(Model, ReachSOCDeposition,
		return RESULT(GrainDeposition) * PARAMETER(GrainSOCDensity);
	)
	
	EQUATION(Model, ReachSOCEntrainment,
		return RESULT(GrainEntrainment) * PARAMETER(GrainSOCDensity);
	)
	
	
	
	EndModule(Model);
}