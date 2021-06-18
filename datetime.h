
//NOTE: Just a very simple datetime library that has what we need for our purposes.
//NOTE: Apparently the C++ standard library can not do all the date processing we need until c++20, so we have to do it ourselves... (could use boost::ptime, but it has to be compiled separately, and that is asking a lot of the user...)

#if !defined(DATETIME_H)

inline bool
IsLeapYear(s32 Year)
{
	if(Year % 4 != 0) return false;
	if(Year % 100 != 0) return true;
	if(Year % 400 != 0) return false;
	return true;
}

inline s32
YearLength(s32 Year)
{
	return 365 + IsLeapYear(Year);
}

inline s32
MonthLength(s32 Year, s32 Month)
{
	//NOTE: Returns the number of days in a month. The months are indexed from 1 in this context.
	s32 Length[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	s32 Days = Length[Month];
	if(Month == 2 && IsLeapYear(Year)) Days += 1;
	return Days;
}

inline s32
MonthOffset(s32 Year, s32 Month)
{
	//NOTE: Returns the number of the days in this year before this month starts. The months are indexed from 1 in this context.
	
	s32 Offset[14] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
	s32 Days = Offset[Month];
	if(Month > 2 && IsLeapYear(Year)) Days += 1;
	return Days;
}

struct datetime
{
private:
	
	inline bool
	SetFromYearMonthDay(s32 Year, s32 Month, s32 Day)
	{
		if(Day < 1 || Day > MonthLength(Year, Month) || Month < 1 || Month > 12)
			return false; 
		
		s64 Result = 0;
		if(Year > 1970)
		{
			for(s32 Y = 1970; Y < Year; ++Y)
				Result += YearLength(Y)*24*60*60;
		}
		else if(Year < 1970)
		{
			for(s32 Y = 1969; Y >= Year; --Y)
				Result -= YearLength(Y)*24*60*60;
		}
		
		Result += MonthOffset(Year, Month)*24*60*60;
		Result += (Day-1)*24*60*60;
		
		SecondsSinceEpoch = Result;
		return true;
	}
	
	
public:
	//NOTE: It is very important that SecondsSinceEpoch is the only data member of datetime, because a datetime is a member of the parameter_value union (mobius_model.h). Changing this may change the size of the parameter_value, which could break things.
	s64 SecondsSinceEpoch;
	
	datetime() : SecondsSinceEpoch(0) {}
	
	datetime(const char *DateString, bool *Success)
	{
		//NOTE: Does not account for leap seconds, but that should not be a problem.
		// Takes a string of the form "yyyy-mm-dd" and puts the number of seconds since "1970-01-01" in the SecondsSinceEpoch. Note that a negative value is computed for dates before "1970-01-01". Returns a bool saying if a correct date was provided.
		
		s32 Day, Month, Year, Hour, Minute, Second;
		
		int Found = sscanf(DateString, "%d-%d-%d %d:%d:%d", &Year, &Month, &Day, &Hour, &Minute, &Second);
		if(Found != 3 && Found != 6)    //TODO: This does not check that somebody didn't write "2004--03 12::"
		{
			*Success = false;
			return;
		}
		
		*Success = SetFromYearMonthDay(Year, Month, Day);
		if(Found == 6)
			*Success = *Success && AddHourMinuteSecond(Hour, Minute, Second);
	}
	
	datetime(s32 Year, s32 Month, s32 Day, bool *Success)
	{
		*Success = SetFromYearMonthDay(Year, Month, Day);
	}
	
	inline bool
	AddHourMinuteSecond(s32 Hour, s32 Minute, s32 Second)
	{
		bool Success = Hour >= 0 && Hour <= 23 && Minute >= 0 && Minute <= 59 && Second >= 0 && Second <= 59;
		SecondsSinceEpoch += 3600*Hour + 60*Minute + Second;
		return Success;
	}
	
	inline void
	DayOfYear(s32 *DayOut, s32 *YearOut) const
	{
		//Computes the day of year (Starting at january 1st = day 1)
		
		s32 Year = 1970;
		s32 DOY = 0;
		s64 SecondsLeft = SecondsSinceEpoch;
		if(SecondsLeft > 0)
		{
			while(true)
			{
				s64 SecondsThisYear = YearLength(Year)*24*60*60;
				if(SecondsLeft >= SecondsThisYear)
				{
					Year++;
					SecondsLeft -= SecondsThisYear;
				}
				else break;
			}
			DOY = SecondsLeft / (24*60*60);
		}
		else if(SecondsLeft < 0)
		{
			SecondsLeft = -SecondsLeft;
			Year = 1969;
			s64 SecondsThisYear;
			while(true)
			{
				SecondsThisYear = YearLength(Year)*24*60*60;
				if(SecondsLeft > SecondsThisYear)
				{
					Year--;
					SecondsLeft -= SecondsThisYear;
				}
				else break;
			}
			s64 DaysThisYear = 365 + IsLeapYear(Year);

			SecondsThisYear = DaysThisYear*24*60*60;
			DOY = (SecondsThisYear - SecondsLeft) / (24*60*60);
		}
		
		*YearOut = Year;
		*DayOut  = DOY + 1;
	}
	
	inline void
	YearMonthDay(s32 *YearOut, s32 *MonthOut, s32 *DayOut) const
	{
		//Computes the year, month and day (of month) for a seconds since epoch timestamp.
	
		s32 Day;
		DayOfYear(&Day, YearOut);
		
		for(s32 Month = 1; Month <= 12; ++Month)
		{
			if(Day <= MonthOffset(*YearOut, Month+1))
			{
				*MonthOut = Month;
				*DayOut = Day - MonthOffset(*YearOut, Month);
				break;
			}
		}
	}
	
	//TODO: Consider removing this (it is used in Modules/Preprocessing/ThornthwaitePET.h only)
	inline void
	AdvanceDays(s32 NumberOfDays)
	{
		SecondsSinceEpoch += 24*60*60*((s64)NumberOfDays);
	}
	
	inline s64
	SecondOfDay() const
	{
		if(SecondsSinceEpoch >= 0)
			return SecondsSinceEpoch % 86400;
		else
			return (SecondsSinceEpoch % 86400 + 86400) % 86400;
		return 0;
	}
	
	inline const char *
	ToString() const
	{
		//Important: note that this one is overwritten whenever you call it. So you should make a copy of the string if you want to keep it.
		s32 Year, Month, Day, Hour, Minute, Second;
		YearMonthDay(&Year, &Month, &Day);
		static char Buf[64];
		if(SecondsSinceEpoch % 86400 == 0)
			sprintf(Buf, "%04d-%02d-%02d", Year, Month, Day);
		else
		{
			s64 SOD = SecondOfDay();
			s32 Hour = (SOD / 3600);
			s32 Minute = (SOD / 60) % 60;
			s32 Second = SOD % 60;
			sprintf(Buf, "%04d-%02d-%02d %02d:%02d:%02d", Year, Month, Day, Hour, Minute, Second);
		}
		return Buf;
	}
	
	bool
	operator<(const datetime& Other)
	{
		return SecondsSinceEpoch < Other.SecondsSinceEpoch;
	}
	
	bool
	operator<=(const datetime& Other)
	{
		return SecondsSinceEpoch <= Other.SecondsSinceEpoch;
	}
};



enum timestep_unit
{
	Timestep_Second = 0,
	Timestep_Month  = 1,
	//Timestep_Year,    //TODO: Should probably have this as an optimization!
};

struct timestep_size
{
	timestep_unit Unit;
	s32           Magnitude;
};

timestep_size
ParseTimestepSize(const char *Format)
{
	// Format is "XY", where X is a number and Y is one of "s", "m", "h", "D", "M", "Y".
	
	timestep_size Result;
	char Type;
	int Found = sscanf(Format, "%d%c", &Result.Magnitude, &Type);
	if(Found != 2 || Result.Magnitude <= 0)
		FatalError("The size of the timestep must be declared on the format \"nx\", where n is a positive whole number, and x is one of 's', 'm', 'h', 'D', 'M', or 'Y'.\n");
	if(Type == 's')
		Result.Unit = Timestep_Second;
	else if(Type == 'm')
	{
		Result.Unit = Timestep_Second;
		Result.Magnitude *= 60;
	}
	else if(Type == 'h')
	{
		Result.Unit = Timestep_Second;
		Result.Magnitude *= 3600;
	}
	else if(Type == 'D')
	{
		Result.Unit = Timestep_Second;
		Result.Magnitude *= 86400;
	}
	else if(Type == 'M')
		Result.Unit = Timestep_Month;
	else if(Type == 'Y')
	{
		Result.Unit = Timestep_Month;
		Result.Magnitude *= 12;
	}
	else
		FatalError("The size of the timestep must be declared on the format \"nx\", where n is a positive whole number, and x is one of 's', 'm', 'h', 'D', 'M', or 'Y'.\n");
	
	return Result;
}

token_string
TimestepSizeAsUnitName(const char *Format, bucket_allocator *Alloc)
{
	//NOTE: This one doesn't do error checking, instead we assume that Format has already been passed to ParseTimestepSize once
	char Type;
	s32  Magnitude;
	int Found = sscanf(Format, "%d%c", &Magnitude, &Type);

	const char *Name;
	if(Type == 's')
		Name = "seconds";
	else if(Type == 'm')
		Name = "minutes";
	else if(Type == 'h')
		Name = "hours";
	else if(Type == 'D')
		Name = "days";
	else if(Type == 'M')
		Name = "months";
	else if(Type == 'Y')
		Name = "years";
	
	if(Magnitude != 1)
	{
		char Buffer[256];
		sprintf(Buffer, "%d %s", Magnitude, Name);
		Name = Buffer;
	}
	
	token_string StrName(Name);
	return StrName.Copy(Alloc);
}

struct expanded_datetime
{
	datetime DateTime;
	s32      Year;
	s32      Month;
	s32      DayOfYear;
	s32      DayOfMonth;
	s64      SecondOfDay;
	
	s32      DaysThisYear;
	s32      DaysThisMonth;
	
	timestep_size Timestep;
	s64      StepLengthInSeconds;
	
	expanded_datetime(datetime Base, timestep_size Timestep)
	{
		this->DateTime = Base;
		this->Timestep = Timestep;
		
		//NOTE: This does double work, but it should not matter that much.
		DateTime.YearMonthDay(&Year, &Month, &DayOfMonth);
		DateTime.DayOfYear(&DayOfYear, &Year);
		
		DaysThisYear = YearLength(Year);
		DaysThisMonth = MonthLength(Year, Month);
		
		SecondOfDay = DateTime.SecondOfDay();
		
		ComputeNextStepSize();
	}
	
	expanded_datetime()
	{
		Year = 1970;
		Month = 1;
		DayOfYear = 1;
		DayOfMonth = 1;
		DaysThisYear = 365;
		DaysThisMonth = 30;
		StepLengthInSeconds = 86400;
		Timestep.Unit = Timestep_Second;
		Timestep.Magnitude = 86400;
		SecondOfDay = 0;
	}
	
	void
	Advance()
	{
		if(Timestep.Unit == Timestep_Second)
		{
			DateTime.SecondsSinceEpoch += StepLengthInSeconds;
			SecondOfDay                += StepLengthInSeconds;
			
			s32 Days = SecondOfDay / 86400;
			SecondOfDay -= 86400*Days;
			DayOfYear   += Days;
			DayOfMonth  += Days;
		}
		else
		{
			assert(Timestep.Unit == Timestep_Month);
			
			DateTime.SecondsSinceEpoch += StepLengthInSeconds;
			DayOfMonth                 += StepLengthInSeconds / 86400;
		}
		
		while(DayOfMonth > DaysThisMonth)
		{
			DayOfMonth -= DaysThisMonth;
			Month++;
			
			if(Month > 12)
			{
				
				DayOfYear -= DaysThisYear;
				Year++;
				DaysThisYear = YearLength(Year);
				Month = 1;
			}
			
			DaysThisMonth = MonthLength(Year, Month);
		}
		
		if(Timestep.Unit == Timestep_Month)
			ComputeNextStepSize();
	}
	
	void
	ComputeNextStepSize()
	{
		if(Timestep.Unit == Timestep_Second)
			StepLengthInSeconds = Timestep.Magnitude;
		else
		{
			assert(Timestep.Unit == Timestep_Month);
			
			StepLengthInSeconds = 0;
			s32 Y = Year;
			s32 M = Month;
			for(s32 MM = 0; MM < Timestep.Magnitude; ++MM)
			{
				StepLengthInSeconds += 86400*MonthLength(Y, M);
				M++;
				if(M > 12) {M = 1; Y++; } 
			}
		}
	}
	
};

std::ostream& operator<<(std::ostream& Os, const expanded_datetime &Dt)
{
	Os << Dt.DateTime.ToString();
	return Os;
}


static s64
DivideDown(s64 a, s64 b)
{
    s64 r = a/b;
    if(r < 0 && r*b != a)
        return r - 1;
    return r;
}

static s64
FindTimestep(datetime Start, datetime Current, timestep_size Timestep)
{
	s64 Diff;
	if(Timestep.Unit == Timestep_Second)
		Diff = Current.SecondsSinceEpoch - Start.SecondsSinceEpoch;
	else
	{
		assert(Timestep.Unit == Timestep_Month);
		
		s32 SY, SM, SD, CY, CM, CD;
		
		//TODO: This could probably be optimized
		Start.YearMonthDay(&SY, &SM, &SD);
		Current.YearMonthDay(&CY, &CM, &CD);

		Diff = (CM - SM + 12*(CY-SY));
	}
	
	return DivideDown(Diff, (s64)Timestep.Magnitude);
}



#define DATETIME_H
#endif