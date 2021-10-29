

#include "../../Src/boost_solvers.h"


static void
AddINCAMacroplasticsModel(mobius_model *Model)
{
	BeginModule(Model, "INCA-Macroplastics", "_dev_0.1");
	//NOTE: Is designed to work with PERSiST
	
	SetModuleDescription(Model, R""""(
INCA-Macroplastics is in early development
)"""");

	auto Dimensionless = RegisterUnit(Model);
	auto Metres        = RegisterUnit(Model, "m");
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
	auto Newton        = RegisterUnit(Model, "N");
	auto SPerMDay      = RegisterUnit(Model, "1 / ((m/s)day)");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");
	auto Class = RegisterIndexSet(Model, "Litter class");



	auto OverallParameters = RegisterParameterGroup(Model, "Overall parameters");
	
	auto BankAttachmentTuning = RegisterParameterDouble(Model, OverallParameters, "Bank attachment rate tuning parameter", PerDay, 0.1, 0.0, 100.0, "Reference banking rate for curving radius = 1000, width = 50, and c_d*A = 0.05");
	auto DetachmentThresholdBase  = RegisterParameterDouble(Model, OverallParameters, "Stress threshold tuning parameter", Newton, 1.0, 0.0, 100.0, "Force needed to detach a hard plastic item from a non-vegetation bank");
	auto DetachmentRateTuning = RegisterParameterDouble(Model, OverallParameters, "Detachment rate tuning parameter", PerNDay, 0.1, 0.0, 100.0, "Detachment rate per unit of excess share force");
	auto BagTearing           = RegisterParameterDouble(Model, OverallParameters, "Bag tearing tuning parameter", PerNDay, 0.01, 0.0, 10.0, "Bag breakdown rate as effect of drag forces");
	auto GrindingRate         = RegisterParameterDouble(Model, OverallParameters, "Micro-grinding tuning parameter", SPerMDay, 1e-6, 0.0, 1.0, "Grinding of plastic into microplastic by river sediments relative to flow velocity.");
	
	
	auto ClassParameters = RegisterParameterGroup(Model, "Litter class", Class);

	//TODO: it should be unnecessary to provide the list of strings in the registration of the enum (?).
	auto LitterType                    = RegisterParameterEnum(Model, ClassParameters, "Litter type", {"Open_bottle", "Capped_bottle", "Bag", "Margarine_tub", "PVC_piece"}, "Open_bottle");
	auto OpenBottle                    = EnumValue(Model, LitterType, "Open_bottle");
	auto CappedBottle                  = EnumValue(Model, LitterType, "Capped_bottle");
	auto Bag                           = EnumValue(Model, LitterType, "Bag");
	auto MargarineTub                  = EnumValue(Model, LitterType, "Margarine_tub");
	auto PVCPiece                      = EnumValue(Model, LitterType, "PVC_piece");

	auto ItemDim                       = RegisterParameterDouble(Model, ClassParameters, "Item major axis", Metres, 0.2, 0.0, 1.0);
	auto ItemMass                      = RegisterParameterDouble(Model, ClassParameters, "Item average mass", Kg, 0.1, 1e-3, 10.0);
	auto PropensityToStayStuck         = RegisterParameterDouble(Model, ClassParameters, "Propensity to stay stuck", Dimensionless, 1.0, 0.0, 100.0, "Tuning parameter that makes it easier or harder for this item class to be remobilized");

	auto ReachParameters      = RegisterParameterGroup(Model, "Reach characteristics", Reach);
	
	auto AverageCurvingRadius          = RegisterParameterDouble(Model, ReachParameters, "Average curving radius", Metres, 1000.0, 20.0, 1e10, "If the river is straight, just set this to a high number (e.g. 1e10)");
	auto BankVegetationType            = RegisterParameterEnum(Model, ReachParameters, "Bank vegetation type", {"None", "Bushes", "Reeds", "Grass"}, "None");
	auto BankVegetationDensity         = RegisterParameterDouble(Model, ReachParameters, "Bank vegetation density", Dimensionless, 0.0, 0.0, 1.0);
	auto RiverVegetationType           = RegisterParameterEnum(Model, ReachParameters, "River vegetation type", {"None", "Reeds", "Mangrove"}, "None");
	auto RiverVegetationDensity        = RegisterParameterDouble(Model, ReachParameters, "River vegetation density", Dimensionless, 0.0, 0.0, 1.0);
	
	auto ClassReachParameters = RegisterParameterGroup(Model, "Litter by reach and class", Class, Reach);

	auto InputToBank                   = RegisterParameterDouble(Model, ClassReachParameters, "Litter input to bank", ItemsPerMPerDay, 0.0, 0.0, 100.0, "Constant daily litter to the river bank from land sources");

	
	
	auto DragCoefficient = RegisterParameterDouble(Model, ClassParameters, "Drag coefficient", Dimensionless, 0.47);
	auto SetDragCoefficient = RegisterEquationInitialValue(Model, "Set drag coefficient", Dimensionless);
	ParameterIsComputedBy(Model, DragCoefficient, SetDragCoefficient, true);
	
	EQUATION(Model, SetDragCoefficient,
		//NOTE: These values are just based on common values for shapes https://en.wikipedia.org/wiki/Drag_coefficient
		auto type = PARAMETER(LitterType);
		if(     type == OpenBottle)
			return 0.85;         //TODO: get a good value here
		else if(type == CappedBottle)
			return 0.82;
		else if(type == Bag)
			return 1.2;
		else if(type == MargarineTub)
			return 1.2;
		else if(type == PVCPiece)
			return 1.05;
		else
			FatalError("ERROR: INCA-Macroplastics: unsupported litter type!\n");
		return 0.0;
	)
	
	auto DragArea = RegisterParameterDouble(Model, ClassParameters, "Drag area", M2, 0.04);
	auto SetDragArea = RegisterEquationInitialValue(Model, "Set drag area", M2);
	ParameterIsComputedBy(Model, DragArea, SetDragArea, true);
	
	EQUATION(Model, SetDragArea,
		//TODO: Should this be different per item class?
		return PARAMETER(ItemDim)*PARAMETER(ItemDim)*0.5;
	)
	
	//auto WindSpeed                     = RegisterInput(Model, "Wind speed");
	


	//auto SubcatchmentArea = GetParameterDoubleHandle(Model, "Terrestrial catchment area");
	auto ReachLength      = GetParameterDoubleHandle(Model, "Reach length");
	//auto Percent          = GetParameterDoubleHandle(Model, "%");
	auto MeanChannelSlope = GetParameterDoubleHandle(Model, "Reach slope");
	auto ReachBottomWidth = GetParameterDoubleHandle(Model, "Reach bottom width");
	
	//auto Rainfall      = GetEquationHandle(Model, "Rainfall");
	//auto RunoffToReach = GetEquationHandle(Model, "Runoff to reach");
	auto ReachHydraulicRadius = GetEquationHandle(Model, "Reach hydraulic radius");
	auto ReachVelocity        = GetEquationHandle(Model, "Reach velocity");
	
	
	auto ReachSolver = GetSolverHandle(Model, "Reach solver");
	auto PlasticSolver = RegisterSolver(Model, "In-stream plastic solver", 0.1, BoostRosenbrock4, 1e-3, 1e-3);
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
	
	auto BankLitterInputs     = RegisterEquation(Model, "River bank litter inputs", ItemsPerMPerDay);
	auto FloatingLitterUpstream = RegisterEquation(Model, "Floating litter input from upstream", ItemsPerDay);
	
	auto BankDragForce        = RegisterEquation(Model, "Drag force on banked items", Newton, PlasticSolver);
	auto BankDetachmentRate   = RegisterEquation(Model, "Bank detachment rate", PerDay, PlasticSolver);
	auto BankAttachment       = RegisterEquation(Model, "Banking", ItemsPerMPerDay, PlasticSolver);
	auto BankDetachment       = RegisterEquation(Model, "Bank detachment", ItemsPerMPerDay, PlasticSolver);
	
	auto RiverVegDragForce    = RegisterEquation(Model, "Drag force on in-river vegetation-attached items", Newton, PlasticSolver);
	auto RiverVegAttachment   = RegisterEquation(Model, "In-river vegetation attachment", ItemsPerMPerDay, PlasticSolver);
	auto RiverVegDetachment   = RegisterEquation(Model, "In-river vegetation detachment", ItemsPerMPerDay, PlasticSolver);
	
	auto FloatingLitterOutput = RegisterEquation(Model, "Floating litter output", ItemsPerDay, PlasticSolver);
	auto FloatingLitterOutputMass = RegisterEquation(Model, "Floating litter output (mass)", KgPerDay, PlasticSolver);
	auto TotalFloatingLitterOutputMass = RegisterEquationCumulative(Model, "Total floating litter output (mass)", FloatingLitterOutputMass, Class);
	
	auto BankLitterBreakdown   = RegisterEquation(Model, "Bank litter breakdown", KgPerDay, PlasticSolver);
	auto VegLitterBreakdown    = RegisterEquation(Model, "River vegetation litter breakdown", KgPerDay, PlasticSolver);
	
	auto RiverBankLitter       = RegisterEquationODE(Model, "River bank litter", ItemsPerM, PlasticSolver);
	auto RiverBankLitterMass   = RegisterEquation(Model, "River bank litter (mass)", KgPerM, PlasticSolver);
	auto FloatingLitter        = RegisterEquationODE(Model, "Floating litter", ItemsPerM, PlasticSolver);
	auto FloatingLitterMass    = RegisterEquation(Model, "Floating litter (mass)", KgPerM, PlasticSolver);
	auto RiverVegetationLitter = RegisterEquationODE(Model, "River vegetation litter", ItemsPerM, PlasticSolver);
	auto RiverVegetationLitterMass = RegisterEquation(Model, "River vegetation litter (mass)", KgPerM, PlasticSolver);
	
	
	
	
	
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
		if(!std::isfinite(curvature)) curv_contrib = std::min(curv_contrib, curvature*1000.0);
		double width_contrib = std::max(0.01, 1.0 - (w-50.0)/300.0);
		double drag_contrib = std::max(0.01, C_DA);
		
		return PARAMETER(BankAttachmentTuning)*curv_contrib*width_contrib*drag_contrib*RESULT(FloatingLitter);
	)
	
	
	//TODO: Detachment thresholds have to be modified by item total mass...
	
	double ThresholdModBank[4] = {1.0, 3.0, 1.5, 1.2}; // none, bushes, reeds, grass..  //TODO: arbitrary numbers...
	
	EQUATION(Model, BankDragForce,
		double C_DA = PARAMETER(DragCoefficient)*PARAMETER(DragArea);
		return RESULT(ReachShearStress) * C_DA;
	)
	
	EQUATION(Model, BankDetachmentRate,
		double threshold = PARAMETER(DetachmentThresholdBase) * PARAMETER(PropensityToStayStuck);
		double threshold2 = threshold * ThresholdModBank[(int)PARAMETER(BankVegetationType)];
		threshold = threshold + (threshold2 - threshold)*PARAMETER(BankVegetationDensity);
		
		double excess = std::max(0.0, RESULT(BankDragForce) - threshold);
		return excess * PARAMETER(DetachmentRateTuning) / PARAMETER(ItemMass);
	)
	
	EQUATION(Model, BankDetachment,
		return RESULT(BankDetachmentRate) * RESULT(RiverBankLitter);
	)
	
	EQUATION(Model, RiverVegAttachment,
		u64 type = PARAMETER(RiverVegetationType);
		double rate = RESULT(FloatingLitter)*RESULT(ReachVelocity)*PARAMETER(RiverVegetationDensity)*86400.0;
		if(type == 0) rate = 0.0;
		return rate;
		
	)
	
	
	double ThresholdModRiver[4] = {0.0, 1.5, 0.1}; // none, reeds, mangrove..  //TODO: arbitrary numbers...
	
	EQUATION(Model, RiverVegDragForce,
		double waterdensity = 1e3;
		double C_DA = PARAMETER(DragCoefficient)*PARAMETER(DragArea);
		double U = RESULT(ReachVelocity); //TODO: Should we use shear velocity here like in bank detachment?
		double shearstress = waterdensity*U*U;
		return shearstress * C_DA;
	)
	
	EQUATION(Model, RiverVegDetachment,
		double threshold = PARAMETER(DetachmentThresholdBase) * PARAMETER(PropensityToStayStuck);
		double threshold2 = threshold * ThresholdModRiver[(int)PARAMETER(RiverVegetationType)];
		threshold = threshold + (threshold2 - threshold)*PARAMETER(RiverVegetationDensity);
		
		double excess = std::max(0.0, RESULT(RiverVegDragForce) - threshold);
		return excess * PARAMETER(DetachmentRateTuning) * RESULT(RiverVegetationLitter) / PARAMETER(ItemMass);
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
			
		return RESULT(RiverBankLitterMass) * breakdown_rate;
	)
	
	EQUATION(Model, VegLitterBreakdown,
		u64 type = PARAMETER(LitterType);
		double breakdown_rate = PARAMETER(GrindingRate) * RESULT(ReachVelocity) / HardnessMod[type];
		
		double drag_force = RESULT(RiverVegDragForce);
		if(type == Bag) breakdown_rate += PARAMETER(BagTearing) * drag_force;
			
		return RESULT(RiverVegetationLitterMass) * breakdown_rate;
	)
	
	
	
	EQUATION(Model, BankLitterInputs,
		return PARAMETER(InputToBank);
	)
	
	EQUATION(Model, RiverBankLitter,
		return
		  RESULT(BankLitterInputs)
		+ RESULT(BankAttachment)
		- RESULT(BankDetachment)
		- RESULT(BankLitterBreakdown)/PARAMETER(ItemMass);
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
		- RESULT(BankAttachment)
		+ RESULT(RiverVegDetachment)
		- RESULT(RiverVegAttachment);
	)
	
	EQUATION(Model, RiverBankLitterMass,
		return RESULT(RiverBankLitter) * PARAMETER(ItemMass);
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

	EQUATION(Model, FloatingLitterUpstream,
		double sum = 0.0;
		index_t ClassIndex = CURRENT_INDEX(Class);
		for(index_t Input : BRANCH_INPUTS(Reach))
			sum += RESULT(FloatingLitterOutput, Input, ClassIndex);
	
		return sum;
	)


	
	
	
	
	
	
	EndModule(Model);
}


