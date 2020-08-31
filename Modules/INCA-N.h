

#if !defined INCAN_MODEL_H

static void
AddIncaNModel(mobius_model *Model)
{
	//NOTE: Uses Persist, SoilTemperature, WaterTemperature
	BeginModule(Model, "INCA-N", "1.2");
	
	SetModuleDescription(Model, R""""(
An early version of INCA-N is described in

[^https://hess.copernicus.org/articles/6/559/2002/hess-6-559-2002.pdf^ A.J.Wade et. al. 2002, A nitrogen model for European catchments: INCA, new model structure and equations, Hydrol. Earth Syst. Sci., 6(3), 559-582]

This version is based on the INCA-N software developed by Dan Butterfield, which saw many revisions after the initial papers. New to this version is that it is integrated with the PERSiST hydrology model instead of the earlier hydrology model that was specific to INCA-N.
)"""");
	
	auto LandscapeUnits = GetIndexSetHandle(Model, "Landscape units");
	auto Reach          = GetIndexSetHandle(Model, "Reaches");
	
	auto Dimensionless      = RegisterUnit(Model);
	auto JulianDay          = RegisterUnit(Model, "Julian day");
	auto MetresPerDay       = RegisterUnit(Model, "m/day");
	auto KgPerHectarePerDay = RegisterUnit(Model, "kg/Ha/day");
	auto KgPerHectarePerYear= RegisterUnit(Model, "kg/Ha/year");
	auto KgPerHectare       = RegisterUnit(Model, "kg/Ha");
	auto Metres             = RegisterUnit(Model, "m");
	auto MilliMetres        = RegisterUnit(Model, "mm");
	auto Hectares           = RegisterUnit(Model, "Ha");
	auto PerDay             = RegisterUnit(Model, "/day");
	auto MgPerL             = RegisterUnit(Model, "mg/l");
	auto M3PerKm2           = RegisterUnit(Model, "m3/km2");
	auto CumecsPerKm2       = RegisterUnit(Model, "m3/km2/s");
	auto Kg                 = RegisterUnit(Model, "kg");
	auto KgPerKm2PerDay     = RegisterUnit(Model, "kg/km2/day");
	auto KgPerDay           = RegisterUnit(Model, "kg/day");
	auto KgPerKm2           = RegisterUnit(Model, "kg/km2");
	auto KgPerM3            = RegisterUnit(Model, "kg/m3");
	auto DegreesCelsius     = RegisterUnit(Model, "°C");
	
	auto Land = RegisterParameterGroup(Model, "Nitrate by land class", LandscapeUnits);
	
	auto DirectRunoffInitialNitrateConcentration      = RegisterParameterDouble(Model, Land, "Direct runoff initial nitrate concentration", MgPerL, 0.0, 0.0, 100.0, "Initial nitrate concentration in quick flow");
	auto DirectRunoffInitialAmmoniumConcentration     = RegisterParameterDouble(Model, Land, "Direct runoff initial ammonium concentration", MgPerL, 0.0, 0.0, 100.0, "Initial ammonium concentration in quick flow");
	auto SoilwaterInitialNitrateConcentration         = RegisterParameterDouble(Model, Land, "Soil water initial nitrate concentration", MgPerL, 5.0, 0.0, 100.0, "Initial nitrate concentration in soil water");
	auto SoilwaterInitialAmmoniumConcentration        = RegisterParameterDouble(Model, Land, "Soil water initial ammonium concentration", MgPerL, 0.0, 0.0, 100.0, "Initial ammonium concentration in soil water");
	
	auto GrowthCurveOffset              = RegisterParameterDouble(Model, Land, "Growth curve offset", Dimensionless, 0.66, 0.0, 10.0, "Vertical shift in the sine curve describing the seasonal N uptake coefficient. Must be greater than growth curve amplitude");
	auto GrowthCurveAmplitude           = RegisterParameterDouble(Model, Land, "Growth curve amplitude", Dimensionless, 0.34, 0.0, 5.0, "Amplitude of sine curve describing the seasonal N uptake coefficient");
	auto PlantGrowthStartDay            = RegisterParameterUInt(Model, Land, "Plant growth start day", JulianDay, 50, 1, 365, "Day of year when N uptake begins, and also the horizontal shift in the seasonal plant N uptake coefficient");
	auto PlantGrowthPeriod              = RegisterParameterUInt(Model, Land, "Plant growth period", JulianDay, 365, 0, 365, "Length of plant growth period in days");
	auto NitratePlantUptakeRate         = RegisterParameterDouble(Model, Land, "Nitrate plant uptake rate", MetresPerDay, 0.2, 0.0, 100.0, "Coefficient to scale the NO3 plant uptake rate");
	auto SoilwaterDenitrificationRate   = RegisterParameterDouble(Model, Land, "Soil water denitrification rate", MetresPerDay, 0.0, 0.0, 100.0, "Rate coefficient to scale the loss of nitrate from the system by denitrification");
	auto AmmoniumNitrificationRate      = RegisterParameterDouble(Model, Land, "Ammonium nitrification rate", MetresPerDay, 0.0, 0.0, 100.0, "Coefficient to scale the rate of conversion of ammonium to nitrate via nitrification in the soil water");
	auto NitrogenFixationRate           = RegisterParameterDouble(Model, Land, "Nitrogen fixation rate", KgPerHectarePerDay, 0.0, 0.0, 100.0, "Coefficient to scale the rate of NO3 inputs to soilwater via N2 fixation");
	auto MaximumNitrogenUptake          = RegisterParameterDouble(Model, Land, "Maximum nitrogen uptake", KgPerHectarePerYear, 300.0, 0.0, 5000.0, "Maximum annual nitrate plus ammonium plant uptake from the soilwater, above which uptake ceases");
	auto FertilizerAdditionStartDay     = RegisterParameterUInt(Model, Land, "Fertilizer addition start day", JulianDay, 50, 1, 365, "Day of year when fertiliser application begins");
	auto FertilizerAdditionPeriod       = RegisterParameterUInt(Model, Land, "Fertilizer addition period", JulianDay, 365, 0, 365, "Length of fertiliser addition period in days");
	auto FertilizerNitrateAdditionRate  = RegisterParameterDouble(Model, Land, "Fertilizer nitrate addition rate", KgPerHectarePerDay, 0.0, 0.0, 100.0, "Amount of nitrate added as fertiliser on each day of fertiliser addition period");
	auto FertilizerAmmoniumAdditionRate = RegisterParameterDouble(Model, Land, "Fertilizer ammonium addition rate", KgPerHectarePerDay, 0.0, 0.0, 100.0, "Amount of ammonium added as fertiliser on each day of fertiliser addition period");
	auto AmmoniumPlantUptakeRate        = RegisterParameterDouble(Model, Land, "Ammonium plant uptake rate", MetresPerDay, 0.0, 0.0, 100.0, "Rate coefficient to scale the rate of NH4 plant uptake");
	auto AmmoniumImmobilisationRate     = RegisterParameterDouble(Model, Land, "Ammonium immobilisation rate", MetresPerDay, 0.0, 0.0, 100.0, "Coefficient to scale the rate of ammonium loss from the soil water via immobilisation");
	auto AmmoniumMineralisationRate     = RegisterParameterDouble(Model, Land, "Ammonium mineralisation rate", KgPerHectarePerDay, 0.0, 0.0, 100.0, "Coefficient to scale the rate of ammonium input to the soil water via mineralisation");
	auto ZeroRateDepth                  = RegisterParameterDouble(Model, Land, "Zero rate depth", MilliMetres, 0.0, 0.0, 9999.0, "Soil water depth at which N processing rates are 0 due to soil moisture limitation. Should be less than the Max rate depth parameter");
	auto MaxRateDepth                   = RegisterParameterDouble(Model, Land, "Max rate depth", MilliMetres, 0.0, 0.0, 9999.0, "Soil water depth above which N processing rates are no longer moisture-limited. The soil retained water depth is likely appropriate");
	auto ResponseToA10DegreeChange      = RegisterParameterDouble(Model, Land, "Response to a 10° soil temperature change", Dimensionless, 2.0, 0.0, 20.0, "Rate response to a 10°C soil temperature change for all processes");
	auto BaseTemperature                = RegisterParameterDouble(Model, Land, "Base temperature at which response is 1", DegreesCelsius, 20.0, -10.0, 50.0, "Base temperature for all processes at which the rate response is 1");
    
	
	auto Reaches = RegisterParameterGroup(Model, "Nitrate by subcatchment", Reach);
	
	auto GroundwaterInitialNitrateConcentration  = RegisterParameterDouble(Model, Reaches, "Groundwater initial nitrate concentration", MgPerL, 4.0, 0.0, 100.0, "Initial nitrate concentration in groundwater");
	auto GroundwaterInitialAmmoniumConcentration = RegisterParameterDouble(Model, Reaches, "Groundwater initial ammonium concentration", MgPerL, 0.0, 0.0, 100.0, "Initial ammonium concentration in groundwater");
	auto GroundwaterDenitrificationRate          = RegisterParameterDouble(Model, Reaches, "Groundwater denitrification rate", MetresPerDay, 0.0, 0.0, 1000.0, "Groundwater denitrification rate");
	auto NitrateDryDeposition                    = RegisterParameterDouble(Model, Reaches, "Nitrate dry deposition", KgPerHectarePerDay, 0.0, 0.0, 1.0, "Daily nitrate dry deposition rate");
	auto NitrateWetDeposition                    = RegisterParameterDouble(Model, Reaches, "Nitrate wet deposition", KgPerHectarePerDay, 0.0, 0.0, 1.0, "Daily nitrate wet deposition rate");
	auto AmmoniumDryDeposition                   = RegisterParameterDouble(Model, Reaches, "Ammonium dry deposition", KgPerHectarePerDay, 0.0, 0.0, 1.0, "Daily ammonium dry deposition rate");
	auto AmmoniumWetDeposition                   = RegisterParameterDouble(Model, Reaches, "Ammonium wet deposition", KgPerHectarePerDay, 0.0, 0.0, 1.0, "Daily ammonium wet deposition rate");
	auto ReachDenitrificationRate                = RegisterParameterDouble(Model, Reaches, "Reach denitrification rate", PerDay, 0.0, 0.0, 100.0, "Coefficient to scale loss of nitrate from the reach via denitrification");
	auto ReachNitrificationRate                  = RegisterParameterDouble(Model, Reaches, "Reach nitrification rate", PerDay, 0.0, 0.0, 100.0, "Coefficient to scale reach nitrification (conversion of ammonium to nitrate)");
	auto ReachEffluentNitrateConcentration       = RegisterParameterDouble(Model, Reaches, "Reach effluent nitrate concentration", MgPerL, 0.0, 0.0, 1000.0, "Concentration of nitrate in effluent inputs to reach");
	auto ReachEffluentAmmoniumConcentration      = RegisterParameterDouble(Model, Reaches, "Reach effluent ammonium concentration", MgPerL, 0.0, 0.0, 1000.0, "Concentration of ammonium in effluent inputs to reach");
	
	auto InitialStreamNitrateConcentration = RegisterParameterDouble(Model, Reaches, "Initial stream nitrate concentration", MgPerL, 0.0, 0.0, 1000.0, "Initial stream nitrate concentration");
	auto InitialStreamAmmoniumConcentration = RegisterParameterDouble(Model, Reaches, "Initial stream ammonium concentration", MgPerL, 0.0, 0.0, 1000.0, "Initial stream ammonium concentration");
	

	auto NitrateFertilizerTimeseries             = RegisterInput(Model, "Fertilizer nitrate", KgPerHectarePerDay);
	auto AmmoniumFertilizerTimeseries            = RegisterInput(Model, "Fertilizer ammonium", KgPerHectarePerDay);
	auto NitrateWetDepositionTimeseries          = RegisterInput(Model, "Nitrate wet deposition", KgPerHectarePerDay);
	auto NitrateDryDepositionTimeseries          = RegisterInput(Model, "Nitrate dry deposition", KgPerHectarePerDay);
	auto AmmoniumWetDepositionTimeseries         = RegisterInput(Model, "Ammonium wet deposition", KgPerHectarePerDay);
	auto AmmoniumDryDepositionTimeseries         = RegisterInput(Model, "Ammonium dry deposition", KgPerHectarePerDay);
	auto NitrateEffluentConcentrationTimeseries  = RegisterInput(Model, "Effluent nitrate concentration", MgPerL);
	auto AmmoniumEffluentConcentrationTimeseries = RegisterInput(Model, "Effluent ammonium concentration", MgPerL);
	auto GrowthCurveOffsetTimeseries             = RegisterInput(Model, "Growth curve offset", Dimensionless);
	auto GrowthCurveAmplitudeTimeseries          = RegisterInput(Model, "Growth curve amplitude", Dimensionless);
	
	auto AbstractionTimeseries = GetInputHandle(Model, "Abstraction flow");
	auto EffluentTimeseries    = GetInputHandle(Model, "Effluent flow");
	
	auto ActualPrecipitation   = GetInputHandle(Model, "Actual precipitation");

	
	
	auto IncaSolver = RegisterSolver(Model, "Inca solver", 0.1, IncaDascru);
	
	auto Soils = GetIndexSetHandle(Model, "Soils");
	
	auto DirectRunoff = RequireIndex(Model, Soils, "Direct runoff");
	auto Soilwater    = RequireIndex(Model, Soils, "Soil water");
	auto Groundwater  = RequireIndex(Model, Soils, "Groundwater");
	
	//NOTE: These are from PERSiST:
	auto WaterDepth3           = GetEquationHandle(Model, "Water depth 3"); //NOTE: This is right before percolation and runoff is subtracted.
	auto WaterDepth            = GetEquationHandle(Model, "Water depth");   //NOTE: This is after everything is subtracted.
	auto RunoffToReach         = GetEquationHandle(Model, "Runoff to reach");
	auto SaturationExcessInput = GetEquationHandle(Model, "Saturation excess input");
	auto SoilTemperature       = GetEquationHandle(Model, "Soil temperature");
	auto PercolationInput      = GetEquationHandle(Model, "Percolation input");
	auto PercolationOut        = GetEquationHandle(Model, "Percolation out");
	
	
	auto SoilwaterVolume             = RegisterEquation(Model, "Soil water volume", M3PerKm2);
	auto GroundwaterVolume           = RegisterEquation(Model, "Groundwater volume", M3PerKm2);
	auto DirectRunoffToReachFraction = RegisterEquation(Model, "Direct runoff to reach fraction", PerDay);
	auto DirectRunoffToSoilFraction  = RegisterEquation(Model, "Direct runoff to soil fraction", PerDay);
	auto SoilToDirectRunoffFraction  = RegisterEquation(Model, "Soil to direct runoff fraction", PerDay);
	auto SoilToGroundwaterFraction   = RegisterEquation(Model, "Soil to groundwater fraction", PerDay);
	auto SoilToReachFraction         = RegisterEquation(Model, "Soil to reach fraction", PerDay);
	auto GroundwaterToReachFraction  = RegisterEquation(Model, "Groundwater to reach fraction", PerDay);
	
	auto NitrateDryDepositionToSoil  = RegisterEquation(Model, "Nitrate dry deposition to soil", KgPerHectarePerDay);
	auto NitrateDryStorage           = RegisterEquation(Model, "Nitrate dry storage", KgPerHectare);
	auto AmmoniumDryDepositionToSoil = RegisterEquation(Model, "Ammonium dry deposition to soil", KgPerHectarePerDay);
	auto AmmoniumDryStorage          = RegisterEquation(Model, "Ammonium dry storage", KgPerHectare);
	auto NitrateFertilizerAddition   = RegisterEquation(Model, "Nitrate fertilizer addition", KgPerHectarePerDay);
	auto AmmoniumFertilizerAddition  = RegisterEquation(Model, "Ammonium fertilizer addition", KgPerHectarePerDay);
	auto CurrentGrowthCurveAmplitude = RegisterEquation(Model, "Growth curve amplitude", Dimensionless);
	auto CurrentPlantGrowthStartDay  = RegisterEquation(Model, "Plant growth start day", Dimensionless);
	
	auto DirectRunoffInitialNitrate = RegisterEquationInitialValue(Model, "Direct runoff initial nitrate", KgPerKm2);
	auto DirectRunoffNitrate        = RegisterEquationODE(Model, "Direct runoff nitrate", KgPerKm2);
	SetSolver(Model, DirectRunoffNitrate, IncaSolver);
	SetInitialValue(Model, DirectRunoffNitrate, DirectRunoffInitialNitrate);
	auto DirectRunoffInitialAmmonium = RegisterEquationInitialValue(Model, "Direct runoff initial ammonium", KgPerKm2);
	auto DirectRunoffAmmonium       = RegisterEquationODE(Model, "Direct runoff ammonium", KgPerKm2);
	SetSolver(Model, DirectRunoffAmmonium, IncaSolver);
	SetInitialValue(Model, DirectRunoffAmmonium, DirectRunoffInitialAmmonium);
	auto DrynessFactor = RegisterEquation(Model, "Dryness factor", Dimensionless);
	auto SeasonalGrowthFactor = RegisterEquation(Model, "Seasonal growth factor", Dimensionless);
	auto TemperatureFactor    = RegisterEquation(Model, "Temperature factor", Dimensionless);
	auto YearlyAccumulatedNitrogenUptake = RegisterEquation(Model, "Yearly accumulated nitrogen uptake", KgPerHectare);
	auto NitrateUptake = RegisterEquation(Model, "Nitrate uptake", KgPerKm2PerDay);
	SetSolver(Model, NitrateUptake, IncaSolver);
	auto Denitrification = RegisterEquation(Model, "Denitrification", KgPerKm2PerDay);
	SetSolver(Model, Denitrification, IncaSolver);
	auto Nitrification = RegisterEquation(Model, "Nitrification", KgPerKm2PerDay);
	SetSolver(Model, Nitrification, IncaSolver);
	auto Fixation = RegisterEquation(Model, "Fixation", KgPerKm2PerDay);
	SetSolver(Model, Fixation, IncaSolver);
	auto SoilwaterNitrateInput = RegisterEquation(Model, "Soil water nitrate input", KgPerKm2PerDay);
	auto SoilwaterInitialNitrate = RegisterEquationInitialValue(Model, "Soil water initial nitrate", KgPerKm2);
	auto SoilwaterNitrate = RegisterEquationODE(Model, "Soil water nitrate", KgPerKm2);
	SetSolver(Model, SoilwaterNitrate, IncaSolver);
	SetInitialValue(Model, SoilwaterNitrate, SoilwaterInitialNitrate);
	auto AmmoniumUptake = RegisterEquation(Model, "Ammonium uptake", KgPerKm2PerDay);
	SetSolver(Model, AmmoniumUptake, IncaSolver);
	auto Immobilisation = RegisterEquation(Model, "Immobilisation", KgPerKm2PerDay);
	SetSolver(Model, Immobilisation, IncaSolver);
	auto Mineralisation = RegisterEquation(Model, "Mineralisation", KgPerKm2PerDay);
	SetSolver(Model, Mineralisation, IncaSolver);
	auto SoilwaterAmmoniumInput  = RegisterEquation(Model, "Soil water ammonium input", KgPerKm2PerDay);
	auto SoilwaterInitialAmmonium = RegisterEquationInitialValue(Model, "Soil water initial ammmonium", KgPerKm2);
	auto SoilwaterAmmonium = RegisterEquationODE(Model, "Soil water ammonium", KgPerKm2);
	SetSolver(Model, SoilwaterAmmonium, IncaSolver);
	SetInitialValue(Model, SoilwaterAmmonium, SoilwaterInitialAmmonium);
	auto GroundwaterDenitrification = RegisterEquation(Model, "Groundwater denitrification", KgPerKm2PerDay);
	SetSolver(Model, GroundwaterDenitrification, IncaSolver);
	auto GroundwaterInitialNitrate = RegisterEquationInitialValue(Model, "GroundwaterInitialNitrate", KgPerKm2);
	auto GroundwaterNitrate = RegisterEquationODE(Model, "Groundwater nitrate", KgPerKm2);
	SetSolver(Model, GroundwaterNitrate, IncaSolver);
	SetInitialValue(Model, GroundwaterNitrate, GroundwaterInitialNitrate);
	auto GroundwaterInitialAmmonium = RegisterEquationInitialValue(Model, "Groundwater initial ammmonium", KgPerKm2);
	auto GroundwaterAmmonium = RegisterEquationODE(Model, "Groundwater ammonium", KgPerKm2);
	SetSolver(Model, GroundwaterAmmonium, IncaSolver);
	SetInitialValue(Model, GroundwaterAmmonium, GroundwaterInitialAmmonium);
	
	auto SoilwaterNitrateConcentration = RegisterEquation(Model, "Soil water nitrate concentration", MgPerL);
	SetSolver(Model, SoilwaterNitrateConcentration, IncaSolver);
	auto GroundwaterNitrateConcentration = RegisterEquation(Model, "Groundwater nitrate concentration", MgPerL);
	SetSolver(Model, GroundwaterNitrateConcentration, IncaSolver);
	auto SoilwaterAmmoniumConcentration = RegisterEquation(Model, "Soil water ammonium concentration", MgPerL);
	SetSolver(Model, SoilwaterAmmoniumConcentration, IncaSolver);
	auto GroundwaterAmmoniumConcentration = RegisterEquation(Model, "Groundwater ammonium concentration", MgPerL);
	SetSolver(Model, GroundwaterAmmoniumConcentration, IncaSolver);
	
	
	EQUATION(Model, DirectRunoffInitialNitrate,
		return PARAMETER(DirectRunoffInitialNitrateConcentration) * RESULT(WaterDepth, DirectRunoff);
	)
	
	EQUATION(Model, DirectRunoffInitialAmmonium,
		return PARAMETER(DirectRunoffInitialAmmoniumConcentration)* RESULT(WaterDepth, DirectRunoff);
	)
	
	EQUATION(Model, SoilwaterInitialNitrate,
		return PARAMETER(SoilwaterInitialNitrateConcentration)* RESULT(WaterDepth, Soilwater);
	)
	
	EQUATION(Model, SoilwaterInitialAmmonium,
		return PARAMETER(SoilwaterInitialAmmoniumConcentration) * RESULT(WaterDepth, Soilwater);
	)
	
	EQUATION(Model, GroundwaterInitialNitrate,
		return PARAMETER(GroundwaterInitialNitrateConcentration) * RESULT(WaterDepth, Groundwater);
	)
	
	EQUATION(Model, GroundwaterInitialAmmonium,
		return PARAMETER(GroundwaterInitialAmmoniumConcentration) * RESULT(WaterDepth, Groundwater);
	)
	
	
	
	EQUATION(Model, SoilwaterVolume,
		return RESULT(WaterDepth, Soilwater) * 1000.0;
	)
	
	EQUATION(Model, GroundwaterVolume,
		return RESULT(WaterDepth, Groundwater) * 1000.0;
	)
	
	EQUATION(Model, SoilwaterNitrateConcentration,
		return SafeDivide(RESULT(SoilwaterNitrate), RESULT(SoilwaterVolume)) * 1000.0;
	)
	
	EQUATION(Model, GroundwaterNitrateConcentration,
		return SafeDivide(RESULT(GroundwaterNitrate), RESULT(GroundwaterVolume)) * 1000.0;
	)
	
	EQUATION(Model, SoilwaterAmmoniumConcentration,
		return SafeDivide(RESULT(SoilwaterAmmonium), RESULT(SoilwaterVolume)) * 1000.0;
	)
	
	EQUATION(Model, GroundwaterAmmoniumConcentration,
		return SafeDivide(RESULT(GroundwaterAmmonium), RESULT(GroundwaterVolume)) * 1000.0;
	)
	
	
	EQUATION(Model, DirectRunoffToReachFraction,
		return SafeDivide(RESULT(RunoffToReach, DirectRunoff), RESULT(WaterDepth3, DirectRunoff));
	)
	
	EQUATION(Model, DirectRunoffToSoilFraction,
		return SafeDivide(RESULT(PercolationInput, Soilwater), RESULT(WaterDepth3, DirectRunoff));
	)
	
	EQUATION(Model, SoilToDirectRunoffFraction,
		return SafeDivide(RESULT(SaturationExcessInput, DirectRunoff), RESULT(WaterDepth3, Soilwater));
	)
	
	EQUATION(Model, SoilToReachFraction,
		return SafeDivide(RESULT(RunoffToReach, Soilwater), RESULT(WaterDepth3, Soilwater));
	)
	
	EQUATION(Model, SoilToGroundwaterFraction,
		return SafeDivide(RESULT(PercolationInput, Groundwater), RESULT(WaterDepth3, Soilwater));
	)

	EQUATION(Model, GroundwaterToReachFraction,
		return SafeDivide(RESULT(RunoffToReach, Groundwater), RESULT(WaterDepth3, Groundwater));
	)
	
	
	EQUATION(Model, NitrateDryDepositionToSoil,
		double storage = LAST_RESULT(NitrateDryStorage);
		double deposition = IF_INPUT_ELSE_PARAMETER(NitrateDryDepositionTimeseries, NitrateDryDeposition);
		
		if(INPUT(ActualPrecipitation) > 0.0)
		{
			return storage + deposition;
		}
		return 0.0;
	)
	
	EQUATION(Model, NitrateDryStorage,
		double storage = LAST_RESULT(NitrateDryStorage);
		double deposition = IF_INPUT_ELSE_PARAMETER(NitrateDryDepositionTimeseries, NitrateDryDeposition);
		if(INPUT(ActualPrecipitation) == 0.0)
		{
			return storage + deposition;
		}
		return 0.0;
	)
	
	EQUATION(Model, AmmoniumDryDepositionToSoil,
		double storage = LAST_RESULT(AmmoniumDryStorage);
		double deposition = IF_INPUT_ELSE_PARAMETER(AmmoniumDryDepositionTimeseries, AmmoniumDryDeposition);
		
		if(INPUT(ActualPrecipitation) > 0.0)
		{
			return storage + deposition;
		}
		return 0.0;
	)
	
	EQUATION(Model, AmmoniumDryStorage,
		double storage = LAST_RESULT(AmmoniumDryStorage);
		double deposition = IF_INPUT_ELSE_PARAMETER(AmmoniumDryDepositionTimeseries, AmmoniumDryDeposition);
		if(INPUT(ActualPrecipitation) == 0.0)
		{
			return storage + deposition;
		}
		return 0.0;
	)
	
	EQUATION(Model, NitrateFertilizerAddition,
		double startday = (double)PARAMETER(FertilizerAdditionStartDay);
		double endday   = startday + (double)PARAMETER(FertilizerAdditionPeriod);
		double currentday = (double)CURRENT_TIME().DayOfYear;
		double additionrate = IF_INPUT_ELSE_PARAMETER(NitrateFertilizerTimeseries, FertilizerNitrateAdditionRate);
		
		if(!INPUT_WAS_PROVIDED(NitrateFertilizerTimeseries) && (currentday < startday || currentday > endday))
		{
			additionrate = 0.0;
		}
		return additionrate;
	)
	
	EQUATION(Model, AmmoniumFertilizerAddition,
		double startday = (double)PARAMETER(FertilizerAdditionStartDay);
		double endday   = startday + (double)PARAMETER(FertilizerAdditionPeriod);
		double currentday = (double)CURRENT_TIME().DayOfYear;
		double additionrate = IF_INPUT_ELSE_PARAMETER(AmmoniumFertilizerTimeseries, FertilizerAmmoniumAdditionRate);
		
		if(!INPUT_WAS_PROVIDED(AmmoniumFertilizerTimeseries) && (currentday < startday || currentday > endday))
		{
			additionrate = 0.0;
		}
		return additionrate;
	)
	
	EQUATION(Model, DrynessFactor,
		double depth = RESULT(WaterDepth, Soilwater);
		double maxratedepth = PARAMETER(MaxRateDepth);
		double zeroratedepth = PARAMETER(ZeroRateDepth);
		
		return SCurveResponse(depth, zeroratedepth, maxratedepth, 0.0, 1.0);
	)
	
	EQUATION(Model, CurrentGrowthCurveAmplitude,
		//NOTE: The only purpose of this equation is so that we can look up LAST_RESULT of it below (since there is no LAST_INPUT lookup)
		return IF_INPUT_ELSE_PARAMETER(GrowthCurveAmplitudeTimeseries, GrowthCurveAmplitude);
	)
	
	EQUATION(Model, CurrentPlantGrowthStartDay,
		double start = LAST_RESULT(CurrentPlantGrowthStartDay);
		double startparam = (double)PARAMETER(PlantGrowthStartDay);
		
		if(LAST_RESULT(CurrentGrowthCurveAmplitude) != RESULT(CurrentGrowthCurveAmplitude))
		{
			start = (double)CURRENT_TIME().DayOfYear;
		}
		
		if(!INPUT_WAS_PROVIDED(GrowthCurveOffsetTimeseries)) start = startparam;
		
		return start;
	)

	EQUATION(Model, SeasonalGrowthFactor,
		double startday = (double)RESULT(CurrentPlantGrowthStartDay);
		double daysthisyear = (double)CURRENT_TIME().DaysThisYear;
		double currentday   = (double)CURRENT_TIME().DayOfYear;
		double endday   = startday + (double)PARAMETER(PlantGrowthPeriod);
		
		double offset    = IF_INPUT_ELSE_PARAMETER(GrowthCurveOffsetTimeseries, GrowthCurveOffset);
		double amplitude = IF_INPUT_ELSE_PARAMETER(GrowthCurveAmplitudeTimeseries, GrowthCurveAmplitude);
		
		double curve = offset + amplitude * sin(2.0 * Pi * (currentday - startday) / daysthisyear );
		
		if(!INPUT_WAS_PROVIDED(GrowthCurveOffsetTimeseries) && (currentday < startday || currentday > endday)) return 0.0;
		
		return curve;
	)
	
	EQUATION(Model, TemperatureFactor,
		return pow(PARAMETER(ResponseToA10DegreeChange), (RESULT(SoilTemperature) - PARAMETER(BaseTemperature)) * 0.1);
	)
	
	

	EQUATION(Model, DirectRunoffNitrate,
		return 
			  RESULT(SoilwaterNitrate) * RESULT(SoilToDirectRunoffFraction)
			- RESULT(DirectRunoffNitrate) * (RESULT(DirectRunoffToSoilFraction) + RESULT(DirectRunoffToReachFraction));
	)
	
	EQUATION(Model, DirectRunoffAmmonium,
		return
			  RESULT(SoilwaterAmmonium) * RESULT(SoilToDirectRunoffFraction)
			- RESULT(DirectRunoffAmmonium) * (RESULT(DirectRunoffToSoilFraction) + RESULT(DirectRunoffToReachFraction));
	)
	
	EQUATION(Model, YearlyAccumulatedNitrogenUptake,
		double accumulated = LAST_RESULT(YearlyAccumulatedNitrogenUptake) + (LAST_RESULT(NitrateUptake) + LAST_RESULT(AmmoniumUptake)) / 100.0; //NOTE convert 1/km2 to 1/Ha
		if(CURRENT_TIME().DayOfYear == 1) accumulated = 0.0;
		return accumulated;
	)
	
	EQUATION(Model, NitrateUptake,
		double nitrateuptake = 
			  PARAMETER(NitratePlantUptakeRate) 
			* RESULT(TemperatureFactor)
			* RESULT(SoilwaterNitrateConcentration)
			* RESULT(DrynessFactor)
			* RESULT(SeasonalGrowthFactor)
			* 1000.0;

		if(RESULT(YearlyAccumulatedNitrogenUptake) > PARAMETER(MaximumNitrogenUptake)) return 0.0;
		
		return nitrateuptake;
	)

	EQUATION(Model, Denitrification,
		return
			  PARAMETER(SoilwaterDenitrificationRate)
			* RESULT(TemperatureFactor)
			* RESULT(SoilwaterNitrateConcentration)
			* RESULT(DrynessFactor)
			* 1000.0;
	)
	
	EQUATION(Model, Nitrification,
		return
			  PARAMETER(AmmoniumNitrificationRate)
			* RESULT(TemperatureFactor)
			* RESULT(SoilwaterAmmoniumConcentration)
			* RESULT(DrynessFactor)
			* 1000.0;
	)
  
	EQUATION(Model, Fixation,
		return PARAMETER(NitrogenFixationRate) * RESULT(TemperatureFactor) * 100.0;
	)

	EQUATION(Model, SoilwaterNitrateInput,
		double wetdeposition = IF_INPUT_ELSE_PARAMETER(NitrateWetDepositionTimeseries, NitrateWetDeposition);  //TODO: This is incorrect! Wet deposition if provided as a parameter should be spread out according to precipitation
	
		return 100.0 * (
			  RESULT(NitrateFertilizerAddition)
			+ RESULT(NitrateDryDepositionToSoil)
			+ wetdeposition);
	)
    
	EQUATION(Model, SoilwaterNitrate,
		return
			  RESULT(SoilwaterNitrateInput)
			- RESULT(NitrateUptake)
			- RESULT(Denitrification)
			+ RESULT(Nitrification)
			+ RESULT(Fixation)
			
			- RESULT(SoilwaterNitrate) * (RESULT(SoilToDirectRunoffFraction) + RESULT(SoilToGroundwaterFraction) + RESULT(SoilToReachFraction))
			+ RESULT(DirectRunoffNitrate) * RESULT(DirectRunoffToSoilFraction);
	)
	
	EQUATION(Model, AmmoniumUptake,
		double uptake =
			  PARAMETER(AmmoniumPlantUptakeRate)
			* RESULT(TemperatureFactor)
			* RESULT(SoilwaterAmmoniumConcentration)
			* RESULT(DrynessFactor)
			* RESULT(SeasonalGrowthFactor)
			* 1000.0;
			
		if(RESULT(YearlyAccumulatedNitrogenUptake) > PARAMETER(MaximumNitrogenUptake)) uptake = 0.0;
		
		return uptake;
	)
	
	EQUATION(Model, Immobilisation,
		return
			  PARAMETER(AmmoniumImmobilisationRate)
			* RESULT(TemperatureFactor)
			* RESULT(SoilwaterAmmoniumConcentration)
			* RESULT(DrynessFactor)
			* 1000.0;
	)

	EQUATION(Model, Mineralisation,
		return PARAMETER(AmmoniumMineralisationRate) * RESULT(TemperatureFactor) * RESULT(DrynessFactor) * 100.0;
	)

	EQUATION(Model, SoilwaterAmmoniumInput,
		double wetdeposition = IF_INPUT_ELSE_PARAMETER(AmmoniumWetDepositionTimeseries, AmmoniumWetDeposition);  //TODO: This is incorrect! Wet deposition if provided as a parameter should be spread out according to precipitation
	
		return 100.0 * (
			  RESULT(AmmoniumFertilizerAddition)
			+ RESULT(AmmoniumDryDepositionToSoil)
			+ wetdeposition);
	)

	EQUATION(Model, SoilwaterAmmonium,
		return
			  RESULT(SoilwaterAmmoniumInput)
			- RESULT(AmmoniumUptake)
			- RESULT(Nitrification)
			- RESULT(Immobilisation)
			+ RESULT(Mineralisation)
			
			- RESULT(SoilwaterAmmonium) * (RESULT(SoilToDirectRunoffFraction) + RESULT(SoilToGroundwaterFraction) + RESULT(SoilToReachFraction))
			+ RESULT(DirectRunoffAmmonium) * RESULT(DirectRunoffToSoilFraction);
	)
	
	EQUATION(Model, GroundwaterDenitrification,
		return SafeDivide(RESULT(GroundwaterNitrate) * PARAMETER(GroundwaterDenitrificationRate) * RESULT(TemperatureFactor), RESULT(GroundwaterVolume)) * 1000000.0;
	)

	
	EQUATION(Model, GroundwaterNitrate,
		return
			  RESULT(SoilwaterNitrate) * RESULT(SoilToGroundwaterFraction)
			- RESULT(GroundwaterNitrate) * RESULT(GroundwaterToReachFraction)
			- RESULT(GroundwaterDenitrification);
	)
	
	
	EQUATION(Model, GroundwaterAmmonium,
		return
			  RESULT(SoilwaterAmmonium) * RESULT(SoilToGroundwaterFraction)
			- RESULT(GroundwaterAmmonium) * RESULT(GroundwaterToReachFraction);
	)
	
	
	auto TotalNitrateToStream = RegisterEquation(Model, "Total nitrate to stream", KgPerKm2PerDay);
	SetSolver(Model, TotalNitrateToStream, IncaSolver);
	auto DiffuseNitrate = RegisterEquation(Model, "Diffuse nitrate", KgPerDay);
	auto TotalDiffuseNitrateOutput = RegisterEquationCumulative(Model, "Total diffuse nitrate output", DiffuseNitrate, LandscapeUnits);
	auto TotalAmmoniumToStream = RegisterEquation(Model, "Total ammonium to stream", KgPerKm2PerDay);
	SetSolver(Model, TotalAmmoniumToStream, IncaSolver);
	auto DiffuseAmmonium = RegisterEquation(Model, "Diffuse ammonium", KgPerDay);
	auto TotalDiffuseAmmoniumOutput = RegisterEquationCumulative(Model, "Total diffuse ammonium output", DiffuseAmmonium, LandscapeUnits);
	
	auto TerrestrialCatchmentArea = GetParameterDoubleHandle(Model, "Terrestrial catchment area"); //NOTE: From persist
	auto Percent                  = GetParameterDoubleHandle(Model, "%");                          //NOTE: From persist
	
	EQUATION(Model, TotalNitrateToStream,
		return
			RESULT(DirectRunoffNitrate) * RESULT(DirectRunoffToReachFraction)
		  + RESULT(SoilwaterNitrate) * RESULT(SoilToReachFraction)
		  + RESULT(GroundwaterNitrate) * RESULT(GroundwaterToReachFraction); 
	)
	
	EQUATION(Model, DiffuseNitrate,
		double percent = PARAMETER(Percent);
		return RESULT(TotalNitrateToStream) * PARAMETER(TerrestrialCatchmentArea) * percent / 100.0;
	)
	
	EQUATION(Model, TotalAmmoniumToStream,
		return 
		  RESULT(DirectRunoffAmmonium) * RESULT(DirectRunoffToReachFraction)
		+ RESULT(SoilwaterAmmonium) * RESULT(SoilToReachFraction)
		+ RESULT(GroundwaterAmmonium) * RESULT(GroundwaterToReachFraction);
	)
	
	EQUATION(Model, DiffuseAmmonium,
		double percent = PARAMETER(Percent);
		return RESULT(TotalAmmoniumToStream) * PARAMETER(TerrestrialCatchmentArea) * percent / 100.0;
	)
	
	
	
	auto ReachSolver = GetSolverHandle(Model, "Reach solver"); //NOTE: from persist
	auto ReachFlow   = GetEquationHandle(Model, "Reach flow");
	auto ReachVolume = GetEquationHandle(Model, "Reach volume");
	auto ReachAbstraction = GetEquationHandle(Model, "Reach abstraction");
	
	auto ReachNitrateOutput = RegisterEquation(Model, "Reach nitrate output", KgPerDay);
	SetSolver(Model, ReachNitrateOutput, ReachSolver);
	auto WaterTemperatureFactor = RegisterEquation(Model, "Water temperature factor", Dimensionless);
	auto ReachDenitrification = RegisterEquation(Model, "Reach denitrification", KgPerDay);
	SetSolver(Model, ReachDenitrification, ReachSolver);
	auto ReachNitrification = RegisterEquation(Model, "Reach nitrification", KgPerDay);
	SetSolver(Model, ReachNitrification, ReachSolver);
	auto ReachUpstreamNitrate = RegisterEquation(Model, "Reach upstream nitrate", KgPerDay);
	auto ReachEffluentNitrate = RegisterEquation(Model, "Reach effluent nitrate", KgPerDay);
	auto ReachTotalNitrateInput = RegisterEquation(Model, "Reach total nitrate input", KgPerDay);
	auto ReachNitrateAbstraction = RegisterEquation(Model, "Reach nitrate abstraction", KgPerDay);
	SetSolver(Model, ReachNitrateAbstraction, ReachSolver);
	auto ReachNitrateInitialValue = RegisterEquationInitialValue(Model, "Reach nitrate initial value", Kg);
	auto ReachNitrate = RegisterEquationODE(Model, "Reach nitrate", Kg);
	SetSolver(Model, ReachNitrate, ReachSolver);
	SetInitialValue(Model, ReachNitrate, ReachNitrateInitialValue);
	auto ReachUpstreamAmmonium = RegisterEquation(Model, "Reach upstream ammonium", KgPerDay);
	auto ReachEffluentAmmonium = RegisterEquation(Model, "Reach effluent ammonium", KgPerDay);
	auto ReachTotalAmmoniumInput = RegisterEquation(Model, "Reach total ammonium input", KgPerDay);
	auto ReachAmmoniumAbstraction = RegisterEquation(Model, "Reach ammonium abstraction", KgPerDay);
	SetSolver(Model, ReachAmmoniumAbstraction, ReachSolver);
	auto ReachAmmoniumOutput = RegisterEquation(Model, "Reach ammonium output", KgPerDay);
	SetSolver(Model, ReachAmmoniumOutput, ReachSolver);
	auto ReachAmmoniumInitialValue = RegisterEquationInitialValue(Model, "Reach ammmonium initial value", Kg);
	auto ReachAmmonium = RegisterEquationODE(Model, "Reach ammonium", Kg);
	SetSolver(Model, ReachAmmonium, ReachSolver);
	SetInitialValue(Model, ReachAmmonium, ReachAmmoniumInitialValue);
	
	EQUATION(Model, ReachNitrateOutput,
		return RESULT(ReachNitrate) * SafeDivide(RESULT(ReachFlow) * 86400.0, RESULT(ReachVolume));
	)
	
	auto WaterTemperature = GetEquationHandle(Model, "Water temperature");
	
	EQUATION(Model, WaterTemperatureFactor,
		return pow(1.047, RESULT(WaterTemperature) - 20.0);
	)
	
	EQUATION(Model, ReachDenitrification,
		return
			  PARAMETER(ReachDenitrificationRate)
			* RESULT(ReachNitrate)
			* RESULT(WaterTemperatureFactor);
	)
	
	EQUATION(Model, ReachNitrification,
		return
			  PARAMETER(ReachNitrificationRate)
			* RESULT(ReachAmmonium)
			* RESULT(WaterTemperatureFactor);
	)
	
	EQUATION(Model, ReachUpstreamNitrate,
		double reachInput = 0.0;
		FOREACH_INPUT(Reach,
			reachInput += RESULT(ReachNitrateOutput, *Input);
		)
		return reachInput;
	)
	
	auto EffluentFlow = GetParameterDoubleHandle(Model, "Effluent flow");
	auto ReachHasEffluentInput = GetParameterBoolHandle(Model, "Reach has effluent input");
	
	EQUATION(Model, ReachEffluentNitrate,
		double effluentflow        = IF_INPUT_ELSE_PARAMETER(EffluentTimeseries, EffluentFlow);
		double effluentnitrateconc = IF_INPUT_ELSE_PARAMETER(NitrateEffluentConcentrationTimeseries, ReachEffluentNitrateConcentration);
		
		double effluentnitrate = effluentflow * effluentnitrateconc * 86.4;
		
		if(!PARAMETER(ReachHasEffluentInput)) effluentnitrate = 0.0;
		return effluentnitrate;
	)
	
	EQUATION(Model, ReachTotalNitrateInput,
		return RESULT(ReachUpstreamNitrate) + RESULT(TotalDiffuseNitrateOutput) + RESULT(ReachEffluentNitrate);
	)
	
	EQUATION(Model, ReachNitrateAbstraction,
		return RESULT(ReachNitrate) * SafeDivide(RESULT(ReachAbstraction) * 86400.0, RESULT(ReachVolume)); 
	)
	
	EQUATION(Model, ReachNitrateInitialValue,
		return PARAMETER(InitialStreamNitrateConcentration) * RESULT(ReachVolume) / 1000.0;
	)
	
	EQUATION(Model, ReachNitrate,
		return
			  RESULT(ReachTotalNitrateInput)
			- RESULT(ReachNitrateOutput)
			- RESULT(ReachDenitrification)
			+ RESULT(ReachNitrification)
			- RESULT(ReachNitrateAbstraction);
	)
	
	EQUATION(Model, ReachUpstreamAmmonium,
		double reachInput = 0.0;
		FOREACH_INPUT(Reach,
			reachInput += RESULT(ReachAmmoniumOutput, *Input);
		)
		return reachInput;
	)
	
	EQUATION(Model, ReachEffluentAmmonium,
		double effluentflow = IF_INPUT_ELSE_PARAMETER(EffluentTimeseries, EffluentFlow);
		double effluentammoniumconc = IF_INPUT_ELSE_PARAMETER(AmmoniumEffluentConcentrationTimeseries, ReachEffluentAmmoniumConcentration);
		
		double effluentammonium = effluentflow * effluentammoniumconc * 86.4;

		if(!PARAMETER(ReachHasEffluentInput)) effluentammonium = 0.0;
		return effluentammonium;
	)
	
	EQUATION(Model, ReachTotalAmmoniumInput,
		return RESULT(ReachUpstreamAmmonium) + RESULT(TotalDiffuseAmmoniumOutput) + RESULT(ReachEffluentAmmonium);
	)
	
	EQUATION(Model, ReachAmmoniumOutput,
		return RESULT(ReachAmmonium) * SafeDivide(RESULT(ReachFlow) * 86400.0, RESULT(ReachVolume));
	)
	
	EQUATION(Model, ReachAmmoniumAbstraction,
		return RESULT(ReachAmmonium) * SafeDivide(RESULT(ReachAbstraction) * 86400.0, RESULT(ReachVolume)); 
	)
	
	EQUATION(Model, ReachAmmoniumInitialValue,
		return PARAMETER(InitialStreamAmmoniumConcentration) * RESULT(ReachVolume) / 1000.0;
	)
	
	EQUATION(Model, ReachAmmonium,
		return
			  RESULT(ReachTotalAmmoniumInput)
			- RESULT(ReachAmmoniumOutput)
			- RESULT(ReachNitrification)
			- RESULT(ReachAmmoniumAbstraction);
	)
	
	auto ReachNitrateConcentration = RegisterEquation(Model, "Reach nitrate concentration", MgPerL);
	auto ReachAmmoniumConcentration = RegisterEquation(Model, "Reach ammonium concentration", MgPerL);
	
	EQUATION(Model, ReachNitrateConcentration,
		return SafeDivide(RESULT(ReachNitrate), RESULT(ReachVolume)) * 1000.0;
	)
	
	EQUATION(Model, ReachAmmoniumConcentration,
		return SafeDivide(RESULT(ReachAmmonium), RESULT(ReachVolume)) * 1000.0;
	)
	
	EndModule(Model);
}

#define INCAN_MODEL_H
#endif
