

//NOTE: This model is IN DEVELOPMENT

	
inline double
SaturationVaporPressure(double Temperature)
{
	//NOTE: Saturation vapor pressure routine based on 
	// P. R. Lowe, 1977, "An approximating polynomial for the computation of saturation vapor pressure, J. Appl. Meteor., 16, 100-103.

	//TODO: Could some of this be unified with code in PET.h ?
	
	//TODO: There should be a separate formula for Temperature < 0.0
	
	// Takes temperature in celsius
	// Returns saturation vapor pressure in millibar=hectopascal.
	
	double a0 = 6.107799961;
	double a1 = 4.436518521e-1;
	double a2 = 1.428945805e-2;
	double a3 = 2.650648471e-4;
	double a4 = 3.031240396e-6;
	double a5 = 2.034080948e-8;
	double a6 = 6.136820929e-11;
	double t = Temperature;
	
	return (a0 + t*(a1 + t*(a2 + t*(a3 + t*(a4 + t*(a5 + t*a6))))));
}


inline double
WaterDensity(double WaterTemperature)
{
	double dtemp = (WaterTemperature + 273.15 - 277.13); // Difference between temperature and reference temperature
	return 999.98*(1.0 - 0.5*1.6509e-5*dtemp*dtemp);   //(Farmer, Carmack 1981)
}


static void
AddEasyLakePhysicalModule(mobius_model *Model)
{
	BeginModule(Model, "Easy-Lake physical", "0.3");
	
	SetModuleDescription(Model, R""""(
This is a very simple lake model for use along with cathcment models.
The physical part of the model simulates water balance and temperature.

The water balance part of the model is conceptually similar to VEMALA
[A National-Scale Nutrient Loading Model for Finnish Watersheds - VEMALA, Inse Huttunen et. al. 2016, Environ Model Assess 21, 83-109](https://doi.org/10.1007/s10666-015-9470-6)

Air-lake heat fluxes are based off of
[Air-Sea bulk transfer coefficients in diabatic conditions, Junsei Kondo, 1975, Boundary-Layer Meteorology 9(1), 91-112](https://link.springer.com/article/10.1007/BF00232256)
The implementation is informed by the implementation in [GOTM](https://github.com/gotm-model).
)"""");
	
	
	auto Dimensionless  = RegisterUnit(Model);
	auto M              = RegisterUnit(Model, "m");
	auto M2             = RegisterUnit(Model, "m2");
	auto M3             = RegisterUnit(Model, "m3");
	auto Days           = RegisterUnit(Model, "days");
	auto MPerS          = RegisterUnit(Model, "m/s");
	auto M3PerS         = RegisterUnit(Model, "m3/s");
	auto M3PerDay       = RegisterUnit(Model, "m3/day");
	auto MmPerDay       = RegisterUnit(Model, "mm/day");
	auto MPerDay        = RegisterUnit(Model, "m/day");
	auto MPerDay2       = RegisterUnit(Model, "m/day2");
	auto PerM           = RegisterUnit(Model, "1/m");
	auto MPerM          = RegisterUnit(Model, "m/m");
	auto Degrees        = RegisterUnit(Model, "°");
	auto DegreesCelsius = RegisterUnit(Model, "°C");
	auto KgPerKg        = RegisterUnit(Model, "kg/kg");
	auto KgPerM3        = RegisterUnit(Model, "kg/m3");
	auto Percent        = RegisterUnit(Model, "%");
	auto Pascal         = RegisterUnit(Model, "Pa");
	auto HPa            = RegisterUnit(Model, "HPa");
	auto WPerM2         = RegisterUnit(Model, "W/m2");
	auto Watts          = RegisterUnit(Model, "W");
	auto NPerM2         = RegisterUnit(Model, "N/m2");
	//auto Days           = RegisterUnit(Model, "days");

#ifdef EASYLAKE_STANDALONE
	auto LakeInflow       = RegisterInput(Model, "Lake inflow", M3PerS);
	
	auto PhysParams = RegisterParameterGroup(Model, "Lake physical");
#else
	auto Reach            = GetIndexSetHandle(Model, "Reaches");
	auto PhysParams = RegisterParameterGroup(Model, "Lake physical", Reach);
	auto IsLake     = RegisterParameterBool(Model, PhysParams, "This section is a lake", false, "If false this is a river section: ignore the parameters below");
#endif

#ifdef EASYLAKE_SIMPLYQ	
	auto FlowInputFromUpstream = GetEquationHandle(Model, "Flow input from upstream");
	auto FlowInputFromLand     = GetEquationHandle(Model, "Flow input from land");
#endif

#ifdef EASYLAKE_PERSIST
	auto FlowInputFromUpstream = GetEquationHandle(Model, "Flow input from upstream");
	auto FlowInputFromLand     = GetEquationHandle(Model, "Total diffuse flow output");
#endif

	auto InitialLakeSurfaceArea         = RegisterParameterDouble(Model, PhysParams, "Initial lake surface area", M2, 1e3, 0.0, 371e9);
	auto LakeLength                     = RegisterParameterDouble(Model, PhysParams, "Lake length", M, 300.0, 0.0, 1.03e6);
	auto LakeShoreSlope                 = RegisterParameterDouble(Model, PhysParams, "Lake shore slope", MPerM, 0.2, 0.0, 4.0, "This parameter should be adjusted when calibrating lake outflow. Slope is roughly 2*depth/width");
	auto WaterLevelAtWhichOutflowIsZero = RegisterParameterDouble(Model, PhysParams, "Water level at which outflow is 0", M, 10.0, 0.0, 1642.0);
	auto OutflowRatingCurveShape        = RegisterParameterDouble(Model, PhysParams, "Outflow rating curve shape", Dimensionless, 0.3, 0.0, 1.0, "0 if rating curve is linear, 1 if rating curve is a parabola. Values in between give linear interpolation between these types of curves.");
	auto OutflowRatingCurveMagnitude    = RegisterParameterDouble(Model, PhysParams, "Outflow rating curve magnitude", Dimensionless, 1.0, 0.01, 100.0, "Outflow is proportional to 10^(magnitude)");
	
	
	auto TemperatureCalibrationDepth = RegisterIndexSet(Model, "Temperature calibration depth");
	auto TempDepths                  = RegisterParameterGroup(Model, "Temperature calibration depths", TemperatureCalibrationDepth);
	auto CalibDepth                  = RegisterParameterDouble(Model, TempDepths, "Calibration depth", M, 0.0, 0.0, 1642.0);
	
#ifdef EASYLAKE_PERSIST
	auto Precipitation    = RegisterInput(Model, "Actual precipitation", MmPerDay);
#else
	auto Precipitation    = RegisterInput(Model, "Precipitation", MmPerDay);
#endif
	auto AirTemperature   = RegisterInput(Model, "Air temperature", DegreesCelsius);
	
	//NOTE: Some of these may be hard to come by in some instances. We should provide ways to estimate them such as in PET.h
	auto WindSpeed        = RegisterInput(Model, "Wind speed at 10m", MPerS);
	auto RelativeHumidity = RegisterInput(Model, "Relative humidity", Percent);
	auto AirPressure      = RegisterInput(Model, "Air pressure", HPa);
	auto CloudCover       = RegisterInput(Model, "Cloud cover", Dimensionless);
	auto GlobalRadiation  = RegisterInput(Model, "Global radiation", WPerM2); // Shortwave radiation falling at earth surface.
	
	auto LakeSolver = RegisterSolver(Model, "Lake solver", 0.1, IncaDascru);
	
	auto LakeVolume        = RegisterEquationODE(Model, "Lake volume", M3, LakeSolver);
	auto InitialLakeVolume = RegisterEquationInitialValue(Model, "Initial lake volume", M3);
	SetInitialValue(Model, LakeVolume, InitialLakeVolume);
	
	auto LakeSurfaceArea   = RegisterEquation(Model, "Lake surface area", M2, LakeSolver);
	SetInitialValue(Model, LakeSurfaceArea, InitialLakeSurfaceArea);
	
	auto WaterLevel = RegisterEquationODE(Model, "Water level", M, LakeSolver);
	SetInitialValue(Model, WaterLevel, WaterLevelAtWhichOutflowIsZero);
	
	auto EpilimnionVolume  = RegisterEquation(Model, "Epilimnion volume", M3, LakeSolver);  //NOTE: In their current form, they are just put on the solver to not cause circular references in EasyLakeCNP.
	auto HypolimnionVolume = RegisterEquation(Model, "Hypolimnion volume", M3, LakeSolver);
	
	auto DVDT        = RegisterEquation(Model, "Change in lake volume", M3PerDay, LakeSolver);
	auto OutletWaterLevel = RegisterEquation(Model, "Outlet water level", M, LakeSolver);
	auto LakeOutflow = RegisterEquation(Model, "Lake outflow", M3PerS, LakeSolver);
	auto Evaporation = RegisterEquation(Model, "Evaporation", MmPerDay, LakeSolver);
	
	auto DailyMeanLakeOutflow = RegisterEquationODE(Model, "Lake outflow (daily mean)", M3PerS, LakeSolver);
	ResetEveryTimestep(Model, DailyMeanLakeOutflow);
	
	
	auto LakeLengthComputation = RegisterEquationInitialValue(Model, "Lake length computation", M);
	ParameterIsComputedBy(Model, LakeLength, LakeLengthComputation, true);
	
	EQUATION(Model, LakeLengthComputation,
		return 0.5*PARAMETER(InitialLakeSurfaceArea)*PARAMETER(LakeShoreSlope)/PARAMETER(WaterLevelAtWhichOutflowIsZero);
	)
	
	
	/*
        Conceptual model for water balance:
		
                              |   ^
                              |   |
                              P   E
                              |   |
                              v   | 
---Qin-->------ w ------*------------ L ------------* -- Qout -->
         \      |      /                           /  ----------
          \     |     /                           /   |
           \    |    /                           /    |
            \   h   /      V = 0.5 * w * L * h  /     h0
             \  |  /                           /      |
              \ | /                           /       |
               \|/___________________________/        |
	
		w  - lake width (m)
		h  - lake depth (m)
		h0 - depth at which outflow is 0 (m)
		L  - lake length (m)
		S  - surface area              = w * L (m^2)
		Qin - inflow (m^3/s)
		Qout - outflow (m^3/s)         = rating_curve_formula(h - h0)
		P  - precipitation (mm/day)
		E  - evaporation (mm/day)
		t  - lake slope                = 2 * h / w                    (1)
		V  - volume                    = 0.5 * w * L * h              (2)
		dV/dt = (Qin - Qout)*86400 + 1e-3*(P - E)*S                   (3)
		=>
		w = 2 * h / t                                                from (1)
		V = 0.5 * L * h * (2 * h / t) = L * h^2 / t                  from (2)
		dV/dt = (L / t) d(h^2)/dt = (L / t) 2*h * dh/dt
		=>
		dh/dt = (0.5 t / (h * L)) * dV/dt  --- solve this as an ODE equation along with (3)
	*/
	
	EQUATION(Model, InitialLakeVolume,
		return 0.5 * PARAMETER(WaterLevelAtWhichOutflowIsZero) * PARAMETER(InitialLakeSurfaceArea);
	)
	
	EQUATION(Model, DVDT,
		//NOTE: We don't care about ice when it comes to the water balance. We may figure out later if this matters for the computation of outflow.
		//NOTE: In the conceptualisation, the surface area is actually not constant but varies with the water level. However, that is probably not important for precip & evaporation.
#ifdef EASYLAKE_STANDALONE
		double inflow = INPUT(LakeInflow);
#endif
#ifdef EASYLAKE_SIMPLYQ
		double inflow = RESULT(FlowInputFromUpstream) + RESULT(FlowInputFromLand);
#endif
#ifdef EASYLAKE_PERSIST
		double inflow = RESULT(FlowInputFromUpstream) + RESULT(FlowInputFromLand);   //TODO: Effluents and abstraction
#endif
		return (inflow - RESULT(LakeOutflow)) * 86400.0 + 1e-3 * (INPUT(Precipitation) - RESULT(Evaporation)) * RESULT(LakeSurfaceArea);
	)
	
	EQUATION(Model, LakeVolume,
		return RESULT(DVDT);
	)
	
	EQUATION(Model, LakeSurfaceArea,
		return 2.0 * RESULT(WaterLevel)*PARAMETER(LakeLength)/PARAMETER(LakeShoreSlope);
	)
	
	EQUATION(Model, WaterLevel,
		return 0.5 * (PARAMETER(LakeShoreSlope) / (PARAMETER(LakeLength) * RESULT(WaterLevel))) * RESULT(DVDT);
	)
	
	EQUATION(Model, OutletWaterLevel,
		return Max(0.0, RESULT(WaterLevel) - PARAMETER(WaterLevelAtWhichOutflowIsZero));
	)
	
	EQUATION(Model, LakeOutflow,
		double outletlevel = RESULT(OutletWaterLevel);
		double C3 = PARAMETER(OutflowRatingCurveShape);
		return std::pow(10.0, PARAMETER(OutflowRatingCurveMagnitude)) * (C3*outletlevel + (1.0 - C3)*outletlevel*outletlevel);
	)
	
	EQUATION(Model, DailyMeanLakeOutflow,
		return RESULT(LakeOutflow);
	)
	
	
	auto Latitude         = RegisterParameterDouble(Model, PhysParams, "Latitude", Degrees, 60.0, -90.0, 90.0);
	auto InitialEpilimnionTemperature = RegisterParameterDouble(Model, PhysParams, "Initial epilimnion temperature", DegreesCelsius, 20.0, 0.0, 50.0);
	auto InitialBottomTemperature     = RegisterParameterDouble(Model, PhysParams, "Initial bottom temperature", DegreesCelsius, 4.0, 0.0, 50.0);
	
	auto EpilimnionWinterThickness = RegisterParameterDouble(Model, PhysParams, "Epilimnion winter thickness", M, 5.0, 0.0, 20.0);
	auto EpilimnionThickeningRate     = RegisterParameterDouble(Model, PhysParams, "Epilimnion thickening rate", MPerDay2, 0.005, 0.0, 0.05, "Empirical rate of how fast the thickness of the epilimnion grows during summer.");
	//auto InitialEpilimnionThickness   = RegisterParameterDouble(Model, PhysParams, "Initial epilimnion thickness", M, 5.0, 0.0, 20.0);
	
	
	auto InitialIceThickness          = RegisterParameterDouble(Model, PhysParams, "Initial ice thickness", M, 0.0, 0.0, 10.0);
	auto FreezingSpeed                = RegisterParameterDouble(Model, PhysParams, "Freezing thermal conductivity", Dimensionless, 2000.0, 0.0, 20000.0, "Should be left unchanged in most cases"); 
	auto IceFormationTemperature      = RegisterParameterDouble(Model, PhysParams, "Ice formation temperature", DegreesCelsius, 0.0, -2.0, 2.0, "Calibration parameter to allow for differences in surface temperature and mean epilimnion temperature");
	auto FrazilThreshold              = RegisterParameterDouble(Model, PhysParams, "Frazil threshold", M, 0.05, 0.0, 0.1, "Thickness of ice before it changes surface properties of the lake");
	
	
	auto SaturationSpecificHumidity           = RegisterEquation(Model, "Saturation specific humidity", KgPerKg, LakeSolver);
	auto ActualVaporPressure                  = RegisterEquation(Model, "Actual vapor pressure", HPa);
	auto ActualSpecificHumidity               = RegisterEquation(Model, "Actual specific humidity", KgPerKg);
	auto AirDensity                           = RegisterEquation(Model, "Air density", KgPerM3);
	auto Stability                            = RegisterEquation(Model, "Stability", Dimensionless, LakeSolver);               //TODO: this probably has another unit
	auto TransferCoefficientForLatentHeatFlux = RegisterEquation(Model, "Transfer coefficient for latent heat flux", Dimensionless, LakeSolver); //Correct unit?
	auto TransferCoefficientForSensibleHeatFlux = RegisterEquation(Model, "Transfer coefficient for sensible heat flux", Dimensionless, LakeSolver); // unit?
	auto SurfaceStressCoefficient             = RegisterEquation(Model, "Surface stress coefficient", Dimensionless, LakeSolver);
	auto LatentHeatOfVaporization             = RegisterEquation(Model, "Latent heat of vaporization", Dimensionless, LakeSolver); //TODO: Unit!
	auto LatentHeatFlux                       = RegisterEquation(Model, "Latent heat flux", WPerM2, LakeSolver);
	auto SensibleHeatFlux                     = RegisterEquation(Model, "Sensible heat flux", WPerM2, LakeSolver);
	auto SurfaceStress                        = RegisterEquation(Model, "Surface stress", NPerM2, LakeSolver);
	auto IsMixing                             = RegisterEquation(Model, "The lake is being mixed", Dimensionless, LakeSolver);
	//auto MixingPower                          = RegisterEquation(Model, "Mixing power", Watts, LakeSolver);
	//auto MixingVelocity                       = RegisterEquation(Model, "Mixing velocity", MPerDay, LakeSolver);
	
	auto EmittedLongwaveRadiation             = RegisterEquation(Model, "Emitted longwave radiation", WPerM2, LakeSolver);
	auto DownwellingLongwaveRadation          = RegisterEquation(Model, "Downwelling longwave radiation", WPerM2);
	auto LongwaveRadiation                    = RegisterEquation(Model, "Net longwave radiation", WPerM2, LakeSolver);
	auto ShortwaveRadiation                   = RegisterEquation(Model, "Net shortwave radiation", WPerM2, LakeSolver);
	auto EpilimnionShortwave                  = RegisterEquation(Model, "Epilimnion incoming shortwave radiation", WPerM2, LakeSolver);
	

	
	//Stuff below here is loosely based on FLake (Mironov 05)
	//auto ConvectiveHeatFluxScale = RegisterEquation(Model, "Convective heat flux scale", WPerM2, LakeSolver);

	//auto SurfaceShearVelocity = RegisterEquation(Model, "Surface shear velocity", MPerS, LakeSolver);
	
	auto InitialMeanLakeTemperature = RegisterEquationInitialValue(Model, "Initial mean lake temperature", DegreesCelsius);
	auto MeanLakeTemperature = RegisterEquationODE(Model, "Mean lake temperature", DegreesCelsius, LakeSolver);
	SetInitialValue(Model, MeanLakeTemperature, InitialMeanLakeTemperature); //TODO!
	
	auto EpilimnionTemperature = RegisterEquation(Model, "Epilimnion temperature", DegreesCelsius, LakeSolver);
	SetInitialValue(Model, EpilimnionTemperature, InitialEpilimnionTemperature);
	
	auto MeanHypolimnionTemperature = RegisterEquation(Model, "Mean hypolimnion temperature", DegreesCelsius, LakeSolver);
	
	auto LastMixing          = RegisterEquation(Model, "Last mixing", Days, LakeSolver);
	
	auto InitialEpilimnionThickness = RegisterEquationInitialValue(Model, "Initial epilimnion thickness", M);
	auto EpilimnionThickness = RegisterEquation(Model, "Epilimnion thickness", M, LakeSolver);
	SetInitialValue(Model, EpilimnionThickness, InitialEpilimnionThickness);
	
	auto BottomTemperature   = RegisterEquation(Model, "Bottom temperature", DegreesCelsius);
	SetInitialValue(Model, BottomTemperature, InitialBottomTemperature);
	
	auto SurfaceHeatFlux     = RegisterEquation(Model, "Surface heat flux", WPerM2, LakeSolver);
	
	auto IceAttenuationCoefficient = RegisterEquation(Model, "Ice attenuation coefficient", Dimensionless, LakeSolver);
	auto IsIce               = RegisterEquation(Model, "There is ice", Dimensionless, LakeSolver);
	auto IceEnergy           = RegisterEquation(Model, "Ice energy", WPerM2, LakeSolver);
	auto IceThickness        = RegisterEquationODE(Model, "Ice thickness", M, LakeSolver);
	
	SetInitialValue(Model, IceThickness, InitialIceThickness);
	SetInitialValue(Model, IsIce, IsIce); //Force this computation to be run in the initial value setup too.
	
	auto TemperatureAtDepth  = RegisterEquation(Model, "Temperature at depth", DegreesCelsius);

#ifdef EASYLAKE_SIMPLYQ
	auto DailyMeanReachFlow    = GetEquationHandle(Model, "Reach flow (daily mean, cumecs)");
#endif
#ifdef EASYLAKE_PERSIST
	auto DailyMeanReachFlow    = GetEquationHandle(Model, "Reach flow (daily mean)");
#endif

#ifndef EASYLAKE_STANDALONE
	auto InflowTemperature   = RegisterEquation(Model, "Inflow temperature", DegreesCelsius);
	auto ReachTemperature    = RegisterEquation(Model, "Reach temperature", DegreesCelsius);   //Ideally this should not go in this module.

	auto ThisIsALake = RegisterConditionalExecution(Model, "This is a lake", IsLake, true);
	auto ThisIsARiver = RegisterConditionalExecution(Model, "This is a river", IsLake, false);
	
	SetConditional(Model, LakeSolver, ThisIsALake);
	SetConditional(Model, ActualVaporPressure, ThisIsALake);
	SetConditional(Model, ActualSpecificHumidity, ThisIsALake);
	SetConditional(Model, AirDensity, ThisIsALake);
	SetConditional(Model, DownwellingLongwaveRadation, ThisIsALake);
	//SetConditional(Model, ShortwaveRadiation, ThisIsALake);
	//SetConditional(Model, EpilimnionThickness, ThisIsALake);
	SetConditional(Model, BottomTemperature, ThisIsALake);
	SetConditional(Model, TemperatureAtDepth, ThisIsALake);
	
	SetConditional(Model, ReachTemperature, ThisIsARiver);
	
	
	EQUATION_OVERRIDE(Model, FlowInputFromUpstream,
		double upstreamflow = 0.0;

		for(index_t Input : BRANCH_INPUTS(Reach))
		{
			if(PARAMETER(IsLake, Input))
				upstreamflow += RESULT(DailyMeanLakeOutflow, Input);
			else
				upstreamflow += RESULT(DailyMeanReachFlow, Input);
		}
		return upstreamflow;
	)
	
	EQUATION(Model, ReachTemperature,
		//TODO: Ideally this should depend on inflow temperature too, in case of rivers running out of large lakes and into smaller ones.
	
		double lagfactor = 3.0; //TODO: parameterize!
		double mintemp   = 4.0;
		
		return ((lagfactor - 1.0) / lagfactor) * LAST_RESULT(ReachTemperature)
		+ (1.0 / lagfactor) * std::max(mintemp, INPUT(AirTemperature));
		
	)
	
	EQUATION(Model, InflowTemperature,
		double upstreamflow = 0.0;
		double upstreamtempvol = 0.0;

		for(index_t Input : BRANCH_INPUTS(Reach))
		{
			if(PARAMETER(IsLake, Input))
			{
				upstreamflow += RESULT(DailyMeanLakeOutflow, Input);
				upstreamtempvol += upstreamflow * RESULT(EpilimnionTemperature, Input);
			}	
			else
			{
				upstreamflow += RESULT(DailyMeanReachFlow, Input);
				upstreamtempvol += upstreamflow * RESULT(ReachTemperature, Input);
			}
		}
		return SafeDivide(upstreamtempvol, upstreamflow);
	)
	
#endif
	
	
#ifdef EASYLAKE_SIMPLYQ
	//NOTE: If we do the following, we exclude groundwater computations from lake subcatchments, and we have to have a separate computation of flow input from land. Instead we just run the river computations, and ignore their values. It could be confusing for users though, so we have to see if we have to add something for the UI for this.
	/*
	auto ThisIsARiver = RegisterConditionalExecution(Model, "This is a river", IsLake, false);
	
	auto ReachSolver = GetSolverHandle(Model, "SimplyQ reach solver");
	auto ReachFlowMM = GetEquationHandle(Model, "Reach flow (daily mean, mm/day)");
	auto Control     = GetEquationHandle(Model, "Control");
	
	SetConditional(Model, ReachSolver, ThisIsARiver);
	SetConditional(Model, ReachFlowMM, ThisIsARiver);
	SetConditional(Model, Control, ThisIsARiver);
	*/
#endif

#ifdef EASYLAKE_PERSIST
	
	
	auto ReachSolver = GetSolverHandle(Model, "Reach solver");
	SetConditional(Model, ReachSolver, ThisIsARiver);
#endif
	
	
	
	/*
		Specific humidity is mass_vapor / mass_air (kg/kg)         (mass_air = mass_vapor + mass_dry_air)
		Saturation specific humidity is the specific humidity when the air is fully saturated with water vapor (function of temperature)
		
		Mixing ratio is  mass_vapor / mass_dry_air
		Saturation mixing ration is mixing ratio when air is saturated
		
		Vapor pressure is the partial pressure of water vapor. 
		Saturation vapor pressure is the partial pressure of water vapor when the air is fully saturated (function of temperature)
		
		pressure_air = pressure_dry_air + pressure_vapor   (Dalton's law)
		
		pressure * volume = moles * boltzmann_constant * temperature_kelvin    (Ideal gas law)
		
		=> pressure_vapor / pressure_dry_air = moles_vapor / moles_dry_air = (1/0.62198) * mass_vapor / mass_dry_air = (1/0.62198) * mixing_ratio
		
		=> mixing_ratio = 0.62198 * pressure_vapor / (pressure_air - pressure_vapor)
		
		specific_humidity = mixing_ratio / (1 + mixing_ratio)   ~= mixing_ratio      (approximately correct if vapor mass is significantly smaller than total air mass, i.e. mixing_ratio is much smaller than 1, which is usually is)
		
		Relative humidity is   vapor_pressure / saturation_vapor_pressure   = mixing_ratio / saturation_mixing_ratio


		NOTE: Thes saturation specific humidity computed here is that of the air touching the lake surface, while the actual specific humidity is computed for air a little above that.
	*/
	
	EQUATION(Model, SaturationSpecificHumidity,
		//NOTE: This assumes 'mixing ratio' ~= 'specific humidity', which is ok if vapor mass is significantly smaller than total air mass.
	
		double ratioconvertionfactor = 0.62198; //Converting molar ratio to mass ratio
		
		double svap = SaturationVaporPressure(RESULT(EpilimnionTemperature));   //TODO: Change to lake surface temperature if that is ever something else
		
		//return ratioconvertionfactor * svap / (INPUT(AirPressure) - 0.377 * svap); //TODO: Find out what 0.377 is for. Shouldn't that just be 1?
		return ratioconvertionfactor * svap / (INPUT(AirPressure) - svap);
	)
	
	EQUATION(Model, ActualVaporPressure,
		double relhum = 0.01 * INPUT(RelativeHumidity); // percent -> fraction
		
		double svap = SaturationVaporPressure(INPUT(AirTemperature));
		return relhum * svap;
	)
	
	EQUATION(Model, ActualSpecificHumidity,
		double ratioconvertionfactor = 0.62198;
		double actualvaporpressure = RESULT(ActualVaporPressure);
		
		//return ratioconvertionfactor * actualvaporpressure / (INPUT(AirPressure) - 0.377*actualvaporpressure);
		return ratioconvertionfactor * actualvaporpressure / (INPUT(AirPressure) - actualvaporpressure);
	)


	EQUATION(Model, AirDensity,
		double tempkelvin = INPUT(AirTemperature) + 273.15;
		double specificgasconstdryair = 287.058; // J/(kg K)
		double specificgasconstvapor  = 461.495; // (J/(kg K))
		return ((INPUT(AirPressure) - RESULT(ActualVaporPressure)) / (tempkelvin*specificgasconstdryair)  +  RESULT(ActualVaporPressure) / (tempkelvin * specificgasconstvapor))* 100.0;   //Multiply by 100 for HPa -> Pa
	)

	EQUATION(Model, Stability,
		double WW = (INPUT(WindSpeed) + 1e-10);
		double s0 = 0.25 * (RESULT(EpilimnionTemperature) - INPUT(AirTemperature)) / (WW * WW);   // change epi temp to surface temp if that is ever something else
		return s0 * std::abs(s0) / (std::abs(s0) + 0.01);
	)
	
	EQUATION(Model, TransferCoefficientForLatentHeatFlux,
		double W = INPUT(WindSpeed);
		
		double ae_e; double be_e; double ce_e; double pe_e;   //NOTE: we can't use commas inside the EQUATION macro, or it screws up the comma counting of the preprocessor.
		if(W < 2.2)        { ae_e = 0.0;   be_e = 1.23;    ce_e = 0.0;     pe_e = -0.16;}
		else if (W < 5.0)  { ae_e = 0.969; be_e = 0.0521;  ce_e = 0.0;     pe_e = 1.0;  }
		else if (W < 8.0)  { ae_e = 1.18;  be_e = 0.01;    ce_e = 0.0;     pe_e = 1.0;  }
		else if (W < 25.0) { ae_e = 1.196; be_e = 0.008;   ce_e = -0.0004; pe_e = 1.0;  }
		else               { ae_e = 1.68;  be_e = -0.016;  ce_e = 0.0;     pe_e = 1.0;  }
	
		double WM8 = (W - 8.0);
		double ced = (ae_e + be_e*std::exp(pe_e * std::log(W + 1e-12)) + ce_e*WM8*WM8)*1e-3;
		
		double s = RESULT(Stability);
		if(s < 0.0)
		{
			double x;
			if(s > -3.3) 	x = 0.1 + 0.03*s + 0.9*std::exp(4.8 * s);
			else            x = 0.0;
			
			ced *= x;
		}
		else
			ced *= (1.0 + 0.63 * std::sqrt(s));
		
		if(W==0.0) ced =0.0;
		
		return ced;
	)
	
	
	EQUATION(Model, TransferCoefficientForSensibleHeatFlux,
		double W = INPUT(WindSpeed);
		
		double ae_h; double be_h; double ce_h; double pe_h;
		if(W < 2.2)        { ae_h = 0.0;   be_h = 1.185;   ce_h = 0.0;      pe_h = -0.157;}
		else if (W < 5.0)  { ae_h = 0.927; be_h = 0.0546;  ce_h = 0.0;      pe_h = 1.0;  }
		else if (W < 8.0)  { ae_h = 1.15;  be_h = 0.01;    ce_h = 0.0;      pe_h = 1.0;  }
		else if (W < 25.0) { ae_h = 1.17;  be_h = 0.0075;  ce_h = -0.00045; pe_h = 1.0;  }
		else               { ae_h = 1.652; be_h = -0.017;  ce_h = 0.0;      pe_h = 1.0;  }
	
		double WM8 = (W - 8.0);
		double chd = (ae_h + be_h*std::exp(pe_h * std::log(W + 1e-12)) + ce_h*WM8*WM8)*1e-3;
		
		double s = RESULT(Stability);
		if(s < 0.0)
		{
			double x;
			if(s > -3.3) 	x = 0.1 + 0.03*s + 0.9*std::exp(4.8 * s);
			else            x = 0.0;
			
			chd *= x;
		}
		else
			chd *= (1.0 + 0.63 * std::sqrt(s));
		
		return chd;
	)
	
	EQUATION(Model, SurfaceStressCoefficient,
		double W = INPUT(WindSpeed);
		
		double ae_d; double be_d; double ce_d; double pe_d;
		if(W < 2.2)        { ae_d = 0.0;   be_d = 1.08;   pe_d = -0.15;}
		else if (W < 5.0)  { ae_d = 0.771; be_d = 0.0858; pe_d = 1.0;  }
		else if (W < 8.0)  { ae_d = 0.867; be_d = 0.0667; pe_d = 1.0;  }
		else if (W < 25.0) { ae_d = 1.2;   be_d = 0.0025; pe_d = 1.0;  }
		else               { ae_d = 0.0;   be_d = 0.07;   pe_d = 1.0;  }
	
		double cdd = (ae_d + be_d*std::exp(pe_d * std::log(W + 1e-12)))*1e-3;
		
		double s = RESULT(Stability);
		if(s < 0.0)
		{
			double x;
			if(s > -3.3) 	x = 0.1 + 0.03*s + 0.9*std::exp(4.8 * s);
			else            x = 0.0;
			
			cdd *= x;
		}
		else
			cdd *= (1.0 + 0.47 * std::sqrt(s));
		
		return cdd;
	)
	
	EQUATION(Model, LatentHeatOfVaporization,
		//TODO: Should be different for snow..
		return (2.5 - 0.00234*RESULT(EpilimnionTemperature))*1e6;  //TODO: Figure out unit of this!     //Change epi temp to surface temp if that is something else
	)
	
	EQUATION(Model, LatentHeatFlux,
		double latentheat = RESULT(TransferCoefficientForLatentHeatFlux) * RESULT(LatentHeatOfVaporization) * RESULT(AirDensity) * INPUT(WindSpeed) * (RESULT(ActualSpecificHumidity) - RESULT(SaturationSpecificHumidity));
		
		//NOTE: Again, this is probably not correct for snow and ice.
		return latentheat;
	)
	
	EQUATION(Model, SensibleHeatFlux,
		double cpa        = 1008.0;  //Specific heat capacity of air [J/kg/K]
		double airodynamic_coeff = RESULT(TransferCoefficientForSensibleHeatFlux);
		
		return airodynamic_coeff * cpa * RESULT(AirDensity) * INPUT(WindSpeed) * (INPUT(AirTemperature) - RESULT(EpilimnionTemperature)); //Change epi temp to surface temp if that is something else
	)
	
	EQUATION(Model, Evaporation,
		double ref_density = 1025.0;
		double evap = (RESULT(AirDensity) / ref_density) * RESULT(TransferCoefficientForLatentHeatFlux) * INPUT(WindSpeed) * (RESULT(ActualSpecificHumidity) - RESULT(SaturationSpecificHumidity));
		
		return -86400000.0*evap; //NOTE: Convert m/s to mm/day. Also, we want positive sign for value.
	)
	
	EQUATION(Model, SurfaceStress,
		//TODO: Also possible to correct this for rainfall.
		double Wind = INPUT(WindSpeed);
		return RESULT(SurfaceStressCoefficient) * RESULT(AirDensity) * Wind * Wind;
	)
	
	/*
	EQUATION(Model, MixingPower,
		double As = RESULT(LakeSurfaceArea);
		double tau = RESULT(SurfaceStress);
		double shelter_coeff = 1.0 - std::exp(-0.3*As);
		double rho = WaterDensity(RESULT(EpilimnionTemperature));
		double power = shelter_coeff*As*std::sqrt(tau*tau*tau / rho);
		
		if(RESULT(IsIce)) power = 0.0;
		
		return power;
	)
	*/
	
	EQUATION(Model, IsMixing,
		/*
		double power = RESULT(MixingPower);
		double g = 9.81;
		//double As = RESULT(LakeSurfaceArea);
		double epiT = RESULT(EpilimnionTemperature);
		double meanHypT = RESULT(MeanHypolimnionTemperature);
		double delta_rho = WaterDensity(meanHypT) - WaterDensity(epiT);
		//double z_e = PARAMETER();
		double V = LAST_RESULT(LakeVolume);
		
		if(RESULT(IsIce)) return 0.0;
		
		double v = 0.0;
		if(delta_rho >= 0.0)
			v = 86400.0 * power / (g*delta_rho*V);
		
		if(v >= 1.0 || !std::isfinite(v)) //NOTE: The 1.0 threshold for v is a bit arbitrary. TODO: Test how it impacts the result!
			return 1.0;
		*/
		
		double dT = std::abs(RESULT(EpilimnionTemperature) - RESULT(MeanHypolimnionTemperature));
		
		return (double)(dT < 0.4);
	)
	
	EQUATION(Model, InitialEpilimnionThickness,
		return PARAMETER(EpilimnionWinterThickness); //TODO: Should start relative to the day of year
	)
	
	EQUATION(Model, LastMixing,
		bool   ismix  = (bool)RESULT(IsMixing);
		double prev   = LAST_RESULT(LastMixing);
		if(ismix) return (double)CURRENT_TIME().DayOfYear;
		return prev;
	)
	
	EQUATION(Model, EpilimnionThickness,
		bool   ismix      = (bool)RESULT(IsMixing);
		double z_e_winter = PARAMETER(EpilimnionWinterThickness);
		double dz0        = PARAMETER(EpilimnionThickeningRate);
		double z_e_prev   = LAST_RESULT(EpilimnionThickness);		
		double z_b        = LAST_RESULT(WaterLevel);
		double T_e        = LAST_RESULT(EpilimnionTemperature);
		double T_b        = LAST_RESULT(BottomTemperature);

		double z_e = z_e_winter;
		
		double day = (double)CURRENT_TIME().DayOfYear;
		double prevmix = RESULT(LastMixing);
		
		double dz = dz0*(1.0 + 2.0*(day - prevmix));
		
		if(!ismix && T_e > T_b)   // The last clause is to not get deepening in winter.
			z_e = z_e_prev + dz;
		
		return std::min(z_e, z_b-1.0);
	)
	
	/*
	EQUATION(Model, MixingVelocity,
		//TODO: Fix!
		bool ismix = (bool)RESULT(IsMixing);
		bool isice = (bool)RESULT(IsIce);
		double Slope = PARAMETER(EpilimnionThickeningRate);
		double Depth = RESULT(WaterLevel);
		if(ismix) return 5.0*Depth; //NOTE: Just to get a sufficiently large mixing speed.
		if(isice) return 0.0;
		return Slope;
	)
	*/
	
	
	EQUATION(Model, EmittedLongwaveRadiation,
		double watertkelv = RESULT(EpilimnionTemperature) + 273.15;   //Change epi temp to surface temp if that is something else
		double stefanBoltzmannConst = 5.670367e-8;
		double emissivity = 0.98;
		
		//TODO: emissivity could be different for snow/ice
		return emissivity * stefanBoltzmannConst * watertkelv * watertkelv * watertkelv * watertkelv;
	)
	
	EQUATION(Model, DownwellingLongwaveRadation,
		//Downwelling long-wave radiation,  Josey (2003)
	
		double airtkelv = INPUT(AirTemperature) + 273.15;
		double stefanBoltzmannConst = 5.670367e-8;
		
		double cloudcover = INPUT(CloudCover);
		double vaporpressure = RESULT(ActualVaporPressure);
		
		double dew_point_temperature = 34.07 + 4157.0 / std::log(2.1718e8 / vaporpressure);    //(Henderson-Sellers 1984)
		double dew_point_depression = dew_point_temperature - airtkelv;
		
		double cloud_effect = 10.77 * cloudcover * cloudcover +   2.34 * cloudcover  - 18.44;
		double vapor_pressure_effect = 0.84 * (dew_point_depression + 4.01);
		
		double effectiveairtemp = airtkelv + cloud_effect + vapor_pressure_effect;
		
		return stefanBoltzmannConst * effectiveairtemp * effectiveairtemp * effectiveairtemp * effectiveairtemp;
	)
	
	EQUATION(Model, LongwaveRadiation,
		//NOTE: should we have different albedo for longwave and shortwave?
		double albedo = 0.045;
		return (1.0 - albedo) * RESULT(DownwellingLongwaveRadation) - RESULT(EmittedLongwaveRadiation);
	)
	

	EQUATION(Model, ShortwaveRadiation,
		double albedo = 0.045; //TODO: Maybe needs correction for solar angle!
		if(RESULT(IsIce)) albedo = 0.4;
		return (1.0 - albedo) * INPUT(GlobalRadiation);
	)
	
	/*
	EQUATION(Model, ConvectiveHeatFluxScale,
		double surfaceheatflux = RESULT(LatentHeatFlux) + RESULT(SensibleHeatFlux) + RESULT(LongwaveRadiation);
		double surfaceshortwave = RESULT(ShortwaveRadiation);
		
		//Assuming shortwave radiation at depth h is given by
		// I(h) = s_0*e^-ah,     a = PARAMETER(ShortwaveAbsorbance), s_0=surfaceshortwave
		
		double absorb = PARAMETER(ShortwaveAbsorbance);
		double thickness = RESULT(EpilimnionThickness);
		
		double expah = std::exp(-absorb*thickness);
		
		//TODO: Correct this for lake shape
		return surfaceheatflux + surfaceshortwave * (1.0 + expah + (2.0 / (absorb*thickness))*(1.0 - expah)); 
	)
	
	EQUATION(Model, EpilimnionWaterDensity,
		double dtemp = (RESULT(EpilimnionTemperature) + 273.15 - 277.13); // Difference between temperature and reference temperature
		return 999.98*(1.0 - 0.5*1.6509e-5*dtemp*dtemp);   //(Farmer, Carmack 1981)
	)
	
	EQUATION(Model, SurfaceShearVelocity,
		return std::sqrt(RESULT(SurfaceStress) / RESULT(EpilimnionWaterDensity));    //TODO: Why do FLake doc say to involve specific heat capacity here?
	)
	
	EQUATION(Model, ObukhovLength,
		double shearvel = RESULT(SurfaceShearVelocity);
		double specificHeatCapacityOfWater = 4186.0;
		double buoy = 9.81 * 1.6509e-5 * (RESULT(EpilimnionTemperature) + 273.15 - 277.13);
		
		return shearvel * shearvel * shearvel / (buoy * RESULT(ConvectiveHeatFluxScale)) * RESULT(EpilimnionWaterDensity) * specificHeatCapacityOfWater;
	)
	
	EQUATION(Model, EquilibriumEpilimnionThickess,
		
	)
	*/
	
	EQUATION(Model, IceAttenuationCoefficient,
		double lambda_ice = 5.0;
		double ice_attn = 1.0 - std::exp(-lambda_ice * RESULT(IceThickness));
		if(!RESULT(IsIce)) ice_attn = 0.0;
		return ice_attn;
	)
	
	EQUATION(Model, EpilimnionShortwave,
		return RESULT(ShortwaveRadiation) * (1.0 - RESULT(IceAttenuationCoefficient));
	)
	
	EQUATION(Model, IceEnergy,
		//double cw = 4.18e+6;   //volumetric heat capacity of water (J K-1 m-3)
		double Kw = PARAMETER(FreezingSpeed); //Heat transfer coefficent
		double surfaceLayerThickness = 1.0; //NOTE: Thickness of layer that ice formation can draw energy from.
		double iceformationtemp = PARAMETER(IceFormationTemperature);
		
		double energy = (iceformationtemp - RESULT(EpilimnionTemperature)) * Kw * surfaceLayerThickness;
		if(RESULT(IceThickness)<1e-6 && energy < 0.0)
			return 0.0;       //No melting when there is no ice.
		return energy;
	)
	
	EQUATION(Model, SurfaceHeatFlux,
		double surfaceheat = RESULT(LongwaveRadiation) + RESULT(LatentHeatFlux) + RESULT(SensibleHeatFlux);
		if(RESULT(IsIce)) surfaceheat = 0.0;
		return surfaceheat;
	)

	EQUATION(Model, IceThickness,
		double iceDensity = 917.0;              // kg m-3
		double latentHeatOfFreezing = 333500.0; // J kg-1
		double iceHeatConductionCoeff = 2.1; // W/m/K
		
		double iceenergy = RESULT(IceEnergy);   // W/m2
		double airT = INPUT(AirTemperature);
		double precip = INPUT(Precipitation);
		double iceformationtemp = PARAMETER(IceFormationTemperature);
		bool is_ice = (bool)RESULT(IsIce);
		double Hice = RESULT(IceThickness);
		
		double surfaceheatflux = RESULT(SurfaceHeatFlux);
		double shortwavein     = RESULT(ShortwaveRadiation)*RESULT(IceAttenuationCoefficient);
		
		//NOTE: The amount of shortwave radiation that is absorbed by the ice and contributes to melting
		
		double dIce_dt = iceenergy;
		
		if((airT <= iceformationtemp) && is_ice)
		{
			double factor = (1.0 / (10.0 * Hice));
			double iceT = (factor * iceformationtemp + airT) / (1.0 + factor);  // Approximation of ice temperature. TODO: factor out to separate equation?
			dIce_dt += 2.0*iceHeatConductionCoeff * (iceformationtemp-iceT) / (2.0*Hice);
			//NOTE: Stefan's law, but turned into ODE equation:
			// Hice_n = sqrt(Hice_{n-1}^2 + Y)
			// -> d(Hice^2)/dt = Y
			// and d(Hice^2)/dt = 2*Hice*d(Hice)/dt
		}
		else if(is_ice)
		{
			dIce_dt += - shortwavein - surfaceheatflux;
		}
		
		dIce_dt = 86400.0*dIce_dt / (latentHeatOfFreezing*iceDensity);
		

		if(is_ice && (airT <= iceformationtemp)) dIce_dt += 0.001*precip;  //when it is warm we just allow overwater to pass "through". Should really have different densities of precip and ice, but we don't correct water level for that either.
		
		return dIce_dt;
	)
	
	EQUATION(Model, IsIce,
		return (RESULT(IceThickness) > PARAMETER(FrazilThreshold));
	)

	
	EQUATION(Model, MeanLakeTemperature,
		double meanT = RESULT(MeanLakeTemperature);
		//NOTE: Simplification: assume uniform lake density for this purpose... Actually we should maybe keep this constant, because we don't contract the volume with changes in temperature.
		double waterDensity = WaterDensity(meanT);
		double specificHeatCapacityOfWater = 4186.0; // J/kg/K
		
		double volume = RESULT(LakeVolume);
		double mass   = volume * waterDensity;
		
		double area   = RESULT(LakeSurfaceArea);
		
		double surfaceheatflux = RESULT(SurfaceHeatFlux);
		if(RESULT(IsIce)) surfaceheatflux = 0.0;
		
		
		// NOTE: The amount of shortwave that penetrates the ice and contributes to heating the water below. Currently this assumes that no shortwave reaches the lake bottom and is absorbed there instead.
		double surfaceshortwave = /*RESULT(ShortwaveRadiation);*/RESULT(EpilimnionShortwave);
		
		
		double heat = area * (surfaceheatflux + surfaceshortwave); // W/m2 * m2 = W = J/s
		
		double iceEnergy = RESULT(IceEnergy) * area;
		
		//Correction from flow temperature and rainfall
#ifdef EASYLAKE_STANDALONE
		double inflowT = Max(0.0, INPUT(AirTemperature));     //TODO: should take it as an input time series?
#else
		double inflowT = RESULT(InflowTemperature);
#endif
		double rainT   = Max(0.0, INPUT(AirTemperature));
		double outflowT = RESULT(EpilimnionTemperature);
		
#ifdef EASYLAKE_STANDALONE
		double inflowQ = 86400.0*INPUT(LakeInflow);
#else
		double inflowQ = 86400.0*(RESULT(FlowInputFromUpstream) + RESULT(FlowInputFromLand)); //TODO: effluents in PERSiST? But what is their temperature?
#endif
		double rainQ   = INPUT(Precipitation)*area*1e-3;
		double outflowQ = -RESULT(LakeOutflow)*86400.0;
		
		double inflow_dT = (inflowT  - meanT)*inflowQ/volume;
		double rain_dT   = (rainT    - meanT)*rainQ/volume;
		if(rainT == 0.0 && meanT <= 0.0) rain_dT = 0.0; //NOTE: We don't want falling snow that just adds to the ice to heat the lake.
		double outflow_dT= (outflowT - meanT)*outflowQ/volume;
		
		return 86400.0*(heat + iceEnergy) / (specificHeatCapacityOfWater * mass) + inflow_dT + rain_dT + outflow_dT;
		// s/day * J/s / (J/(K*kg) * kg) = K/day = oC/day
	)
	
	//TODO: We recompute volumes a lot in these equations when we could just use RESULT(EpilimnionVolume) etc...
	
	EQUATION(Model, InitialMeanLakeTemperature,
		double T_e = RESULT(EpilimnionTemperature);
		double T_b = RESULT(BottomTemperature);
		double z_e = RESULT(EpilimnionThickness);
		double z_b = RESULT(WaterLevel);
		double a   = RESULT(LakeSurfaceArea);
	
		
		double V   = RESULT(LakeVolume); // = 0.5*a*z_D;
		double V_h = a*(z_b-z_e)*(z_b-z_e)/(2.0*z_b);
		double V_e = V - V_h;
		
		// Linear temperature profile below epilimnion:        integrate ((Y + (T-Y)*(b-x) / (b-e))*A*(b-x)/b) dx from e to b
		//return ( T_e*(V_e + (2.0/3.0)*V_h) + T_b*(1.0/3.0)*V_h)  / V;
		
		// Parabolic temperature profile below epilimnion:     integrate ((Y + (T-Y)*(x-b)^2 / (e-b)^2)*A*(b-x)/b) dx from e to b
		return ( T_e*(V_e + 0.5*V_h) + T_b*0.5*V_h ) / V;
	)
	
	EQUATION(Model, EpilimnionTemperature,
		double T_m = RESULT(MeanLakeTemperature);
		double T_b = RESULT(BottomTemperature);
		double z_e = LAST_RESULT(EpilimnionThickness);   //NOTE: Not ideal with LAST_RESULT, but must be like this in order to avoid circularity
		double z_b = RESULT(WaterLevel);
		double a   = RESULT(LakeSurfaceArea);
	
		double V   = RESULT(LakeVolume); // = 0.5*a*z_b;
		double V_h = a*(z_b-z_e)*(z_b-z_e)/(2.0*z_b);
		double V_e = V - V_h;
		
		// Linear:
		//return (T_m*V - T_b*(1.0/3.0)*V_h) / (V_e + (2.0/3.0)*V_h);
		
		// Parabolic:
		return (T_m*V - 0.5*T_b*V_h) / (V_e + 0.5*V_h);
	)
	
	EQUATION(Model, MeanHypolimnionTemperature,
		double T_m = RESULT(MeanLakeTemperature);
		double T_e = RESULT(EpilimnionTemperature);
		
		double z_e = LAST_RESULT(EpilimnionThickness); //NOTE: Not ideal with LAST_RESULT, but must be like this in order to avoid circularity
		double z_b = RESULT(WaterLevel);
		double a   = RESULT(LakeSurfaceArea);
	
		double V   = RESULT(LakeVolume); // = 0.5*a*z_b;
		double V_h = a*(z_b-z_e)*(z_b-z_e)/(2.0*z_b);
		double V_e = V - V_h;
		
		// V_h*T_h + V_e*T_e = T_m*V
		return (T_m*V - T_e*V_e) / V_h;
	)
	
	EQUATION(Model, TemperatureAtDepth,
		double z = PARAMETER(CalibDepth);
		double T_e = RESULT(EpilimnionTemperature);
		double T_b = RESULT(BottomTemperature);
		double z_e = RESULT(EpilimnionThickness);
		double z_b = RESULT(WaterLevel);
		
		if(z < 0 || z > z_b) return std::numeric_limits<double>::quiet_NaN();
		if(z < z_e) return T_e;
		//return LinearInterpolate(z, z_e, z_b, T_e, T_b);
		return (z-z_b)*(z-z_b)*(T_e-T_b)/((z_e-z_b)*(z_e-z_b)) + T_b;
	)
	
	EQUATION(Model, BottomTemperature,
		return LAST_RESULT(BottomTemperature);      //Note: may eventually be dynamic
	)
	
	EQUATION(Model, EpilimnionVolume,
		double ze = RESULT(EpilimnionThickness);
		double zd = RESULT(WaterLevel);
		return RESULT(LakeSurfaceArea)*ze*(1.0 - 0.5*ze/zd);
	)
	
	EQUATION(Model, HypolimnionVolume,
		double ze = RESULT(EpilimnionThickness);
		double zd = RESULT(WaterLevel);
		return RESULT(LakeSurfaceArea)*0.5*(zd-ze)*(zd-ze)/zd;
	)
	
	EndModule(Model);
}
	