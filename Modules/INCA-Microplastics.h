

#include "../boost_solvers.h"


static double
ParticleEquivalentDiameter(double a, double Ratio, int ShapeType)
{
	double b = a / Ratio;
	double c = b;

	double d_equi;
	if(ShapeType == 0)
		d_equi = std::cbrt(a*b*c);
	else
		d_equi = c;
	
	return d_equi;
}

static double
ComputeTerminalSettlingVelocity(double a, double b, double c, double nu, double rho_f, double rho_p, int ShapeType)
{
	// Computation based on
	// Effects of Particle Properties on the Settling and Rise Velocities of Microplastics in Freshwater under Laboratory Conditions, K. Waldschläger and H. Schüttrumph, Environ. Sci. Technol. 2019, 53, 1958-1966
	// The iteration process is pretty standard, see for instance https://www.me.psu.edu/cimbala/me405/Lectures/Iteration_terminal_settling_speed.pdf
	
	// a, b, c                      -     longest, intermediate, shortest particle side lengths (m)
	// nu                           -     Water kinematic viscosity, (m2/s)
	// rho_f, rho_p                 -     Densities of water and plastic (kg/m3)
	// ShapeType                    -     0=fragment, 1=fiber
	
	if(rho_p < rho_f)
		FatalError("ERROR: Currently, only plastic that is heavier than water is supported.\n");
	
	if(ShapeType < 0 || ShapeType > 2)
		FatalError("ERROR: Unsupported shape type: ", ShapeType, ".\n");
	
	// Earth surface gravity (m/s2)
	double g = 9.807;
	
	// Equivalent particle diameter
	double d_equi;
	if(ShapeType == 0)
		d_equi = std::cbrt(a*b*c);
	else
		d_equi = c;
	
	// Corey Shape Factor
	double CSF = c / std::sqrt(a*b);
	
	//Initial guess of settling velocity using Stoke's law for a small spherical particle
	double W = (rho_p - rho_f)*g*d_equi*d_equi / (18.0*nu);
	
	bool Converged = false;
	for(size_t It = 0; It < 1000; ++It)
	{
		// Particle Reynold's number for the given velocity
		double Re = W * d_equi / nu;
		
		// Drag coefficient
		double C_D;
		if(ShapeType == 0)
			C_D = 3.0 / (CSF * std::cbrt(Re));
		else
			C_D = 4.7/std::sqrt(Re) + std::sqrt(CSF);
		
		double new_W = std::sqrt((4.0/3.0) * (d_equi/C_D) * ((rho_p - rho_f)/rho_f) * g);
		
		if(std::abs(W - new_W) < 1e-6) Converged = true;
		
		W = new_W;
		
		if(std::isnan(W)) break;
		
		if(Converged)
		{
			//std::cout << "Converged at iteration " << It << "\n";
			break;
		} 
	}
	
	if(!Converged)
		FatalError("ERROR: Terminal settling velocity computation failed to converge.\n");
	
	return W;
}






static void
AddINCAMicroplasticsModel(mobius_model *Model)
{
	BeginModule(Model, "INCA-Microplastics", "0.12");
	//NOTE: Is designed to work with PERSiST
	
	SetModuleDescription(Model, R""""(
INCA-Microplastics is a modified version of INCA-Sediments:
	
[^https://doi.org/10.1016/j.scitotenv.2010.02.030^ Lazar, A. N. et. al. 2010, An assessment of the fine sediment dynamics in an upland river system: INCA-Sed modifications and implications for fisheries. Sci. Tot. En. 408, 2555-2566]

Changes include:
[i100 [O1 Each grain class can have a separate density&][O1 Each grain class has separate on-land mobilisation and store&][O1 There can be mass transfer between classes (simulating breakdown of larger particles into smaller)&]]
	
Moreover, in-stream settling and erosion behaviour are now computed based on newer experiments with microplastics:

[^https://doi.org/10.1021/acs.est.9b05394^ Erosion Behavior of Different Microplastic Particles in Comparison to Natural Sediments, K. Waldschläger and H. Schüttrumph, Environ. Sci. Technol. 2019, 53, 13219-13227]

[^https://doi.org/10.1021/acs.est.8b06794^ Effects of Particle Properties on the Settling and Rise Velocities of Microplastics in Freshwater under Laboratory Conditions, K. Waldschläger and H. Schüttrumph, Environ. Sci. Technol. 2019, 53, 1958-1966]
)"""");

	auto Dimensionless = RegisterUnit(Model);
	auto Day           = RegisterUnit(Model, "day");
	auto MPerS         = RegisterUnit(Model, "m/s");
	auto M2PerS        = RegisterUnit(Model, "m2/s");
	auto Kg            = RegisterUnit(Model, "kg");
	auto KgPerM        = RegisterUnit(Model, "kg/m");
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
	auto MgPerL        = RegisterUnit(Model, "mg/L");
	auto Metres        = RegisterUnit(Model, "m");
	auto S2PerKg       = RegisterUnit(Model, "s2/kg");
	auto NewtonPerM2   = RegisterUnit(Model, "N/m2");
	auto MmPerDay      = RegisterUnit(Model, "mm/day");
	auto PerDay        = RegisterUnit(Model, "1/day");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto SoilBoxes = GetIndexSetHandle(Model, "Soils");
	
	auto DirectRunoff = RequireIndex(Model, SoilBoxes, "Direct runoff");
	
	auto Reaches = RegisterParameterGroup(Model, "Erosion and transport by subcatchment", Reach);
	
	//TODO : Find default/min/max/description for these
	
	auto TransportCapacityScalingFactor         = RegisterParameterDouble(Model, Reaches, "Transport capacity scaling factor", KgPerM2PerKm2, 0.074, 0.0, 1.0);
	auto TransportCapacityDirectRunoffThreshold = RegisterParameterDouble(Model, Reaches, "Transport capacity direct runoff threshold", MmPerDay, 0, 0.0, 1000.0, "The amount of direct runoff needed before there is any transport of particles");
	auto TransportCapacityNonlinearCoefficient  = RegisterParameterDouble(Model, Reaches, "Transport capacity non-linear coefficient", Dimensionless, 1.0);
	auto FlowErosionScalingFactor               = RegisterParameterDouble(Model, Reaches, "Flow erosion scaling factor", KgPerM2PerKm2, 0.074, 0.0, 1.0);
	auto FlowErosionDirectRunoffThreshold       = RegisterParameterDouble(Model, Reaches, "Flow erosion direct runoff threshold", MmPerDay, 0.0, 0.0, 1000.0, "The amount of direct runoff needed before there is any flow erosion of particles");
	auto FlowErosionNonlinearCoefficient        = RegisterParameterDouble(Model, Reaches, "Flow erosion non-linear coefficient", Dimensionless, 1.0);
	
	
	auto Class = RegisterIndexSet(Model, "Grain class");
	auto GrainClass = RegisterParameterGroup(Model, "Grain class", Class);
	
	auto ClassShapeType                 = RegisterParameterEnum(Model, GrainClass, "Shape type", {"Fragment", "Fiber"}, "Fragment");
	auto SmallestDiameterOfClass        = RegisterParameterDouble(Model, GrainClass, "Smallest major diameter of grain in class", Metres, 0.0, 0.0, 2e3);
	auto LargestDiameterOfClass         = RegisterParameterDouble(Model, GrainClass, "Largest major diameter of grain in class", Metres, 2e-6, 0.0, 2e3);
	auto RatioOfMajorToMinor            = RegisterParameterDouble(Model, GrainClass, "Ratio of major to minor diameter", Dimensionless, 1.0, 1.0, 1000.0);
	auto DensityOfClass                 = RegisterParameterDouble(Model, GrainClass, "Density of grain class", KgPerM3, 1000.0);
	
	auto Land = RegisterParameterGroup(Model, "Erosion by land class", LandscapeUnits);
	auto SplashDetachmentSoilErodibility        = RegisterParameterDouble(Model, Land, "Splash detachment soil erodibility", Dimensionless, 1.0, 0.0, 1.0, "How much the vegetation affects the splash detachment rate");
	auto VegetationIndex                        = RegisterParameterDouble(Model, Land, "Vegetation index", Dimensionless, 1.0, 0.0, 9.9, "How much of the land is covered in vegetation");
	auto GrowingSeasonBegin                     = RegisterParameterUInt(Model, Land, "First day of year of the growing season", Day, 60, 1, 365);
	auto GrowingSeasonDuration                  = RegisterParameterUInt(Model, Land, "Growing season duration", Day, 200, 1, 365);
	
	
	auto SedimentLand = RegisterParameterGroup(Model, "Erosion by grain and land class", Class, LandscapeUnits);
	
	auto SplashDetachmentScalingFactor          = RegisterParameterDouble(Model, SedimentLand, "Splash detachment scaling factor", KgPerM, 0.001, 0.0, 1.0, "Particle class specific splash detachment behaviour");
	auto FlowErosionPotential                   = RegisterParameterDouble(Model, SedimentLand, "Flow erosion potential", Dimensionless, 1.0, 0.0, 1000.0, "Particle class specific flow erosion scaling");
	auto InfiltrationRate                       = RegisterParameterDouble(Model, SedimentLand, "Grain infiltration rate", PerDay, 0.0, 0.0, 1.0, "The rate at which particles infiltrate from the surface store to the immobile store");
	auto InitialSurfaceStore                    = RegisterParameterDouble(Model, SedimentLand, "Initial surface grain store", KgPerKm2, 0.0, 0.0, 1e10);
	auto InitialImmobileStore                   = RegisterParameterDouble(Model, SedimentLand, "Initial immobile grain store", KgPerKm2, 0.0, 0.0, 1e10);
	auto GrainInput                             = RegisterParameterDouble(Model, SedimentLand, "Grain input to land", KgPerKm2PerDay, 0.0, 0.0, 1e10, "Is overridden by the time series of the same name if it is provided");
	
	auto TransferMatrix = RegisterParameterGroup(Model, "Transfer matrix", Class, Class);
	auto LandMassTransferRateBetweenClasses = RegisterParameterDouble(Model, TransferMatrix, "Mass transfer rate between classes on land", PerDay, 0.0, 0.0, 1.0);
	
	auto SubcatchmentArea = GetParameterDoubleHandle(Model, "Terrestrial catchment area");
	auto ReachLength      = GetParameterDoubleHandle(Model, "Reach length");
	auto Percent          = GetParameterDoubleHandle(Model, "%");
	
	auto Rainfall      = GetEquationHandle(Model, "Rainfall");
	auto RunoffToReach = GetEquationHandle(Model, "Runoff to reach");
	
	///////////// Erosion and transport ////////////////
	
	auto GrainInputTimeseries = RegisterInput(Model, "Grain input to land", KgPerKm2PerDay);
	
	auto SurfaceTransportCapacity     = RegisterEquation(Model, "Land surface transport capacity", KgPerKm2PerDay);
	auto ImmobileGrainStoreBeforeMobilisation = RegisterEquation(Model, "Immobile grain store before mobilisation", KgPerKm2);
	auto MobilisedViaSplashDetachment = RegisterEquation(Model, "Grain mass mobilised via splash detachment", KgPerKm2PerDay);
	auto SurfaceGrainStoreBeforeTransport = RegisterEquation(Model, "Surface grain store before transport", KgPerKm2);
	auto TransportBeforeFlowErosion   = RegisterEquation(Model, "Transport before flow erosion", KgPerKm2PerDay);
	auto PotentiallyMobilisedViaFlowErosion = RegisterEquation(Model, "Grain mass potentially mobilised via flow erosion", KgPerKm2PerDay);
	auto MobilisedViaFlowErosion      = RegisterEquation(Model, "Grain mass mobilised via flow erosion", KgPerKm2PerDay);
	
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
		double qquick = RESULT(RunoffToReach, DirectRunoff);
		double flow = Max(0.0, qquick - PARAMETER(TransportCapacityDirectRunoffThreshold));
		return PARAMETER(TransportCapacityScalingFactor) 
			* pow(1e3*(PARAMETER(SubcatchmentArea) / PARAMETER(ReachLength))*flow, PARAMETER(TransportCapacityNonlinearCoefficient));
	)
	
	EQUATION(Model, ImmobileGrainStoreBeforeMobilisation,
		return LAST_RESULT(ImmobileGrainStore) + LAST_RESULT(SurfaceGrainStore)*PARAMETER(InfiltrationRate);
	)
	
	EQUATION(Model, MobilisedViaSplashDetachment,
		s32 DOY          = (s32)CURRENT_TIME().DayOfYear;
		s32 GrowBegin    = (s32)PARAMETER(GrowingSeasonBegin);
		s32 GrowDuration = (s32)PARAMETER(GrowingSeasonDuration);
		
		double VegIndex = PARAMETER(VegetationIndex);
		if(DOY < GrowBegin || DOY >= GrowBegin+GrowDuration)
			VegIndex = Min(VegIndex, 1.0);
	
		double Reffq = RESULT(Rainfall)*1e-3;
		double SSD = PARAMETER(SplashDetachmentScalingFactor) * Reffq * pow(PARAMETER(SplashDetachmentSoilErodibility), 10.0 / (10.0 - VegIndex));
		return Min(SSD, RESULT(ImmobileGrainStoreBeforeMobilisation));
	)
	
	EQUATION(Model, SurfaceGrainStoreBeforeTransport,
		return LAST_RESULT(SurfaceGrainStore)*(1.0 - PARAMETER(InfiltrationRate)) + RESULT(MobilisedViaSplashDetachment) + IF_INPUT_ELSE_PARAMETER(GrainInputTimeseries, GrainInput);
	)
	
	auto TotalSurfaceGrainstoreBeforeTransport = RegisterEquationCumulative(Model, "Total surface grain store before transport", SurfaceGrainStoreBeforeTransport, Class);
	
	EQUATION(Model, TransportBeforeFlowErosion,
		double potential = RESULT(SurfaceGrainStoreBeforeTransport);
		double capacity  = RESULT(SurfaceTransportCapacity);
		double totalpotential = RESULT(TotalSurfaceGrainstoreBeforeTransport);
		if(totalpotential > capacity) return SafeDivide(potential, totalpotential) * capacity;
		return potential;
	)
	
	
	EQUATION(Model, PotentiallyMobilisedViaFlowErosion,
		double qquick = RESULT(RunoffToReach, DirectRunoff);
		double flow = Max(0.0, qquick - PARAMETER(FlowErosionDirectRunoffThreshold));
		double SFE = 86400.0 * PARAMETER(FlowErosionScalingFactor) * PARAMETER(FlowErosionPotential) 
			* pow( 1e3*(PARAMETER(SubcatchmentArea) / PARAMETER(ReachLength))*flow, PARAMETER(FlowErosionNonlinearCoefficient));
		
		double surfacestorebeforeerosion = RESULT(SurfaceGrainStoreBeforeTransport) - RESULT(TransportBeforeFlowErosion);
		double m_immob = RESULT(ImmobileGrainStoreBeforeMobilisation) - RESULT(MobilisedViaSplashDetachment);
		
		if(surfacestorebeforeerosion > 0.0) return 0.0; //NOTE: If there is any surface sediment left we know that we already exceeded our transport capacity, and so we can not mobilise any more.
		
		return Min(SFE, m_immob);
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
		return PARAMETER(SubcatchmentArea) * (PARAMETER(Percent) / 100.0) * RESULT(GrainDeliveryToReach);
	)

	
	///////////////// Suspended in reach ////////////////////////////////
	
	auto InstreamSedimentSolver = RegisterSolver(Model, "In-stream sediment solver", 0.1, BoostRosenbrock4, 1e-3, 1e-3);
	
	auto SedimentReach = RegisterParameterGroup(Model, "Grain characteristics by subcatchment", Class, Reach);
	
	auto EffluentGrainConcentration       = RegisterParameterDouble(Model, SedimentReach, "Effluent grain concentration", MgPerL, 0.0, 0.0, 1e3, "Mass concentration of this type of particle in effluent input flow");
	auto InitialMassOfBedGrainPerUnitArea = RegisterParameterDouble(Model, SedimentReach, "Initial mass of bed grain per unit area", KgPerM2, 0.0, 0.0, 1e3);
	auto InitialSuspendedGrainMass        = RegisterParameterDouble(Model, SedimentReach, "Initial suspended grain mass", Kg, 0.0, 0.0, 1e8);
	auto ConstantGrainDepositionToReach   = RegisterParameterDouble(Model, SedimentReach, "Constant grain deposition to reach", KgPerDay, 0.0, 0.0, 1e6, "Direct deposition to the reach not depending on erosion");
	
	auto ShearStressCoefficient           = RegisterParameterDouble(Model, Reaches, "Shear stress coefficient", Dimensionless, 1.0, 0.01, 10.0, "Tuning parameter to account for the shear stress not being the same as in ideal conditions");
	auto EntrainmentCoefficient           = RegisterParameterDouble(Model, Reaches, "Entrainment coefficient", S2PerKg, 1.0, 1e-8, 1.0, "Tuning parameter to determine how the entrainment flux depends on the excess shear stress");  //TODO: Need new unit
	auto MedianSedimentGrainSize          = RegisterParameterDouble(Model, Reaches, "Median bed sediment grain size", Metres, 0.00045, 0.0, 0.05, "The median of of the size of mineral bed sediments. Higher values shade small partices from entrainment.");
	
	auto ReachMassTransferRateBetweenClasses = RegisterParameterDouble(Model, TransferMatrix, "Mass transfer rate between classes in the reach", Dimensionless, 0.0, 0.0, 1.0);
	
	auto ReachKinematicViscosity             = RegisterEquation(Model, "Reach kinematic viscosity", M2PerS);
	
	auto TerminalSettlingVelocity            = RegisterEquation(Model, "Terminal settling velocity", MPerS);
	
	auto ReachShearStress                    = RegisterEquation(Model, "Reach shear stress", NewtonPerM2);
	auto SedimentBedCriticalShieldsParameter = RegisterEquation(Model, "Sediment bed critical Shields parameter", Dimensionless);
	auto ClassCriticalShieldsParameter       = RegisterEquation(Model, "Critical Shields parameter (dimensionless critical shear stress) for class", Dimensionless);
	auto ClassCriticalShearStress            = RegisterEquation(Model, "Critical shear stress for entrainment of class", NewtonPerM2);
	auto MaxGrainSizeForEntrainment          = RegisterEquation(Model, "Max grain size that can be entrained", Metres);
	
	auto ProportionOfGrainThatCanBeEntrained = RegisterEquation(Model, "Proportion of grain that can be entrained", Dimensionless);
	
	auto ReachUpstreamSuspendedGrain         = RegisterEquation(Model, "Reach upstream suspended grain", KgPerDay);
	
	auto EffluentGrain                       = RegisterEquation(Model, "Effluent grain inputs", KgPerDay);
	
	auto GrainAbstraction                    = RegisterEquation(Model, "Grain abstraction", KgPerDay, InstreamSedimentSolver);
	auto GrainEntrainment                    = RegisterEquation(Model, "Grain entrainment", KgPerM2PerDay, InstreamSedimentSolver);
	auto GrainDeposition                     = RegisterEquation(Model, "Grain deposition", KgPerM2PerDay, InstreamSedimentSolver);
	auto ReachSuspendedGrainOutput           = RegisterEquation(Model, "Reach suspended grain output", KgPerDay, InstreamSedimentSolver);
	auto MassOfBedGrainPerUnitArea           = RegisterEquationODE(Model, "Mass of bed grain per unit area", KgPerM2, InstreamSedimentSolver);
	SetInitialValue(Model, MassOfBedGrainPerUnitArea, InitialMassOfBedGrainPerUnitArea);
	
	auto TotalMassOfBedGrainPerUnitArea      = RegisterEquationCumulative(Model, "Total mass of bed grain per unit area", MassOfBedGrainPerUnitArea, Class);
	
	auto SuspendedGrainMass                  = RegisterEquationODE(Model, "Suspended grain mass", Kg, InstreamSedimentSolver);
	SetInitialValue(Model, SuspendedGrainMass, InitialSuspendedGrainMass);
	
	auto SuspendedGrainConcentration         = RegisterEquation(Model, "Suspended grain concentration", MgPerL);
	
	
	auto EffluentFlow = GetParameterDoubleHandle(Model, "Effluent flow");
	auto EffluentTimeseries = GetInputHandle(Model, "Effluent flow");
	
	auto MeanChannelSlope = GetParameterDoubleHandle(Model, "Reach slope");
	
	auto ReachBottomWidth = GetParameterDoubleHandle(Model, "Reach bottom width");
	
	auto ReachDepth    = GetEquationHandle(Model, "Reach depth");
	auto ReachFlow     = GetEquationHandle(Model, "Reach flow");
	auto ReachVolume   = GetEquationHandle(Model, "Reach volume");
	auto ReachVelocity = GetEquationHandle(Model, "Reach velocity");
	auto ReachHydraulicRadius = GetEquationHandle(Model, "Reach hydraulic radius");
	auto ReachAbstraction = GetEquationHandle(Model, "Reach abstraction");
	
	auto WaterTemperature = GetEquationHandle(Model, "Water temperature");
	
	EQUATION(Model, ReachKinematicViscosity,
		return 0.00285 * std::exp(-0.027*(RESULT(WaterTemperature) + 273.15));
	)
	
	EQUATION(Model, TerminalSettlingVelocity,
		//NOTE: Assume a particle with major diameter of the median of the class is representative for the class.
		double a = (PARAMETER(LargestDiameterOfClass) + PARAMETER(SmallestDiameterOfClass)) / 2.0;
		
		//NOTE: The below assumption of b=c works for either round or long cylindrical and fibre shapes. Not so much for disk-like shapes.
		double b = a / PARAMETER(RatioOfMajorToMinor);
		double c = b;
		double waterkinematicviscosity = RESULT(ReachKinematicViscosity);
		double waterdensity            = 1e3;       //TODO: Should also be temperature-adjusted?
		double plasticdensity          = PARAMETER(DensityOfClass);
		int    ShapeType               = (int) PARAMETER(ClassShapeType);
		
		// Hack to not make it crash on setup evaluation
		if(!RunState__->Running) return 0.0;
		
		//TODO: Support more shape types and geometries later
		return ComputeTerminalSettlingVelocity(a, b, c, waterkinematicviscosity, waterdensity, plasticdensity, ShapeType);
	)
	
	EQUATION(Model, ReachShearStress,
		double waterdensity = 1e3;
		double earthsurfacegravity = 9.807;
		return PARAMETER(ShearStressCoefficient) * PARAMETER(MeanChannelSlope) * RESULT(ReachHydraulicRadius) * waterdensity * earthsurfacegravity;
	)
	
	EQUATION(Model, SedimentBedCriticalShieldsParameter,
		double earthsurfacegravity = 9.807;
		double waterdensity = 1000.0;
		double waterkinematicviscosity = RESULT(ReachKinematicViscosity);
		double sedimentdensity = 2650.0;
		
		//Dimensionless particle diameter for the median mineral sediment particle.
		double dstar = std::cbrt(((sedimentdensity - waterdensity)/waterdensity)*earthsurfacegravity/(waterkinematicviscosity*waterkinematicviscosity))*PARAMETER(MedianSedimentGrainSize);
		
		if(dstar <= 4.0)   return 0.24 / dstar;
		if(dstar <= 10.0)  return 0.14*std::pow(dstar, -0.64);
		if(dstar <= 20.0)  return 0.04*std::pow(dstar, -0.1);
		if(dstar <= 150.0) return 0.013*std::pow(dstar, 0.29);
		return 0.055;
	)
	
	EQUATION(Model, ClassCriticalShieldsParameter,

		// Equivalent particle diameter of minimal particle
		double d_equi = ParticleEquivalentDiameter(PARAMETER(SmallestDiameterOfClass), PARAMETER(RatioOfMajorToMinor), (int)PARAMETER(ClassShapeType));
		
		return 0.5588*RESULT(SedimentBedCriticalShieldsParameter) * std::pow(d_equi / PARAMETER(MedianSedimentGrainSize), -0.503);
	)
	
	EQUATION(Model, ClassCriticalShearStress,
		
		// Equivalent particle diameter of minimal particle
		double d_equi = ParticleEquivalentDiameter(PARAMETER(SmallestDiameterOfClass), PARAMETER(RatioOfMajorToMinor), (int)PARAMETER(ClassShapeType));
		
		double earthsurfacegravity = 9.807;
		double waterdensity = 1000.0;  // TODO: Temperature dependence?
		return RESULT(ClassCriticalShieldsParameter) * (PARAMETER(DensityOfClass) - waterdensity) * earthsurfacegravity * d_equi;
	)
	
	EQUATION(Model, MaxGrainSizeForEntrainment,
		double earthsurfacegravity = 9.807;
		double waterdensity = 1000.0;  // TODO: Temperature dependence?
		
		return std::pow(RESULT(ReachShearStress) / (0.5588*RESULT(SedimentBedCriticalShieldsParameter)*std::pow(PARAMETER(MedianSedimentGrainSize), 0.503)*(PARAMETER(DensityOfClass) - waterdensity)*earthsurfacegravity), 1.0 / (1.0 - 0.503));
	)
	
	EQUATION(Model, ReachUpstreamSuspendedGrain,
		double sum = 0.0;
		index_t GrainIndex = CURRENT_INDEX(Class);
		for(index_t Input : BRANCH_INPUTS(Reach))
			sum += RESULT(ReachSuspendedGrainOutput, Input, GrainIndex);
	
		return sum;
	)
	
	EQUATION(Model, ReachSuspendedGrainOutput,
		return 86400.0 * RESULT(SuspendedGrainMass) * SafeDivide(RESULT(ReachFlow), RESULT(ReachVolume));
	)
	
	EQUATION(Model, ProportionOfGrainThatCanBeEntrained,
		double Dcrit = RESULT(MaxGrainSizeForEntrainment);
		double Dmin = ParticleEquivalentDiameter(PARAMETER(SmallestDiameterOfClass), PARAMETER(RatioOfMajorToMinor), (int)PARAMETER(ClassShapeType));
		double Dmax = ParticleEquivalentDiameter(PARAMETER(LargestDiameterOfClass), PARAMETER(RatioOfMajorToMinor), (int)PARAMETER(ClassShapeType));
		if(Dcrit < Dmin) return 0.0;
		if(Dcrit > Dmax) return 1.0;
		return (Dcrit - Dmin) / (Dmax - Dmin);
	)
	
	EQUATION(Model, GrainEntrainment,
		double excessstress = Max(0.0, RESULT(ReachShearStress) - RESULT(ClassCriticalShearStress)); // NOTE: It shouldn't be necessary to clamp to 0 here since ProportionOfGrainThatCanBeEntrained will be 0 if this is < 0, but we can do it for safety in case there are small numerical errors.
		
		return 86400.0  * RESULT(MassOfBedGrainPerUnitArea) * PARAMETER(EntrainmentCoefficient) * RESULT(ProportionOfGrainThatCanBeEntrained) * excessstress;
	)
	 
	EQUATION(Model, GrainDeposition,
		return 86400.0 * RESULT(TerminalSettlingVelocity) * SafeDivide(RESULT(SuspendedGrainMass), RESULT(ReachVolume));
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
		
		return effluentflow * effluentconc;
	)
	
	EQUATION(Model, SuspendedGrainMass,
		//TODO: This uses only the reach bottom for entrainment and deposition. Should maybe include the banks
		return 
			  RESULT(TotalGrainDeliveryToReach)
			+ PARAMETER(ConstantGrainDepositionToReach)
			+ RESULT(EffluentGrain)
			+ RESULT(ReachUpstreamSuspendedGrain) 
			- RESULT(ReachSuspendedGrainOutput)
			- RESULT(GrainAbstraction)
			+ PARAMETER(ReachLength) * PARAMETER(ReachBottomWidth) * (RESULT(GrainEntrainment) - RESULT(GrainDeposition));
	)
	
	EQUATION(Model, SuspendedGrainConcentration,
		return 1000.0 * SafeDivide(RESULT(SuspendedGrainMass), RESULT(ReachVolume));
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
	
	EndModule(Model);
}


