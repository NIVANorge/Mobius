

//#include "../../boost_solvers.h"


static void
AddINCAMacroplasticsModel(mobius_model *Model)
{
	BeginModule(Model, "INCA-Macroplastics", "_dev_0.0");
	//NOTE: Is designed to work with PERSiST
	
	SetModuleDescription(Model, R""""(
INCA-Macroplastics is in early development
)"""");

	auto Dimensionless = RegisterUnit(Model);
	auto KgPerM        = RegisterUnit(Model, "kg/m");
	auto KgPerMPerDay  = RegisterUnit(Model, "kg/(m day)");
	auto NPerM2        = RegisterUnit(Model, "N/m2");
	auto M2PerNDay     = RegisterUnit(Model, "m2/(N day)");
	auto KgPerDay      = RegisterUnit(Model, "kg/day");
	auto PerDay        = RegisterUnit(Model, "1/day");
	
	auto Reach = GetIndexSetHandle(Model, "Reaches");

	auto Class = RegisterIndexSet(Model, "Litter class");
	auto ClassParameters = RegisterParameterGroup(Model, "Litter class", Class);
	
	//auto ItemMass                      = RegisterParameterDouble(Model, ClassParameters, "Average mass", Kg, 1.0, 0.01, 100.0);
	
	auto DetachmentCriticalShearStress = RegisterParameterDouble(Model, ClassParameters, "Detachment critical shear stress", NPerM2, 0.0, 0.0, 1e3, "How much shear stress is needed to detach an item of this class from the river bank");
	auto DetachmentLikelihood          = RegisterParameterDouble(Model, ClassParameters, "Detachment likelihood", M2PerNDay, 0.0, 0.0, 500.0, "Rate of detachment of an item of this class per day per unit excess shear stress (N/m2)");
	auto AttachmentLikelihood          = RegisterParameterDouble(Model, ClassParameters, "Attachment likelihood", PerDay, 0.0, 0.0, 500.0, "Rate of re-attachment to the river bank of an item of this class per day");

	auto ClassReachParameters = RegisterParameterGroup(Model, "Litter by reach and class", Class, Reach);

	auto InputToBank                   = RegisterParameterDouble(Model, ClassReachParameters, "Litter input to bank", KgPerMPerDay, 0.0, 0.0, 100.0, "Constant daily litter to the river bank from land sources");


	//auto SubcatchmentArea = GetParameterDoubleHandle(Model, "Terrestrial catchment area");
	auto ReachLength      = GetParameterDoubleHandle(Model, "Reach length");
	//auto Percent          = GetParameterDoubleHandle(Model, "%");
	auto MeanChannelSlope = GetParameterDoubleHandle(Model, "Reach slope");
	
	//auto Rainfall      = GetEquationHandle(Model, "Rainfall");
	//auto RunoffToReach = GetEquationHandle(Model, "Runoff to reach");
	auto ReachHydraulicRadius = GetEquationHandle(Model, "Reach hydraulic radius");
	auto ReachVelocity        = GetEquationHandle(Model, "Reach velocity");
	
	
	auto ReachSolver = GetSolverHandle(Model, "Reach solver");
	//auto InstreamSedimentSolver = RegisterSolver(Model, "In-stream sediment solver", 0.1, BoostRosenbrock4, 1e-3, 1e-3);
	
	auto ReachShearStress = RegisterEquation(Model, "Reach shear stress", NPerM2, ReachSolver);
	if(!Model->Parameters.Has("Shear stress coefficient"))
	{
		auto ReachParameters = RegisterParameterGroup(Model, "Reach shear stress", Reach);
		auto ShearStressCoefficient = RegisterParameterDouble(Model, ReachParameters, "Shear stress coefficient", Dimensionless, 1.0, 0.01, 10.0, "Tuning parameter to account for the shear stress not being the same as in ideal conditions");
		
		
		
		EQUATION(Model, ReachShearStress,
			double waterdensity = 1e3;
			double earthsurfacegravity = 9.807;
			return PARAMETER(ShearStressCoefficient) * PARAMETER(MeanChannelSlope) * RESULT(ReachHydraulicRadius) * waterdensity * earthsurfacegravity;
		)
	}
	
	auto BankLitterInputs     = RegisterEquation(Model, "River bank litter inputs", KgPerMPerDay);
	auto FloatingLitterUpstream = RegisterEquation(Model, "Floating litter input from upstream", KgPerDay);
	
	auto BankLitterDetachment = RegisterEquation(Model, "River bank litter detachment", KgPerMPerDay, ReachSolver);
	auto BankLitterAttachment = RegisterEquation(Model, "River bank litter attachment", KgPerMPerDay, ReachSolver);
	auto FloatingLitterOutput = RegisterEquation(Model, "Floating litter output", KgPerDay, ReachSolver);
	auto RiverBankLitter      = RegisterEquationODE(Model, "River bank litter", KgPerM, ReachSolver);
	auto FloatingLitter       = RegisterEquationODE(Model, "Floating litter", KgPerM, ReachSolver);
	
	
	
	
	EQUATION(Model, BankLitterDetachment,
		double excess_stress = Max(0.0, RESULT(ReachShearStress) - PARAMETER(DetachmentCriticalShearStress));
		
		return PARAMETER(DetachmentLikelihood) * RESULT(RiverBankLitter) * excess_stress;
	)
	
	EQUATION(Model, BankLitterAttachment,
		return PARAMETER(AttachmentLikelihood) * RESULT(FloatingLitter); //TODO: More likely when it is slow moving? or does that just go into them being more easily detached again?
	)
	
	EQUATION(Model, BankLitterInputs,
		return PARAMETER(InputToBank);
	)
	
	EQUATION(Model, RiverBankLitter,
		return
		  RESULT(BankLitterInputs)
		+ RESULT(BankLitterAttachment)
		- RESULT(BankLitterDetachment);
	)
	
	EQUATION(Model, FloatingLitterOutput,
		return RESULT(FloatingLitter) * RESULT(ReachVelocity) * 86400.0;   // m/s -> m/day
	)

	EQUATION(Model, FloatingLitterUpstream,
		double sum = 0.0;
		index_t ClassIndex = CURRENT_INDEX(Class);
		for(index_t Input : BRANCH_INPUTS(Reach))
			sum += RESULT(FloatingLitterOutput, Input, ClassIndex);
	
		return sum;
	)


	EQUATION(Model, FloatingLitter,
		return
		  RESULT(FloatingLitterUpstream) / PARAMETER(ReachLength)
		- RESULT(FloatingLitterOutput) / PARAMETER(ReachLength)
		+ RESULT(BankLitterDetachment)
		- RESULT(BankLitterAttachment);
	)
	
	
	
	
	
	EndModule(Model);
}


