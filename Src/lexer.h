
enum token_type : char
{
	TokenType_Unknown = 0,
	TokenType_Identifier,
	TokenType_QuotedString,
	TokenType_Double,       // Number type only guaranteed to be a valid double
	TokenType_UInt,         // Number type that is guaranteed to be a valid uint, but also valid double.
	TokenType_Bool,
	TokenType_Date,
	TokenType_Time,
	TokenType_EOF,
};

constexpr char MobiusMaxMulticharTokenType = 20; // We won't need more multi-character token types, and this fits nicely with the ascii table

inline const char *TokenTypeName(token_type &Type)
{
	//NOTE: WARNING: This has to match the token_type enum!!!
	const char *TokenNames[9] =
	{
		"(unknown)",
		"identifier",
		"quoted string",
		"number",
		"number",
		"boolean",
		"date",
		"time",
		"(end of file)",
	};
	if(Type <= MobiusMaxMulticharTokenType)                   
		return TokenNames[Type];
	else
		return (char *)&Type;
}

inline const char *TokenTypeArticle(token_type Type)
{
	if(Type == TokenType_Unknown || Type == TokenType_Identifier || Type == TokenType_EOF)
		return "an";
	return "a";
}

inline bool IsNumeric(token_type Type)
{
	return (Type == TokenType_Double) || (Type == TokenType_UInt);
}


struct token
{
	token_string StringValue;
	
	// NOTE: This is almost the same as parameter_value, so we could use that, but it is nice to separate complexities just in case.
	union
	{
		u64      UIntValue;
		// WARNING: DoubleValue should never be read directly, instead use GetDoubleValue() below. This is because something that is a valid uint could also be interpreted as a double.
		// TODO:    How to enforce that in the compiler without making too much boilerplate for accessing the other values?
		double   DoubleValue;
		u64      BoolValue;
		datetime DateValue;
	};
	
	token_type   Type;
	
	double GetDoubleValue()
	{
		if(Type == TokenType_Double) return DoubleValue;
		return (double)UIntValue;
	}
};

struct token_stream
{
	token_stream(const char *Filename)
	{
		this->Filename = Filename;
		
		AtChar = -1;
		
		FileData = ReadEntireFile(Filename);
		
		//NOTE: In case the file has a BOM (byte order mark), which Notepad tends to do on Windows.
		if(FileData.Length >= 3 
				&& FileData[0] == (char) 0xEF
				&& FileData[1] == (char) 0xBB
				&& FileData[2] == (char) 0xBF)
			AtChar = 2;
		
		StartLine = 0; StartColumn = 0; Line = 0; Column = 0; PreviousColumn = 0;
	}
	
	~token_stream()
	{
		if(FileData.Data) free((void *)FileData.Data); // NOTE: Casting away constness, but it doesn't matter since we just allocated it ourselves.
	}
	
	token ReadToken();
	token PeekToken(s64 PeekAt = 0);
	token ExpectToken(token_type);
	token ExpectToken(char);
	
	double       ExpectDouble();
	u64          ExpectUInt();
	bool         ExpectBool();
	datetime     ExpectDateTime();
	token_string ExpectQuotedString();
	token_string ExpectIdentifier();
	
	void ReadQuotedStringList(std::vector<token_string> &ListOut);
	void ReadParameterSeries(std::vector<parameter_value> &ListOut, const parameter_spec &Spec);
	
	void PrintErrorHeader(bool CurrentColumn=false);

	const char *Filename;

private:
	s32 StartLine;
	s32 StartColumn;
	
	s32 Line;
	s32 Column;
	s32 PreviousColumn;
	
	token_string FileData;
	s64          AtChar;
	
	peek_queue<token> TokenQueue;
	
	char NextChar();
	void PutbackChar();
	
	void ReadNumber(token &Token);
	void ReadDateOrTime(token &Token, s32 FirstPart);
	void ReadString(token &Token);
	void ReadIdentifier(token &Token);
	void ReadTokenInternal_(token &Token);
	const token & PeekInternal_(s64 PeekAt = 0);
};

void
token_stream::PrintErrorHeader(bool CurrentColumn)
{
	u32 Col = StartColumn;
	if(CurrentColumn) Col = Column;
	ErrorPrint("ERROR: In file ", Filename, " line ", (StartLine+1), " column ", Col, ": ");
}

const token & token_stream::PeekInternal_(s64 PeekAt)
{
	if(PeekAt < 0) FatalError("ERROR (internal): Tried to peek backwards on already consumed tokens when parsing a file.\n");
	
	TokenQueue.Reserve(PeekAt+1);
	while(TokenQueue.MaxPeek() < PeekAt)
	{
		token & Token = TokenQueue.Append();
		ReadTokenInternal_(Token);
	}
	
	return TokenQueue.Peek(PeekAt);
}

token
token_stream::PeekToken(s64 PeekAt)
{
	return PeekInternal_(PeekAt);
}

token
token_stream::ReadToken()
{
	const token & Token = PeekInternal_();
	TokenQueue.Advance();
	return Token;
}

token
token_stream::ExpectToken(token_type Type)
{
	token Token = ReadToken();
	if(Token.Type != Type)
	{
		PrintErrorHeader();
		ErrorPrint("Expected a token of type ", TokenTypeName(Type), ", got ", TokenTypeArticle(Token.Type), " ", TokenTypeName(Token.Type));
		if(Token.Type == TokenType_QuotedString || Token.Type == TokenType_Identifier)
			ErrorPrint(" \"", Token.StringValue, "\"");
		FatalError('\n');
	}
	return Token;
}

token
token_stream::ExpectToken(char Type)
{
	return ExpectToken((token_type)Type);
}

inline bool
IsIdentifier(char c)
{
	return isalpha(c) || c == '_';
}



char
token_stream::NextChar()
{
	++AtChar;
	char c = '\0';
	if(AtChar < FileData.Length) c = FileData[AtChar];
	
	else if(c == '\n')
	{
		++Line;
		PreviousColumn = Column;
		Column = 0;
	}
	else
		++Column;
	
	return c;
}

void
token_stream::PutbackChar()
{
	// If we are at the end, there is no point in putting back characters, because we are not reading more tokens any way.
	if(AtChar == FileData.Length) return;
	
	char c = FileData[AtChar];
	if(c == '\n')
	{
		--Line;
		Column = PreviousColumn;
	}
	else
		--Column;
	AtChar--;
}

void
token_stream::ReadTokenInternal_(token &Token)
{
	Token = {}; // 0-initialize
	
	bool SkipComment = false;
	
	while(true)
	{
		char c = NextChar();
		
		if(SkipComment) // If we hit a # symbol outside a string token, SkipComment is true, and we continue skipping characters until we hit a newline or end of file.
		{
			if(c == '\n' || c == '\0') SkipComment = false;
			continue;
		}
		
		//NOTE: This is very subtle, but this clause has to be below the check for SkipComment, if we are in a comment we have to check for \n, (and \n isspace and would be skipped by this continue)
		if(isspace(c)) continue; // Always skip whitespace between tokens.
		
		// NOTE: Try to identify the type of the token.
		
		if(c == '\0')     Token.Type = TokenType_EOF;
		else if(c == ':') Token.Type = (token_type)c;    // NOTE: single-character tokens have type values equal to their char value.
		else if(c == '{') Token.Type = (token_type)c;
		else if(c == '}') Token.Type = (token_type)c;
		else if(c == '"') Token.Type = TokenType_QuotedString;
		else if(c == '-' || c == '.' || isdigit(c)) Token.Type = TokenType_Double;  // NOTE: Could also be UInt, Date or Time. That is figured out later when reading it.
		else if(IsIdentifier(c)) Token.Type = TokenType_Identifier;
		else if(c == '#')
		{
			SkipComment = true;
			continue;
		}
		else
		{
			PrintErrorHeader(true);
			FatalError("Found a token of unknown type, starting with: ", c, "\n");
		}
		
		StartLine = Line;
		StartColumn = Column;
		
		Token.StringValue = FileData.Substring(AtChar, 0); // We don't know how long it will be yet, the length will be adjusted as we parse
		
		break;
	}

	if(Token.Type > MobiusMaxMulticharTokenType) //NOTE: We have a single-character token.
	{
		Token.StringValue.Length = 1;
		return;
	}

	PutbackChar();  // Put the char back so that we can start processing it afresh below.
	
	// NOTE: Continue processing multi-character tokens:
	
	if(Token.Type == TokenType_QuotedString)
		ReadString(Token);
	else if(Token.Type == TokenType_Identifier)
		ReadIdentifier(Token);
	else if(Token.Type == TokenType_Double)
		ReadNumber(Token);
}

void
token_stream::ReadString(token &Token)
{
	bool FirstQuotationMark = true;
	
	while(true)
	{
		char c = NextChar();
		Token.StringValue.Length++;
		
		if(c == '"')
		{
			//NOTE: Don't count the quotation marks in the string length or data.
			Token.StringValue.Length--;
			if(FirstQuotationMark)
				Token.StringValue.Data++;
			else
				break;
			FirstQuotationMark = false;
		}	
		else if (c == '\n')
		{
			PrintErrorHeader();
			FatalError("New line before quoted string was closed.\n");
		}
		else if (c == '\0')
		{
			PrintErrorHeader();
			FatalError("End of file before quoted string was closed.\n");
		}
	}
}

void token_stream::ReadIdentifier(token &Token)
{
	while(true)
	{
		char c = NextChar();
		Token.StringValue.Length++;
		
		if(!IsIdentifier(c) && !isdigit(c))
		{
			// NOTE: We assume that the latest read char was the start of another token, so go back one char to make the position correct for the next call to ReadTokenInternal_.
			PutbackChar();
			Token.StringValue.Length--;
			break;
		}
	}
	
	// Check for reserved identifiers that are actually a different token type.
	if(Token.StringValue.Equals("true"))
	{
		Token.Type = TokenType_Bool;
		Token.BoolValue = true;
	}
	else if(Token.StringValue.Equals("false"))
	{
		Token.Type = TokenType_Bool;
		Token.BoolValue = false;
	}
	else if(Token.StringValue.Equals("NaN") || Token.StringValue.Equals("nan") || Token.StringValue.Equals("Nan"))
	{
		Token.Type = TokenType_Double;
		Token.DoubleValue = std::numeric_limits<double>::quiet_NaN();
	}
}

inline bool
AppendDigit(u64 *Number, char Digit)
{
	u64 Addendand = (u64)(Digit - '0');
	constexpr u64 MaxU64 = 0xffffffffffffffff;
	if( (MaxU64 - Addendand) / 10  < *Number) return false;
	*Number = *Number * 10 + Addendand;
	return true;
}

inline bool
AppendDigit(s32 *Number, char Digit)
{
	s32 Addendand = (s32)(Digit - '0');
	constexpr s32 MaxS32 = 2147483647;
	if( (MaxS32 - Addendand) / 10  < *Number) return false;
	*Number = *Number * 10 + Addendand;
	return true;
}

inline double
MakeDoubleFast(u64 Base, s64 Exponent, bool IsNegative, bool *Success)
{
	static const double Pow10[] = {
		1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22, 1e23, 1e24, 1e25, 1e26, 1e27, 1e28, 1e29, 1e30, 1e31, 1e32, 1e33, 1e34, 1e35, 1e36, 1e37, 1e38, 1e39, 1e40, 1e41, 1e42, 1e43, 1e44, 1e45, 1e46, 1e47, 1e48, 1e49, 1e50, 1e51, 1e52, 1e53, 1e54, 1e55, 1e56, 1e57, 1e58, 1e59, 1e60, 1e61, 1e62, 1e63, 1e64, 1e65, 1e66, 1e67, 1e68, 1e69, 1e70, 1e71, 1e72, 1e73, 1e74, 1e75, 1e76, 1e77, 1e78, 1e79, 1e80, 1e81, 1e82, 1e83, 1e84, 1e85, 1e86, 1e87, 1e88, 1e89, 1e90, 1e91, 1e92, 1e93, 1e94, 1e95, 1e96, 1e97, 1e98, 1e99, 1e100, 1e101, 1e102, 1e103, 1e104, 1e105, 1e106, 1e107, 1e108, 1e109, 1e110, 1e111, 1e112, 1e113, 1e114, 1e115, 1e116, 1e117, 1e118, 1e119, 1e120, 1e121, 1e122, 1e123, 1e124, 1e125, 1e126, 1e127, 1e128, 1e129, 1e130, 1e131, 1e132, 1e133, 1e134, 1e135, 1e136, 1e137, 1e138, 1e139, 1e140, 1e141, 1e142, 1e143, 1e144, 1e145, 1e146, 1e147, 1e148, 1e149, 1e150, 1e151, 1e152, 1e153, 1e154, 1e155, 1e156, 1e157, 1e158, 1e159, 1e160, 1e161, 1e162, 1e163, 1e164, 1e165, 1e166, 1e167, 1e168, 1e169, 1e170, 1e171, 1e172, 1e173, 1e174, 1e175, 1e176, 1e177, 1e178, 1e179, 1e180, 1e181, 1e182, 1e183, 1e184, 1e185, 1e186, 1e187, 1e188, 1e189, 1e190, 1e191, 1e192, 1e193, 1e194, 1e195, 1e196, 1e197, 1e198, 1e199, 1e200, 1e201, 1e202, 1e203, 1e204, 1e205, 1e206, 1e207, 1e208, 1e209, 1e210, 1e211, 1e212, 1e213, 1e214, 1e215, 1e216, 1e217, 1e218, 1e219, 1e220, 1e221, 1e222, 1e223, 1e224, 1e225, 1e226, 1e227, 1e228, 1e229, 1e230, 1e231, 1e232, 1e233, 1e234, 1e235, 1e236, 1e237, 1e238, 1e239, 1e240, 1e241, 1e242, 1e243, 1e244, 1e245, 1e246, 1e247, 1e248, 1e249, 1e250, 1e251, 1e252, 1e253, 1e254, 1e255, 1e256, 1e257, 1e258, 1e259, 1e260, 1e261, 1e262, 1e263, 1e264, 1e265, 1e266, 1e267, 1e268, 1e269, 1e270, 1e271, 1e272, 1e273, 1e274, 1e275, 1e276, 1e277, 1e278, 1e279, 1e280, 1e281, 1e282, 1e283, 1e284, 1e285, 1e286, 1e287, 1e288, 1e289, 1e290, 1e291, 1e292, 1e293, 1e294, 1e295, 1e296, 1e297, 1e298, 1e299, 1e300, 1e301, 1e302, 1e303, 1e304, 1e305, 1e306, 1e307, 1e308,
	};
	
	if(Exponent < -308 || Exponent > 308)
	{
		*Success = false;
		return 0.0;
	}
	*Success = true;
	
	// Note: this is the correct approximation if(Exponent >= -22 && Exponent <= 22 && Base <= 9007199254740991)
	// See
	// Clinger WD. How to read floating point numbers accurately.
	// ACM SIGPLAN Notices. 1990	
	
	// For other numbers there is some small error due to rounding of large powers of 10 (in the 15th decimal place).
	// TODO: Maybe get the fast_double_parser library instead?
	// atof is not an option since it is super slow (6 seconds parsing time on some typical input files).

	double Result = (double)Base;
	if(Exponent < 0)
		Result /= Pow10[-Exponent];
	else
		Result *= Pow10[Exponent];
	Result = IsNegative ? -Result : Result;
	return Result;
}

void
token_stream::ReadNumber(token &Token)
{
	s32 NumericPos = 0;
	
	bool IsNegative  = false;
	bool HasComma    = false;
	bool HasExponent = false;
	bool ExponentIsNegative = false;
	u64 Base = 0;
	u64 Exponent = 0;
	s32 DigitsAfterComma = 0;
	
	while(true)
	{
		char c = NextChar();
		Token.StringValue.Length++;
		
		if(c == '-')
		{
			if( (HasComma && !HasExponent) || (IsNegative && !HasExponent) || NumericPos != 0)
			{
				if(!HasComma && !HasExponent)
				{
					//NOTE we have encountered something of the form x- where x is a plain number. Assume it continues on as x-y-z, i.e. this is a date.
					Token.Type = TokenType_Date;
					s32 FirstPart = (s32)Base;
					if(IsNegative) FirstPart = -FirstPart; //Years could be negative (though I doubt that will be used in practice).
					ReadDateOrTime(Token, FirstPart);
					return;
				}
				else
				{
					PrintErrorHeader();
					FatalError("Misplaced minus in numeric literal.\n");
				}
			}
			else
			{
				if(HasExponent)
				{
					if(ExponentIsNegative)
					{
						PrintErrorHeader();
						FatalError("Double minus sign in exponent of numeric literal.\n");
					}
					ExponentIsNegative = true;
				}
				else
				{
					if(IsNegative)
					{
						PrintErrorHeader();
						FatalError("Double minus sign in numeric literal");
					}
					IsNegative = true;
				}
				NumericPos = 0;
			}
		}
		else if(c == ':')
		{	
			if(HasComma || HasExponent || IsNegative)
			{
				PrintErrorHeader();
				FatalError("Mixing numeric notation with time notation.\n");
			}
			//NOTE we have encountered something of the form x: where x is a plain number. Assume it continues on as x:y:z, i.e. this is a time.
			Token.Type = TokenType_Time;
			s32 FirstPart = (s32)Base;
			ReadDateOrTime(Token, FirstPart);
			return;
		}
		else if(c == '+')
		{
			if(!HasExponent || NumericPos != 0)
			{
				PrintErrorHeader();
				FatalError("Misplaced plus in numeric literal.\n");
			}
			//ignore the plus.
		}
		else if(c == '.')
		{
			if(HasExponent)
			{
				PrintErrorHeader();
				FatalError("Decimal separator in exponent in numeric literal.\n");
			}
			if(HasComma)
			{
				PrintErrorHeader();
				FatalError("More than one decimal separator in a numeric literal.\n");
			}
			NumericPos = 0;
			HasComma = true;
		}
		else if(c == 'e' || c == 'E')
		{
			if(HasExponent)
			{
				PrintErrorHeader();
				FatalError("More than one exponent sign ('e' or 'E') in a numeric literal.\n");
			}
			NumericPos = 0;
			HasExponent = true;
		}
		else if(isdigit(c))
		{
			
			if(HasExponent)
				AppendDigit(&Exponent, c);
			else
			{
				if(HasComma)
					DigitsAfterComma++;

				if(!AppendDigit(&Base, c))
				{
					// NOTE: Ideally we should use arbitrary precision integers for Base instead, or shift to it if this happens. When parsing doubles, we should allow higher number of digits.
					PrintErrorHeader();
					FatalError("Overflow in numeric literal (too many digits). If this is a double, try to use scientific notation instead.\n");
				}
			}
			++NumericPos;
		}
		else
		{
			// NOTE: We assume that the latest read char was the start of another token, so go back one char to make the position correct for the next call to ReadTokenInternal_.
			PutbackChar();
			Token.StringValue.Length--;
			break;
		}
	}
	
	if(!HasComma && !HasExponent && !IsNegative)
	{
		Token.Type = TokenType_UInt;
		Token.UIntValue = Base;
	}
	else
	{
		s64 SignedExponent = ExponentIsNegative ? -(s64)Exponent : (s64)Exponent;
		SignedExponent -= DigitsAfterComma;
		bool Success;
		Token.DoubleValue = MakeDoubleFast(Base, SignedExponent, IsNegative, &Success);
		if(!Success)  
		{
			PrintErrorHeader();
			FatalError("Numeric overflow when parsing number.\n");
		}
	}
}

void
token_stream::ReadDateOrTime(token &Token, s32 FirstPart)
{
	char Separator = '-';
	const char *Format = "YYYY-MM-DD";
	if(Token.Type == TokenType_Time)
	{
		Separator = ':';
		Format = "hh:mm:ss";
	}
	
	s32 DatePos = 1;
	s32 Date[3] = {FirstPart, 0, 0};
	
	while(true)
	{
		char c = NextChar();
		Token.StringValue.Length++;	
			
		if(c == Separator)
		{
			++DatePos;
			if(DatePos == 3)
			{
				PrintErrorHeader();
				FatalError("Too many '", Separator, "' signs in date or time literal.\n");
				// or we could assume this is the beginning of a new token, i.e. "1997-8-1-5" is interpreted as the two tokens "1997-8-1" "-5". Don't know what is best.
			}
		}
		else if(isdigit(c))
		{
			if(!AppendDigit(&Date[DatePos], c))
			{
				PrintErrorHeader();
				FatalError("Overflow in numeric literal (too many digits).\n");
			}
		}
		else
		{
			// NOTE: We assume that the latest read char was the start of another token, so go back one char to make the position correct for the next call to ReadTokenInternal_.
			PutbackChar();
			Token.StringValue.Length--;
			break;
		}
	}
	
	if(DatePos != 2)
	{
		PrintErrorHeader();
		FatalError("Invalid ", TokenTypeName(Token.Type), " literal. It must be on the form ", Format, ".\n");
	}
	bool Success;
	if(Token.Type == TokenType_Date)
		Token.DateValue = datetime(Date[0], Date[1], Date[2], &Success);
	else
	{
		Token.DateValue = datetime();
		Success = Token.DateValue.AddHourMinuteSecond(Date[0], Date[1], Date[2]);
	}
	
	if(!Success)
	{
		PrintErrorHeader();
		FatalError("The ", TokenTypeName(Token.Type), " ", Token.StringValue, " does not exist.\n");
	}
}


double token_stream::ExpectDouble()
{
	token Token = ReadToken();
	if(!IsNumeric(Token.Type))
	{
		PrintErrorHeader();
		FatalError("Expected a number, got ", TokenTypeArticle(Token.Type), " ", TokenTypeName(Token.Type), ".\n");
	}
	return Token.GetDoubleValue();
}

u64 token_stream::ExpectUInt()
{
	token Token = ExpectToken(TokenType_UInt);
	return Token.UIntValue;
}

bool token_stream::ExpectBool()
{
	token Token = ExpectToken(TokenType_Bool);
	return Token.BoolValue;
}

datetime token_stream::ExpectDateTime()
{
	token DateToken = ExpectToken(TokenType_Date);
	token PotentialTimeToken = PeekToken();
	if(PotentialTimeToken.Type == TokenType_Time)
	{
		ReadToken();
		DateToken.DateValue.SecondsSinceEpoch += PotentialTimeToken.DateValue.SecondsSinceEpoch;
	}
	return DateToken.DateValue;
}

token_string token_stream::ExpectQuotedString()
{
	token Token = ExpectToken(TokenType_QuotedString);
	return Token.StringValue;
}

token_string token_stream::ExpectIdentifier()
{
	token Token = ExpectToken(TokenType_Identifier);
	return Token.StringValue;
}

void
token_stream::ReadQuotedStringList(std::vector<token_string> &ListOut)
{
	ExpectToken('{');
	while(true)
	{
		token Token = ReadToken();
		
		if(Token.Type == TokenType_QuotedString)
			ListOut.push_back(Token.StringValue);
		else if(Token.Type == '}')
			break;
		else if(Token.Type == TokenType_EOF)
		{
			PrintErrorHeader();
			FatalError("End of file before list was ended.\n");
		}
		else
		{
			PrintErrorHeader();
			FatalError("Unexpected token.\n");
		}
	}
}

void
token_stream::ReadParameterSeries(std::vector<parameter_value> &ListOut, const parameter_spec &Spec)
{
	parameter_value Value;
	
	if(Spec.Type == ParameterType_Double)
	{
		while(true)
		{
			token Token = PeekToken();
			if(!IsNumeric(Token.Type)) break;
			Value.ValDouble = ExpectDouble();
			ListOut.push_back(Value);
		}
	}
	else if(Spec.Type == ParameterType_UInt)
	{
		while(true)
		{
			token Token = PeekToken();
			if(!IsNumeric(Token.Type)) break;
			Value.ValUInt = ExpectUInt();
			ListOut.push_back(Value);
		}
	}
	else if(Spec.Type == ParameterType_Bool)
	{
		while(true)
		{
			token Token = PeekToken();
			if(Token.Type != TokenType_Bool) break;
			Value.ValBool = ExpectBool();
			ListOut.push_back(Value);
		}
	}
	else if(Spec.Type == ParameterType_Time)
	{
		while(true)
		{
			token Token = PeekToken();
			if(Token.Type != TokenType_Date) break;
			Value.ValTime = ExpectDateTime();
			ListOut.push_back(Value);
		}
	}
	else if (Spec.Type == ParameterType_Enum)
	{
		while(true)
		{
			token Token = PeekToken();
			if(Token.Type != TokenType_Identifier) break;
			token_string Val = ExpectIdentifier();
			auto Find = Spec.EnumNameToValue.find(Val);
			if(Find == Spec.EnumNameToValue.end())
				FatalError("ERROR: The parameter \"", Spec.Name, "\" does not have a possible enum value called \"", Val, "\".\n");
			Value.ValUInt = Find->second;
			ListOut.push_back(Value);
		}
	}
	else assert(0);  //NOTE: This should be caught by the library implementer. Signifies that this was called with possibly a new type that is not handled yet?
}
