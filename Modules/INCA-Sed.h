

// NOTE NOTE NOTE This module is in development and is not finished!!!!

#include "../boost_solvers.h"

static void
AddINCASedModel(mobius_model *Model)
{
	//NOTE: Is designed to work with PERSiST
	
	auto Dimensionless = RegisterUnit(Model);
	auto SPerM         = RegisterUnit(Model, "s/m");
	auto MPerS         = RegisterUnit(Model, "m/s");
	auto SPerM2        = RegisterUnit(Model, "s/m2");
	auto M3PerSPerKm2  = RegisterUnit(Model, "m3/s/km2");
	auto Kg            = RegisterUnit(Model, "kg");
	auto KgPerM2PerKm2 = RegisterUnit(Model, "kg/m2/km2");
	auto KgPerSPerKm2  = RegisterUnit(Model, "kg/s/km2");
	auto KgPerM2PerS   = RegisterUnit(Model, "kg/m2/s");
	auto PercentU      = RegisterUnit(Model, "%");
	auto KgPerKm2PerDay = RegisterUnit(Model, "kg/km2/day");
	auto KgPerDay      = RegisterUnit(Model, "kg/day");
	auto KgPerKm2      = RegisterUnit(Model, "kg/km2");
	auto KgPerM2       = RegisterUnit(Model, "kg/m2");
	auto KgPerM2PerDay = RegisterUnit(Model, "kg/m2/day");
	auto JPerSPerM2    = RegisterUnit(Model, "J/s/m2");
	auto MgPerL        = RegisterUnit(Model, "mg/L");
	auto Metres        = RegisterUnit(Model, "m");
	auto KgPerM2PerM3SPerDay = RegisterUnit(Model, "kg/m2/m3 s/day");
	auto S2PerKg       = RegisterUnit(Model, "s2/kg");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto SoilBoxes = GetIndexSetHandle(Model, "Soils");
	
	auto DirectRunoff = RequireIndex(Model, SoilBoxes, "Direct runoff");
	
	auto Sediment = RegisterParameterGroup(Model, "Sediments", LandscapeUnits);
	
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
	auto InitialSoilMassInTheOAHorizon          = RegisterParameterDouble(Model, Sediment, "Initial soil mass in the O/A horizon", KgPerKm2, 0.0, 0.0, 1000000.0);
	
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
	
	auto SoilMassInTheOAHorizon               = RegisterEquation(Model, "Soil mass in the O/A horizon", KgPerKm2);
	SetInitialValue(Model, SoilMassInTheOAHorizon, InitialSoilMassInTheOAHorizon);
	
	
	EQUATION(Model, SedimentTransportCapacity,
		double qquick = RESULT(RunoffToReach, DirectRunoff) / 86.4;
		double flow = Max(0.0, qquick - PARAMETER(TransportCapacityDirectRunoffThreshold));
		return 86400.0 * PARAMETER(TransportCapacityScalingFactor) 
			* pow((PARAMETER(SubcatchmentArea) / PARAMETER(ReachLength))*flow, PARAMETER(TransportCapacityNonlinearCoefficient));
	)
	
	//TODO: Documentation says this should use Reffq = "effective precipitation". Is that the same as rainfall?
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
		double Mland = RESULT(SedimentDeliveryToReach);
		
		double SSDthatwastransported = Min(SSD, RESULT(SedimentTransportCapacity));
		
		return LAST_RESULT(SurfaceSedimentStore) + SSD - SSDthatwastransported;
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
	auto LargestDiameterOfSedimentClass         = RegisterParameterDouble(Model, SedimentSizeClass, "Largest diameter of sediment in size class", Metres, 2e-6), 0.0, 1.0;
	
	auto SedimentReach = RegisterParameterGroup(Model, "Sediment reach", Reach);
	SetParentGroup(Model, SedimentReach, SedimentSizeClass);
	
	auto EffluentSedimentConcentration   = RegisterParameterDouble(Model, SedimentReach, "Effluent sediment concentration", MgPerL, 0.0);
	
	auto Reaches = GetParameterGroupHandle(Model, "Reaches");
	
	auto BankErosionScalingFactor        = RegisterParameterDouble(Model, Reaches, "Bank erosion scaling factor", KgPerM2PerM3SPerDay, 1.0);
	auto BankErosionNonlinearCoefficient = RegisterParameterDouble(Model, Reaches, "Bank erosion non-linear coefficient", Dimensionless, 1.0);
	auto ShearVelocityCoefficient        = RegisterParameterDouble(Model, Reaches, "Shear velocity coefficient", Dimensionless, 1.0);
	auto MeanChannelSlope                = RegisterParameterDouble(Model, Reaches, "Mean channel slope", Dimensionless, 2.0);
	auto EntrainmentCoefficient          = RegisterParameterDouble(Model, Reaches, "Entrainment coefficient", S2PerKg, 1.0);
	auto InitialMassOfBedSedimentPerUnitArea = RegisterParameterDouble(Model, SedimentReach, "Initial mass of bed sediment per unit area", KgPerM2, 10);
	
	auto InitialSuspendedSedimentMass    = RegisterParameterDouble(Model, SedimentReach, "Initial suspended sediment mass", Kg, 1e2);
	
	auto SedimentOfSizeClassDeliveredToReach = RegisterEquation(Model, "Sediment of size class delivered to reach", KgPerDay);
	auto ReachUpstreamSuspendedSediment     = RegisterEquation(Model, "Reach upstream suspended sediment", KgPerDay);
	auto ClayReleaseFromChannelBanks        = RegisterEquation(Model, "Clay release from channel banks", KgPerM2PerDay);
	auto ReachFrictionFactor                = RegisterEquation(Model, "Reach friction factor", Dimensionless);
	auto ReachShearVelocity                 = RegisterEquation(Model, "Reach shear velocity", MPerS);
	auto ProportionOfSedimentThatCanBeEntrained = RegisterEquation(Model, "Proportion of sediment that can be entrained", Dimensionless);
	auto StreamPower                        = RegisterEquation(Model, "Stream power", JPerSPerM2);
	
	auto SedimentEntrainment                = RegisterEquation(Model, "Sediment entrainment", KgPerM2PerDay);
	SetSolver(Model, SedimentEntrainment, InstreamSedimentSolver);
	
	auto SedimentDeposition                 = RegisterEquation(Model, "Sediment deposition", KgPerM2PerDay);
	SetSolver(Model, SedimentDeposition, InstreamSedimentSolver);
	
	auto ReachSuspendedSedimentOutput       = RegisterEquation(Model, "Reach suspended sediment output", KgPerDay);
	SetSolver(Model, ReachSuspendedSedimentOutput, InstreamSedimentSolver);
	
	auto MassOfBedSedimentPerUnitArea       = RegisterEquationODE(Model, "Mass of bed sediment per unit area", KgPerM2);
	SetInitialValue(Model, MassOfBedSedimentPerUnitArea, InitialMassOfBedSedimentPerUnitArea);
	SetSolver(Model, MassOfBedSedimentPerUnitArea, InstreamSedimentSolver);
	
	auto SuspendedSedimentMass = RegisterEquationODE(Model, "Suspended sediment mass", Kg);
	SetInitialValue(Model, SuspendedSedimentMass, InitialSuspendedSedimentMass);
	SetSolver(Model, SuspendedSedimentMass, InstreamSedimentSolver);
	
	
	
	auto ReachWidth = GetParameterDoubleHandle(Model, "Reach width");
	auto EffluentFlow = GetParameterDoubleHandle(Model, "Effluent flow");
	auto ReachDepth = GetEquationHandle(Model, "Reach depth");
	auto ReachFlow  = GetEquationHandle(Model, "Reach flow");
	auto ReachVolume = GetEquationHandle(Model, "Reach volume");
	auto ReachVelocity = GetEquationHandle(Model, "Reach velocity");
	
	
	
	EQUATION(Model, SedimentOfSizeClassDeliveredToReach,
		return RESULT(TotalSedimentDeliveryToReach) * PARAMETER(PercentageOfSedimentInGrainSizeClass) / 100.0;
	)
	
	EQUATION(Model, ReachUpstreamSuspendedSediment,
		//CURRENT_INDEX(SizeClass); //TODO: Has to be here until we fix the dependency system some more..
		double sum = 0.0;
		FOREACH_INPUT(Reach,
			sum += RESULT(ReachSuspendedSedimentOutput, *Input);
		)
		return sum;
	)
	
	EQUATION(Model, ReachSuspendedSedimentOutput,
		return 86400.0 * RESULT(SuspendedSedimentMass) * RESULT(ReachFlow) / RESULT(ReachVolume);
	)
	
	EQUATION(Model, ClayReleaseFromChannelBanks,
		return PARAMETER(BankErosionScalingFactor) * pow(RESULT(ReachFlow), PARAMETER(BankErosionNonlinearCoefficient));
	)
	
	EQUATION(Model, ReachFrictionFactor,
		return 4.0 * RESULT(ReachDepth) / (2.0 * RESULT(ReachDepth) + PARAMETER(ReachWidth));
	)
	
	EQUATION(Model, ReachShearVelocity,
		double earthsurfacegravity = 9.807;
		return sqrt(earthsurfacegravity * RESULT(ReachDepth) * PARAMETER(ShearVelocityCoefficient) * PARAMETER(MeanChannelSlope));
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
		return waterdensity * earthsurfacegravity * PARAMETER(MeanChannelSlope) * RESULT(ReachVelocity) * RESULT(ReachDepth);
	)
	
	EQUATION(Model, SedimentEntrainment,
		double value = 86400.0 * PARAMETER(EntrainmentCoefficient) * RESULT(MassOfBedSedimentPerUnitArea) * RESULT(ProportionOfSedimentThatCanBeEntrained) * RESULT(StreamPower) * RESULT(ReachFrictionFactor);
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
	
	EQUATION(Model, SuspendedSedimentMass,
		double clayrelease = RESULT(ClayReleaseFromChannelBanks);
		if(CURRENT_INDEX(SizeClass) != 0) clayrelease = 0.0;      //NOTE: This assumes that index 0 is always clay though...
		
		return 
			  RESULT(SedimentOfSizeClassDeliveredToReach) 
			+ PARAMETER(EffluentSedimentConcentration) * PARAMETER(EffluentFlow)
			+ RESULT(ReachUpstreamSuspendedSediment) 
			- RESULT(ReachSuspendedSedimentOutput) 
			+ PARAMETER(ReachLength) * PARAMETER(ReachWidth) * (RESULT(SedimentEntrainment) + clayrelease - RESULT(SedimentDeposition));
	)
}


