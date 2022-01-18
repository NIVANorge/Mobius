
//NOTE: Is designed to work with PERSiST 1.4 or later

#include "../../Src/boost_solvers.h"

static void
AddINCASedModel(mobius_model *Model)
{
	BeginModule(Model, "INCA-Sed", "0.9");
	
	SetModuleDescription(Model, R""""(
The most up-to date description can be found in

[Lazar, A. N. et. al. 2010, An assessment of the fine sediment dynamics in an upland river system: INCA-Sed modifications and implications for fisheries. Sci. Tot. En. 408, 2555-2566](https://doi.org/10.1016/j.scitotenv.2010.02.030)

A notable difference with this implementation is that it is rigged to be run in combination with the PERSiST hydrology model.
)"""");
	
	
	auto Dimensionless = RegisterUnit(Model);
	auto SPerM         = RegisterUnit(Model, "s/m");
	auto MPerS         = RegisterUnit(Model, "m/s");
	auto SPerM2        = RegisterUnit(Model, "s/m^2");
	auto M3PerSPerKm2  = RegisterUnit(Model, "m^3/s/km^2");
	auto Kg            = RegisterUnit(Model, "kg");
	auto KgPerM2PerKm2 = RegisterUnit(Model, "kg/m^2/km^2");
	auto KgPerSPerKm2  = RegisterUnit(Model, "kg/s/km^2");
	auto KgPerM2PerS   = RegisterUnit(Model, "kg/m^2/s");
	auto PercentU      = RegisterUnit(Model, "%");
	auto KgPerKm2PerDay = RegisterUnit(Model, "kg/km^2/day");
	auto KgPerDay      = RegisterUnit(Model, "kg/day");
	auto KgPerKm2      = RegisterUnit(Model, "kg/km^2");
	auto KgPerM2       = RegisterUnit(Model, "kg/m^2");
	auto KgPerM2PerDay = RegisterUnit(Model, "kg/m^2/day");
	auto JPerSPerM2    = RegisterUnit(Model, "J/s/m^2");
	auto MgPerL        = RegisterUnit(Model, "mg/l");
	auto Metres        = RegisterUnit(Model, "m");
	auto KgPerM2PerM3SPerDay = RegisterUnit(Model, "kg/m^2/m^3 s/day");
	auto S2PerKg       = RegisterUnit(Model, "s^2/kg");
	auto M             = RegisterUnit(Model, "m");
	auto KgPerM3       = RegisterUnit(Model, "kg/m^3");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto SoilBoxes = GetIndexSetHandle(Model, "Soils");
	
	auto DirectRunoff = RequireIndex(Model, SoilBoxes, "Direct runoff");
	
	auto Sediment = RegisterParameterGroup(Model, "Sediment mobilisation", LandscapeUnits);
	
	//TODO : Find default/min/max/description for these e.g. in the INCA-P documentation
	
	//NOTE: The first 6 of these should index over reach instead, but that seems to break things.. Fix!
	auto FlowErosionScalingFactor               = RegisterParameterDouble(Model, Sediment, "Flow erosion scaling factor", SPerM2, 1.0);
	auto FlowErosionDirectRunoffThreshold       = RegisterParameterDouble(Model, Sediment, "Flow erosion direct runoff threshold", M3PerSPerKm2, 0.001);
	auto FlowErosionNonlinearCoefficient        = RegisterParameterDouble(Model, Sediment, "Flow erosion non-linear coefficient", Dimensionless, 1.0);
	auto TransportCapacityScalingFactor         = RegisterParameterDouble(Model, Sediment, "Transport capacity scaling factor", KgPerM2PerKm2, 1.0);
	auto TransportCapacityDirectRunoffThreshold = RegisterParameterDouble(Model, Sediment, "Transport capacity direct runoff threshold", M3PerSPerKm2, 0.001);
	auto TransportCapacityNonlinearCoefficient  = RegisterParameterDouble(Model, Sediment, "Transport capacity non-linear coefficient", Dimensionless, 1.0);
	
	auto SplashDetachmentScalingFactor          = RegisterParameterDouble(Model, Sediment, "Splash detachment scaling factor", SPerM, 0.001);
	auto FlowErosionPotential                   = RegisterParameterDouble(Model, Sediment, "Flow erosion potential", KgPerSPerKm2, 0.074);
	auto SplashDetachmentSoilErodibility        = RegisterParameterDouble(Model, Sediment, "Splash detachment soil erodibility", KgPerM2PerS, 1.0);
	auto VegetationIndex                        = RegisterParameterDouble(Model, Sediment, "Vegetation index", Dimensionless, 1.0);
	
	auto InitialSurfaceSedimentStore            = RegisterParameterDouble(Model, Sediment, "Initial surface sediment store", KgPerKm2, 0.0, 0.0, 10.0);
	auto SoilDepthInTheOAHorizon                = RegisterParameterDouble(Model, Sediment, "Average soil depth in the O/A horizon", M, 2.0);
	auto SoilBulkDensity                        = RegisterParameterDouble(Model, Sediment, "Soil bulk density", KgPerM3, 600.0);
	
	auto SubcatchmentArea = GetParameterDoubleHandle(Model, "Terrestrial catchment area");
	auto ReachLength      = GetParameterDoubleHandle(Model, "Reach length");
	auto Percent          = GetParameterDoubleHandle(Model, "%");
	
	auto Rainfall = GetEquationHandle(Model, "Rainfall");
	auto RunoffToReach = GetEquationHandle(Model, "Runoff to reach");
	
	
	///////////// Erosion and sediment transport ////////////////
	
	auto SedimentMobilisedViaSplashDetachment = RegisterEquation(Model, "Sediment mobilised via splash detachment", KgPerKm2PerDay);
	auto SedimentMobilisedViaFlowErosion      = RegisterEquation(Model, "Sediment mobilised via flow erosion", KgPerKm2PerDay);
	auto FlowErosionKFactor                   = RegisterEquation(Model, "Flow erosion K factor", KgPerKm2PerDay);
	auto SedimentTransportCapacity            = RegisterEquation(Model, "Sediment transport capacity", KgPerKm2PerDay);
	
	auto SurfaceSedimentStore   = RegisterEquation(Model, "Surface sediment store", KgPerKm2);
	SetInitialValue(Model, SurfaceSedimentStore, InitialSurfaceSedimentStore);
	
	auto SedimentDeliveryToReach              = RegisterEquation(Model, "Sediment delivery to reach", KgPerKm2PerDay);
	auto AreaScaledSedimentDeliveryToReach    = RegisterEquation(Model, "Area scaled sediment delivery to reach", KgPerDay);
	
	auto TotalSedimentDeliveryToReach         = RegisterEquationCumulative(Model, "Total sediment delivery to reach", AreaScaledSedimentDeliveryToReach, LandscapeUnits);
	
	auto InitialSoilMassInTheOAHorizon        = RegisterEquationInitialValue(Model, "Initial soil mass in the O/A horizon", KgPerKm2);
	auto SoilMassInTheOAHorizon               = RegisterEquation(Model, "Soil mass in the O/A horizon", KgPerKm2);
	SetInitialValue(Model, SoilMassInTheOAHorizon, InitialSoilMassInTheOAHorizon);
	
	
	EQUATION(Model, InitialSoilMassInTheOAHorizon,
		return 1e6 * PARAMETER(SoilDepthInTheOAHorizon) * PARAMETER(SoilBulkDensity);
	)
	
	
	EQUATION(Model, SedimentTransportCapacity,
		double qquick = RESULT(RunoffToReach, DirectRunoff) / 86.4;
		double flow = Max(0.0, qquick - PARAMETER(TransportCapacityDirectRunoffThreshold));
		return 86400.0 * PARAMETER(TransportCapacityScalingFactor) 
			* pow((PARAMETER(SubcatchmentArea) / PARAMETER(ReachLength))*flow, PARAMETER(TransportCapacityNonlinearCoefficient));
	)
	
	EQUATION(Model, SedimentMobilisedViaSplashDetachment,
		double Reffq = RESULT(Rainfall) / 86.4;
		return 86400.0 * PARAMETER(SplashDetachmentScalingFactor) * Reffq * pow(PARAMETER(SplashDetachmentSoilErodibility), 10.0 / (10.0 - PARAMETER(VegetationIndex)));
	)
	
	EQUATION(Model, FlowErosionKFactor,
		double qquick = RESULT(RunoffToReach, DirectRunoff) / 86.4;
		double flow = Max(0.0, qquick - PARAMETER(FlowErosionDirectRunoffThreshold));
		return 86400.0 * PARAMETER(FlowErosionScalingFactor) * PARAMETER(FlowErosionPotential) 
			* pow( (1e-3 * PARAMETER(SubcatchmentArea) / PARAMETER(ReachLength))*flow, PARAMETER(FlowErosionNonlinearCoefficient));
	)
	
	EQUATION(Model, SedimentMobilisedViaFlowErosion,
		double TC = RESULT(SedimentTransportCapacity);
		
		double SFE = RESULT(FlowErosionKFactor)
				* SafeDivide(TC - RESULT(SedimentMobilisedViaSplashDetachment),
							TC + RESULT(FlowErosionKFactor));
		SFE = Max(0.0, SFE);
		
		double SSD = RESULT(SedimentMobilisedViaSplashDetachment);
		
		double surfacestorebeforeerosion = LAST_RESULT(SurfaceSedimentStore) + SSD;
	
		if(TC > surfacestorebeforeerosion)
		{
			double remainingcap = TC - surfacestorebeforeerosion;
			return Min(remainingcap, SFE);
		}
		
		return 0.0;
	)
	
	EQUATION(Model, SedimentDeliveryToReach,
		double SSD = RESULT(SedimentMobilisedViaSplashDetachment);
		double SFE = RESULT(SedimentMobilisedViaFlowErosion);
		double TC = RESULT(SedimentTransportCapacity);
		
		double surfacestorebeforeerosion = LAST_RESULT(SurfaceSedimentStore) + SSD;
	
		if(surfacestorebeforeerosion > TC)
		{
			return TC;
		}
		return surfacestorebeforeerosion + SFE;
	)
	
	EQUATION(Model, AreaScaledSedimentDeliveryToReach,
		return PARAMETER(SubcatchmentArea) * PARAMETER(Percent) / 100.0 * RESULT(SedimentDeliveryToReach);
	)
	
	EQUATION(Model, SurfaceSedimentStore,
		double SSD = RESULT(SedimentMobilisedViaSplashDetachment);
		
		double surfacestorebeforeerosion = LAST_RESULT(SurfaceSedimentStore) + SSD;
		
		return Max(0.0, surfacestorebeforeerosion - RESULT(SedimentTransportCapacity));
	)
	
	EQUATION(Model, SoilMassInTheOAHorizon,
		return LAST_RESULT(SoilMassInTheOAHorizon) - RESULT(SedimentDeliveryToReach);
	)
	
	///////////////// Suspended sediment ////////////////////////////////
	
	auto InstreamSedimentSolver = RegisterSolver(Model, "In-stream sediment solver", 0.1, BoostRosenbrock4, 1e-3, 1e-3);
	
	auto SizeClass = RegisterIndexSet(Model, "Sediment size class");
	
	auto SedimentSizeClass = RegisterParameterGroup(Model, "Sediment size class", SizeClass);
	
	auto PercentageOfSedimentInGrainSizeClass   = RegisterParameterDouble(Model, SedimentSizeClass, "Percentage of sediment in grain size class", PercentU, 20);
	auto SmallestDiameterOfSedimentClass        = RegisterParameterDouble(Model, SedimentSizeClass, "Smallest diameter of sediment in size class", Metres, 0.0, 0.0, 1.0);
	auto LargestDiameterOfSedimentClass         = RegisterParameterDouble(Model, SedimentSizeClass, "Largest diameter of sediment in size class", Metres, 2e-6, 0.0, 1.0);
	
	auto SedimentReach = RegisterParameterGroup(Model, "Sediment by subcatchment", SizeClass, Reach);
	
	auto EffluentSedimentConcentration   = RegisterParameterDouble(Model, SedimentReach, "Effluent sediment concentration", MgPerL, 0.0);
	auto EffluentSedimentConcentrationTimeseries = RegisterInput(Model, "Effluent sediment concentration", MgPerL);
	
	auto Reaches = RegisterParameterGroup(Model, "Erosion and transport by subcatchment", Reach);
	
	auto BankErosionScalingFactor        = RegisterParameterDouble(Model, Reaches, "Bank erosion scaling factor", KgPerM2PerM3SPerDay, 1.0);
	auto BankErosionNonlinearCoefficient = RegisterParameterDouble(Model, Reaches, "Bank erosion non-linear coefficient", Dimensionless, 1.0);
	auto ShearVelocityCoefficient        = RegisterParameterDouble(Model, Reaches, "Shear velocity coefficient", Dimensionless, 1.0);
	//auto MeanChannelSlope                = RegisterParameterDouble(Model, Reaches, "Mean channel slope", Dimensionless, 2.0);
	auto EntrainmentCoefficient          = RegisterParameterDouble(Model, Reaches, "Entrainment coefficient", S2PerKg, 1.0);
	auto InitialMassOfBedSedimentPerUnitArea = RegisterParameterDouble(Model, SedimentReach, "Initial mass of bed sediment per unit area", KgPerM2, 10);
	
	auto InitialSuspendedSedimentConcentration    = RegisterParameterDouble(Model, SedimentReach, "Initial suspended sediment concentration", MgPerL, 0.001);
	
	auto SedimentOfSizeClassDeliveredToReach = RegisterEquation(Model, "Sediment of size class delivered to reach", KgPerDay);
	auto ReachUpstreamSuspendedSediment     = RegisterEquation(Model, "Reach upstream suspended sediment", KgPerDay);
	auto ClayReleaseFromChannelBanks        = RegisterEquation(Model, "Clay release from channel banks", KgPerM2PerDay);
	auto ReachShearVelocity                 = RegisterEquation(Model, "Reach shear velocity", MPerS);
	auto ProportionOfSedimentThatCanBeEntrained = RegisterEquation(Model, "Proportion of sediment that can be entrained", Dimensionless);
	auto StreamPower                        = RegisterEquation(Model, "Stream power", JPerSPerM2);
	
	auto SedimentEntrainment                = RegisterEquation(Model, "Sediment entrainment", KgPerM2PerDay, InstreamSedimentSolver);
	auto SedimentDeposition                 = RegisterEquation(Model, "Sediment deposition", KgPerM2PerDay, InstreamSedimentSolver);
	auto ReachSuspendedSedimentOutput       = RegisterEquation(Model, "Reach suspended sediment output", KgPerDay, InstreamSedimentSolver);
	
	auto MassOfBedSedimentPerUnitArea       = RegisterEquationODE(Model, "Mass of bed sediment per unit area", KgPerM2, InstreamSedimentSolver);
	SetInitialValue(Model, MassOfBedSedimentPerUnitArea, InitialMassOfBedSedimentPerUnitArea);
	
	auto InitialSuspendedSedimentMass = RegisterEquationInitialValue(Model, "Initial suspended sediment mass", Kg);
	auto SuspendedSedimentMass = RegisterEquationODE(Model, "Suspended sediment mass", Kg, InstreamSedimentSolver);
	SetInitialValue(Model, SuspendedSedimentMass, InitialSuspendedSedimentMass);
	
	auto MassOfBedSediment = RegisterEquation(Model, "Mass of bed sediment", Kg);
	
	auto TotalBedSedimentMass       = RegisterEquationCumulative(Model, "Total bed sediment mass", MassOfBedSediment, SizeClass);
	auto TotalBedSedimentMassPerUnitArea = RegisterEquationCumulative(Model, "Total mass of bed sediment per unit area", MassOfBedSedimentPerUnitArea, SizeClass);
	auto TotalSuspendedSedimentMass = RegisterEquationCumulative(Model, "Total suspended sediment mass", SuspendedSedimentMass, SizeClass);
	auto TotalEntrainment           = RegisterEquationCumulative(Model, "Total sediment entrainment", SedimentEntrainment, SizeClass);
	auto TotalDeposition            = RegisterEquationCumulative(Model, "Total sediment deposition", SedimentDeposition, SizeClass);
	
	auto ReachWidth = GetParameterDoubleHandle(Model, "Reach bottom width");
	auto EffluentFlow = GetParameterDoubleHandle(Model, "Effluent flow");
	auto MeanChannelSlope = GetParameterDoubleHandle(Model, "Reach slope");
	
	auto ReachHydraulicRadius = GetEquationHandle(Model, "Reach hydraulic radius");
	auto ReachFlow  = GetEquationHandle(Model, "Reach flow");
	auto ReachVolume = GetEquationHandle(Model, "Reach volume");
	auto ReachVelocity = GetEquationHandle(Model, "Reach velocity");
	
	auto EffluentTimeseries = GetInputHandle(Model, "Effluent flow");
	
	EQUATION(Model, InitialSuspendedSedimentMass,
		//NOTE: probably have to have the CURRENT_INDEX(Reach) for it to work correctly. Because of problem in initial value system.
		return 1e-3 * RESULT(ReachVolume, CURRENT_INDEX(Reach)) * PARAMETER(InitialSuspendedSedimentConcentration);
	)
	
	EQUATION(Model, SedimentOfSizeClassDeliveredToReach,
		return RESULT(TotalSedimentDeliveryToReach) * PARAMETER(PercentageOfSedimentInGrainSizeClass) / 100.0;
	)
	
	EQUATION(Model, ReachUpstreamSuspendedSediment,
		index_t SedClass = CURRENT_INDEX(SizeClass);
		
		double sum = 0.0;
		for(index_t Input : BRANCH_INPUTS(Reach))
			sum += RESULT(ReachSuspendedSedimentOutput, Input, SedClass);
		
		return sum;
	)
	
	EQUATION(Model, ReachSuspendedSedimentOutput,
		return 86400.0 * RESULT(SuspendedSedimentMass) * RESULT(ReachFlow) / RESULT(ReachVolume);
	)
	
	EQUATION(Model, ClayReleaseFromChannelBanks,
		return PARAMETER(BankErosionScalingFactor) * pow(RESULT(ReachFlow), PARAMETER(BankErosionNonlinearCoefficient));
	)
	
	EQUATION(Model, ReachShearVelocity,
		double earthsurfacegravity = 9.807;
		return sqrt(earthsurfacegravity * RESULT(ReachHydraulicRadius) * PARAMETER(ShearVelocityCoefficient) * PARAMETER(MeanChannelSlope));
	)
	
	EQUATION(Model, ProportionOfSedimentThatCanBeEntrained,
		double Dmax = 9.99 * pow(RESULT(ReachShearVelocity), 2.52);
		double Dlow = PARAMETER(SmallestDiameterOfSedimentClass);
		double Dupp = PARAMETER(LargestDiameterOfSedimentClass);
		if(Dmax < Dlow) return 0.0;
		if(Dmax > Dupp) return 1.0;
		return (Dmax - Dlow) / (Dupp - Dlow);
	)
	
	EQUATION(Model, StreamPower,
		double waterdensity = 1000.0;
		double earthsurfacegravity = 9.807;
		return waterdensity * earthsurfacegravity * PARAMETER(MeanChannelSlope) * RESULT(ReachVelocity) * RESULT(ReachHydraulicRadius);
	)
	
	EQUATION(Model, SedimentEntrainment,
		double value = 86400.0 * PARAMETER(EntrainmentCoefficient) * RESULT(MassOfBedSedimentPerUnitArea) * RESULT(ProportionOfSedimentThatCanBeEntrained) * RESULT(StreamPower);
		return value;
	)
	
	EQUATION(Model, SedimentDeposition,
		double mediangrainsize = (PARAMETER(SmallestDiameterOfSedimentClass) + PARAMETER(LargestDiameterOfSedimentClass)) / 2.0;
		double sedimentdensity = 2650.0;
		double waterdensity    = 1000.0;
		double earthsurfacegravity = 9.807;
		double fluidviscosity = 0.001;
		double terminalsettlingvelocity = (sedimentdensity - waterdensity) * earthsurfacegravity * Square(mediangrainsize) / (18.0 * fluidviscosity);
		
		double value = 86400.0 * terminalsettlingvelocity * RESULT(SuspendedSedimentMass) / RESULT(ReachVolume);
		
		return value;
	)
	
	EQUATION(Model, MassOfBedSedimentPerUnitArea,
		return RESULT(SedimentDeposition) - RESULT(SedimentEntrainment);
	)
	
	EQUATION(Model, MassOfBedSediment,
		return RESULT(MassOfBedSedimentPerUnitArea) * PARAMETER(ReachWidth) * PARAMETER(ReachLength);
	)
	
	EQUATION(Model, SuspendedSedimentMass,
		double clayrelease = RESULT(ClayReleaseFromChannelBanks);
		if(CURRENT_INDEX(SizeClass) != 0) clayrelease = 0.0;      //NOTE: This assumes that index 0 is always clay though...
		
		//TODO: Abstraction??
		double effluentflow = IF_INPUT_ELSE_PARAMETER(EffluentTimeseries, EffluentFlow);
		double effluentconc = IF_INPUT_ELSE_PARAMETER(EffluentSedimentConcentrationTimeseries, EffluentSedimentConcentration);
		
		return 
			  RESULT(SedimentOfSizeClassDeliveredToReach) 
			+ effluentconc * effluentflow * 86.4
			+ RESULT(ReachUpstreamSuspendedSediment) 
			- RESULT(ReachSuspendedSedimentOutput) 
			+ PARAMETER(ReachLength) * PARAMETER(ReachWidth) * (RESULT(SedimentEntrainment) + clayrelease - RESULT(SedimentDeposition));
	)
	
	auto SuspendedSedimentConcentration = RegisterEquation(Model, "Total suspended sediment concentration", MgPerL);
	
	EQUATION(Model, SuspendedSedimentConcentration,
		return 1000.0 * SafeDivide(RESULT(TotalSuspendedSedimentMass), RESULT(ReachVolume));
	)
	
	
	EndModule(Model);
}


