
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
	//NOTE: Returns the number of days in a month. The months are indexed from 0 in this context.
	s32 Length[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	s32 Days = Length[Month];
	if(Month == 1 && IsLeapYear(Year)) Days += 1;
	return Days;
}

struct datetime
{
private:

	inline s32
	MonthOffset(s32 Year, s32 Month)
	{
		//NOTE: Returns the number of the day of year (starting at january 1st = day 0) that this month starts on. The months are indexed from 0 in this context.
		
		s32 Offset[13] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};
		s32 Days = Offset[Month];
		if(Month >= 2 && IsLeapYear(Year)) Days += 1;
		return Days;
	}
	
	inline bool
	SetFromYearMonthDay(s32 Year, s32 Month, s32 Day)
	{
		if(Day < 1 || Day > MonthLength(Year, Month-1) || Month < 1 || Month > 12)
		{
			return false; 
		}
		
		s64 Result = 0;
		if(Year > 1970)
		{
			for(s32 Y = 1970; Y < Year; ++Y)
			{
				Result += YearLength(Y)*24*60*60;
			}
		}
		else if(Year < 1970)
		{
			for(s32 Y = 1969; Y >= Year; --Y)
			{
				Result -= YearLength(Y)*24*60*60;
			}
		}
		
		Result += MonthOffset(Year, Month-1)*24*60*60;
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
		
		s32 Day, Month, Year;
		
		int Found = sscanf(DateString, "%d-%d-%d", &Year, &Month, &Day);
		if(Found != 3)
		{
			*Success = false;
			return;
		}
		
		*Success = SetFromYearMonthDay(Year, Month, Day);
	}
	
	datetime(s32 Year, s32 Month, s32 Day, bool *Success)
	{
		*Success = SetFromYearMonthDay(Year, Month, Day);
	}
	
	inline void
	DayOfYear(s32 *DayOut, s32 *YearOut)
	{
		//Computes the day of year (Starting at january 1st = day 1)
		
		s32 Year = 1970;
		s32 DayOfYear = 0;
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
			DayOfYear = SecondsLeft / (24*60*60);
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
			DayOfYear = (SecondsThisYear - SecondsLeft) / (24*60*60);
		}
		
		*YearOut = Year;
		*DayOut  = DayOfYear + 1;
	}
	
	inline void
	YearMonthDay(s32 *YearOut, s32 *MonthOut, s32 *DayOut)
	{
		//Computes the year, month and day (of month) for a seconds since epoch timestamp.
	
		s32 Day;
		DayOfYear(&Day, YearOut);
		
		for(s32 Month = 0; Month < 12; ++Month)
		{
			if(Day <= MonthOffset(*YearOut, Month+1))
			{
				*MonthOut = (Month+1);
				if(Month == 0) *DayOut = Day;
				else *DayOut = (s32)Day - (s32)MonthOffset(*YearOut, Month);
				break;
			}
		}
	}
	
	inline void
	AdvanceDays(s32 NumberOfDays)
	{
		SecondsSinceEpoch += 24*60*60*((s64)NumberOfDays);
	}
	
	inline char *
	ToString()
	{
		//Important: note that this one is overwritten whenever you call it. So you should make a copy of the string if you want to keep it.
		s32 Year, Month, Day;
		YearMonthDay(&Year, &Month, &Day);
		static char Buf[32];
		sprintf(Buf, "%04d-%02d-%02d", Year, Month, Day);
		return Buf;
	}
	
	inline s32
	DaysUntil(datetime DateTime)
	{
		s64 OtherSeconds = DateTime.SecondsSinceEpoch;
		
		if(OtherSeconds >= SecondsSinceEpoch)
		{
			return (OtherSeconds - SecondsSinceEpoch) / (24*60*60);
		}
		else
		{
			return (OtherSeconds - SecondsSinceEpoch - 1) / (24*60*60);
		}
	}
};






#define DATETIME_H
#endif