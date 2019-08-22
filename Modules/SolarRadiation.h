

//NÒTE: This module is based on the formulas given in "Crop evapotranspiration - Guidelines for computing crop water requirements - FAO Irrigation and drainage paper 56" http://www.fao.org/3/X0490E/x0490e07.htm


inline double
ShortWaveRadiationOnAClearSkyDay(double Elevation, double ExtraterrestrialRadiation)
{
	return (0.75 + 2e-5*Elevation)*ExtraterrestrialRadiation;
}

inline double
DailyExtraTerrestrialRadiation(double Latitude, s32 DayOfYear)
{
	const double SolarConstant = 0.0820;
	
	double InverseRelativeDistanceEarthSun = 1.0 + 0.033*cos(2.0*Pi*(double)DayOfYear / 365.0);
	
	double SolarDecimation = 0.409*sin(2.0*Pi*(double)DayOfYear / 365.0 - 1.39);
	
	double LatitudeRad = Latitude * Pi / 180.0;
	
	double SunsetHourAngle = acos(-tan(LatitudeRad)*tan(SolarDecimation));
	
	return (24.0 * 60.0 / Pi) * SolarConstant * InverseRelativeDistanceEarthSun * (SunsetHourAngle * sin(LatitudeRad) * sin(SolarDecimation) + cos(LatitudeRad) * cos(SolarDecimation) * sin(SunsetHourAngle));
}

static void
AddSolarRadiationModule(mobius_model *Model)
{
	auto Degrees        = RegisterUnit(Model, "°");
	auto Metres         = RegisterUnit(Model, "m");
	auto WPerM2         = RegisterUnit(Model, "W/m^2");
	
	auto System = GetParameterGroupHandle(Model, "System");
	
	auto Latitude  = RegisterParameterDouble(Model, System, "Latitude", Degrees, 60.0, -90.0, 90.0, "Used to compute solar radiation if no solar radiation timeseries was provided in the input data.");
	auto Elevation = RegisterParameterDouble(Model, System, "Elevation", Metres, 0.0, 0.0, 8848.0, "Used to compute solar radiation if no solar radiation timeseries was provided in the input data.");
	
	auto SolarRadiationTimeseries = RegisterInput(Model, "Solar radiation", WPerM2);
	
	auto SolarRadiation = RegisterEquation(Model, "Solar radiation", WPerM2);
	
	EQUATION(Model, SolarRadiation,
		double sradin = INPUT(SolarRadiationTimeseries);
		
		double latitude = PARAMETER(Latitude);
		double elevation = PARAMETER(Elevation);
	
		if(INPUT_WAS_PROVIDED(SolarRadiationTimeseries)) return sradin;
		
		s32 DOY = (s32)CURRENT_DAY_OF_YEAR();
		double DETR = DailyExtraTerrestrialRadiation(latitude, DOY);
		double SWR  = ShortWaveRadiationOnAClearSkyDay(elevation, DETR);
		
		return SWR * 11.5740741;  //NOTE: Converting MJ/m2/day to W/m2
	)
	
}
