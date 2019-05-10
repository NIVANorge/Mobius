

// NOTE NOTE NOTE This module is in development and is not finished!!!!

#include "../boost_solvers.h"

static void
AddINCAMicroplasticsModel(mobius_model *Model)
{
	//NOTE: Is designed to work with PERSiST
	
	auto Dimensionless = RegisterUnit(Model);
	auto SPerM         = RegisterUnit(Model, "s/m");
	auto MPerS         = RegisterUnit(Model, "m/s");
	auto SPerM2        = RegisterUnit(Model, "s/m2");
	auto M3PerSPerKm2  = RegisterUnit(Model, "m3/s/km2");
	auto Kg            = RegisterUnit(Model, "kg");
	auto KgPerM3       = RegisterUnit(Model, "kg/m3");
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
	
	auto Reaches = GetParameterGroupHandle(Model, "Reaches");
	
	//TODO : Find default/min/max/description for these
	
	//TODO: Some of these should probably also be per grain class
	//MNF20190510 - yes, "per grain class" would make sense
	auto FlowErosionScalingFactor               = RegisterParameterDouble(Model, Reaches, "Flow erosion scaling factor", SPerM2, 1.0);
	auto FlowErosionDirectRunoffThreshold       = RegisterParameterDouble(Model, Reaches, "Flow erosion direct runoff threshold", M3PerSPerKm2, 0.001);
	auto FlowErosionNonlinearCoefficient        = RegisterParameterDouble(Model, Reaches, "Flow erosion non-linear coefficient", Dimensionless, 1.0);
	auto TransportCapacityScalingFactor         = RegisterParameterDouble(Model, Reaches, "Transport capacity scaling factor", KgPerM2PerKm2, 1.0);
	auto TransportCapacityDirectRunoffThreshold = RegisterParameterDouble(Model, Reaches, "Transport capacity direct runoff threshold", M3PerSPerKm2, 0.001);
	auto TransportCapacityNonlinearCoefficient  = RegisterParameterDouble(Model, Reaches, "Transport capacity non-linear coefficient", Dimensionless, 1.0);
	
	
	auto SubcatchmentArea = GetParameterDoubleHandle(Model, "Terrestrial catchment area");
	auto ReachLength      = GetParameterDoubleHandle(Model, "Reach length");
	auto Percent          = GetParameterDoubleHandle(Model, "%");
	
	auto Rainfall = GetEquationHandle(Model, "Rainfall");
	auto RunoffToReach = GetEquationHandle(Model, "Runoff to reach");
	
	auto Class = RegisterIndexSet(Model, "Grain class");
	auto GrainClass = RegisterParameterGroup(Model, "Grain class", Class);
	
	auto SmallestDiameterOfClass        = RegisterParameterDouble(Model, GrainClass, "Smallest diameter of grain in class", Metres, 0.0, 0.0, 2e3);
	auto LargestDiameterOfClass         = RegisterParameterDouble(Model, GrainClass, "Largest diameter of grain in class", Metres, 2e-6);
	auto DensityOfClass                 = RegisterParameterDouble(Model, GrainClass, "Density of grain class", KgPerM3, 1000.0);
	//MNF2090510
	auto ImmobileToProtected            = RegisterParameterDouble(Model, GrainClass, "Rate at which particles become unavailable for erosion", Dimensionless, 0.0, 0.0, 1.0);
	
	auto Land = GetParameterGroupHandle(Model, "Landscape units");
	auto VegetationIndex                        = RegisterParameterDouble(Model, Land, "Vegetation index", Dimensionless, 1.0); //Could this be the same as the canopy interception though?
	
	auto SedimentLand = RegisterParameterGroup(Model, "Sediment land", LandscapeUnits);
	SetParentGroup(Model, SedimentLand, GrainClass);
	auto SplashDetachmentScalingFactor          = RegisterParameterDouble(Model, SedimentLand, "Splash detachment scaling factor", SPerM, 0.001);
	auto FlowErosionPotential                   = RegisterParameterDouble(Model, SedimentLand, "Flow erosion potential", KgPerSPerKm2, 0.074);
	auto SplashDetachmentSoilErodibility        = RegisterParameterDouble(Model, SedimentLand, "Splash detachment soil erodibility", KgPerM2PerS, 1.0);
	auto InitialSurfaceStore                    = RegisterParameterDouble(Model, SedimentLand, "Initial surface grain store", KgPerKm2, 100.0);
	auto InitialImmobileStore                   = RegisterParameterDouble(Model, SedimentLand, "Initial immobile grain store", KgPerKm2, 100.0);
	//MNF20190510
	auto InitialProtectedStore                  = RegisterParameterDOuble(Model, SedimentLand, "Initial protected grain store", KgPerKm2, 100.0);
	auto GrainInput                             = RegisterParameterDouble(Model, SedimentLand, "Grain input to land", KgPerKm2PerDay, 0.0);
	
	
	auto TransferMatrix = RegisterParameterGroup(Model, "Transfer matrix", Class);
	SetParentGroup(Model, TransferMatrix, GrainClass);
	auto LandMassTransferRateBetweenClasses = RegisterParameterDouble(Model, TransferMatrix, "Mass transfer rate between classes on land", Dimensionless, 0.0, 0.0, 1.0);
	
	///////////// Erosion and transport ////////////////
	
	auto GrainInputTimeseries = RegisterInput(Model, "Grain input to land", KgPerKm2PerDay);
	
	auto SurfaceTransportCapacity     = RegisterEquation(Model, "Land surface transport capacity", KgPerKm2PerDay);
	auto ImmobileGrainStoreBeforeMobilisation = RegisterEquation(Model, "Immobile grain store before mobilisation", KgPerKm2);
	auto MobilisedViaSplashDetachment = RegisterEquation(Model, "Grain mass mobilised via splash detachment", KgPerKm2PerDay);
	auto FlowErosionKFactor           = RegisterEquation(Model, "Flow erosion K factor", KgPerKm2PerDay);
	auto SurfaceGrainStoreBeforeTransport = RegisterEquation(Model, "Surface grain store before transport", KgPerKm2);
	auto TransportBeforeFlowErosion   = RegisterEquation(Model, "Transport before flow erosion", KgPerKm2PerDay);
	auto PotentiallyMobilisedViaFlowErosion = RegisterEquation(Model, "Grain mass potentially mobilised via flow erosion", KgPerKm2PerDay);
	auto MobilisedViaFlowErosion      = RegisterEquation(Model, "Grain mass mobilised via flow erosion", KgPerKm2PerDay);
	
	//MNF20190510
	auto ProtectedGrainStore = RegisterEquation(Model, "Immobile Grain Store rendered premanently inaccessible", KgPerKm2PerDay);
	SetInitialValue(Model, ProtectedGrainStore, InitialProtectedGrainStore);
	
	auto SurfaceGrainStoreAfterAllTransport = RegisterEquation(Model, "Surface grain store after transport", KgPerKm2);
	auto SurfaceGrainStoreMassTransferFromOtherClasses = RegisterEquation(Model, "Mass transfer to and from other classes in the surface grain store", KgPerKm2PerDay);
	
	auto SurfaceGrainStore   = RegisterEquation(Model, "Surface grain store", KgPerKm2);
	SetInitialValue(Model, SurfaceGrainStore, InitialSurfaceStore);
	
	auto ImmobileGrainStoreAfterMobilisation = RegisterEquation(Model, "Immobile grain store after mobilisation", KgPerKm2);
	auto ImmobileGrainStoreMassTransferFromOtherClasses = RegisterEquation(Model, "Mass transfer to and from other classes in the immobile grain store", KgPerKm2PerDay);
	
	auto ImmobileGrainStore     = RegisterEquation(Model, "Immobile grain store", KgPerKm2);
	SetInitialValue(Model, ImmobileGrainStore, InitialImmobileStore);
	
	auto GrainDeliveryToReach              = RegisterEquation(Model, "Grain delivery to reach", KgPerKm2PerDay);

	auto AreaScaledGrainDeliveryToReach    = RegisterEquation(Model, "Area scaled grain delivery to reach", KgPerDay);
	
	auto TotalGrainDeliveryToReach         = RegisterEquationCumulative(Model, "Total grain delivery to reach", AreaScaledGrainDeliveryToReach, LandscapeUnits);
	
	EQUATION(Model, SurfaceTransportCapacity,
		double qquick = RESULT(RunoffToReach, DirectRunoff) / 86.4;
		double flow = Max(0.0, qquick - PARAMETER(TransportCapacityDirectRunoffThreshold));
		return 86400.0 * PARAMETER(TransportCapacityScalingFactor) 
			* pow((PARAMETER(SubcatchmentArea) / PARAMETER(ReachLength))*flow, PARAMETER(TransportCapacityNonlinearCoefficient));
	)
	
	EQUATION(Model, ImmobileGrainStoreBeforeMobilisation,
		return LAST_RESULT(ImmobileGrainStore) + IF_INPUT_ELSE_PARAMETER(GrainInputTimeseries, GrainInput);
	)
	
	//TODO: Documentation says this should use Reffq = "effective precipitation". Is that the same as rainfall? Or rainfall + snowmelt?
	//MNF20190510 This is a little tricky as splash detachment should probably only be driven by actual rainfall. Snowmelt does not have the 
	//gravitaitonal energy to induce splash detachment
	EQUATION(Model, MobilisedViaSplashDetachment,
		double Reffq = RESULT(Rainfall) / 86.4;
		double SSD = 86400.0 * PARAMETER(SplashDetachmentScalingFactor) * Reffq * pow(PARAMETER(SplashDetachmentSoilErodibility), 10.0 / (10.0 - PARAMETER(VegetationIndex)));
		return Min(SSD, RESULT(ImmobileGrainStoreBeforeMobilisation));
	)
	
	EQUATION(Model, SurfaceGrainStoreBeforeTransport,
		//TODO: Should we also allow inputs to the surface store?
		//MNF20190510 - we could go either way on this as we have a way of getting mucroplastics into the system. Having a way to 
		//add material to the surface store could be useful if, e.g., we wanted to add atmospheric deposition of microplastics
		return LAST_RESULT(SurfaceGrainStore) + RESULT(MobilisedViaSplashDetachment);
	)
	
	auto TotalSurfaceGrainstoreBeforeTransport = RegisterEquationCumulative(Model, "Total surface grain store before transport", SurfaceGrainStoreBeforeTransport, Class);
	
	EQUATION(Model, TransportBeforeFlowErosion,
		double potential = RESULT(SurfaceGrainStoreBeforeTransport);
		double capacity  = RESULT(SurfaceTransportCapacity);
		double totalpotential = RESULT(TotalSurfaceGrainstoreBeforeTransport);
		if(totalpotential > capacity) return SafeDivide(potential, totalpotential) * capacity;
		return potential;
	)
	
	EQUATION(Model, FlowErosionKFactor,
		double qquick = RESULT(RunoffToReach, DirectRunoff) / 86.4;
		double flow = Max(0.0, qquick - PARAMETER(FlowErosionDirectRunoffThreshold));
		return 86400.0 * PARAMETER(FlowErosionScalingFactor) * PARAMETER(FlowErosionPotential) 
			* pow( (1e-3 * PARAMETER(SubcatchmentArea) / PARAMETER(ReachLength))*flow, PARAMETER(FlowErosionNonlinearCoefficient));
	)
	
	EQUATION(Model, PotentiallyMobilisedViaFlowErosion,
		double TC = RESULT(SurfaceTransportCapacity);
	
		double SFE = RESULT(FlowErosionKFactor)
				* SafeDivide(TC - RESULT(MobilisedViaSplashDetachment),
							TC + RESULT(FlowErosionKFactor));
		SFE = Max(0.0, SFE);
		
		double surfacestorebeforeerosion = RESULT(SurfaceGrainStoreBeforeTransport) - RESULT(TransportBeforeFlowErosion);
		
		if(surfacestorebeforeerosion > 0.0) return 0.0; //NOTE: If there is any surface sediment left we know that we already exceeded our transport capacity, and so we can not mobilise any more.
		
		return Min(SFE, RESULT(ImmobileGrainStoreBeforeMobilisation) - RESULT(MobilisedViaSplashDetachment));
	)
	
	auto TotalPotentiallyMobilisedViaFlowErosion = RegisterEquationCumulative(Model, "Total potentially mobilised via flow erosion", PotentiallyMobilisedViaFlowErosion, Class);
	
	EQUATION(Model, MobilisedViaFlowErosion,
		double potential = RESULT(PotentiallyMobilisedViaFlowErosion);
		double totalpotential = RESULT(TotalPotentiallyMobilisedViaFlowErosion);
		double capacity = RESULT(SurfaceTransportCapacity) - RESULT(TransportBeforeFlowErosion);
		
		if(totalpotential > capacity) return SafeDivide(potential, totalpotential) * capacity;
		return potential;
	)
	
	EQUATION(Model, GrainDeliveryToReach,
		return RESULT(TransportBeforeFlowErosion) + RESULT(MobilisedViaFlowErosion);
	)
	
	EQUATION(Model, SurfaceGrainStoreAfterAllTransport,
		return RESULT(SurfaceGrainStoreBeforeTransport) - RESULT(TransportBeforeFlowErosion); //NOTE: Flow erosion is removed from the immobile store directly.
	)
	
	EQUATION(Model, SurfaceGrainStoreMassTransferFromOtherClasses,
		double aftertransport   = RESULT(SurfaceGrainStoreAfterAllTransport);
		
		double fromotherclasses = 0.0;
		
		for(index_t OtherClass = FIRST_INDEX(Class); OtherClass < INDEX_COUNT(Class); ++OtherClass)
		{
			double otherclassmass = RESULT(SurfaceGrainStoreAfterAllTransport, OtherClass);
			fromotherclasses += PARAMETER(LandMassTransferRateBetweenClasses, OtherClass, CURRENT_INDEX(Class)) * otherclassmass;
		}
		
		double tootherclasses = 0.0;
		
		for(index_t OtherClass = FIRST_INDEX(Class); OtherClass < INDEX_COUNT(Class); ++OtherClass)
		{
			tootherclasses += PARAMETER(LandMassTransferRateBetweenClasses, CURRENT_INDEX(Class), OtherClass) * aftertransport;
		}
		
		return fromotherclasses - tootherclasses;
	)
	
	EQUATION(Model, SurfaceGrainStore,
		return RESULT(SurfaceGrainStoreAfterAllTransport) + RESULT(SurfaceGrainStoreMassTransferFromOtherClasses);
	)
	
	EQUATION(Model, ImmobileGrainStoreAfterMobilisation,
		return RESULT(ImmobileGrainStoreBeforeMobilisation) - RESULT(MobilisedViaSplashDetachment) - RESULT(MobilisedViaFlowErosion);
	)
	
	EQUATION(Model, ImmobileGrainStoreMassTransferFromOtherClasses,
		double aftermob   = RESULT(ImmobileGrainStoreAfterMobilisation);
		
		double fromotherclasses = 0.0;
		
		for(index_t OtherClass = FIRST_INDEX(Class); OtherClass < INDEX_COUNT(Class); ++OtherClass)
		{
			double otherclassmass = RESULT(ImmobileGrainStoreAfterMobilisation, OtherClass);
			fromotherclasses += PARAMETER(LandMassTransferRateBetweenClasses, OtherClass, CURRENT_INDEX(Class)) * otherclassmass;
		}
		
		double tootherclasses = 0.0;
		
		for(index_t OtherClass = FIRST_INDEX(Class); OtherClass < INDEX_COUNT(Class); ++OtherClass)
		{
			tootherclasses += PARAMETER(LandMassTransferRateBetweenClasses, CURRENT_INDEX(Class), OtherClass) * aftermob;
		}
		
		return fromotherclasses - tootherclasses;
	)
	
	EQUATION(Model, ImmobileGrainStore,
		return RESULT(ImmobileGrainStoreAfterMobilisation) + RESULT(ImmobileGrainStoreMassTransferFromOtherClasses);
	)
	
	EQUATION(Model, AreaScaledGrainDeliveryToReach,
		return PARAMETER(SubcatchmentArea) * PARAMETER(Percent) / 100.0 * RESULT(GrainDeliveryToReach);
	)

	
	///////////////// Suspended in reach ////////////////////////////////
	
	auto InstreamSedimentSolver = RegisterSolver(Model, "In-stream sediment solver", 0.1, BoostRosenbrock4, 1e-3, 1e-3);
	
	auto SedimentReach = RegisterParameterGroup(Model, "Sediment reach", Reach);
	SetParentGroup(Model, SedimentReach, GrainClass);
	
	auto EffluentGrainConcentration       = RegisterParameterDouble(Model, SedimentReach, "Effluent grain concentration", MgPerL, 0.0);
	auto InitialMassOfBedGrainPerUnitArea = RegisterParameterDouble(Model, SedimentReach, "Initial mass of bed grain per unit area", KgPerM2, 10);
	auto InitialSuspendedGrainMass        = RegisterParameterDouble(Model, SedimentReach, "Initial suspended grain mass", Kg, 1e2);
	
	//auto BankErosionScalingFactor        = RegisterParameterDouble(Model, Reaches, "Bank erosion scaling factor", KgPerM2PerM3SPerDay, 1.0);
	//auto BankErosionNonlinearCoefficient = RegisterParameterDouble(Model, Reaches, "Bank erosion non-linear coefficient", Dimensionless, 1.0);
	auto ShearVelocityCoefficient        = RegisterParameterDouble(Model, Reaches, "Shear velocity coefficient", Dimensionless, 1.0);
	auto MeanChannelSlope                = RegisterParameterDouble(Model, Reaches, "Mean channel slope", Dimensionless, 2.0);
	auto EntrainmentCoefficient          = RegisterParameterDouble(Model, Reaches, "Entrainment coefficient", S2PerKg, 1.0);
	
	auto ReachMassTransferRateBetweenClasses = RegisterParameterDouble(Model, TransferMatrix, "Mass transfer rate between classes in the reach", Dimensionless, 0.0, 0.0, 1.0);
	
	auto ReachUpstreamSuspendedGrain         = RegisterEquation(Model, "Reach upstream suspended grain", KgPerDay);
	//auto ClayReleaseFromChannelBanks       = RegisterEquation(Model, "Clay release from channel banks", KgPerM2PerDay);
	auto ReachFrictionFactor                 = RegisterEquation(Model, "Reach friction factor", Dimensionless);
	auto ReachShearVelocity                  = RegisterEquation(Model, "Reach shear velocity", MPerS);
	auto ProportionOfGrainThatCanBeEntrained = RegisterEquation(Model, "Proportion of grain that can be entrained", Dimensionless);
	auto StreamPower                         = RegisterEquation(Model, "Stream power", JPerSPerM2);
	
	auto EffluentGrain                   = RegisterEquation(Model, "Effluent grain inputs", KgPerDay);
	auto GrainAbstraction                = RegisterEquation(Model, "Grain abstraction", KgPerDay);
	SetSolver(Model, GrainAbstraction, InstreamSedimentSolver);
	
	auto GrainEntrainment                = RegisterEquation(Model, "Grain entrainment", KgPerM2PerDay);
	SetSolver(Model, GrainEntrainment, InstreamSedimentSolver);
	
	auto GrainDeposition                 = RegisterEquation(Model, "Grain deposition", KgPerM2PerDay);
	SetSolver(Model, GrainDeposition, InstreamSedimentSolver);
	
	auto ReachSuspendedGrainOutput       = RegisterEquation(Model, "Reach suspended grain output", KgPerDay);
	SetSolver(Model, ReachSuspendedGrainOutput, InstreamSedimentSolver);
	
	auto MassOfBedGrainPerUnitArea       = RegisterEquationODE(Model, "Mass of bed grain per unit area", KgPerM2);
	SetInitialValue(Model, MassOfBedGrainPerUnitArea, InitialMassOfBedGrainPerUnitArea);
	SetSolver(Model, MassOfBedGrainPerUnitArea, InstreamSedimentSolver);
	
	auto SuspendedGrainMass = RegisterEquationODE(Model, "Suspended grain mass", Kg);
	SetInitialValue(Model, SuspendedGrainMass, InitialSuspendedGrainMass);
	SetSolver(Model, SuspendedGrainMass, InstreamSedimentSolver);
	
	
	
	auto ReachWidth   = GetParameterDoubleHandle(Model, "Reach width");
	auto EffluentFlow = GetParameterDoubleHandle(Model, "Effluent flow");
	auto ReachHasEffluentInputs = GetParameterBoolHandle(Model, "Reach has effluent input");
	auto EffluentTimeseries = GetInputHandle(Model, "Effluent flow");
	
	auto ReachDepth    = GetEquationHandle(Model, "Reach depth");
	auto ReachFlow     = GetEquationHandle(Model, "Reach flow");
	auto ReachVolume   = GetEquationHandle(Model, "Reach volume");
	auto ReachVelocity = GetEquationHandle(Model, "Reach velocity");
	auto ReachAbstraction = GetEquationHandle(Model, "Reach abstraction");
	
	
	EQUATION(Model, ReachUpstreamSuspendedGrain,
		double sum = 0.0;
		index_t GrainIndex = CURRENT_INDEX(Class);
		FOREACH_INPUT(Reach,
			sum += RESULT(ReachSuspendedGrainOutput, *Input, GrainIndex);
		)
		return sum;
	)
	
	EQUATION(Model, ReachSuspendedGrainOutput,
		return 86400.0 * RESULT(SuspendedGrainMass) * SafeDivide(RESULT(ReachFlow), RESULT(ReachVolume));
	)
	
	/*
	EQUATION(Model, ClayReleaseFromChannelBanks,
		return PARAMETER(BankErosionScalingFactor) * pow(RESULT(ReachFlow), PARAMETER(BankErosionNonlinearCoefficient));
	)
	*/
	
	EQUATION(Model, ReachFrictionFactor,
		return 4.0 * RESULT(ReachDepth) / (2.0 * RESULT(ReachDepth) + PARAMETER(ReachWidth));
	)
	
	EQUATION(Model, ReachShearVelocity,
		double earthsurfacegravity = 9.807;
		return sqrt(earthsurfacegravity * RESULT(ReachDepth) * PARAMETER(ShearVelocityCoefficient) * PARAMETER(MeanChannelSlope));
	)
	
	EQUATION(Model, ProportionOfGrainThatCanBeEntrained,
		double Dmax = 9.99 * pow(RESULT(ReachShearVelocity), 2.52);
		double Dlow = PARAMETER(SmallestDiameterOfClass);
		double Dupp = PARAMETER(LargestDiameterOfClass);
		if(Dmax < Dlow) return 0.0;
		if(Dmax > Dupp) return 1.0;
		return (Dmax - Dlow) / (Dupp - Dlow);
	)
	
	EQUATION(Model, StreamPower,
		double waterdensity = 1000.0;
		double earthsurfacegravity = 9.807;
		return waterdensity * earthsurfacegravity * PARAMETER(MeanChannelSlope) * RESULT(ReachVelocity) * RESULT(ReachDepth);
	)
	
	EQUATION(Model, GrainEntrainment,
		double value = 86400.0 * PARAMETER(EntrainmentCoefficient) * RESULT(MassOfBedGrainPerUnitArea) * RESULT(ProportionOfGrainThatCanBeEntrained) * RESULT(StreamPower) * RESULT(ReachFrictionFactor);
		return value;
	)
	 
	EQUATION(Model, GrainDeposition,
		double mediangrainsize = (PARAMETER(SmallestDiameterOfClass) + PARAMETER(LargestDiameterOfClass)) / 2.0;
		double sedimentdensity = PARAMETER(DensityOfClass);
		double waterdensity    = 1000.0;
		double earthsurfacegravity = 9.807;
		double fluidviscosity = 0.001;
		double terminalsettlingvelocity = (sedimentdensity - waterdensity) * earthsurfacegravity * Square(mediangrainsize) / (18.0 * fluidviscosity);
		
		double value = 86400.0 * terminalsettlingvelocity * SafeDivide(RESULT(SuspendedGrainMass), RESULT(ReachVolume));
		
		return value;
	)
	
	EQUATION(Model, MassOfBedGrainPerUnitArea,
		return RESULT(GrainDeposition) - RESULT(GrainEntrainment);
	)
	
	EQUATION(Model, GrainAbstraction,
		return RESULT(ReachAbstraction) * 86400.0 * SafeDivide(RESULT(SuspendedGrainMass), RESULT(ReachVolume)); 
	)
	
	EQUATION(Model, EffluentGrain,
		double effluentflow = IF_INPUT_ELSE_PARAMETER(EffluentTimeseries, EffluentFlow) * 86400.0;
		double effluentconc = PARAMETER(EffluentGrainConcentration) * 1e-3;
		
		if(PARAMETER(ReachHasEffluentInputs)) return effluentflow * effluentconc;
		
		return 0.0;
	)
	
	EQUATION(Model, SuspendedGrainMass,
		return 
			  RESULT(TotalGrainDeliveryToReach) 
			+ RESULT(EffluentGrain)
			+ RESULT(ReachUpstreamSuspendedGrain) 
			- RESULT(ReachSuspendedGrainOutput)
			- RESULT(GrainAbstraction)
			+ PARAMETER(ReachLength) * PARAMETER(ReachWidth) * (RESULT(GrainEntrainment) - RESULT(GrainDeposition));
	)
	
	
	/// Transferring mass between classes due to breakdown and other causes.
	
	auto SuspendedGrainMassTransferFromOtherClasses = RegisterEquation(Model, "Mass transfer to and from other classes in the suspended grain store", KgPerDay);
	auto BedGrainMassTransferFromOtherClasses       = RegisterEquation(Model, "Mass transfer to and from other classes in the bed grain store", KgPerM2PerDay);
	auto ReachMassTransferControl                   = RegisterEquation(Model, "(Code that facilitates mass transfer between classes in the reach. Return value of this equation has no meaning)", Dimensionless);
	
	EQUATION(Model, SuspendedGrainMassTransferFromOtherClasses,
		double suspendedmass = RESULT(SuspendedGrainMass);
		
		double fromotherclasses = 0.0;
		
		for(index_t OtherClass = FIRST_INDEX(Class); OtherClass < INDEX_COUNT(Class); ++OtherClass)
		{
			double otherclassmass = RESULT(SuspendedGrainMass, OtherClass);
			fromotherclasses += PARAMETER(ReachMassTransferRateBetweenClasses, OtherClass, CURRENT_INDEX(Class)) * otherclassmass;
		}
		
		double tootherclasses = 0.0;
		
		for(index_t OtherClass = FIRST_INDEX(Class); OtherClass < INDEX_COUNT(Class); ++OtherClass)
		{
			tootherclasses += PARAMETER(ReachMassTransferRateBetweenClasses, CURRENT_INDEX(Class), OtherClass) * suspendedmass;
		}
		
		return fromotherclasses - tootherclasses;
	)
	
	EQUATION(Model, BedGrainMassTransferFromOtherClasses,
		double bedmass = RESULT(MassOfBedGrainPerUnitArea);
		
		double fromotherclasses = 0.0;
		
		for(index_t OtherClass = FIRST_INDEX(Class); OtherClass < INDEX_COUNT(Class); ++OtherClass)
		{
			double otherclassmass = RESULT(MassOfBedGrainPerUnitArea, OtherClass);
			fromotherclasses += PARAMETER(ReachMassTransferRateBetweenClasses, OtherClass, CURRENT_INDEX(Class)) * otherclassmass;
		}
		
		double tootherclasses = 0.0;
		
		for(index_t OtherClass = FIRST_INDEX(Class); OtherClass < INDEX_COUNT(Class); ++OtherClass)
		{
			tootherclasses += PARAMETER(ReachMassTransferRateBetweenClasses, CURRENT_INDEX(Class), OtherClass) * bedmass;
		}
		
		return fromotherclasses - tootherclasses;
	)
	
	EQUATION(Model, ReachMassTransferControl,
		double suspendedmass = RESULT(SuspendedGrainMass);
		double suspendedmasstransfer = RESULT(SuspendedGrainMassTransferFromOtherClasses);
		
		SET_RESULT(SuspendedGrainMass, suspendedmass + suspendedmasstransfer);
		
		double bedmass = RESULT(MassOfBedGrainPerUnitArea);
		double bedmasstransfer = RESULT(BedGrainMassTransferFromOtherClasses);
		
		SET_RESULT(MassOfBedGrainPerUnitArea, bedmass + bedmasstransfer);
		
		return 0.0;
	)
}


