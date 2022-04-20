

static void
AddHBVSnowModule(mobius_model *Model)
{
	BeginModule(Model, "HBV-Snow", "0.0");
	
	SetModuleDescription(Model, R""""(
This is an adaption of the hydrology module from HBV-Nordic (Sælthun 1995)

[NVE home page](https://www.nve.no/vann-og-vassdrag/vannets-kretslop/analysemetoder-og-modeller/hbv-modellen/)

[Model description](https://www.uio.no/studier/emner/matnat/geofag/nedlagte-emner/GEO4430/v06/undervisningsmateriale/HBVMOD.PDF)
)"""");

	auto Mm                = RegisterUnit(Model, "mm");
	auto MmPerDay          = RegisterUnit(Model, "mm/day");
	auto DegC              = RegisterUnit(Model, "°C");
	auto MmPerDegreePerDay = RegisterUnit(Model, "mm/°C/day");
	auto Dimensionless     = RegisterUnit(Model);
	
	
	auto SnowDistributionBox = RegisterIndexSet(Model, "Snow distribution box");
	
	
	auto Precipitation  = RegisterInput(Model, "Precipitation", MmPerDay);
	auto AirTemperature = RegisterInput(Model, "Air temperature", DegC);
	
	auto SnowPar        = RegisterParameterGroup(Model, "Snow");
	
	auto SnowFallTemp        = RegisterParameterDouble(Model, SnowPar, "Temperature below which precipitation falls as snow", DegC, 0.0, -4.0, 4.0, "", "Tsnow");
	auto SnowMeltTemp        = RegisterParameterDouble(Model, SnowPar, "Temperature above which snow melts", DegC, 0.0, -4.0, 4.0, "", "Tmelt");
	auto DegreeDaySnowMelt   = RegisterParameterDouble(Model, SnowPar, "Degree-day factor for snow melt", MmPerDegreePerDay, 2.74, 0.0, 5.0, "", "DDFmelt");
	auto LiquidWaterFraction = RegisterParameterDouble(Model, SnowPar, "Liquid water fraction", Dimensionless, 0.1, 0.0, 1.0, "Amount of melt water each unit of snow can hold before it is released", "LF");
	auto RefreezeEfficiency  = RegisterParameterDouble(Model, SnowPar, "Refreeze efficiency", Dimensionless, 0.5, 0.0, 1.0, "RFeff");
	auto DistCoeffOfVar      = RegisterParameterDouble(Model, SnowPar, "Snow distribution coefficient of variation", Dimensionless, 0.5, 0.0, 1.0,
		"0 gives even snow distribution among boxes, 1 or higher gives a very skew distribution.", "Cvar");
	auto DistMin             = RegisterParameterDouble(Model, SnowPar, "Minimal snow depth before snow fall is distributed unevenly", Mm, 0.0, 0.0, 50000.0, "Dmin");
	auto DepthAtFullCover    = RegisterParameterDouble(Model, SnowPar, "Snow depth at which snow cover is considered full", Mm, 50.0, 0.0, 1000.0);
	auto InitialSnowDepth    = RegisterParameterDouble(Model, SnowPar, "Initial snow depth as water equivalent", Mm, 0.0, 0.0, 50000.0);
	
	auto SnowDist       = RegisterParameterGroup(Model, "Snow distribution", SnowDistributionBox);
	
	auto BoxArea             = RegisterParameterDouble(Model, SnowDist, "Snow box area fraction", Dimensionless, 0.0, 0.0, 1.0);
	auto BoxQuantile         = RegisterParameterDouble(Model, SnowDist, "Snow box quantile", Dimensionless, 0.0, -100.0, 100.0);
	auto BoxDistr            = RegisterParameterDouble(Model, SnowDist, "Snow box distribution coefficient", Dimensionless, 0.0, 0.0, 1.0);
	
	auto ComputeBoxQuantile  = RegisterEquationInitialValue(Model, "Compute snow box quantile", Dimensionless);
	auto ComputeBoxDistr     = RegisterEquationInitialValue(Model, "Compute snow box distribution", Dimensionless);
	ParameterIsComputedBy(Model, BoxQuantile, ComputeBoxQuantile, true);
	ParameterIsComputedBy(Model, BoxDistr, ComputeBoxDistr, true);
	
	//NOTE this does not give exactly the same as they set up in the HBV source code, but it is close. I don't know how they computed the coefficients there (they are hard coded).
	EQUATION(Model, ComputeBoxQuantile,
		index_t Box = CURRENT_INDEX(SnowDistributionBox);
		double p = 0.0;
		for(index_t Box2 = FIRST_INDEX(SnowDistributionBox); Box2 < Box; ++Box2)
			p += PARAMETER(BoxArea, Box2);
		
		p += 0.5*PARAMETER(BoxArea);
		if(!RunState__->Running) return 0.0; // NOTE: Hack to not make it have an error in the setup phase.
		return NormalCDFInverse(p);
	)
	
	EQUATION(Model, ComputeBoxDistr,
		index_t Box = CURRENT_INDEX(SnowDistributionBox);
		double ThisCoeff;
		double SumCoeff = 0.0;
		//NOTE: A little annoying that we have to compute all the coefficients again per box here. Alternative is to make a preprocessing step.
		for(index_t Box2 = FIRST_INDEX(SnowDistributionBox); Box2 < INDEX_COUNT(SnowDistributionBox); ++Box2)
		{
			double Coeff = std::exp(PARAMETER(BoxQuantile, Box2) * PARAMETER(DistCoeffOfVar));
			if(Box2 == Box) ThisCoeff = Coeff;
			SumCoeff += Coeff * PARAMETER(BoxArea, Box2);
		}
		return ThisCoeff / SumCoeff;
	)
	
	
	auto PrecipFallingAsRain = RegisterEquation(Model, "Precipitation falling as rain", MmPerDay);
	auto PrecipFallingAsSnow = RegisterEquation(Model, "Precipitation falling as snow", MmPerDay);
	
	auto SnowMelt            = RegisterEquation(Model, "Snow melt", MmPerDay);
	auto Refreeze            = RegisterEquation(Model, "Refreeze", MmPerDay);
	auto MeltWaterToSoil     = RegisterEquation(Model, "Melt water to soil", MmPerDay);
	
	auto WaterInSnow         = RegisterEquation(Model, "Water in snow", Mm);
	auto SnowDepth           = RegisterEquation(Model, "Snow depth as water equivalent per box", Mm);
	SetInitialValue(Model, SnowDepth, InitialSnowDepth);
	SetInitialValue(Model, WaterInSnow, 0.0);
	
	auto AreaAvgMeltWaterToSoil = RegisterEquationCumulative(Model, "Area averaged melt water to soil", MeltWaterToSoil, SnowDistributionBox, BoxArea);
	auto AvgSnowDepth        = RegisterEquationCumulative(Model, "Snow depth as water equivalent", SnowDepth, SnowDistributionBox, BoxArea);
	auto AvgSnowFall         = RegisterEquationCumulative(Model, "Area averaged precipitation falling as snow", PrecipFallingAsSnow, SnowDistributionBox, BoxArea);
	auto SnowCover           = RegisterEquation(Model, "Snow cover", Dimensionless);
	auto HydrolInputToSoil   = RegisterEquation(Model, "Hydrological input to soil box", MmPerDay);
	
	
	EQUATION(Model, PrecipFallingAsRain,
		double pr = INPUT(Precipitation);
		if(INPUT(AirTemperature) <= PARAMETER(SnowFallTemp))
			return 0.0;
		return pr;
	)
	
	EQUATION(Model, PrecipFallingAsSnow,
		double fraction = PARAMETER(BoxDistr);
		double pr = INPUT(Precipitation);
		if(LAST_RESULT(AvgSnowDepth) <= PARAMETER(DistMin)) fraction = 1.0;
		pr *= fraction;
		if(INPUT(AirTemperature) > PARAMETER(SnowFallTemp))
			return 0.0;
		return pr;
	)
	
	EQUATION(Model, SnowMelt,
		double snow = LAST_RESULT(SnowDepth);
		double ddf  = PARAMETER(DegreeDaySnowMelt);   // TODO: Could be made more complex
		double potential_melt = (INPUT(AirTemperature) - PARAMETER(SnowMeltTemp))*ddf;
		potential_melt = std::max(potential_melt, 0.0);
		potential_melt = std::min(potential_melt, snow);
		return potential_melt;
	)
	
	EQUATION(Model, Refreeze,
		double snow_water = LAST_RESULT(WaterInSnow);
		double ddf = PARAMETER(DegreeDaySnowMelt) * PARAMETER(RefreezeEfficiency);  // Should degree-day be split in the more advanced version here too?
		double potential_refreeze = (PARAMETER(SnowMeltTemp) - INPUT(AirTemperature))*ddf;
		potential_refreeze = std::max(potential_refreeze, 0.0);
		potential_refreeze = std::min(potential_refreeze, snow_water);
		return potential_refreeze;
	)
	
	EQUATION(Model, MeltWaterToSoil,
		double threshold = RESULT(SnowDepth) * PARAMETER(LiquidWaterFraction);
		double snow_water = LAST_RESULT(WaterInSnow) + RESULT(SnowMelt);
		return std::max(0.0, snow_water - threshold);
	)
	
	EQUATION(Model, SnowDepth,
		return LAST_RESULT(SnowDepth) + RESULT(PrecipFallingAsSnow) + RESULT(Refreeze) - RESULT(SnowMelt);
	)
	
	EQUATION(Model, WaterInSnow,
		return LAST_RESULT(WaterInSnow) + RESULT(SnowMelt) - RESULT(Refreeze) - RESULT(MeltWaterToSoil);
	)
	
	EQUATION(Model, SnowCover,
		double fulldepth = PARAMETER(DepthAtFullCover);
		double cover = 0.0;
		for(index_t Box = FIRST_INDEX(SnowDistributionBox); Box < INDEX_COUNT(SnowDistributionBox); ++Box)
			cover += LinearResponse(RESULT(SnowDepth, Box), 0.0, fulldepth, 0.0, 1.0) * PARAMETER(BoxArea, Box);

		return cover;
	)
	
	EQUATION(Model, HydrolInputToSoil,
		return RESULT(PrecipFallingAsRain) + RESULT(AreaAvgMeltWaterToSoil);
	)


	EndModule(Model);
}