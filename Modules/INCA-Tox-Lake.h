
//NOTE: This is an extremely simple lake module that was made for just one specific project. It may not be widely applicable

inline double
AttnIndefiniteIntegral(double z, double a, double D)
{
	return exp(-a*z)*(a*(z-D)) / (a*a*D);
}


#define CONST_LN_2 0.69314718056

void
AddIncaToxLakeModule(mobius_model *Model)
{
	BeginModule(Model, "INCA-Tox Lake", "_dev");
	
	SetModuleDescription(Model, R""""(
This is an extremely simple lake module for INCA-Tox that was made for just one specific project. It may not be widely applicable. Specifically, it does not work well with hydrophobic contaminants.

It is not complete yet.
)"""");
	
	auto Dimensionless  = RegisterUnit(Model);
	auto Metres         = RegisterUnit(Model, "m");
	auto M2             = RegisterUnit(Model, "m2");
	auto Ng             = RegisterUnit(Model, "ng");
	auto NgPerM3        = RegisterUnit(Model, "ng/m3");
	auto NgPerDay       = RegisterUnit(Model, "ng/day");
	auto Days           = RegisterUnit(Model, "day");
	auto DegreesCelsius = RegisterUnit(Model, "°C");
	auto WPerM2         = RegisterUnit(Model, "W/m^2");
	auto HPa            = RegisterUnit(Model, "HPa");
	auto Percent        = RegisterUnit(Model, "%");
	auto KgPerM3        = RegisterUnit(Model, "kg/m3");
	
	
	
	auto Reach        = GetIndexSetHandle(Model, "Reaches");
	auto Contaminant  = GetIndexSetHandle(Model, "Contaminants");
	
	auto LakeParams = RegisterParameterGroup(Model, "Lake Tox");
	
	
	auto SecchiDepth            = RegisterParameterDouble(Model, LakeParams, "Secchi depth", Metres, 2.0);
	auto DiffusiveExchangeScale = RegisterParameterDouble(Model, LakeParams, "Diffusive exchange scaling factor", Dimensionless, 1.0);
	
	auto Photomin   = RegisterParameterGroup(Model, "Photomineralization", Contaminant);
	
	auto OpticalCrossection     = RegisterParameterDouble(Model, Photomin, "Optical cross-section", Dimensionless, 255.0, 0.0, 1000.0, "For photo-degradation"); //TODO: Other unit
	
	//auto LakeSolver = GetSolverHandle(Model, "Lake solver");
	auto LakeContaminantSolver = RegisterSolver(Model, "Lake contaminant solver", 0.1, IncaDascru);
	
	auto EpilimnionDegradationTemperatureModifier  = RegisterEquation(Model, "Temperature modifier for degradation in epilimnion", Dimensionless, LakeContaminantSolver);
	auto HypolimnionDegradationTemperatureModifier = RegisterEquation(Model, "Temperature modifier for degradation in hypolimnion", Dimensionless, LakeContaminantSolver);
	
	auto DepositionOnLakeSurface                   = RegisterEquation(Model, "Deposition on lake surface", NgPerDay, LakeContaminantSolver);
	
	auto EpilimnionContaminantDegradation          = RegisterEquation(Model, "Epilimnion contaminant degradation", NgPerDay, LakeContaminantSolver);
	auto HypolimnionContaminantDegradation         = RegisterEquation(Model, "Hypolimnion contaminant degradation", NgPerDay, LakeContaminantSolver);
	
	auto EpilimnionContaminantMass                 = RegisterEquationODE(Model, "Epilimnion contaminant mass", Ng, LakeContaminantSolver);
	auto EpilimnionContaminantConc                 = RegisterEquation(Model, "Epilimnion contaminant concentration", NgPerM3, LakeContaminantSolver);
	
	auto HypolimnionContaminantMass                = RegisterEquationODE(Model, "Hypolimnion contaminant mass", Ng, LakeContaminantSolver);
	auto HypolimnionContaminantConc                = RegisterEquation(Model, "Hypolimnion contaminant concentration", NgPerM3, LakeContaminantSolver);
	
	auto LayerExchange                             = RegisterEquation(Model, "Lake layer contaminant exchange", NgPerDay, LakeContaminantSolver);
	auto EpilimnionAttn                            = RegisterEquation(Model, "Epilimnion attenuation fraction", Dimensionless, LakeContaminantSolver);
	auto PhotoDegradation                          = RegisterEquation(Model, "Photo-degradation", NgPerDay, LakeContaminantSolver);
	
	auto DiffusiveAirLakeExchangeFlux              = RegisterEquation(Model, "Diffusive air-lake exchange flux", NgPerDay, LakeContaminantSolver);
	auto LakeContaminantFlux                       = RegisterEquation(Model, "Lake contaminant flux", NgPerDay, LakeContaminantSolver);
	
	//PERSiST:
	auto ReachFlow                          = GetEquationHandle(Model, "Reach flow"); // m3/s
	
	//INCA-Tox
	
	//NOTE: This does not take account of any sediment-bound particles. Currently only appropriate for highly dissolvable contaminants.
	auto ReachContaminantFlux               = GetEquationHandle(Model, "Total reach contaminant flux");          // ng/day
	auto ContaminantInputFromLand           = GetEquationHandle(Model, "Total diffuse contaminant output");     // ng/day
	auto ContaminantInputFromUpstream       = GetEquationHandle(Model, "Reach contaminant input from upstream"); // ng/day
	//NOTE: We reuse these instead of computing them for the lake. This is ONLY OK because we have a slow-flowing river in our particular case!!
	auto AirWaterTransferVelocity           = GetEquationHandle(Model, "Reach overall air-water transfer velocity");
	auto AirWaterPartitioningCoefficient    = GetEquationHandle(Model, "Reach air-water partitioning coefficient");
	
	//EasyLake
	auto EpilimnionTemperature              = GetEquationHandle(Model, "Epilimnion temperature");
	auto HypolimnionTemperature             = GetEquationHandle(Model, "Mean hypolimnion temperature");
	auto LakeOutflow                        = GetEquationHandle(Model, "Lake outflow");
	auto EpilimnionVolume                   = GetEquationHandle(Model, "Epilimnion volume");
	auto HypolimnionVolume                  = GetEquationHandle(Model, "Hypolimnion volume");
	auto MixingVelocity                     = GetEquationHandle(Model, "Mixing velocity");
	auto LakeSurfaceArea                    = GetEquationHandle(Model, "Lake surface area");
	auto LakeDepth                          = GetEquationHandle(Model, "Water level");
	auto EpilimnionThickness                = GetEquationHandle(Model, "Epilimnion thickness");
	auto EpilimnionShortwave                = GetEquationHandle(Model, "Epilimnion incoming shortwave radiation");
	auto IsIce                              = GetEquationHandle(Model, "There is ice");
	
	auto AtmosphericContaminantConcentration           = GetParameterDoubleHandle(Model, "Atmospheric contaminant concentration");
	auto ReachContaminantHalfLife                      = GetParameterDoubleHandle(Model, "Reach contaminant half life");
	auto DegradationResponseToTemperature              = GetParameterDoubleHandle(Model, "Degradation rate response to 10°C change in temperature");
	auto TemperatureAtWhichDegradationRatesAreMeasured = GetParameterDoubleHandle(Model, "Temperature at which degradation rates are measured");
	auto IsLake                                        = GetParameterBoolHandle(Model, "This section is a lake");
	
	auto AtmosphericContaminantConcentrationIn  = GetInputHandle(Model, "Atmospheric contaminant concentration");
	auto DepositionToLand                       = GetInputHandle(Model, "Contaminant deposition to land");
	auto DepositionToLandPar                    = GetParameterDoubleHandle(Model, "Deposition to land");


	auto ThisIsARiver                           = GetConditionalHandle(Model, "This is a river");
	auto ThisIsALake                            = GetConditionalHandle(Model, "This is a lake");
	
	auto SedimentSolver                         = GetSolverHandle(Model, "In-stream sediment solver");
	auto ReachContaminantSolver                 = GetSolverHandle(Model, "Reach contaminant solver");
	
	auto ReachShearVelocity                     = GetEquationHandle(Model, "Reach shear velocity");
	auto ProportionOfSedimentThatCanBeEntrained = GetEquationHandle(Model, "Proportion of sediment that can be entrained");
	auto StreamPower                            = GetEquationHandle(Model, "Stream power");
	auto ClayReleaseFromChannelBanks            = GetEquationHandle(Model, "Clay release from channel banks");
	auto TotalBedSedimentMassPerUnitArea        = GetEquationHandle(Model, "Total mass of bed sediment per unit area");
	auto PoreWaterVolume                        = GetEquationHandle(Model, "Pore water volume");
	auto DiffusivityOfDissolvedCompound         = GetEquationHandle(Model, "Diffusivity of dissolved compound in water");
	
	auto ReachSuspendedSOCFlux                  = GetEquationHandle(Model, "Reach suspended SOC flux");
	auto ReachSuspendedSOCMass                  = GetEquationHandle(Model, "Reach suspended SOC mass");
	auto BedSOCMass                             = GetEquationHandle(Model, "Stream bed SOC mass");
	auto ReachSOCDeposition                     = GetEquationHandle(Model, "Reach SOC deposition");
	auto ReachSOCEntrainment                    = GetEquationHandle(Model, "Reach SOC entrainment");
	auto TotalSOCFlux                           = GetEquationHandle(Model, "Total reach SOC flux");
	auto TotalSuspendedSOCMass                  = GetEquationHandle(Model, "Total suspended SOC mass");
	auto TotalBedSOCMass                        = GetEquationHandle(Model, "Total stream bed SOC mass");
	auto TotalSOCDeposition                     = GetEquationHandle(Model, "Total reach SOC deposition");
	auto TotalSOCEntrainment                    = GetEquationHandle(Model, "Total reach SOC entrainment");
	
	SetConditional(Model, SedimentSolver, ThisIsARiver);
	SetConditional(Model, ReachContaminantSolver, ThisIsARiver);
	SetConditional(Model, LakeContaminantSolver, ThisIsALake);
	
	SetConditional(Model, ReachShearVelocity, ThisIsARiver);
	SetConditional(Model, ProportionOfSedimentThatCanBeEntrained, ThisIsARiver);
	SetConditional(Model, StreamPower, ThisIsARiver);
	SetConditional(Model, ClayReleaseFromChannelBanks, ThisIsARiver);
	SetConditional(Model, ReachSuspendedSOCFlux, ThisIsARiver);
	SetConditional(Model, ReachSuspendedSOCMass, ThisIsARiver);
	SetConditional(Model, BedSOCMass, ThisIsARiver);
	SetConditional(Model, ReachSOCDeposition, ThisIsARiver);
	SetConditional(Model, ReachSOCEntrainment, ThisIsARiver);
	SetConditional(Model, TotalSOCFlux, ThisIsARiver);
	SetConditional(Model, TotalSuspendedSOCMass, ThisIsARiver);
	SetConditional(Model, TotalBedSOCMass, ThisIsARiver);
	SetConditional(Model, TotalSOCDeposition, ThisIsARiver);
	SetConditional(Model, TotalSOCEntrainment, ThisIsARiver);
	SetConditional(Model, TotalBedSedimentMassPerUnitArea, ThisIsARiver);
	SetConditional(Model, PoreWaterVolume, ThisIsARiver);
	SetConditional(Model, DiffusivityOfDissolvedCompound, ThisIsARiver);

	
	EQUATION_OVERRIDE(Model, ContaminantInputFromUpstream,
		double upstreamflux = 0.0;
		
		for(index_t Input : BRANCH_INPUTS(Reach))
		{
			if(PARAMETER(IsLake, Input))
				upstreamflux += RESULT(LakeContaminantFlux, Input);
			else
				upstreamflux += RESULT(ReachContaminantFlux, Input);
		}
		
		return upstreamflux;
	)
	
	
	
	EQUATION(Model, HypolimnionDegradationTemperatureModifier,
		return pow(PARAMETER(DegradationResponseToTemperature), 0.1*(RESULT(HypolimnionTemperature) - PARAMETER(TemperatureAtWhichDegradationRatesAreMeasured)));
	)
	
	EQUATION(Model, EpilimnionDegradationTemperatureModifier,
		return pow(PARAMETER(DegradationResponseToTemperature), 0.1*(RESULT(EpilimnionTemperature) - PARAMETER(TemperatureAtWhichDegradationRatesAreMeasured)));
	)
	
	EQUATION(Model, EpilimnionContaminantDegradation,
		return (CONST_LN_2 / PARAMETER(ReachContaminantHalfLife)) * RESULT(EpilimnionContaminantMass) * RESULT(EpilimnionDegradationTemperatureModifier);
	)
	
	EQUATION(Model, HypolimnionContaminantDegradation,
		return (CONST_LN_2 / PARAMETER(ReachContaminantHalfLife)) * RESULT(HypolimnionContaminantMass) * RESULT(HypolimnionDegradationTemperatureModifier);
	)
	
	EQUATION(Model, EpilimnionContaminantMass,
		
		return
			  RESULT(ContaminantInputFromLand)
			+ RESULT(ContaminantInputFromUpstream)
			+ RESULT(DepositionOnLakeSurface)
			- RESULT(LayerExchange)
			- RESULT(LakeContaminantFlux)
			- RESULT(DiffusiveAirLakeExchangeFlux)
			- RESULT(PhotoDegradation)
			- RESULT(EpilimnionContaminantDegradation);
	)
	
	EQUATION(Model, DepositionOnLakeSurface,
		return IF_INPUT_ELSE_PARAMETER(DepositionToLand, DepositionToLandPar)*RESULT(LakeSurfaceArea);
	)
	
	EQUATION(Model, LakeContaminantFlux,
		return RESULT(EpilimnionContaminantConc) * RESULT(LakeOutflow) * 86400.0;
	)
	
	EQUATION(Model, EpilimnionContaminantConc,
		return SafeDivide(RESULT(EpilimnionContaminantMass), RESULT(EpilimnionVolume));
	)
	
	EQUATION(Model, HypolimnionContaminantMass,
		return RESULT(LayerExchange) - RESULT(HypolimnionContaminantDegradation);
	)
	
	EQUATION(Model, HypolimnionContaminantConc,
		return SafeDivide(RESULT(HypolimnionContaminantMass), RESULT(HypolimnionVolume));
	)
	
	EQUATION(Model, LayerExchange,
		return RESULT(MixingVelocity)*RESULT(LakeSurfaceArea)*(RESULT(EpilimnionContaminantConc) - RESULT(HypolimnionContaminantConc));
	)
	
	EQUATION(Model, PhotoDegradation,
		double oc_Nitro = PARAMETER(OpticalCrossection); // m2/mol Nitro            Optical cross-section of DOM
		double qy_Nitro = 0.45;  // mol Nitro /mol quanta   Quantum yield
		
		double f_uv = 0.06;      // Fract.              of PAR in incoming solar radiation
		double e_uv = 351843.0;  // J/mol               Average energy of Par photons
		
		//‒oc_Nitro * qy_Nitro f_par(1/e_par)*(86400)*Qsw*Attn_epilimnion * [Nitrosamines]" in mg N m-3 d-1
		double shortwave = RESULT(EpilimnionShortwave)*RESULT(EpilimnionAttn);
		return oc_Nitro * qy_Nitro * (f_uv / e_uv) * 86400.0 * shortwave * RESULT(EpilimnionContaminantMass);
	)
	
	
	EQUATION(Model, EpilimnionAttn,
		double a = 2.7/PARAMETER(SecchiDepth);
		double D = RESULT(LakeDepth);
		double d = RESULT(EpilimnionThickness);

		double top = AttnIndefiniteIntegral(0.0, a, D);
		double bot = AttnIndefiniteIntegral(d, a, D);
		
		double vv = (D - (1.0 - d/D)*(D-d));
		
		//return (bot - top);
		return 3.0*(bot - top)/vv;
	)
	
	EQUATION(Model, DiffusiveAirLakeExchangeFlux,
		double atmospheric = IF_INPUT_ELSE_PARAMETER(AtmosphericContaminantConcentrationIn, AtmosphericContaminantConcentration);
		double flux = RESULT(AirWaterTransferVelocity) * (RESULT(EpilimnionContaminantConc) - atmospheric/RESULT(AirWaterPartitioningCoefficient)) * RESULT(LakeSurfaceArea);
		if(RESULT(IsIce) > 0.0) flux = 0.0;
		return PARAMETER(DiffusiveExchangeScale)*flux;
	)
	
	
	EndModule(Model);
}


#undef CONST_LN_2



