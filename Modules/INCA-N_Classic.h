

//NOTE: This model is currently not fully supported. There are a few errors in how it is coded, that may make it perform incorrectly.


#if !defined(INCAN_CLASSIC_MODEL_H)

static void
AddINCANClassicModel(mobius_model *Model)
{
	auto LandscapeUnits = RegisterIndexSet(Model, "Landscape units");
	auto Reach = RegisterIndexSetBranched(Model, "Reaches");
	
    auto Cumecs         = RegisterUnit(Model, "m3/s");
	auto JulianDay      = RegisterUnit(Model, "Julian day");
	auto Days           = RegisterUnit(Model, "days");
	auto MetresPerDay   = RegisterUnit(Model, "m/day");
	auto KgPerHectarePerDay = RegisterUnit(Model, "kg/Ha/day");
	auto Metres         = RegisterUnit(Model, "m");
	auto Millimetres    = RegisterUnit(Model, "mm");
	auto Seconds        = RegisterUnit(Model, "s");
	auto Hectares       = RegisterUnit(Model, "Ha");
	auto PerDay         = RegisterUnit(Model, "/day");
	auto MgPerL         = RegisterUnit(Model, "mg/l");
	auto DegreesCelsius = RegisterUnit(Model, "°C");
	auto Dimensionless  = RegisterUnit(Model, "dimensionless");
	auto SquareKm = RegisterUnit(Model, "km2");
	auto MmPerDay = RegisterUnit(Model, "mm/day");
	auto KgPerDay = RegisterUnit(Model, "kg/day");
	auto InverseMetresSquared = RegisterUnit(Model, "1/m2");
	auto DecimalDegrees = RegisterUnit(Model, "decimal degrees");
	auto MetresCubed = RegisterUnit(Model, "m3");
	auto KgPerKm2PerDay = RegisterUnit(Model, "kg/km2/day");
	
	auto Land = RegisterParameterGroup(Model, "Landscape units", LandscapeUnits);

	auto DirectRunoffInitialFlow                  = RegisterParameterDouble(Model, Land, "Direct runoff initial flow", Cumecs, 10.0);
	auto SoilWaterInitialFlow                     = RegisterParameterDouble(Model, Land, "Soil water initial flow",    Cumecs, 10.0);
	auto DirectRunoffInitialNitrateConcentration  = RegisterParameterDouble(Model, Land, "Direct runoff initial nitrate concentration", MgPerL, 10.0);
	auto DirectRunoffInitialAmmoniumConcentration = RegisterParameterDouble(Model, Land, "Direct runoff initial ammonium concentration", MgPerL, 10.0);
	auto SoilWaterInitialNitrateConcentration     = RegisterParameterDouble(Model, Land, "Soil water initial nitrate concentration",     MgPerL, 10.0);
	auto SoilWaterInitialAmmoniumConcentration    = RegisterParameterDouble(Model, Land, "Soil water initial ammonium concentration",     MgPerL, 10.0);
	auto SoilWaterDenitrificationRate             = RegisterParameterDouble(Model, Land, "Soil water denitrification rate", MetresPerDay, 20.0);
	auto NitrogenFixationRate                     = RegisterParameterDouble(Model, Land, "Nitrogen fixation rate", KgPerHectarePerDay, 20.0);
	auto NitratePlantUptakeRate                   = RegisterParameterDouble(Model, Land, "Nitrate plant uptake rate", MetresPerDay, 20.0);
	auto MaximumNitrogenUptakeRate                = RegisterParameterDouble(Model, Land, "Maximum nitrogen uptake rate", KgPerHectarePerDay, 20.0);
	auto FertilizerNitrateAdditionRate            = RegisterParameterDouble(Model, Land, "Fertilizer nitrate addition rate", KgPerHectarePerDay, 20.0);
	auto AmmoniumNitrificationRate                = RegisterParameterDouble(Model, Land, "Ammonium nitrification rate", MetresPerDay, 20.0);
	auto AmmoniumMineralisationRate               = RegisterParameterDouble(Model, Land, "Ammonium mineralisation rate", KgPerHectarePerDay, 20.0);
	auto AmmoniumImmobilisationRate               = RegisterParameterDouble(Model, Land, "Ammonium immobilisation rate", MetresPerDay, 20.0);
	auto FertilizerAmmoniumAdditionRate           = RegisterParameterDouble(Model, Land, "Fertilizer addition rate", KgPerHectarePerDay, 20.0);
	auto AmmoniumPlantUptakeRate                  = RegisterParameterDouble(Model, Land, "Ammonium plant uptake rate", MetresPerDay, 20.0);
	auto PlantGrowthStartDay                      = RegisterParameterUInt(Model, Land, "Plant growth start day", JulianDay, 20);
	auto PlantGrowthPeriod                        = RegisterParameterUInt(Model, Land, "Plant growth period",    Days, 20);
	auto FertilizerAdditionStartDay               = RegisterParameterUInt(Model, Land, "Fertilizer addition start day", JulianDay, 20);
	auto FertilizerAdditionPeriod                 = RegisterParameterUInt(Model, Land, "Fertilizer addition period", Days, 20);
	auto SoilMoistureDeficitMaximum               = RegisterParameterDouble(Model, Land, "Soil moisture deficit maximum", Millimetres, 20.0);
	auto MaximumTemperatureDifference             = RegisterParameterDouble(Model, Land, "Maximum temperature difference", DegreesCelsius, 20.0);
	auto DenitrificationTemperatureThreshold      = RegisterParameterDouble(Model, Land, "Denitrification temperature threshold", DegreesCelsius, 20.0);
	auto NitrificationTemperatureThreshold        = RegisterParameterDouble(Model, Land, "Nitrification temperature threshold", DegreesCelsius, 20.0);
	auto MineralisationTemperatureThreshold       = RegisterParameterDouble(Model, Land, "Mineralisation temperature threshold", DegreesCelsius, 20.0);
	auto ImmobilisationTemperatureThreshold       = RegisterParameterDouble(Model, Land, "Immobilisation temperature threshold", DegreesCelsius, 20.0);
	auto SoilWaterSustainableFlow                 = RegisterParameterDouble(Model, Land, "Soil water sustainable flow", Cumecs, 20.0);
	auto DirectRunoffSustainableFlow              = RegisterParameterDouble(Model, Land, "Direct runoff sustainable flow", Cumecs, 20.0);
	auto DenitrificationResponseToA10ChangeInTemperature = RegisterParameterDouble(Model, Land, "Denitrification response to a 10° change in temperature", Dimensionless, 20.0);
	auto DenitrificationBaseTemperatureResponse   = RegisterParameterDouble(Model, Land, "Denitrification base temperature response", DegreesCelsius, 20.0);
	auto FixationResponseToA10ChangeInTemperature = RegisterParameterDouble(Model, Land, "Fixation response to a 10° change in temperature", Dimensionless, 20.0);
	auto FixationBaseTemperatureResponse          = RegisterParameterDouble(Model, Land, "Fixation base temperature response", DegreesCelsius, 20.0);
	auto NitrificationResponseToA10ChangeInTemperature = RegisterParameterDouble(Model, Land, "Nitrification response to a 10° change in temperature", Dimensionless, 20.0);
	auto NitrificationBaseTemperatureResponse     = RegisterParameterDouble(Model, Land, "Nitrification base temperature response", DegreesCelsius, 20.0);
	auto MineralisationResponseToA10ChangeInTemperature = RegisterParameterDouble(Model, Land, "Mineralisation response to a 10° change in temperature", Dimensionless, 20.0);
	auto MineralisationBaseTemperatureResponse    = RegisterParameterDouble(Model, Land, "Mineralisation base temperature response", DegreesCelsius, 20.0);
	auto ImmobilisationResponseToA10ChangeInTemperature = RegisterParameterDouble(Model, Land, "Immobilisation response to a 10° change in temperature", Dimensionless, 20.0);
	auto ImmobilisationBaseTemperatureResponse    = RegisterParameterDouble(Model, Land, "Immobilisation base temperature response", DegreesCelsius, 20.0);
	auto NO3UptakeResponseToA10ChangeInTemperature = RegisterParameterDouble(Model, Land, "NO3 uptake response to a 10° change in temperature", Dimensionless, 20.0);
	auto NO3UptakeBaseTemperatureResponse         = RegisterParameterDouble(Model, Land, "NO3 uptake base temperature response", DegreesCelsius, 20.0);
	auto NH4UptakeResponseToA10ChangeInTemperature = RegisterParameterDouble(Model, Land, "NH4 uptake response to a 10° change in temperature", Dimensionless, 20.0);
	auto NH4UptakeBaseTemperatureResponse         = RegisterParameterDouble(Model, Land, "NH4 uptake base temperature response", DegreesCelsius, 20.0);
	auto GrowthCurveOffset                        = RegisterParameterDouble(Model, Land, "Growth curve offset", Dimensionless, 20.0);
	auto GrowthCurveAmplitude                     = RegisterParameterDouble(Model, Land, "Growth curve amplitude", Dimensionless, 20.0);
	auto DirectRunoffTimeConstant                 = RegisterParameterDouble(Model, Land, "Direct runoff time constant", Days, 1.0);
	auto SoilWaterTimeConstant                    = RegisterParameterDouble(Model, Land, "Soil water time constant", Days, 10.0);
	auto RatioOfTotalToAvailableWaterInSoil       = RegisterParameterDouble(Model, Land, "Ratio of total to available water in soil", Dimensionless, 10.0);

	auto Reaches = RegisterParameterGroup(Model, "Reaches", Reach);

	auto TerrestrialCatchmentArea = RegisterParameterDouble(Model, Reaches, "Terrestrial catchment area", SquareKm, 500.0);
	auto BaseFlowIndex            = RegisterParameterDouble(Model, Reaches, "Base flow index", Dimensionless, 0.9);
	auto ThresholdSoilZoneFlow    = RegisterParameterDouble(Model, Reaches, "Threshold soil zone flow", Cumecs, 0.9);
	auto RainfallExcessProportion = RegisterParameterDouble(Model, Reaches, "Rainfall excess proportion", Dimensionless, 0.9);
	auto MaximumInfiltrationRate  = RegisterParameterDouble(Model, Reaches, "Maximum infiltration rate", MmPerDay, 0.9);
	auto GroundWaterDenitrificationRate = RegisterParameterDouble(Model, Reaches, "Groundwater denitrification rate", MetresPerDay, 20.0);
	auto MaximumGroundwaterEffectiveDepth = RegisterParameterDouble(Model, Reaches, "Maximum groundwater effective depth", Metres, 0.9);
	auto ProportionOfFilledPoreSpace = RegisterParameterDouble(Model, Reaches, "Proportion of filled pore space", Dimensionless, 0.9);
	auto GroundWaterResidenceTime = RegisterParameterDouble(Model, Reaches, "Groundwater residence time", Days, 0.9);
	auto GroundWaterInitialFlow   = RegisterParameterDouble(Model, Reaches, "Groundwater initial flow", Cumecs, 0.9);
	auto GroundWaterInitialNitrateConcentration = RegisterParameterDouble(Model, Reaches, "Groundwater initial nitrate concentration", MgPerL, 10.0 );
	auto GroundWaterInitialAmmoniumConcentration = RegisterParameterDouble(Model, Reaches, "Groundwater initial ammonium concentration", MgPerL, 0.0);
	auto GroundWaterSustainableFlow = RegisterParameterDouble(Model, Reaches, "Groundwater sustainable flow", Cumecs, 0.0);
	auto NitrateDryDeposition     = RegisterParameterDouble(Model, Reaches, "Nitrate dry deposition", KgPerHectarePerDay, 20.0);
	auto NitrateWetDeposition     = RegisterParameterDouble(Model, Reaches, "Nitrate wet deposition", KgPerHectarePerDay, 20.0);
	auto AmmoniumDryDeposition    = RegisterParameterDouble(Model, Reaches, "Ammonium dry deposition", KgPerHectarePerDay, 20.0);
	auto AmmoniumWetDeposition    = RegisterParameterDouble(Model, Reaches, "Ammonium wet deposition", KgPerHectarePerDay, 20.0);

	auto ReachLength              = RegisterParameterDouble(Model, Reaches, "Reach length", Metres, 1000.0);
	auto A                        = RegisterParameterDouble(Model, Reaches, "a", InverseMetresSquared, 0.04);
	auto B                        = RegisterParameterDouble(Model, Reaches, "b", Dimensionless, 0.67);
	auto ReachDenitrificationRate = RegisterParameterDouble(Model, Reaches, "Reach denitrification rate", PerDay, 20.0);
	auto ReachNitrificationRate   = RegisterParameterDouble(Model, Reaches, "Reach nitrification rate", PerDay, 20.0);
	auto ReachEffluentNitrateConcentration = RegisterParameterDouble(Model, Reaches, "Reach effluent nitrate concentration", MgPerL, 0.0);
	auto ReachEffluentAmmoniumConcentration = RegisterParameterDouble(Model, Reaches, "Reach effluent ammonium concentration", MgPerL, 0.0);
	auto EffluentFlow             = RegisterParameterDouble(Model, Reaches, "Effluent flow", Cumecs, 0.0);
	auto AbstractionFlow          = RegisterParameterDouble(Model, Reaches, "Abstraction flow", Cumecs, 0.0);
	auto Latitude                 = RegisterParameterDouble(Model, Reaches, "Latitude", DecimalDegrees, 0.0);
	auto Longitude                = RegisterParameterDouble(Model, Reaches, "Longitude", DecimalDegrees, 0.0);
	auto ReachHasAbstraction      = RegisterParameterBool(Model, Reaches, "Reach has abstraction", false);
	auto ReachHasEffluentInput    = RegisterParameterBool(Model, Reaches, "Reach has effluent input", false);

	auto LandUsePercentages = RegisterParameterGroup(Model, "Landscape percentages", LandscapeUnits);
	SetParentGroup(Model, LandUsePercentages, Reaches);
	
	auto PercentU = RegisterUnit(Model, "%");
	
    auto Percent = RegisterParameterDouble(Model, LandUsePercentages, "%", PercentU, 25.0);
	
	//NOTE: These should have been added to streams, but have to be added to reaches for now:
	auto InitialStreamFlow = RegisterParameterDouble(Model, Reaches, "Initial stream flow", Cumecs, 10.0);
	auto InitialStreamNitrateConcentration = RegisterParameterDouble(Model, Reaches, "Initial stream nitrate concentration", MgPerL, 10.0);
	auto InitialStreamAmmoniumConcentration = RegisterParameterDouble(Model, Reaches, "Initial stream ammonium concentration", MgPerL, 10.0);



	auto IncaLandSolver = RegisterSolver(Model, "Terrestrial ODE solver", 0.1, IncaDascru);
	
	auto HerInput            = RegisterInput(Model, "HER");
	auto AirTemperature      = RegisterInput(Model, "Air temperature");
	auto ActualPrecipitation = RegisterInput(Model, "Actual precipitation");
	auto SoilMoistureDeficit = RegisterInput(Model, "Soil moisture deficit");
	
	auto SoilTemperature = GetEquationHandle(Model, "Soil temperature");

	auto Her = RegisterEquation(Model, "HER", Cumecs);
	SetInitialValue(Model, Her, 0.042824074074074077);
	auto InfiltrationExcess = RegisterEquation(Model, "Infiltration excess", Millimetres);
	SetInitialValue(Model, InfiltrationExcess, 0.0125644627);
	auto SoilWaterRetentionVolume = RegisterEquation(Model, "Soil water retention volume", Cumecs);
	auto InitialDirectRunoffVolume = RegisterEquationInitialValue(Model, "Initial direct runoff volume", MetresCubed);
	
	EQUATION(Model, Her,
		CURRENT_INDEX(LandscapeUnits); //NOTE: To force a dependency. Should not really be necessary, though...
		return INPUT(HerInput) * 1000.0 / 86400.0;
	)

	EQUATION(Model, InfiltrationExcess,
		double infiltration = 0.0;
		double rain = INPUT(HerInput);
		double liquidPrecipitation = 0.0;
		
		if(INPUT(AirTemperature) > 0.0)
		{
			liquidPrecipitation = INPUT(ActualPrecipitation); // NOTE: This isn't even used. Is there a mistake in the implementation here?
		}
		
		double maxInfiltrationRate = PARAMETER(MaximumInfiltrationRate);
		
		if(RESULT(SoilTemperature) > 0.1)
		{
			infiltration = (maxInfiltrationRate / 86.4 )
							* (1.0 - std::exp( -( ( rain / 86.4) / ( maxInfiltrationRate / 86.4 ) ) ));
		}
		
		return (rain / 86.4) - infiltration;
	)


	EQUATION(Model, SoilWaterRetentionVolume,
		return PARAMETER(RatioOfTotalToAvailableWaterInSoil)
			* (PARAMETER(SoilMoistureDeficitMaximum) - INPUT(SoilMoistureDeficit))
			* 1000.0;
	)
	
	//NOTE: Hmm, doesn't seem to be used either...
	EQUATION(Model, InitialDirectRunoffVolume,
		return PARAMETER(DirectRunoffInitialFlow) * PARAMETER(DirectRunoffTimeConstant) * 86400.0;
	)

	auto SoilWaterFlow                  = RegisterEquationODE(Model, "Soil water flow", Cumecs);
	SetInitialValue(Model, SoilWaterFlow, SoilWaterInitialFlow);
	SetSolver(Model, SoilWaterFlow, IncaLandSolver);
	auto InitialSoilWaterDrainageVolume = RegisterEquationInitialValue(Model, "Initial soil water drainage volume", MetresCubed);
	auto SoilWaterDrainageVolume        = RegisterEquationODE(Model, "Soil water drainage volume", MetresCubed);
	SetInitialValue(Model, SoilWaterDrainageVolume, InitialSoilWaterDrainageVolume);
	SetSolver(Model, SoilWaterDrainageVolume, IncaLandSolver);
	auto SoilWaterVolume                = RegisterEquation(Model, "Soil water volume", MetresCubed);
	SetSolver(Model, SoilWaterVolume, IncaLandSolver);
	auto InitialGroundWaterVolume = RegisterEquationInitialValue(Model, "Initial groundwater volume", MetresCubed);
	auto GroundWaterFlow                = RegisterEquationODE(Model, "Groundwater flow", Cumecs);
	SetInitialValue(Model, GroundWaterFlow, GroundWaterInitialFlow);
	SetSolver(Model, GroundWaterFlow, IncaLandSolver);
	auto GroundWaterVolume              = RegisterEquationODE(Model, "Groundwater volume", MetresCubed);
	SetInitialValue(Model, GroundWaterVolume, InitialGroundWaterVolume);
	SetSolver(Model, GroundWaterVolume, IncaLandSolver);
	auto DirectRunoffFlow               = RegisterEquationODE(Model, "Direct runoff flow", Cumecs);
	SetInitialValue(Model, DirectRunoffFlow, DirectRunoffInitialFlow);
	SetSolver(Model, DirectRunoffFlow, IncaLandSolver);
	auto DirectRunoffVolume             = RegisterEquationODE(Model, "Direct runoff volume", MetresCubed);
	SetInitialValue(Model, DirectRunoffVolume, InitialDirectRunoffVolume);
	SetSolver(Model, DirectRunoffVolume, IncaLandSolver);
	
	EQUATION(Model, SoilWaterFlow,
		double her = INPUT(HerInput) * 1000.0 / 86400.0;
		//std::cout << INPUT(HerInput) << std::endl;
		double flow = (her - RESULT(SoilWaterFlow) ) / PARAMETER(SoilWaterTimeConstant);
		return flow;
	)

	EQUATION(Model, InitialSoilWaterDrainageVolume,
		return PARAMETER(SoilWaterInitialFlow) * PARAMETER(SoilWaterTimeConstant) * 86400.0;
	)

	EQUATION(Model, SoilWaterDrainageVolume,
		double her = INPUT(HerInput) * 1000.0 / 86400.0;
		double volume = (her - RESULT(SoilWaterFlow) ) * 86400.0;
		return volume;
	)
	
	EQUATION(Model, SoilWaterVolume,
		return RESULT(SoilWaterDrainageVolume) + RESULT(SoilWaterRetentionVolume);
	)
	
	EQUATION(Model, InitialGroundWaterVolume,
		return PARAMETER(MaximumGroundwaterEffectiveDepth) * PARAMETER(ProportionOfFilledPoreSpace) * 1000000.0;
	)

    EQUATION(Model, GroundWaterFlow,
		if( RESULT(SoilWaterFlow) > PARAMETER(ThresholdSoilZoneFlow) )
		{
			return ( PARAMETER(BaseFlowIndex)
					* ( RESULT(SoilWaterFlow) - (RESULT(SoilWaterFlow) - PARAMETER(ThresholdSoilZoneFlow) ) ) - RESULT(GroundWaterFlow) )
						/ PARAMETER(GroundWaterResidenceTime);
		}
		
		return ( PARAMETER(BaseFlowIndex) * RESULT(SoilWaterFlow) - RESULT(GroundWaterFlow) ) / PARAMETER(GroundWaterResidenceTime);
	)
	
	EQUATION(Model, GroundWaterVolume,
		if( RESULT(SoilWaterFlow) > PARAMETER(ThresholdSoilZoneFlow) )
		{
			return (PARAMETER(BaseFlowIndex) * PARAMETER(ThresholdSoilZoneFlow) - RESULT(GroundWaterFlow) ) * 86400.0;
		}
		
		return (PARAMETER(BaseFlowIndex) * RESULT(SoilWaterFlow) - RESULT(GroundWaterFlow) ) * 86400.0;
	)

	EQUATION(Model, DirectRunoffFlow,
		double infiltration = 0.0;
		
		if(RESULT(SoilTemperature) > 0.1 )
		{
			infiltration = (PARAMETER(MaximumInfiltrationRate) / 86.4 )
							* (1.0 - std::exp( -( ( INPUT(HerInput) / 86.4 ) / (PARAMETER(MaximumInfiltrationRate) / 86.4)))); //Didn't we do this calculation already?
		}
		double ie = INPUT(HerInput) / 86.4 - infiltration;
		double dr = PARAMETER(RainfallExcessProportion) * ie - RESULT(DirectRunoffFlow);
		double se = RESULT(SoilWaterFlow) - PARAMETER(ThresholdSoilZoneFlow);
		if(RESULT(SoilWaterFlow) > PARAMETER(ThresholdSoilZoneFlow))
		{
			dr += se;
		}
		return dr / PARAMETER(DirectRunoffTimeConstant);
	)

	EQUATION(Model, DirectRunoffVolume,
		double infiltration = 0.0;
		
		if(RESULT(SoilTemperature) > 0.1 )
		{
			infiltration = (PARAMETER(MaximumInfiltrationRate) / 86.4 )
							* (1.0 - std::exp( -( ( INPUT(HerInput) / 86.4 ) / (PARAMETER(MaximumInfiltrationRate) / 86.4)))); //Didn't we do this calculation already?
		}
		double ie = INPUT(HerInput) / 86.4 - infiltration;
		double dr = PARAMETER(RainfallExcessProportion) * ie - RESULT(DirectRunoffFlow);
		double se = RESULT(SoilWaterFlow) - PARAMETER(ThresholdSoilZoneFlow);
		
		if(RESULT(SoilWaterFlow) > PARAMETER(ThresholdSoilZoneFlow))
		{
			dr += se;
		}
		
		return dr * 86400.0;
	)

	auto DirectRunoffNitrateOutput = RegisterEquation(Model, "Direct runoff nitrate output", KgPerKm2PerDay);
	SetSolver(Model, DirectRunoffNitrateOutput, IncaLandSolver);
	auto InitialDirectRunoffNitrate = RegisterEquationInitialValue(Model, "Initial direct runoff nitrate", KgPerKm2PerDay);
	auto DirectRunoffNitrate = RegisterEquationODE(Model, "Direct runoff nitrate", KgPerKm2PerDay);
	SetInitialValue(Model, DirectRunoffNitrate, InitialDirectRunoffNitrate);
	SetSolver(Model, DirectRunoffNitrate, IncaLandSolver);
	auto DirectRunoffAmmoniumOutput = RegisterEquation(Model, "Direct runoff ammonium output", KgPerKm2PerDay);
	SetSolver(Model, DirectRunoffAmmoniumOutput, IncaLandSolver);
	auto InitialDirectRunoffAmmonium = RegisterEquationInitialValue(Model, "Initial direct runoff ammonium", KgPerKm2PerDay);
	auto DirectRunoffAmmonium = RegisterEquationODE(Model, "Direct runoff ammonium", KgPerKm2PerDay);
	SetInitialValue(Model, DirectRunoffAmmonium, InitialDirectRunoffAmmonium);
	SetSolver(Model, DirectRunoffAmmonium, IncaLandSolver);
	auto DrynessFactor        = RegisterEquation(Model, "Dryness factor",         Dimensionless);
	auto SeasonalGrowthFactor = RegisterEquation(Model, "Seasonal growth factor", Dimensionless);
	auto TemperatureFactor    = RegisterEquation(Model, "Temperature factor",     Dimensionless);
	auto MaximumNitrogenUptake = RegisterEquation(Model, "Maximum nitrogen uptake", KgPerKm2PerDay);
	auto NitrateUptake         = RegisterEquation(Model, "Nitrate uptake",          KgPerKm2PerDay);
	SetSolver(Model, NitrateUptake, IncaLandSolver);
	auto Denitrification       = RegisterEquation(Model, "Denitrification",         KgPerKm2PerDay);
	SetSolver(Model, Denitrification, IncaLandSolver);
	auto Nitrification         = RegisterEquation(Model, "Nitrification",           KgPerKm2PerDay);
	SetSolver(Model, Nitrification, IncaLandSolver);
	auto Fixation              = RegisterEquation(Model, "Fixation",                KgPerKm2PerDay);
	SetSolver(Model, Fixation, IncaLandSolver);
	auto SoilWaterNitrateOutput = RegisterEquation(Model, "Soil water nitrate output", KgPerKm2PerDay);
	SetSolver(Model, SoilWaterNitrateOutput, IncaLandSolver);
	auto SoilWaterNitrateInput  = RegisterEquation(Model, "Soil water nitrate input", KgPerKm2PerDay);
	auto InitialSoilWaterNitrate = RegisterEquationInitialValue(Model, "Initial soil water nitrate", KgPerKm2PerDay);
	auto SoilWaterNitrate        = RegisterEquationODE(Model, "Soil water nitrate", KgPerKm2PerDay);
	SetInitialValue(Model, SoilWaterNitrate, InitialSoilWaterNitrate);
	SetSolver(Model, SoilWaterNitrate, IncaLandSolver);
	auto AmmoniumUptake = RegisterEquation(Model, "Ammonium uptake", KgPerKm2PerDay);
	SetSolver(Model, AmmoniumUptake, IncaLandSolver);
	auto Immobilisation = RegisterEquation(Model, "Immobilisation", KgPerKm2PerDay);
	SetSolver(Model, Immobilisation, IncaLandSolver);
	auto Mineralisation = RegisterEquation(Model, "Mineralisation", KgPerKm2PerDay);
	SetSolver(Model, Mineralisation, IncaLandSolver);
	auto SoilWaterAmmoniumOutput = RegisterEquation(Model, "Soil water ammonium output", KgPerKm2PerDay);
	SetSolver(Model, SoilWaterAmmoniumOutput, IncaLandSolver);
	auto SoilWaterAmmoniumInput = RegisterEquation(Model, "Soil water ammonium input", KgPerKm2PerDay);
	auto InitialSoilWaterAmmonium = RegisterEquationInitialValue(Model, "Initial soil water ammonium", KgPerKm2PerDay);
	auto SoilWaterAmmonium        = RegisterEquationODE(Model, "Soil water ammonium", KgPerKm2PerDay);
	SetInitialValue(Model, SoilWaterAmmonium, InitialSoilWaterAmmonium);
	SetSolver(Model, SoilWaterAmmonium, IncaLandSolver);
	
	EQUATION(Model, DirectRunoffNitrateOutput,
		if(RESULT(DirectRunoffVolume) > 0.0)
		{
			return RESULT(DirectRunoffNitrate) * RESULT(DirectRunoffFlow) * 86400.0 / RESULT(DirectRunoffVolume);
		}
		return 0.0;
	)

	EQUATION(Model, InitialDirectRunoffNitrate,
		return PARAMETER(DirectRunoffInitialNitrateConcentration) * RESULT(DirectRunoffVolume) / 1000.0;
	)
	
	EQUATION(Model, DirectRunoffNitrate,
		if( RESULT(SoilWaterFlow) > PARAMETER(ThresholdSoilZoneFlow) )
		{
			return ( ( RESULT(SoilWaterFlow) - PARAMETER(ThresholdSoilZoneFlow) ) * RESULT(SoilWaterNitrate) * 86400.0) / RESULT(SoilWaterVolume)
				- RESULT(DirectRunoffNitrateOutput);
		}
		return -RESULT(DirectRunoffNitrateOutput);
	)
	
	EQUATION(Model, DirectRunoffAmmoniumOutput,
		return RESULT(DirectRunoffAmmonium) * RESULT(DirectRunoffFlow) * 86400.0 / RESULT(DirectRunoffVolume);
	)

	EQUATION(Model, InitialDirectRunoffAmmonium,
		return (PARAMETER(DirectRunoffInitialAmmoniumConcentration) * RESULT(DirectRunoffVolume) ) / 1000.0;
	)

	EQUATION(Model, DirectRunoffAmmonium,
		if(RESULT(SoilWaterFlow) > PARAMETER(ThresholdSoilZoneFlow))
		{
			return (( RESULT(SoilWaterFlow) - PARAMETER(ThresholdSoilZoneFlow) ) * RESULT(SoilWaterAmmonium) * 86400.0 ) / RESULT(SoilWaterVolume)
				- RESULT(DirectRunoffAmmoniumOutput);
		}
		return -RESULT(DirectRunoffAmmoniumOutput);
	)
	
	EQUATION(Model, DrynessFactor,
		if( INPUT(SoilMoistureDeficit) >= PARAMETER(SoilMoistureDeficitMaximum) ) return 0.0;
		return ( PARAMETER(SoilMoistureDeficitMaximum) - INPUT(SoilMoistureDeficit)) / PARAMETER(SoilMoistureDeficitMaximum);
	)

	EQUATION(Model, SeasonalGrowthFactor,
		return PARAMETER(GrowthCurveOffset) + PARAMETER(GrowthCurveAmplitude) 
			* sin(2.0 * Pi * ((double)CURRENT_DAY_OF_YEAR() - (double)PARAMETER(PlantGrowthStartDay) ) / (double)DAYS_THIS_YEAR() );
	)

	EQUATION(Model, TemperatureFactor,
		return pow(1.047, ( RESULT(SoilTemperature) - 20.0) );
	)
	
	EQUATION(Model, MaximumNitrogenUptake,
		double dummy = LAST_RESULT(NitrateUptake) + LAST_RESULT(AmmoniumUptake); //To mark a dependency.
		return dummy;
		//return RESULT(NitrateUptake) + RESULT(AmmoniumUptake); //NOTE: This creates a circular dependency...
	)
	
	EQUATION(Model, NitrateUptake,
		if(RESULT(MaximumNitrogenUptake) > PARAMETER(MaximumNitrogenUptakeRate) ) return 0.0;
		
		double plantGrowthStartDay = (double)PARAMETER(PlantGrowthStartDay);
		double plantGrowthEndDay = plantGrowthStartDay + (double)PARAMETER(PlantGrowthPeriod);
		
		if((double)CURRENT_DAY_OF_YEAR() < plantGrowthStartDay || (double)CURRENT_DAY_OF_YEAR() > plantGrowthEndDay) return 0.0;
		
		return PARAMETER(NitratePlantUptakeRate)
			* RESULT(TemperatureFactor)
			* RESULT(SoilWaterNitrate)
			* RESULT(DrynessFactor)
			* RESULT(SeasonalGrowthFactor)
			/ RESULT(SoilWaterVolume)
			* 1000000.0;
	)
	
	EQUATION(Model, Denitrification,
		return PARAMETER(SoilWaterDenitrificationRate)
			* RESULT(TemperatureFactor)
			* RESULT(SoilWaterNitrate)
			* RESULT(DrynessFactor)
			/ RESULT(SoilWaterVolume)
			* 1000000.0;
	)
	
	EQUATION(Model, Nitrification,
		return PARAMETER(AmmoniumNitrificationRate)
			* RESULT(TemperatureFactor)
			* RESULT(SoilWaterAmmonium)
			* RESULT(DrynessFactor)
			/ RESULT(SoilWaterVolume)
			* 1000000.0;
	)
    
	EQUATION(Model, Fixation,
		return PARAMETER(NitrogenFixationRate) * RESULT(TemperatureFactor) * 100.0;
	)

	EQUATION(Model, SoilWaterNitrateOutput,
		double par = PARAMETER(RatioOfTotalToAvailableWaterInSoil); //NOTE: Hmm: is this just to force a dependency???
		return RESULT(SoilWaterNitrate) * RESULT(SoilWaterFlow) * 86400.0 / RESULT(SoilWaterVolume);
	)

    EQUATION(Model, SoilWaterNitrateInput,
		double nitrateInput = 0.0;
		double startDay = (double)PARAMETER(FertilizerAdditionStartDay);
		double endDay   = startDay + (double)PARAMETER(FertilizerAdditionPeriod);
		
		if((double)CURRENT_DAY_OF_YEAR() >= startDay && (double)CURRENT_DAY_OF_YEAR() <= endDay)
		{
			nitrateInput += PARAMETER(FertilizerNitrateAdditionRate);
		}
		
		nitrateInput += PARAMETER(NitrateDryDeposition);
		nitrateInput += PARAMETER(NitrateWetDeposition);
		
		return nitrateInput * 100.0;
	)
	
	EQUATION(Model, InitialSoilWaterNitrate,
		return PARAMETER(SoilWaterInitialNitrateConcentration) * RESULT(SoilWaterVolume) / 1000.0;
	)

	EQUATION(Model, SoilWaterNitrate,
		//if(RESULT(SoilWaterNitrate) < 0.0) return 0.0; //TODO: Should not be necessary.
		//double swn = RESULT(SoilWaterNitrate);
		//if(swn < 0 || std::abs(swn) > 1e10) std::cout << RESULT(SoilWaterNitrate) << std::endl;
		return RESULT(SoilWaterNitrateInput)
			- RESULT(SoilWaterNitrateOutput)
			- RESULT(NitrateUptake)
			- RESULT(Denitrification)
			+ RESULT(Nitrification)
			+ RESULT(Fixation);
	)
	
	EQUATION(Model, AmmoniumUptake,
		if( RESULT(MaximumNitrogenUptake) > PARAMETER(MaximumNitrogenUptakeRate) ) return 0.0;
		
		return PARAMETER(AmmoniumPlantUptakeRate)
			* RESULT(TemperatureFactor)
			* RESULT(SoilWaterAmmonium)
			* RESULT(DrynessFactor)
			* RESULT(SeasonalGrowthFactor)
			/ RESULT(SoilWaterVolume)
			* 1000000.0;
	)
	
	EQUATION(Model, Immobilisation,
		return PARAMETER(AmmoniumImmobilisationRate)
			* RESULT(TemperatureFactor)
			* RESULT(SoilWaterAmmonium)
			* RESULT(DrynessFactor)
			/ RESULT(SoilWaterVolume)
			* 1000000.0;			
	)

	EQUATION(Model, Mineralisation,
		return PARAMETER(AmmoniumMineralisationRate)
			* RESULT(TemperatureFactor)
			* RESULT(DrynessFactor)
			* 100.0;
	)
	
	EQUATION(Model, SoilWaterAmmoniumOutput,
		return RESULT(SoilWaterAmmonium) * RESULT(SoilWaterFlow) * 86400.0 / RESULT(SoilWaterVolume);
	)
    
	EQUATION(Model, SoilWaterAmmoniumInput,
		double nitrateInput = 0.0;
		double startDay = (double)PARAMETER(FertilizerAdditionStartDay);
		double endDay   = startDay + (double)PARAMETER(FertilizerAdditionPeriod);
		
		if((double)CURRENT_DAY_OF_YEAR() >= startDay && (double)CURRENT_DAY_OF_YEAR() <= endDay)
		{
			nitrateInput += PARAMETER(FertilizerAmmoniumAdditionRate);
		}
		
		nitrateInput += PARAMETER(AmmoniumDryDeposition);
		nitrateInput += PARAMETER(AmmoniumWetDeposition);
		
		return nitrateInput * 100.0;
	)
	
	EQUATION(Model, InitialSoilWaterAmmonium,
		return PARAMETER(SoilWaterInitialAmmoniumConcentration) * RESULT(SoilWaterVolume) / 1000.0;
	)

	EQUATION(Model, SoilWaterAmmonium,
		//if(RESULT(SoilWaterAmmonium) < 0.0) return 0.0; //TODO: Should not be necessary.
		return RESULT(SoilWaterAmmoniumInput)
			- RESULT(SoilWaterAmmoniumOutput)
			- RESULT(AmmoniumUptake)
			- RESULT(Nitrification)
			- RESULT(Immobilisation)
			+ RESULT(Mineralisation);
	)

	auto GroundWaterDenitrification = RegisterEquation(Model, "Groundwater denitrification", KgPerKm2PerDay);
	SetSolver(Model, GroundWaterDenitrification, IncaLandSolver);
	auto GroundWaterNitrateOutput   = RegisterEquation(Model, "Groundwater nitrate output", KgPerKm2PerDay);
	SetSolver(Model, GroundWaterNitrateOutput, IncaLandSolver);
	auto GroundWaterNitrate = RegisterEquationODE(Model, "Groundwater nitrate", KgPerKm2PerDay);
	SetSolver(Model, GroundWaterNitrate, IncaLandSolver);
	auto GroundWaterAmmoniumOutput = RegisterEquation(Model, "Groundwater ammonium output", KgPerKm2PerDay);
	SetSolver(Model, GroundWaterAmmoniumOutput, IncaLandSolver);
	auto GroundWaterAmmonium = RegisterEquationODE(Model, "Groundwater ammonium", KgPerKm2PerDay);
	SetSolver(Model, GroundWaterAmmonium, IncaLandSolver);
	
	EQUATION(Model, GroundWaterDenitrification,
		//std::cout << RESULT(GroundWaterNitrate) << std::endl;
		return PARAMETER(GroundWaterDenitrificationRate)
			* RESULT(GroundWaterNitrate)
			* RESULT(TemperatureFactor)
			/ RESULT(GroundWaterVolume)
			* 1000000.0;
	)

	EQUATION(Model, GroundWaterNitrateOutput,
		return RESULT(GroundWaterNitrate) * RESULT(GroundWaterFlow) * 86400.0 / RESULT(GroundWaterVolume);
	)

	EQUATION(Model, GroundWaterNitrate,
		if(RESULT(SoilWaterFlow) > PARAMETER(ThresholdSoilZoneFlow))
		{
			return (PARAMETER(BaseFlowIndex)
					* (RESULT(SoilWaterFlow) - (RESULT(SoilWaterFlow) - PARAMETER(ThresholdSoilZoneFlow) ) ) // NOTE: Hmm, this is nonsensical...
					* RESULT(SoilWaterNitrate) * 86400.0 ) / RESULT(SoilWaterVolume)
					- RESULT(GroundWaterNitrateOutput)
					- RESULT(GroundWaterDenitrification);
		}
		
		return PARAMETER(BaseFlowIndex) * RESULT(SoilWaterNitrateOutput)
			- RESULT(GroundWaterNitrateOutput)
			- RESULT(GroundWaterDenitrification);
	)
	
	EQUATION(Model, GroundWaterAmmoniumOutput,
		return RESULT(GroundWaterAmmonium) * RESULT(GroundWaterFlow) * 86400.0 / RESULT(GroundWaterVolume);
	)

	EQUATION(Model, GroundWaterAmmonium,
		if( RESULT(SoilWaterFlow) > PARAMETER(ThresholdSoilZoneFlow) )
		{
			return ( PARAMETER(BaseFlowIndex)
				* (RESULT(SoilWaterFlow) - ( RESULT(SoilWaterFlow) - PARAMETER(ThresholdSoilZoneFlow) ) )
				* RESULT(SoilWaterAmmoniumOutput) * 86400.0 ) / RESULT(SoilWaterVolume)
				- RESULT(GroundWaterAmmoniumOutput);
		}
		return PARAMETER(BaseFlowIndex) * RESULT(SoilWaterAmmoniumOutput) - RESULT(GroundWaterAmmoniumOutput);
	)
	
	auto DiffuseWater = RegisterEquation(Model, "Diffuse water", Cumecs);
	//SetSolver(Model, DiffuseWater, IncaLandSolver); //NOTE: This is not the case in the original. We do this to not get impossible dependencies
	auto TotalDiffuseWaterOutput = RegisterEquationCumulative(Model, "Total diffuse water output", DiffuseWater, LandscapeUnits);
	auto TotalNitrateToStream = RegisterEquation(Model, "Total nitrate to stream", KgPerKm2PerDay);
	SetSolver(Model, TotalNitrateToStream, IncaLandSolver);
	auto DiffuseNitrate = RegisterEquation(Model, "Diffuse nitrate", KgPerDay);
	auto TotalDiffuseNitrateOutput = RegisterEquationCumulative(Model, "Total diffuse nitrate output", DiffuseNitrate, LandscapeUnits);
	auto TotalAmmoniumToStream = RegisterEquation(Model, "Total ammonium to stream", KgPerKm2PerDay);
	SetSolver(Model, TotalAmmoniumToStream, IncaLandSolver);
	auto DiffuseAmmonium = RegisterEquation(Model, "Diffuse ammonium", KgPerDay);
	auto TotalDiffuseAmmoniumOutput = RegisterEquationCumulative(Model, "Total diffuse ammonium output", DiffuseAmmonium, LandscapeUnits);
	
	EQUATION(Model, DiffuseWater,
		double soilWaterFlow = (1.0 - PARAMETER(BaseFlowIndex) ) * RESULT(SoilWaterFlow);
		if(RESULT(SoilWaterFlow) > PARAMETER(ThresholdSoilZoneFlow) )
		{
			soilWaterFlow = (1.0 - PARAMETER(BaseFlowIndex) ) * PARAMETER(ThresholdSoilZoneFlow);
		}
		return (soilWaterFlow + RESULT(GroundWaterFlow) + RESULT(DirectRunoffFlow) )
			* PARAMETER(TerrestrialCatchmentArea) * (PARAMETER(Percent) / 100.0);
	)

    EQUATION(Model, TotalNitrateToStream,
		double soilWaterFlow = (1.0 - PARAMETER(BaseFlowIndex) ) * RESULT(SoilWaterFlow);
		if(RESULT(SoilWaterFlow) > PARAMETER(ThresholdSoilZoneFlow) )
		{
			soilWaterFlow = (1.0 - PARAMETER(BaseFlowIndex) ) * PARAMETER(ThresholdSoilZoneFlow);
		}
		double nitrate = ( soilWaterFlow * RESULT(SoilWaterNitrate) * 86400.0 ) / RESULT(SoilWaterVolume) + RESULT(GroundWaterNitrateOutput);
		
		double directRunoffNitrateOutput = RESULT(DirectRunoffNitrateOutput);
		if(RESULT(DirectRunoffVolume) > 0.0)
		{
			nitrate += directRunoffNitrateOutput;
		}
		
		return nitrate;
	)
	
	EQUATION(Model, DiffuseNitrate,
		return RESULT(TotalNitrateToStream) * PARAMETER(TerrestrialCatchmentArea) * PARAMETER(Percent) / 100.0;
	)
	
	EQUATION(Model, TotalAmmoniumToStream,
		double soilWaterFlow = (1.0 - PARAMETER(BaseFlowIndex) ) * RESULT(SoilWaterFlow);
		if(RESULT(SoilWaterFlow) > PARAMETER(ThresholdSoilZoneFlow) )
		{
			soilWaterFlow = (1.0 - PARAMETER(BaseFlowIndex) ) * PARAMETER(ThresholdSoilZoneFlow);
		}
		double ammonium = ( soilWaterFlow * RESULT(SoilWaterAmmonium) * 86400.0 ) / RESULT(SoilWaterVolume) + RESULT(GroundWaterAmmoniumOutput);
		
		if(RESULT(DirectRunoffVolume) > 0.0)
		{
			ammonium += RESULT(DirectRunoffAmmoniumOutput);
		}
		
		return ammonium;
	)

	EQUATION(Model, DiffuseAmmonium,
		return RESULT(TotalAmmoniumToStream) * PARAMETER(TerrestrialCatchmentArea) * PARAMETER(Percent) / 100.0;
	)

 
	auto IncaSolver = RegisterSolver(Model, "Inca solver", 0.1, IncaDascru);

    auto ConvertMassToConcentration = RegisterEquation(Model, "Convert mass to concentration", Dimensionless);
	SetSolver(Model, ConvertMassToConcentration, IncaSolver);
	auto ConvertConcentrationToMass = RegisterEquation(Model, "Convert concentration to mass", Dimensionless);
	SetSolver(Model, ConvertConcentrationToMass, IncaSolver);
	auto ReachFlowInput = RegisterEquation(Model, "Reach flow input", Cumecs);
	auto InitialReachTimeConstant = RegisterEquationInitialValue(Model, "Initial reach time constant", Days);
	auto ReachTimeConstant = RegisterEquation(Model, "Reach time constant", Days);
	SetInitialValue(Model, ReachTimeConstant, InitialReachTimeConstant);
	SetSolver(Model, ReachTimeConstant, IncaSolver);
	auto InitialReachFlow = RegisterEquationInitialValue(Model, "Initial reach flow", Cumecs);
	auto ReachFlow = RegisterEquationODE(Model, "Reach flow", Cumecs);
	SetInitialValue(Model, ReachFlow, InitialReachFlow);
	SetSolver(Model, ReachFlow, IncaSolver);
	auto InitialReachVolume = RegisterEquationInitialValue(Model, "Initial reach volume", MetresCubed);
	auto ReachVolume = RegisterEquationODE(Model, "Reach volume", MetresCubed);
	SetInitialValue(Model, ReachVolume, InitialReachVolume);
	SetSolver(Model, ReachVolume, IncaSolver);
	
	EQUATION(Model, ConvertMassToConcentration,
		return 1000.0 / RESULT(ReachVolume);
	)
	
	EQUATION(Model, ConvertConcentrationToMass,
		return 1.0 / (1000.0 / RESULT(ReachVolume));
	)

	EQUATION(Model, ReachFlowInput,
		double reachInput = RESULT(TotalDiffuseWaterOutput);
		double effluentInput = PARAMETER(EffluentFlow);
		
		FOREACH_INPUT(Reach,
			reachInput += RESULT(ReachFlow, *Input);
		)
		
		if(PARAMETER(ReachHasEffluentInput))
		{
			reachInput += effluentInput;
		}
		return reachInput;
	)

	EQUATION(Model, InitialReachTimeConstant,
		return PARAMETER(ReachLength) / (PARAMETER(A) * pow(RESULT(ReachFlow), PARAMETER(B)) * 86400.0); 
	)

    EQUATION(Model, ReachTimeConstant,
		double tc = RESULT(ReachVolume) / (RESULT(ReachFlow) * 86400.0);
		
		if(RESULT(ReachFlow) > 0.0 && RESULT(ReachVolume) > 0.0)
		{
			return tc;
		}
		
		return 0.0; 
	)

	EQUATION(Model, InitialReachFlow,
		double upstreamFlow = 0.0;
		FOREACH_INPUT(Reach,
			upstreamFlow += RESULT(ReachFlow, *Input);
		)
		double initialFlow  = PARAMETER(InitialStreamFlow);
		
		return (INPUT_COUNT(Reach) == 0) ? initialFlow : upstreamFlow;
	)
	
	EQUATION(Model, ReachFlow,
		double flow = ( RESULT(ReachFlowInput) - RESULT(ReachFlow)) / (RESULT(ReachTimeConstant) * (1.0 - PARAMETER(B)));
		
		if(RESULT(ReachTimeConstant) > 0.0)
		{
			return flow;
		}
		return 0.0;
	)
	
	EQUATION(Model, InitialReachVolume,
		return RESULT(ReachFlow) * RESULT(ReachTimeConstant) * 86400.0;
	)
	
	EQUATION(Model, ReachVolume,
		return ( RESULT(ReachFlowInput) - RESULT(ReachFlow) ) * 86400.0;
	)
	
	auto ReachNitrateOutput = RegisterEquation(Model, "Reach nitrate output", KgPerDay);
	SetSolver(Model, ReachNitrateOutput, IncaSolver);
	auto WaterTemperature = GetEquationHandle(Model, "Water temperature");
	auto WaterTemperatureFactor = RegisterEquation(Model, "Water temperature factor", Dimensionless);
	auto ReachDenitrification = RegisterEquation(Model, "Reach denitrification", KgPerDay);
	SetSolver(Model, ReachDenitrification, IncaSolver);
	auto ReachNitrification = RegisterEquation(Model, "Reach nitrification", KgPerDay);
	SetSolver(Model, ReachNitrification, IncaSolver);
	auto ReachUpstreamNitrate = RegisterEquation(Model, "Reach upstream nitrate", KgPerDay);
	auto ReachEffluentNitrate = RegisterEquation(Model, "Reach effluent nitrate", KgPerDay);
	auto TotalReachNitrateInput = RegisterEquation(Model, "Total reach nitrate input", KgPerDay);
	auto ReachNitrateInitialValue = RegisterEquationInitialValue(Model, "Reach nitrate initial value", KgPerDay);
	auto ReachNitrate = RegisterEquationODE(Model, "Reach nitrate", KgPerDay);
	SetInitialValue(Model, ReachNitrate, ReachNitrateInitialValue);
	SetSolver(Model, ReachNitrate, IncaSolver);
	auto ReachUpstreamAmmonium = RegisterEquation(Model, "Reach upstream ammonium", KgPerDay);
	auto ReachEffluentAmmonium = RegisterEquation(Model, "Reach effluent ammonium", KgPerDay);
	auto TotalReachAmmoniumInput = RegisterEquation(Model, "Total reach ammonium input", KgPerDay);
	auto ReachAmmoniumOutput   = RegisterEquation(Model, "Reach ammonium output", KgPerDay);
	SetSolver(Model, ReachAmmoniumOutput, IncaSolver);
	auto ReachAmmoniumInitialValue = RegisterEquationInitialValue(Model, "Reach ammonium initial value", KgPerDay);
	auto ReachAmmonium = RegisterEquationODE(Model, "Reach ammonium", KgPerDay);
	SetInitialValue(Model, ReachAmmonium, ReachAmmoniumInitialValue);
	SetSolver(Model, ReachAmmonium, IncaSolver);
	
	EQUATION(Model, ReachNitrateOutput,
		double out = RESULT(ReachNitrate) * RESULT(ReachFlow) * 86400.0 / RESULT(ReachVolume);
		if(RESULT(ReachVolume) > 0.0) return out;
		return 0.0;
	)
	
	EQUATION(Model, WaterTemperatureFactor,
		return pow(1.047, (RESULT(WaterTemperature) - 20.0));
	)
	
	EQUATION(Model, ReachDenitrification,	
		return PARAMETER(ReachDenitrificationRate)
			* RESULT(ReachNitrate) * RESULT(ConvertMassToConcentration)
			* RESULT(WaterTemperatureFactor)
			* RESULT(ReachVolume)
			/ 1000.0;
	)
	
	EQUATION(Model, ReachNitrification,
		return PARAMETER(ReachNitrificationRate)
			* RESULT(ReachAmmonium) * RESULT(ConvertMassToConcentration)
			* RESULT(WaterTemperatureFactor)
			* RESULT(ReachVolume)
			/ 1000.0;
	)
	
	EQUATION(Model, ReachUpstreamNitrate,
		double reachInput = 0.0;
		CURRENT_INDEX(Reach); //NOTE: Just to register a dependency
		
		FOREACH_INPUT(Reach,
			reachInput += RESULT(ReachNitrateOutput, *Input);
		)

		return reachInput;
	)

	EQUATION(Model, ReachEffluentNitrate,
		double effluentNitrate = PARAMETER(EffluentFlow) * PARAMETER(ReachEffluentNitrateConcentration) * 86.4;
		if(PARAMETER(ReachHasEffluentInput))
		{
			return effluentNitrate;
		}
		return 0.0;
	)
	
	EQUATION(Model, TotalReachNitrateInput,
		return RESULT(ReachUpstreamNitrate) + RESULT(TotalDiffuseNitrateOutput) + RESULT(ReachEffluentNitrate);
	)
	
	EQUATION(Model, ReachNitrateInitialValue,
		return PARAMETER(InitialStreamNitrateConcentration) * RESULT(ConvertConcentrationToMass);
	)
	
	EQUATION(Model, ReachNitrate,
		return RESULT(TotalReachNitrateInput)
			- RESULT(ReachNitrateOutput)
			- RESULT(ReachDenitrification)
			+ RESULT(ReachNitrification);
	)
	
	EQUATION(Model, ReachUpstreamAmmonium,
		double reachInput = 0.0;
		CURRENT_INDEX(Reach); //NOTE: Just to register a dependency
		
		FOREACH_INPUT(Reach,
			reachInput += RESULT(ReachAmmoniumOutput, *Input);
		)

		return reachInput;
	)
	
	EQUATION(Model, ReachEffluentAmmonium,
		double out = PARAMETER(EffluentFlow) * PARAMETER(ReachEffluentAmmoniumConcentration) * 86.4;
		if(PARAMETER(ReachHasEffluentInput))
		{
			return out;
		}
		return 0.0;
	)
	
	EQUATION(Model, TotalReachAmmoniumInput,
		return RESULT(ReachUpstreamAmmonium) + RESULT(TotalDiffuseAmmoniumOutput) + RESULT(ReachEffluentAmmonium);
	)
	
	EQUATION(Model, ReachAmmoniumOutput,
		return RESULT(ReachAmmonium) * RESULT(ReachFlow) * 86400.0 / RESULT(ReachVolume);
	)
	
	EQUATION(Model, ReachAmmoniumInitialValue,
		return PARAMETER(InitialStreamAmmoniumConcentration) * RESULT(ConvertConcentrationToMass);
	)
	
	EQUATION(Model, ReachAmmonium,
		return RESULT(TotalReachAmmoniumInput)
			- RESULT(ReachAmmoniumOutput)
			- RESULT(ReachNitrification);
	)
}

#define INCAN_CLASSIC_MODEL_H
#endif
