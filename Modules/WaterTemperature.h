

static void
AddWaterTemperatureModel(mobius_model *Model)
{
	auto Dimensionless 				= RegisterUnit(Model);
	auto DegreesCelsius 			= RegisterUnit(Model, "°C");
	
	auto Reach = RegisterIndexSetBranched(Model, "Reaches");
	
	auto Reaches = RegisterParameterGroup(Model, "Reaches", Reach);
	
	
	auto InitialWaterTemperature     = RegisterParameterDouble(Model, Reaches, "Initial water temperature",    DegreesCelsius, 20.0, 0.0, 40.0);
	auto MinimumWaterTemperature     = RegisterParameterDouble(Model, Reaches, "Minimum water temperature",    DegreesCelsius,  0.0, -5.0, 40.0);
	auto WaterTemperatureLagFactor   = RegisterParameterDouble(Model, Reaches, "Water temperature lag factor", Dimensionless,   3.0, 1.0, 10.0);
	
	auto AirTemperature = GetInputHandle(Model, "Air temperature"); //NOTE: Is registered in the snow melt model
	
	auto WaterTemperature = RegisterEquation(Model, "Water temperature", DegreesCelsius);
	SetInitialValue(Model, WaterTemperature, InitialWaterTemperature);
	
	EQUATION(Model, WaterTemperature,
	
		return ( (PARAMETER(WaterTemperatureLagFactor) - 1.0) / PARAMETER(WaterTemperatureLagFactor) ) 
			* LAST_RESULT(WaterTemperature) + (1.0 / PARAMETER(WaterTemperatureLagFactor) )
			* Max( PARAMETER(MinimumWaterTemperature), INPUT(AirTemperature) );
	)
}