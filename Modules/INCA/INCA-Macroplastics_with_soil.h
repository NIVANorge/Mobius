

#include "../../Src/boost_solvers.h"


inline double
Sigmoid(double X)
{
	return 1.0 / (1.0 + std::exp(-X));
}

inline double
WaningExcess(double X, double Threshold)
{
	constexpr double P = 0.1;
	constexpr double Q = 10.0;
	return std::max(X - Threshold, P*Threshold) * Sigmoid(Q*(X / Threshold - 1.0));
}


static void
AddINCAMacroplasticsWithSoilModel(mobius_model *Model)
{
	BeginModule(Model, "INCA-Macroplastics", "_dev_0.1");
	//NOTE: Is designed to work with PERSiST
	
	SetModuleDescription(Model, R""""(
INCA-Macroplastics is in early development
)"""");

	auto Dimensionless = RegisterUnit(Model);
	auto Mm            = RegisterUnit(Model, "mm");
	auto PerMm         = RegisterUnit(Model, "1/mm");
	auto Metres        = RegisterUnit(Model, "m");
	auto MetresPerS    = RegisterUnit(Model, "m/s");
	auto SPerMetres    = RegisterUnit(Model, "s/m");
	auto Kg            = RegisterUnit(Model, "kg");
	auto M2            = RegisterUnit(Model, "m2");
	auto KgPerM        = RegisterUnit(Model, "kg/m");
	auto ItemsPerM     = RegisterUnit(Model, "items/m");
	auto KgPerMPerDay  = RegisterUnit(Model, "kg/(m day)");
	auto ItemsPerMPerDay = RegisterUnit(Model, "items/(m day)");
	auto NPerM2        = RegisterUnit(Model, "N/m2");
	auto PerNDay       = RegisterUnit(Model, "1/(N day)");
	auto KgPerDay      = RegisterUnit(Model, "kg/day");
	auto ItemsPerDay   = RegisterUnit(Model, "items/day");
	auto PerDay        = RegisterUnit(Model, "1/day");
	auto PerKmPerDay   = RegisterUnit(Model, "1/(km day)");
	auto PerKmPerYear  = RegisterUnit(Model, "1/(km year)");
	auto Newton        = RegisterUnit(Model, "N");
	auto SPerMDay      = RegisterUnit(Model, "1 / ((m/s)day)");
	auto Cap           = RegisterUnit(Model, "cap");
	auto KgPerCapPerDay= RegisterUnit(Model, "kg/(cap day)");
	auto CapPerKm2     = RegisterUnit(Model, "cap/km2");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	auto Soilboxes = GetIndexSetHandle(Model, "Soils");
	auto LandTypes = GetIndexSetHandle(Model, "Landscape units");
	auto Class = RegisterIndexSet(Model, "Litter class");


//////////////////////////////////////////////////////////////////////////////////////
	auto OverallParameters = RegisterParameterGroup(Model, "Overall parameters");
	
	//In river attachment - detachment parameters
	auto BankAttachmentTuning = RegisterParameterDouble(Model, OverallParameters, "Bank attachment rate tuning parameter", PerDay, 0.1, 0.0, 100.0, "Reference banking rate for curving radius = 1000, width = 50, and c_d*A = 0.05");
	auto DetachmentThresholdBase  = RegisterParameterDouble(Model, OverallParameters, "Stress threshold tuning parameter", Newton, 1.0, 0.0, 100.0, "Force needed to detach a hard plastic item from a non-vegetation bank");
	auto DetachmentRateTuning = RegisterParameterDouble(Model, OverallParameters, "Detachment rate tuning parameter", PerNDay, 0.1, 0.0, 100.0, "Detachment rate per unit of excess share force");
	auto BagTearing           = RegisterParameterDouble(Model, OverallParameters, "Bag tearing tuning parameter", PerNDay, 0.01, 0.0, 10.0, "Bag breakdown rate as effect of drag forces");
	auto GrindingRate         = RegisterParameterDouble(Model, OverallParameters, "Micro-grinding tuning parameter", SPerMDay, 1e-6, 0.0, 1.0, "Grinding of plastic into microplastic by river sediments relative to flow velocity.");

	//Soil mobilization parameters and wind speed timeseries
	auto WindMobi        = RegisterParameterDouble(Model, OverallParameters, "Wind mobilization probability", SPerMetres, 0.007, 0.001, 0.01);
	auto RainMobi        = RegisterParameterDouble(Model, OverallParameters, "Surface runoff mobilization probability", PerMm, 0.1, 0.5, 0.05);
	auto RunoffThreshold        = RegisterParameterDouble(Model, OverallParameters, "Threshold for surface runoff mobilization", Mm, 0.5, 0.1, 1);
	auto RunoffThresholdExtrem        = RegisterParameterDouble(Model, OverallParameters, "Threshold for surface runoff mobilization of buried items during extreme events", Mm, 6, 3, 10);
	auto WindSpeedThreshold        = RegisterParameterDouble(Model, OverallParameters, "Threshold for wind mobilization", MetresPerS, 25, 5, 50);
	auto WindSpeed                   = RegisterInput(Model, "Wind speed", MetresPerS);

	//Litter input parameters or timeseries
	auto Mismanagement                   = RegisterParameterDouble(Model, OverallParameters, "Mismanagement", Dimensionless, 0.8, 0.0, 1.0, "Proportion of waste that is mismanaged");
	auto MismanagementTimeseries                   = RegisterInput(Model, "Mismanagement", Dimensionless);
	auto MisManagAdjust                  = RegisterParameterDouble(Model, OverallParameters, "Mismanagement adjustment factor", Dimensionless, 0, 0.0, 1.0, "Mismanagement adjustment factor");
	auto PlasticPerc                   = RegisterParameterDouble(Model, OverallParameters, "Plastic percentage", Dimensionless, 0.15, 0.0, 1.0, "Percentage of plastic in waste");
	auto PlasticPercTimeseries                   = RegisterInput(Model, "Plastic percentage", Dimensionless);
	auto TotalPop                   = RegisterParameterDouble(Model, OverallParameters, "Total population", Cap, 1000000, 10, 10000000, "Total population in the whole catchment");
	auto TotalPopTimeseries                   = RegisterInput(Model, "Total population", Cap);
	auto WasteGene                   = RegisterParameterDouble(Model, OverallParameters, "Waste generation rate", KgPerCapPerDay, 0.5, 0.01, 5.0, "Total solid waste generation rate");
	auto WasteGeneTimeseries                   = RegisterInput(Model, "Waste generation rate", KgPerCapPerDay);

//////////////////////////////////////////////////////////////////////////////////////
	auto ClassParameters      = RegisterParameterGroup(Model, "Litter class", Class);

	//TODO: it should be unnecessary to provide the list of strings in the registration of the enum (?).
	//Parameters for litter types
	auto LitterType                    = RegisterParameterEnum(Model, ClassParameters, "Litter type", {"Open_bottle", "Capped_bottle", "Bag", "Margarine_tub", "PVC_piece"}, "Open_bottle");
	auto OpenBottle                    = EnumValue(Model, LitterType, "Open_bottle");
	auto CappedBottle                  = EnumValue(Model, LitterType, "Capped_bottle");
	auto Bag                           = EnumValue(Model, LitterType, "Bag");
	auto MargarineTub                  = EnumValue(Model, LitterType, "Margarine_tub");
	auto PVCPiece                      = EnumValue(Model, LitterType, "PVC_piece");

	auto ItemDim                       = RegisterParameterDouble(Model, ClassParameters, "Item major axis", Metres, 0.2, 0.0, 1.0);
	auto ItemMass                      = RegisterParameterDouble(Model, ClassParameters, "Item average mass", Kg, 0.1, 1e-3, 10.0);
	auto PropensityToStayStuck         = RegisterParameterDouble(Model, ClassParameters, "Propensity to stay stuck", Dimensionless, 1.0, 0.0, 100.0, "Tuning parameter that makes it easier or harder for this item class to be remobilized");
	auto ItemPerc                      = RegisterParameterDouble(Model, ClassParameters, "Item percentage", Dimensionless, 0.25, 0.0, 1.0);
	auto ItemPercTimeseries            = RegisterInput(Model, "Item percentage", Dimensionless);

	auto DragCoefficient = RegisterParameterDouble(Model, ClassParameters, "Drag coefficient", Dimensionless, 0.47);
	auto SetDragCoefficient = RegisterEquationInitialValue(Model, "Set drag coefficient", Dimensionless);
	ParameterIsComputedBy(Model, DragCoefficient, SetDragCoefficient, true);
	/*
		NOTE: Drag coefficients are roughly based on common values for shapes https://en.wikipedia.org/wiki/Drag_coefficient
	
		Capped bottle:
			Long cylinder : From top: 0.82, from the side (cube/rectangle): 1.05
		Open bottle:
			Like capped bottle, but a little higher to accommodate for the extra drag from the hole (if it is aligned against the flow)
		Bag:
			Something close to a flat plane (1.17) or a curved plane. (1.2 - 2.2 depending on facing). Though it is complicated because it could bend to align with the flow.
		MargarineTub:
			Curved plane, between 1.2 and 2.2 depending on facing, but could also be a rectangle (1.05) if aligned sideways
		PVC piece:
			We just assume it is a cube/rectangle.
	*/
	
	auto DragArea = RegisterParameterDouble(Model, ClassParameters, "Drag area", M2, 0.04);
	auto SetDragArea = RegisterEquationInitialValue(Model, "Set drag area", M2);
	ParameterIsComputedBy(Model, DragArea, SetDragArea, true);
	/*
		The drag area could be highly dependent on facing for the different items. For bags it is even more complicated since they could bend with the flow. This is why you are
		allowed to do separate tuning of the "propensity to get stuck".
	*/

//////////////////////////////////////////////////////////////////////////////////////
	auto ReachParameters      = RegisterParameterGroup(Model, "Reach characteristics", Reach);

	// Reach parameters on shape, vegetation and population density
	auto AverageCurvingRadius          = RegisterParameterDouble(Model, ReachParameters, "Average curving radius", Metres, 1000.0, 20.0, 1e10, "If the river is straight, just set this to a high number (e.g. 1e10)");
	auto BankVegetationType            = RegisterParameterEnum(Model, ReachParameters, "Bank vegetation type", {"Bushes", "Reeds", "Grass"}, "Grass");
	auto BankVegetationDensity         = RegisterParameterDouble(Model, ReachParameters, "Bank vegetation density", Dimensionless, 0.0, 0.0, 1.0);
	auto RiverVegetationType           = RegisterParameterEnum(Model, ReachParameters, "River vegetation type", {"None", "Reeds", "Mangrove"}, "None");
	auto RiverVegetationDensity        = RegisterParameterDouble(Model, ReachParameters, "River vegetation density", Dimensionless, 0.0, 0.0, 1.0);
	auto DistanceToRiver               = RegisterParameterDouble(Model, ReachParameters, "Average distance from Soil to River banks", Metres, 5000, 10, 100000);
	auto PopDensityReach               = RegisterParameterDouble(Model, ReachParameters, "Population density in the reach subcatchment at the end of the simulation", CapPerKm2, 2000, 0, 200000);

	// In case the user wants to define litter inputs by reach and litter class - it makes a lot of input parameters or timeseries: n reaches * m types
	/*auto ClassReachParameters = RegisterParameterGroup(Model, "Litter by reach and class", Class, Reach);

	auto InputToSoil                   = RegisterParameterDouble(Model, ClassReachParameters, "Litter input to soil", ItemsPerMPerDay, 0.0, 0.0, 100.0, "Constant daily litter to the soil from land sources");
	auto InputToSoilTimeseries                   = RegisterInput(Model, "Litter input to soil", ItemsPerMPerDay);
	*/

//////////////////////////////////////////////////////////////////////////////////////
	auto SoilRetentionParameters = RegisterParameterGroup(Model, "Soil retention parameters", LandTypes);
	// Parameter for litter soil retention
	auto MobilizProbability      = RegisterParameterDouble(Model, SoilRetentionParameters, "Probability to reach the river in each Land use type", PerKmPerYear, 0.6, 0.0, 0.97);
	auto MobilizProbabilityExtrem      = RegisterParameterDouble(Model, SoilRetentionParameters, "Mobilization probability during extreme rain events in each Land use type", PerKmPerDay, 0.1, 0.0, 0.2);
	auto SoilBurial              = RegisterParameterDouble(Model, SoilRetentionParameters, "Burial proportion in each Land use type", PerDay, 0.001, 0.01, 0.0001);

//////////////////////////////////////////////////////////////////////////////////////
	//Equations for DragCoefficient and DragArea
	EQUATION(Model, SetDragCoefficient,
		
		auto type = PARAMETER(LitterType);
		if(     type == OpenBottle)
			return 0.95;         //TODO: get a good value here
		else if(type == CappedBottle)
			return 0.9;
		else if(type == Bag)
			return 2.0;
		else if(type == MargarineTub)
			return 1.2;
		else if(type == PVCPiece)
			return 1.05;
		else
			FatalError("ERROR: INCA-Macroplastics: unsupported litter type!\n");
		return 0.0;
	)
	
	EQUATION(Model, SetDragArea,            //based on typical dimension of plastic items
		auto type = PARAMETER(LitterType);
		if(     type == OpenBottle)
			return PARAMETER(ItemDim)*PARAMETER(ItemDim)/3.7;
		else if(type == CappedBottle)
			return PARAMETER(ItemDim)*PARAMETER(ItemDim)/3.7;
		else if(type == Bag)
			return PARAMETER(ItemDim)*PARAMETER(ItemDim)/1.7;
		else if(type == MargarineTub)
			return PARAMETER(ItemDim)*PARAMETER(ItemDim)/1.3;
		else if(type == PVCPiece)
			return PARAMETER(ItemDim)*PARAMETER(ItemDim)*0.5;
		else
			FatalError("ERROR: INCA-Macroplastics: unsupported litter type!\n");
		return 0.0;
	)
// MargarineTub PARAMETER(ItemDim)*PARAMETER(ItemDim)/1.3;
// OpenBottle PARAMETER(ItemDim)*PARAMETER(ItemDim)/3.7;
// CappedBottle PARAMETER(ItemDim)*PARAMETER(ItemDim)/3.7;
// PVCPiece PARAMETER(ItemDim)*PARAMETER(ItemDim)*0.5;
// Bag

//////////////////////////////////////////////////////////////////////////////////////
	// Collecting parameters and equation results from PERSiST
	auto ReachLength      = GetParameterDoubleHandle(Model, "Reach length");
	auto ReachArea        = GetParameterDoubleHandle(Model, "Terrestrial catchment area");
	auto MeanChannelSlope = GetParameterDoubleHandle(Model, "Reach slope");
	auto ReachBottomWidth = GetParameterDoubleHandle(Model, "Reach bottom width");
	auto Percent          = GetParameterDoubleHandle(Model, "%");

	auto ReachHydraulicRadius  = GetEquationHandle(Model, "Reach hydraulic radius");
	auto RunoffToReach         = GetEquationHandle(Model, "Runoff to reach");
	auto ReachVelocity         = GetEquationHandle(Model, "Reach velocity");

//////////////////////////////////////////////////////////////////////////////////////
	// Setting up solvers for reach-, soil- and river- specific calculations
	auto ReachSolver    = GetSolverHandle(Model, "Reach solver");
	auto SoilSolver     = RegisterSolver(Model, "Soil plastic solver", 0.1, BoostRosenbrock4, 1e-3, 1e-3);
	auto PlasticSolver  = RegisterSolver(Model, "In-stream plastic solver", 0.1, BoostRosenbrock4, 1e-3, 1e-3);
	//auto PlasticSolver = RegisterSolver(Model, "In-stream plastic solver", 0.1, IncaDascru);
	
	auto ReachShearStress = RegisterEquation(Model, "Reach shear stress", NPerM2, ReachSolver);
	if(!Model->Parameters.Has("Shear stress coefficient"))
	{
		auto ShearStressCoefficient = RegisterParameterDouble(Model, ReachParameters, "Shear stress coefficient", Dimensionless, 1.0, 0.01, 10.0, "Tuning parameter to account for the shear stress not being the same as in ideal conditions");
		
		EQUATION(Model, ReachShearStress,
			double waterdensity = 1e3;
			double earthsurfacegravity = 9.807;
			return PARAMETER(ShearStressCoefficient) * PARAMETER(MeanChannelSlope) * RESULT(ReachHydraulicRadius) * earthsurfacegravity * waterdensity;
		)
	}

//////////////////////////////////////////////////////////////////////////////////////
	// Registering equations for Soil litter inputs
	auto PopCountReach         = RegisterEquation(Model, "Population count reach end", Cap, ReachSolver);
	auto TotalPopEnd = RegisterEquationCumulative(Model, "Total population at the end", PopCountReach, Reach);

	auto SoilLitterInputs     = RegisterEquation(Model, "Soil litter inputs", ItemsPerMPerDay);
	auto SoilLitterInputsMass = RegisterEquation(Model, "Soil litter inputs mass", KgPerMPerDay);
	

//////////////////////////////////////////////////////////////////////////////////////
	// Registering equations for Soil burial and mobilization
	auto SoilDetachmentRate   = RegisterEquation(Model, "Soil detachment rate", PerDay, SoilSolver);
	auto SoilDetachmentRateStuck   = RegisterEquation(Model, "Soil detachment rate (buried)", PerDay, SoilSolver);
	auto SoilDetachment       = RegisterEquation(Model, "Soil detachment", ItemsPerMPerDay, SoilSolver);
	auto SoilDetachmentStuck       = RegisterEquation(Model, "Soil detachment of buried items during extreme events", ItemsPerMPerDay, SoilSolver);
	auto SoilBurialRate       = RegisterEquation(Model, "Soil Burial", ItemsPerMPerDay, SoilSolver);
	auto SoilLitter            = RegisterEquationODE(Model, "Soil litter (not buried)", ItemsPerM, SoilSolver);
	auto SoilLitterStuck       = RegisterEquationODE(Model, "Soil litter (buried)", ItemsPerM, SoilSolver);
	auto SoilLitterMass   = RegisterEquation(Model, "Soil litter (not buried) (mass)", KgPerM, SoilSolver);
	auto SoilLitterStuckMass = RegisterEquation(Model, "Soil litter (buried) (mass)", KgPerM, SoilSolver);
	auto SoilLitterMassTotal = RegisterEquation(Model, "Soil litter (total) (mass)", KgPerM, SoilSolver);

	auto TotalSoilDetachment = RegisterEquationCumulative(Model, "Soil detachment all LandTypes", SoilDetachment, LandTypes);
	auto TotalSoilDetachmentStuck = RegisterEquationCumulative(Model, "Soil detachment all LandTypes (buried)", SoilDetachmentStuck, LandTypes);
	auto TotalSoilLitter = RegisterEquationCumulative(Model, "Soil litter (buried or not) all LandTypes", SoilLitterMassTotal, LandTypes);
	auto TotalTotalSoilLitter = RegisterEquationCumulative(Model, "Total soil litter (buried or not) all LandTypes", TotalSoilLitter, Class);
	auto TotalTotalSoilLitterReach = RegisterEquation(Model, "Total soil litter (buried or not) along reach all LandTypes", Kg, PlasticSolver);
	auto TotalTotalTotalSoilLitter = RegisterEquationCumulative(Model, "Total soil litter (buried or not) in all reaches all LandTypes", TotalTotalSoilLitterReach, Reach);
	auto TotalSoilLitterNB = RegisterEquationCumulative(Model, "Soil litter (not buried) all LandTypes", SoilLitterMass, LandTypes);
	auto TotalTotalSoilLitterNB = RegisterEquationCumulative(Model, "Total soil litter (not buried) all LandTypes", TotalSoilLitterNB, Class);
	auto TotalTotalSoilLitterNBReach = RegisterEquation(Model, "Total soil litter (not buried) along reach all LandTypes", Kg, PlasticSolver);
	auto TotalTotalTotalSoilLitterNB = RegisterEquationCumulative(Model, "Total soil litter (not buried) in all reaches all LandTypes", TotalTotalSoilLitterNBReach, Reach);

//////////////////////////////////////////////////////////////////////////////////////
	// Registering equations for in-river transport and trapping
	auto FloatingLitterUpstream = RegisterEquation(Model, "Floating litter input from upstream", ItemsPerDay);

	auto BankDragForce        = RegisterEquation(Model, "Drag force on banked items", Newton, PlasticSolver);
	auto RiverVegDragForce    = RegisterEquation(Model, "Drag force on in-river vegetation-attached items", Newton, PlasticSolver);

	auto BankDetachmentRate   = RegisterEquation(Model, "Bank detachment rate (not stuck)", PerDay, PlasticSolver);
	auto BankDetachmentRateStuck = RegisterEquation(Model, "Bank detachment rate (stuck)", PerDay, PlasticSolver);

	auto BankAttachment       = RegisterEquation(Model, "Banking", ItemsPerMPerDay, PlasticSolver);
	auto BankDetachment       = RegisterEquation(Model, "Bank detachment (not stuck)", ItemsPerMPerDay, PlasticSolver);
	auto BankDetachmentStuck  = RegisterEquation(Model, "Bank detachment (stuck)", ItemsPerMPerDay, PlasticSolver);

	auto RiverVegAttachment   = RegisterEquation(Model, "In-river vegetation attachment", ItemsPerMPerDay, PlasticSolver);
	auto RiverVegDetachment   = RegisterEquation(Model, "In-river vegetation detachment", ItemsPerMPerDay, PlasticSolver);

	auto BankLitterBreakdown   = RegisterEquation(Model, "Bank litter breakdown", KgPerDay, PlasticSolver);
	auto VegLitterBreakdown    = RegisterEquation(Model, "River vegetation litter breakdown", KgPerDay, PlasticSolver);

	auto RiverBankLitter       = RegisterEquationODE(Model, "River bank litter (not stuck)", ItemsPerM, PlasticSolver);
	auto RiverBankLitterStuck  = RegisterEquationODE(Model, "River bank litter (stuck)", ItemsPerM, PlasticSolver);

	auto FloatingLitter        = RegisterEquationODE(Model, "Floating litter", ItemsPerM, PlasticSolver);
	auto RiverVegetationLitter = RegisterEquationODE(Model, "River vegetation litter", ItemsPerM, PlasticSolver);
	
	auto RiverBankLitterMass   = RegisterEquation(Model, "River bank litter (not stuck) (mass)", KgPerM, PlasticSolver);
	auto RiverBankLitterStuckMass = RegisterEquation(Model, "River bank litter (stuck) (mass)", KgPerM, PlasticSolver);
	auto RiverBankLitterMassTotal = RegisterEquation(Model, "River bank litter (total) (mass)", KgPerM, PlasticSolver);
	auto BankLitterMass = RegisterEquation(Model, "River bank litter (total) along reach (mass)", Kg, PlasticSolver);

	auto FloatingLitterMass    = RegisterEquation(Model, "Floating litter (mass)", KgPerM, PlasticSolver);
	auto RiverVegetationLitterMass = RegisterEquation(Model, "River vegetation litter (mass)", KgPerM, PlasticSolver);
	auto VegetationLitterMass = RegisterEquation(Model, "River vegetation litter along reach (mass)", Kg, PlasticSolver);

	auto FloatingLitterOutput = RegisterEquation(Model, "Floating litter output", ItemsPerDay, PlasticSolver);
	auto FloatingLitterOutputMass = RegisterEquation(Model, "Floating litter output (mass)", KgPerDay, PlasticSolver);
	auto TotalFloatingLitterOutputMass = RegisterEquationCumulative(Model, "Total floating litter output (mass)", FloatingLitterOutputMass, Class);
	auto TotalBankLitterMass = RegisterEquationCumulative(Model, "Total river bank litter (mass)", BankLitterMass, Class);
	auto TotalTotalBankLitterMass = RegisterEquationCumulative(Model, "Total river bank litter in all reaches (mass)", TotalBankLitterMass, Reach);
	auto TotalVegetationLitterMass = RegisterEquationCumulative(Model, "Total river vegetation litter (mass)", VegetationLitterMass, Class);
	auto TotalTotalRiverLitterMass = RegisterEquationCumulative(Model, "Total river vegetation litter in all reaches (mass)", TotalVegetationLitterMass, Reach);

//////////////////////////////////////////////////////////////////////////////////////
	// Equations for everything else

	EQUATION(Model, BankAttachment,
		//double wind = INPUT(WindSpeed);
	
		double R    = PARAMETER(AverageCurvingRadius);
		double w    = PARAMETER(ReachBottomWidth);  //Or use the dynamic width
		double C_DA = PARAMETER(DragCoefficient)*PARAMETER(DragArea);
		
		//NOTE: The following "attachment model" was made by sampling the result of solving an ODE system with a floating piece being transported down a river and seing if it hits the bank. It is probably not very realistic in non-ideal rivers.
		// The values of 1000 and 50 are reference values that you tune against
		// The value of 300 is a bit arbitrary (though determined-ish by the reference model). Could be a tuning parameter.
		
		double curvature = 1.0 / R;
		double curv_contrib = 1.0;
		if(!std::isfinite(curvature)) curv_contrib = std::max(0.01, std::min(curv_contrib, curvature*1000.0));
		double width_contrib = std::max(0.01, 1.0 - (w-50.0)/300.0);
		double drag_contrib = std::max(0.01, C_DA);
		
		return PARAMETER(BankAttachmentTuning)*curv_contrib*width_contrib*drag_contrib*RESULT(FloatingLitter);
	)
	
		
	double ThresholdModBank[3] = {4.0, 2.0, 1.0}; // bushes, reeds, grass. These are proportional to numbers in Table 3 of https://pubs.usgs.gov/wsp/2339/report.pdf . Scaled so that grass=1.
	
	EQUATION(Model, BankDragForce,
		double C_DA = PARAMETER(DragCoefficient)*PARAMETER(DragArea);
		return RESULT(ReachShearStress) * C_DA;
	)
	
	auto DirectRunoff = RequireIndex(Model, Soilboxes, "Direct runoff");
	
	EQUATION(Model, SoilDetachmentRate,
		double wind = INPUT(WindSpeed);
		return std::min(1.0,std::max(0.0,RESULT(RunoffToReach, DirectRunoff)- PARAMETER(RunoffThreshold)) * PARAMETER(RainMobi) + std::max(0.0,wind - PARAMETER(WindSpeedThreshold)) * PARAMETER(WindMobi)) * PARAMETER(MobilizProbability) / 365.25 / ( PARAMETER(DistanceToRiver) / 1000.0 );
	)
	
	EQUATION(Model, SoilDetachmentRateStuck,
		return std::min(1.0,std::max(0.0,RESULT(RunoffToReach, DirectRunoff)- PARAMETER(RunoffThresholdExtrem) ) * PARAMETER(RainMobi)) * PARAMETER(MobilizProbabilityExtrem) / ( PARAMETER(DistanceToRiver) / 1000.0 );
	)
	
	EQUATION(Model, BankDetachmentRate,
		/*
		double threshold = PARAMETER(DetachmentThresholdBase) * PARAMETER(PropensityToStayStuck);
		double threshold2 = threshold * ThresholdModBank[(int)PARAMETER(BankVegetationType)];
		
		double excess1 = WaningExcess(RESULT(BankDragForce), threshold);
		double excess2 = WaningExcess(RESULT(BankDragForce), threshold2);
		
		return
			(PARAMETER(DetachmentRateTuning) / PARAMETER(ItemMass)) * LinearInterpolate(PARAMETER(BankVegetationDensity), 0.0, 1.0, excess1, excess2);
			*/
		return (PARAMETER(DetachmentRateTuning) / PARAMETER(ItemMass)) * RESULT(BankDragForce);
	)
	
	EQUATION(Model, BankDetachmentRateStuck,
		double threshold = PARAMETER(DetachmentThresholdBase) * PARAMETER(PropensityToStayStuck) * ThresholdModBank[(int)PARAMETER(BankVegetationType)];
		
		return (PARAMETER(DetachmentRateTuning) / PARAMETER(ItemMass)) * WaningExcess(RESULT(BankDragForce), threshold);
	)
	
	EQUATION(Model, BankDetachment,
		return RESULT(BankDetachmentRate) * RESULT(RiverBankLitter);
	)
	
	EQUATION(Model, BankDetachmentStuck,
		return RESULT(BankDetachmentRateStuck) * RESULT(RiverBankLitterStuck);
	)
	
	EQUATION(Model, RiverVegAttachment,
		u64 type = PARAMETER(RiverVegetationType);
		double rate = RESULT(FloatingLitter)*RESULT(ReachVelocity)*PARAMETER(RiverVegetationDensity)*86400.0;
		if(type == 0) rate = 0.0;
		return rate;
		
	)
	
	EQUATION(Model, SoilDetachment,
		return RESULT(SoilDetachmentRate) * RESULT(SoilLitter);
	)
	
	EQUATION(Model, SoilDetachmentStuck,
		return RESULT(SoilDetachmentRateStuck) * RESULT(SoilLitterStuck);
	)
	
	EQUATION(Model, PopCountReach,
		return PARAMETER(PopDensityReach) * PARAMETER(ReachArea);
	)
	
	EQUATION(Model, SoilBurialRate,
		return PARAMETER(SoilBurial) * RESULT(SoilLitter);
	)
	
	EQUATION(Model, TotalTotalSoilLitterReach,
		return RESULT(TotalTotalSoilLitter) * PARAMETER(ReachLength);
	)

	EQUATION(Model, TotalTotalSoilLitterNBReach,
		return RESULT(TotalTotalSoilLitterNB) * PARAMETER(ReachLength);
	)
	
	double ThresholdModRiver[4] = {0.0, 1.0, 4.0}; // none, reeds, mangrove..  //TODO: arbitrary numbers...
	
	EQUATION(Model, RiverVegDragForce,
		double waterdensity = 1e3;
		double C_DA = PARAMETER(DragCoefficient)*PARAMETER(DragArea);
		double U = RESULT(ReachVelocity); //TODO: Should we use shear velocity here like in bank detachment?
		double shearstress = waterdensity*U*U;
		return shearstress * C_DA;
	)
	
	EQUATION(Model, RiverVegDetachment,
		double threshold = PARAMETER(DetachmentThresholdBase) * PARAMETER(PropensityToStayStuck) * ThresholdModRiver[(int)PARAMETER(RiverVegetationType)];
		
		return WaningExcess(RESULT(RiverVegDragForce), threshold) * PARAMETER(DetachmentRateTuning) * RESULT(RiverVegetationLitter) / PARAMETER(ItemMass);
	)
	
	// open bottle, capped bottle, bag, margarine tub, pvc piece
	double HardnessMod[5] = { 1.0, 1.0, 0.3, 0.7, 2.0 };          // NOTE: value is loosely based on hardness of plastic types.
	
	
	//TODO: Torn bags are still macroplastic, they should really go in a separate category of less-attachable bags rather than being removed as microplastic.
	
	EQUATION(Model, BankLitterBreakdown,
		u64 type = PARAMETER(LitterType);
		double shear_velocity = std::sqrt(RESULT(ReachShearStress)*1e-3);
		double breakdown_rate = PARAMETER(GrindingRate) * shear_velocity / HardnessMod[type];
		
		double drag_force = RESULT(BankDragForce);
		if(type == Bag) breakdown_rate += PARAMETER(BagTearing) * drag_force;
			
		return RESULT(RiverBankLitterMassTotal) * breakdown_rate;
	)
	
	EQUATION(Model, VegLitterBreakdown,
		u64 type = PARAMETER(LitterType);
		double breakdown_rate = PARAMETER(GrindingRate) * RESULT(ReachVelocity) / HardnessMod[type];
		
		double drag_force = RESULT(RiverVegDragForce);
		if(type == Bag) breakdown_rate += PARAMETER(BagTearing) * drag_force;
			
		return RESULT(RiverVegetationLitterMass) * breakdown_rate;
	)
	
//	EQUATION(Model, BankLitterInputs,
//		double litInputs = IF_INPUT_ELSE_PARAMETER(InputToBankTimeseries, InputToBank);
//		return litInputs;
//	)
	
	EQUATION(Model, RiverBankLitter,
		double ratio = SafeDivide(RESULT(RiverBankLitterMass), RESULT(RiverBankLitterMassTotal) );
		return
		(  RESULT(TotalSoilDetachment) + RESULT(TotalSoilDetachmentStuck) + RESULT(BankAttachment) ) * (1.0 - PARAMETER(BankVegetationDensity))   //TODO: Make smaller items less likely to be stuck in the first place?
		- RESULT(BankDetachment)
		- ratio * RESULT(BankLitterBreakdown)/PARAMETER(ItemMass);
	)
	
	EQUATION(Model, RiverBankLitterStuck,
		double ratio = SafeDivide(RESULT(RiverBankLitterStuckMass), RESULT(RiverBankLitterMassTotal) );
		return
		(  RESULT(TotalSoilDetachment) + RESULT(TotalSoilDetachmentStuck) + RESULT(BankAttachment) ) * PARAMETER(BankVegetationDensity)
		- RESULT(BankDetachmentStuck)
		- ratio * RESULT(BankLitterBreakdown)/PARAMETER(ItemMass);
	)
	
	EQUATION(Model, SoilLitterInputsMass,
		double Waste = IF_INPUT_ELSE_PARAMETER(WasteGeneTimeseries, WasteGene);
		double Plastic = IF_INPUT_ELSE_PARAMETER(PlasticPercTimeseries, PlasticPerc);
		double Mismanag = IF_INPUT_ELSE_PARAMETER(MismanagementTimeseries, Mismanagement);
		double Population = IF_INPUT_ELSE_PARAMETER(TotalPopTimeseries, TotalPop);
		double ItemPercentage = IF_INPUT_ELSE_PARAMETER(ItemPercTimeseries, ItemPerc);
		return Waste * (Population * RESULT(PopCountReach) / RESULT(TotalPopEnd)) * Plastic * (Mismanag +((1.0 - Mismanag) * PARAMETER(MisManagAdjust))) * ItemPercentage / PARAMETER(ReachLength) * PARAMETER(Percent) / 100.0;
	)
	
	EQUATION(Model, SoilLitterInputs,
		return RESULT(SoilLitterInputsMass) / PARAMETER(ItemMass);
	)
	
	EQUATION(Model, SoilLitter,
		return
		RESULT(SoilLitterInputs) 
		- RESULT(SoilBurialRate)
		- RESULT(SoilDetachment);
	)
	
	EQUATION(Model, SoilLitterStuck,
		return
		RESULT(SoilBurialRate)
		- RESULT(SoilDetachmentStuck);
	)
	
	EQUATION(Model, RiverVegetationLitter,
		return
		  RESULT(RiverVegAttachment)
		- RESULT(RiverVegDetachment)
		- RESULT(VegLitterBreakdown)/PARAMETER(ItemMass);
	)
	
	EQUATION(Model, FloatingLitter,
		return
		  RESULT(FloatingLitterUpstream) / PARAMETER(ReachLength)
		- RESULT(FloatingLitterOutput) / PARAMETER(ReachLength)
		+ RESULT(BankDetachment)
		+ RESULT(BankDetachmentStuck)
		- RESULT(BankAttachment)
		+ RESULT(RiverVegDetachment)
		- RESULT(RiverVegAttachment);
	)
	
	EQUATION(Model, RiverBankLitterMass,
		return RESULT(RiverBankLitter) * PARAMETER(ItemMass);
	)
	
	EQUATION(Model, RiverBankLitterStuckMass,
		return RESULT(RiverBankLitterStuck) * PARAMETER(ItemMass);
	)
	
	EQUATION(Model, RiverBankLitterMassTotal,
		return RESULT(RiverBankLitterMass) + RESULT(RiverBankLitterStuckMass);
	)
	
		EQUATION(Model, SoilLitterMass,
		return RESULT(SoilLitter) * PARAMETER(ItemMass);
	)
	
	EQUATION(Model, SoilLitterStuckMass,
		return RESULT(SoilLitterStuck) * PARAMETER(ItemMass);
	)
	
	EQUATION(Model, SoilLitterMassTotal,
		return RESULT(SoilLitterMass) + RESULT(SoilLitterStuckMass);
	)
	
	EQUATION(Model, FloatingLitterMass,
		return RESULT(FloatingLitter) * PARAMETER(ItemMass);
	)
	
	EQUATION(Model, RiverVegetationLitterMass,
		return RESULT(RiverVegetationLitter) * PARAMETER(ItemMass);
	)
	
	EQUATION(Model, FloatingLitterOutput,
		return RESULT(FloatingLitter) * RESULT(ReachVelocity) * 86400.0;   // m/s -> m/day
	)
	
	EQUATION(Model, FloatingLitterOutputMass,
		return RESULT(FloatingLitterOutput) * PARAMETER(ItemMass);
	)

	EQUATION(Model, VegetationLitterMass,
		return RESULT(RiverVegetationLitterMass) * PARAMETER(ReachLength);
	)

	EQUATION(Model, BankLitterMass,
		return RESULT(RiverBankLitterMassTotal) * PARAMETER(ReachLength);
	)

	EQUATION(Model, FloatingLitterUpstream,
		double sum = 0.0;
		index_t ClassIndex = CURRENT_INDEX(Class);
		for(index_t Input : BRANCH_INPUTS(Reach))
			sum += RESULT(FloatingLitterOutput, Input, ClassIndex);
	
		return sum;
	)

	EndModule(Model);
}


