

static void
AddABCDModel(mobius_model *Model)
{
	BeginModule(Model, "ABCD", "0.1");
	SetModuleDescription(Model, R""""(
This is the ABCD hydrological model, with snow sub-model taken from WASMOD.

[Harold A. Thomas Jr. 1981, Improved methods for national water assessment: Final report, U.S. Geol. Surv. Water Resour. Contract WR15249270](https://doi.org/10.3133/70046351)

[Guillermo F. Martinez and Hoshin V. Gupta 2010, Toward improved identification of hydrological models: A diagnostic evaluation of the “abcd” monthly water balance model for the conterminous United States, Water Resources Research 46(8)](https://doi.org/10.1029/2009WR008294)

[C-Y Xu and Sven Halldin (1997). The Effect of Climate Change on River Flow and Snow Cover in the NOPEX Area Simulated by a Simple Water Balance Model. Hydrology Research 28 (4-5), pp. 273–282.](https://doi.org/10.2166/nh.1998.19)
)"""");

	
	auto Dimensionless  = RegisterUnit(Model);
	auto Mm             = RegisterUnit(Model, "mm");
	auto DegreesCelsius = RegisterUnit(Model, "°C");
	auto PerMonth       = RegisterUnit(Model, "1/month");
	auto MmPerMonth     = RegisterUnit(Model, "mm/month");
	
	auto ABCDPars = RegisterParameterGroup(Model, "ABCD hydrology");
	
	auto PropensityForDryRunoff = RegisterParameterDouble(Model, ABCDPars, "a (propensity for runoff below saturation)", Dimensionless, 1.0, 0.5, 1.0, "A value of 1 means no runoff when soil moisture is below field capacity");
	auto FieldCapacity          = RegisterParameterDouble(Model, ABCDPars, "b (field capacity)", Mm, 150, 0.0, 500.0);
	auto BaseflowIndex          = RegisterParameterDouble(Model, ABCDPars, "c (baseflow index)", Dimensionless, 0.5, 0.0, 1.0);
	auto GroundwaterFlowRate    = RegisterParameterDouble(Model, ABCDPars, "d (groundwater flow rate)", PerMonth, 0.1, 0.0, 20.0);
	
	auto SnowMeltsAbove         = RegisterParameterDouble(Model, ABCDPars, "Snow melts above this temperature", DegreesCelsius, 0.0, -4.0, 4.0);
	auto SnowFallOffset         = RegisterParameterDouble(Model, ABCDPars, "Precipitation falls as snow below this temperature", DegreesCelsius, 0.1, 0.001, 10.0, "This is an offset from the snow melt temperature. Must be nonzero");
	auto InitialSnowPack        = RegisterParameterDouble(Model, ABCDPars, "Initial snow pack (water equivalents)", Mm, 0.0, 0.0, 5000.0);
	
	
	
	auto SnowFall               = RegisterEquation(Model, "Snow fall", MmPerMonth);
	auto RainFall               = RegisterEquation(Model, "Rain fall", MmPerMonth);
	auto SnowMelt               = RegisterEquation(Model, "Snow melt", MmPerMonth);
	auto SnowPack               = RegisterEquation(Model, "Snow pack (water equivalents)", Mm);
	
	auto AvailableWater         = RegisterEquation(Model, "Available water", Mm);
	auto ETPOpportunity         = RegisterEquation(Model, "Evapotranspiration opportunity", Mm);
	auto Evapotranspiration     = RegisterEquation(Model, "Evapotranspiration", MmPerMonth);
	auto SoilFlow               = RegisterEquation(Model, "Soil flow", MmPerMonth);
	auto SoilMoisture           = RegisterEquation(Model, "Soil moisture", Mm);
	auto GroundwaterVolume      = RegisterEquation(Model, "Groundwater volume", Mm);
	auto GroundwaterFlow        = RegisterEquation(Model, "Groundwater flow", MmPerMonth);
	auto Runoff                 = RegisterEquation(Model, "Runoff", MmPerMonth);
	
	
	SetInitialValue(Model, GroundwaterVolume, 0.0);
	SetInitialValue(Model, SoilMoisture, FieldCapacity);
	SetInitialValue(Model, SnowPack, InitialSnowPack);
	//TODO: Snow!
	
	auto Precipitation          = RegisterInput(Model, "Precipitation", Mm);
	auto AirTemperature         = RegisterInput(Model, "Air temperature", DegreesCelsius);
	auto PET                    = RegisterInput(Model, "Potential evapotranspiration", Mm);
	
	EQUATION(Model, SnowFall,
		double excess_temp = INPUT(AirTemperature) - (PARAMETER(SnowMeltsAbove) + PARAMETER(SnowFallOffset));
		excess_temp = std::min(excess_temp, 0.0);
		double rootexponent = excess_temp / PARAMETER(SnowFallOffset);
		double fraction = 1.0 - std::exp(-rootexponent*rootexponent);    //NOTE: The publications don't have a minus in the exponent, but the model doesn't work without it!
		return INPUT(Precipitation)*std::max(0.0, fraction); 
	)
	
	EQUATION(Model, RainFall,
		return INPUT(Precipitation) - RESULT(SnowFall);
	)
	
	EQUATION(Model, SnowMelt,
		double excess_temp = INPUT(AirTemperature) - PARAMETER(SnowMeltsAbove);
		excess_temp = std::max(0.0, excess_temp);
		double rootexponent = excess_temp / PARAMETER(SnowFallOffset);
		double fraction = 1.0 - std::exp(-rootexponent*rootexponent);
		return LAST_RESULT(SnowPack)*std::max(0.0, fraction); 
	)
	
	EQUATION(Model, SnowPack,
		return LAST_RESULT(SnowPack) + RESULT(SnowFall) - RESULT(SnowMelt);
	)
	
	EQUATION(Model, AvailableWater,
		return RESULT(RainFall) + RESULT(SnowMelt) + LAST_RESULT(SoilMoisture);    //TODO: WASMOD has a separate path for snowmelt that excempts it from etp. Should we do something similar here?
	)
	
	EQUATION(Model, ETPOpportunity,
		double a = PARAMETER(PropensityForDryRunoff);
		double b = PARAMETER(FieldCapacity);
		double wa = RESULT(AvailableWater);
		
		double factor = (b + wa)/(2.0*a);
		
		return factor - sqrt(factor*factor - wa*b/a);
	)
	
	EQUATION(Model, Evapotranspiration,
		return RESULT(ETPOpportunity)*(1.0 - exp(-INPUT(PET) / PARAMETER(FieldCapacity)));
	)
	
	EQUATION(Model, SoilFlow,
		return RESULT(AvailableWater) - RESULT(ETPOpportunity);
	)
	
	EQUATION(Model, SoilMoisture,
		return RESULT(AvailableWater) - RESULT(Evapotranspiration) - RESULT(SoilFlow);
	)
	
	EQUATION(Model, GroundwaterVolume,
		double d = PARAMETER(GroundwaterFlowRate);
		return (LAST_RESULT(GroundwaterVolume) + PARAMETER(BaseflowIndex)*RESULT(SoilFlow)) / (1.0 + d);
	)
	
	EQUATION(Model, GroundwaterFlow,
		return PARAMETER(GroundwaterFlowRate)*RESULT(GroundwaterVolume);
	)
	
	EQUATION(Model, Runoff,
		return (1.0 - PARAMETER(BaseflowIndex))*RESULT(SoilFlow) + RESULT(GroundwaterFlow);
	)
}