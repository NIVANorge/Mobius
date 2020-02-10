



//NOTE: This is a simple DOC module (simpler than INCA-C) built to be used with INCA-Tox.
// This is built on top of PERSiST and INCA-Sed

// This module does not try to keep track of the carbon balance or soil processes, it just provides a way to set up a simple empirical model of DOC transport in order for contaminants bound in the carbon to be transported correctly.


//NOTE: THIS MODULE IS IN DEVELOPMENT!


static void
AddIncaToxDOCModule(mobius_model *Model)
{
	BeginModule(Model, "INCA-Tox Carbon", "0.1");
	
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
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto Reaches      = GetIndexSetHandle(Model, "Reaches");
	auto Soils        = GetIndexSetHandle(Model, "Soils");
	auto Class        = GetIndexSetHandle(Model, "Sediment size class");
	auto DirectRunoff = RequireIndex(Model, Soils, "Direct runoff");
	auto Soilwater    = RequireIndex(Model, Soils, "Soil water");
	auto Groundwater  = RequireIndex(Model, Soils, "Groundwater");
	
	
	auto Land = RegisterParameterGroup(Model, "Carbon by land class", LandscapeUnits);
	auto Reach = RegisterParameterGroup(Model, "Carbon by subcatchment", Reaches);
	
	auto SoilSOCMass                       = RegisterParameterDouble(Model, Land, "Soil SOC mass", KgPerKm2, 0.0, 0.0, 1e7);
	auto SoilWaterBaselineDOCConcentration = RegisterParameterDouble(Model, Land, "Soil water baseline DOC concentration", MgPerL, 0.0, 0.0, 20.0);
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
	
	
	auto Percent                  = GetParameterDoubleHandle(Model, "%");
	auto TerrestrialCatchmentArea = GetParameterDoubleHandle(Model, "Terrestrial catchment area");
	
	auto SoilWaterDOCConcentration = RegisterEquation(Model, "Soil water DOC concentration", MgPerL);
	auto SoilDOCMass               = RegisterEquation(Model, "Soil DOC mass", KgPerKm2);
	auto SoilDOCFluxToReach        = RegisterEquation(Model, "Soil DOC flux to reach", KgPerKm2PerDay);
	auto SoilDOCFluxToGroundwater  = RegisterEquation(Model, "Soil DOC flux to groundwater", KgPerKm2PerDay);
	auto GroundwaterDOCFluxToReach = RegisterEquation(Model, "Groundwater DOC flux to reach", KgPerKm2PerDay);
	
	auto DiffuseDOCOutput          = RegisterEquation(Model, "Diffuse DOC output", KgPerDay);
	auto TotalDiffuseDOCOutput     = RegisterEquationCumulative(Model, "Total diffuse DOC output", DiffuseDOCOutput, LandscapeUnits);
	
	auto SOCDeliveryToReach        = RegisterEquation(Model, "SOC delivery to reach by erosion", KgPerDay);
	
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
	
	
	EQUATION(Model, SoilWaterDOCConcentration,
		//TODO: Temperature and SO4 controls?
		return PARAMETER(SoilWaterBaselineDOCConcentration);
	)
	
	EQUATION(Model, SoilDOCMass,
		return RESULT(SoilWaterDOCConcentration) * RESULT(WaterDepth, Soilwater); //NOTE: convert mm * mg/l  to kg/km2 (factor is 1)
	)
	
	EQUATION(Model, SoilDOCFluxToReach,
		return RESULT(RunoffToReach, Soilwater) * RESULT(SoilWaterDOCConcentration); //NOTE: convert mm * mg/l  to kg/km2 (factor is 1)
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
		return RESULT(AreaScaledGrainDeliveryToReach) * PARAMETER(GrainSOCDensity);
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
		FOREACH_INPUT(Reaches,
			upstreamdoc += RESULT(ReachDOCOutput, *Input);
		)
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