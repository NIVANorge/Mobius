
enum token_type
{
	TokenType_Unknown = 0,
	TokenType_UnquotedString,
	TokenType_QuotedString,
	TokenType_Colon,
	TokenType_OpenBrace,
	TokenType_CloseBrace,
	TokenType_Numeric,
	TokenType_Bool,
	TokenType_Date,
	TokenType_Time,
	TokenType_EOF,
};

//NOTE: WARNING: This has to match the token_type enum!!!
const char *TokenNames[11] =
{
	"(unknown)",
	"unquoted string",
	"quoted string",
	":",
	"{",
	"}",
	"number",
	"boolean",
	"date",
	"time",
	"(end of file)",
};

struct token
{
	token_type Type;
	token_string StringValue;

	//TODO: It is tempting to put these in a union, but we can't. For some applications it has to store both the uint and double values separately. This is because we can't determine at the lexer stage whether the reader wants a double or uint (and the bit patterns of a double and a u64 don't encode the same number).
	u64 UIntValue;
	double DoubleValue;
	bool BoolValue;
	datetime DateValue;

	bool IsUInt;
};

struct token_stream
{
	token_stream(const char *Filename)
	{
		FileData = 0;
		FileDataLength = 0;
		this->Filename = Filename;
		FILE *File;
#ifdef _WIN32
		std::u16string Filename16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(Filename);
		File = _wfopen((wchar_t *)Filename16.data(), L"rb");
#else
		File = fopen(Filename, "rb");
#endif
		if(!File)
			FatalError("ERROR: Tried to open file ", Filename, ", but was not able to.\n");

		fseek(File, 0, SEEK_END);
		FileDataLength = ftell(File);
		fseek(File, 0, SEEK_SET);
		
		if(FileDataLength == 0)
		{
			fclose(File);
			FatalError("ERROR: File ", Filename, " has 0 length.\n");
		}
		
		FileData = (char *)malloc(FileDataLength + 1);
		if(FileData)
		{
			size_t ReadSize = fread(FileData, 1, FileDataLength, File);
			if(ReadSize != FileDataLength)
			{
				FatalError("ERROR: Was unable to read the entire file ", Filename);
			}
			FileData[FileDataLength] = '\0';
		}
		fclose(File);
		
		if(!FileData)
			FatalError("Unable to allocate enough memory to read in file ", Filename, '\n');
		
		AtChar = -1;
		
		//NOTE: In case the file has a BOM mark
		if(FileDataLength >= 3)
		{
			if(
				   FileData[0] == (char) 0xEF
				&& FileData[1] == (char) 0xBB
				&& FileData[2] == (char) 0xBF
			)
			{
				AtChar = 2;
			}
		}
		
		StartLine = 0; StartColumn = 0; Line = 0; Column = 0; PreviousColumn = 0;
		AtToken = -1;
		
		Tokens.reserve(500); //NOTE: This will usually speed things up.
	}
	
	~token_stream()
	{
		if(FileData) free(FileData);
	}
	
	token ReadToken();
	token PeekToken(size_t PeekAhead = 1);
	token ExpectToken(token_type);
	
	double   ExpectDouble();
	u64      ExpectUInt();
	bool     ExpectBool();
	datetime ExpectDateTime();
	token_string ExpectQuotedString();
	token_string ExpectUnquotedString();
	
	void ReadQuotedStringList(std::vector<token_string> &ListOut);
	void ReadQuotedStringList(std::vector<const char *> &ListOut);
	void ReadDoubleSeries(std::vector<double> &ListOut);
	void ReadParameterSeries(std::vector<parameter_value> &ListOut, const parameter_spec &Spec);
	
	void PrintErrorHeader(bool CurrentColumn=false);

	const char *Filename;

private:
	s32 StartLine;
	s32 StartColumn;
	s32 Line;
	s32 Column;
	s32 PreviousColumn;
	
	bool AtEnd = false;
	
	char   *FileData;
	size_t FileDataLength;
	s64    AtChar;
	
	std::vector<token> Tokens;   //TODO: It is not really necessary to store all the tokens we have read, only the ones that are "ahead" and have not been consumed yet.
	s64 AtToken;
	
	void ReadTokenInternal_();
	
};

void
token_stream::PrintErrorHeader(bool CurrentColumn)
{
	u32 Col = StartColumn;
	if(CurrentColumn) Col = Column;
	ErrorPrint("ERROR: In file ", Filename, " line ", (StartLine+1), " column ", Col, ": ");
}

token
token_stream::ReadToken()
{
	AtToken++;
	if(AtToken >= (s64)Tokens.size())
	{
		ReadTokenInternal_();
	}

	return Tokens[AtToken];
}

token
token_stream::PeekToken(size_t PeekAhead)
{
	while(AtToken + PeekAhead >= Tokens.size())
	{
		ReadTokenInternal_();
	}
	return Tokens[AtToken + PeekAhead];
}

token
token_stream::ExpectToken(token_type Type)
{
	token Token = ReadToken();
	if(Token.Type != Type)
	{
		PrintErrorHeader();
		ErrorPrint("Expected a token of type ", TokenNames[Type], ", got a(n) ", TokenNames[Token.Type]);
		if(Token.Type == TokenType_QuotedString || Token.Type == TokenType_UnquotedString)
			ErrorPrint(" (", Token.StringValue, ")");
		FatalError('\n');
	}
	return Token;
}

static bool
IsIdentifier(char c)
{
	return isalpha(c) || c == '_';
}

static bool
MultiplyByTenAndAdd(u64 *Number, u64 Addendand)
{
	u64 MaxU64 = 0xffffffffffffffff;
	if( (MaxU64 - Addendand) / 10  < *Number) return false;
	*Number = *Number * 10 + Addendand;
	return true;
}

static bool
MultiplyByTenAndAdd(s32 *Number, s32 Addendand)
{
	s32 MaxS32 = 2147483647;
	if( (MaxS32 - Addendand) / 10  < *Number) return false;
	*Number = *Number * 10 + Addendand;
	return true;
}

void
token_stream::ReadTokenInternal_()
{
	//TODO: we should do heavy cleanup of this function, especially the floating point reading.
	
	Tokens.resize(Tokens.size()+1, {});
	token &Token = Tokens[Tokens.size()-1];
	
	if(AtEnd)
	{
		Token.Type = TokenType_EOF;
		return;
	}
	
	s32 NumericPos = 0;
	bool IsNegative  = false;
	bool HasComma    = false;
	bool HasExponent = false;
	bool ExponentIsNegative = false;
	u64 BeforeComma = 0;
	u64 AfterComma = 0;
	u64 Exponent = 0;
	u64 DigitsAfterComma = 0;
	
	s32 Date[3] = {0, 0, 0};
	u32 DatePos = 0;
	
	
	bool TokenHasStarted = false;
	
	bool SkipLine = false;
	
	while(true)
	{
		++AtChar;
		
		char c = FileData[AtChar];
		
		if(c == EOF || c == '\0')
		{
			if(!TokenHasStarted)
			{
				Token.Type = TokenType_EOF;
				AtEnd = true;
				return;
			}
			//break;
		}
		
		if(c == '\n')
		{
			++Line;
			PreviousColumn = Column;
			Column = 0;
		}
		else ++Column;
		
		if(SkipLine)
		{
			if(c == '\n') SkipLine = false;
			continue;
		}
		
		if(!TokenHasStarted && isspace(c)) continue;
		
		if(!TokenHasStarted)
		{
			if(c == ':') Token.Type = TokenType_Colon;
			else if(c == '{') Token.Type = TokenType_OpenBrace;
			else if(c == '}') Token.Type = TokenType_CloseBrace;
			else if(c == '"') Token.Type = TokenType_QuotedString;
			else if(c == '-' || c == '.' || isdigit(c)) Token.Type = TokenType_Numeric;
			else if(IsIdentifier(c)) Token.Type = TokenType_UnquotedString;
			else if(c == '#')
			{
				SkipLine = true;
				continue;
			}
			else
			{
				PrintErrorHeader(true);
				FatalError("Found a token of unknown type.\n");
			}
			TokenHasStarted = true;
			StartLine = Line;
			StartColumn = Column;
			
			Token.StringValue.Data = FileData + AtChar;
			Token.StringValue.Length = 1;
			
			if(Token.Type == TokenType_QuotedString)
			{
				//NOTE: For quoted strings we don't want to record the actual " symbol into the sting value.
				Token.StringValue.Data = FileData + AtChar + 1;
				Token.StringValue.Length = 0;
			}
		}
		else
		{
			Token.StringValue.Length++;
		}
		
		if(Token.Type == TokenType_Colon || Token.Type == TokenType_OpenBrace || Token.Type == TokenType_CloseBrace)
		{
			//NOTE: These tokens are always one character only
			return;
		}
		
		if(Token.Type == TokenType_QuotedString)
		{
			if(c == '"' && Token.StringValue.Length > 0)
			{
				Token.StringValue.Length--; //NOTE: Don't record the " in the string value.
				
				//std::cout << Token.StringValue << std::endl;
				
				return;
			}
			else if (c != '"' && c == '\n')
			{
				PrintErrorHeader();
				FatalError("Newline within quoted string.\n");
			}
		}
		else if(Token.Type == TokenType_UnquotedString)
		{
			if(!IsIdentifier(c))
			{
				// NOTE: We assume that the latest read char was the start of another token, so go back one char to make the position correct for the next call to ReadTokenInternal_.
				if(c == '\n')
				{
					Line--;
					Column = PreviousColumn;
				}
				else
					Column--;
				
				Token.StringValue.Length--;
				AtChar--;
				
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
					Token.Type = TokenType_Numeric;
					Token.DoubleValue = std::numeric_limits<double>::quiet_NaN();
				}
				//else std::cout << Token.StringValue << std::endl;
				
				return;
			}
		}
		else if(Token.Type == TokenType_Numeric)
		{
			if(c == '-')
			{
				//TODO: Should we have a guard against two - signs following each other?
				
				if( (HasComma && !HasExponent) || (IsNegative && !HasExponent) || NumericPos != 0)
				{
					if(!HasComma && !HasExponent)
					{
						//NOTE we have encountered something of the form x- where x is a plain number. Assume it continues on as x-y-z
						Token.Type = TokenType_Date;
						Date[0] = (s32)BeforeComma;
						if(IsNegative) Date[0] = -Date[0];
						DatePos = 1;
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
						ExponentIsNegative = true;
					}
					else
					{
						IsNegative = true;
					}
					NumericPos = 0;
				}
			}
			else if(c == ':')
			{
				Token.Type = TokenType_Time;
				Date[0] = (s32)BeforeComma;
				DatePos = 1;
				if(HasComma || HasExponent || IsNegative)
				{
					PrintErrorHeader();
					FatalError("Mixing numeric notation with time notation.\n");
				}
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
					FatalError("Comma in exponent in numeric literal.\n");
				}
				if(HasComma)
				{
					PrintErrorHeader();
					FatalError("More than one comma in a numeric literal.\n");
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
				{
					MultiplyByTenAndAdd(&Exponent, (u64)(c - '0'));
					u64 MaxExponent = 308; //NOTE: This is not a really thorough test, because we could overflow still..
					if(Exponent > MaxExponent)
					{
						PrintErrorHeader();
						FatalError("Too large exponent in numeric literal.\n");
					}
				}
				else if(HasComma)
				{
					if(!MultiplyByTenAndAdd(&AfterComma, (u64)(c - '0')))
					{
						PrintErrorHeader();
						FatalError("Overflow after comma in numeric literal (too many digits).\n");
					}
					DigitsAfterComma++;
				}
				else
				{
					if(!MultiplyByTenAndAdd(&BeforeComma, (u64)(c - '0')))
					{
						PrintErrorHeader();
						FatalError("Overflow in numeric literal (too many digits). If this is a double, try to use scientific notation instead.\n");
					}
				}
				++NumericPos;
			}
			else
			{
				// NOTE: We assume that the latest read char was the start of another token, so go back one char to make the position correct for the next call to ReadTokenInternal_.
				if(c == '\n')
				{
					Line--;
					Column = PreviousColumn;
				}
				else
					Column--;
				
				Token.StringValue.Length--;
				AtChar--;
				
				break;
			}
		}
		else if(Token.Type == TokenType_Date || Token.Type == TokenType_Time)
		{
			char Separator = Token.Type == TokenType_Date ? '-' : ':';
			
			if(c == Separator)
			{
				++DatePos;
				if(DatePos == 3)
				{
					PrintErrorHeader();
					FatalError("Too many '", Separator, "' signs in date or time literal.\n");
				}
			}
			else if(isdigit(c))
			{
				if(!MultiplyByTenAndAdd(&Date[DatePos], (u64)(c - '0')))
				{
					PrintErrorHeader();
					FatalError("Overflow in numeric literal (too many digits).\n");
				}
			}
			else
			{
				// NOTE: We assume that the latest read char was the start of another token, so go back one char to make the position correct for the next call to ReadTokenInternal_.
				if(c == '\n')
				{
					Line--;
					Column = PreviousColumn;
				}
				else
					Column--;
				
				Token.StringValue.Length--;
				AtChar--;
				
				break;
			}
		}
	}
	
	if(Token.Type == TokenType_Numeric)
	{
		if(!HasComma && !HasExponent && !IsNegative)
		{
			Token.IsUInt = true;
		}
		
		Token.UIntValue = BeforeComma;
		
		//NOTE: Alternatively, we could also just use sscanf(Token.StringValue, "%f", &Token.DoubleValue), which may do a better job at this than us..
		double BC = (double)BeforeComma;
		double AC = (double)AfterComma;
		for(size_t I = 0; I < DigitsAfterComma; ++I) AC *= 0.1;
		Token.DoubleValue = BC + AC;
		if(IsNegative) Token.DoubleValue = -Token.DoubleValue;
		if(HasExponent)
		{
			double Multiplier = ExponentIsNegative ? 0.1 : 10.0;
			for(u64 Ex = 0; Ex < Exponent; ++Ex)
			{
				Token.DoubleValue *= Multiplier;
			}
		}
	}
	
	if(Token.Type == TokenType_Date || Token.Type == TokenType_Time)
	{
		if(DatePos != 2)
		{
			PrintErrorHeader();
			FatalError("Invalid date (time) literal. Has to be on the form YYYY-MM-DD (hh:mm:ss).\n");
		}
		bool Success;
		if(Token.Type == TokenType_Date)
		{
			Token.DateValue = datetime(Date[0], Date[1], Date[2], &Success);
		}
		else
		{
			Token.DateValue = datetime();
			Success = Token.DateValue.AddHourMinuteSecond(Date[0], Date[1], Date[2]);
		}
		if(!Success)
		{
			PrintErrorHeader();
			FatalError("The date or time ", Token.StringValue, " does not exist.\n");
		}
	}
	
	return;
}

double token_stream::ExpectDouble()
{
	token Token = ExpectToken(TokenType_Numeric);
	return Token.DoubleValue;
}

u64 token_stream::ExpectUInt()
{
	token Token = ExpectToken(TokenType_Numeric);
	if(!Token.IsUInt)
	{
		PrintErrorHeader();
		FatalError("Got a value that is signed, with a comma or with an exponent when expecting an unsigned integer.\n");
	}
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

token_string token_stream::ExpectUnquotedString()
{
	token Token = ExpectToken(TokenType_UnquotedString);
	return Token.StringValue;
}

void
token_stream::ReadQuotedStringList(std::vector<token_string> &ListOut)
{
	ExpectToken(TokenType_OpenBrace);
	while(true)
	{
		token Token = ReadToken();
		
		if(Token.Type == TokenType_QuotedString)
		{
			ListOut.push_back(Token.StringValue);
		}
		else if(Token.Type == TokenType_CloseBrace)
		{
			break;
		}
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

/*
//NOTE: removed this for now as we don't use it in active code, and it leaks. Should take a bucket allocator for the Copy()

void
token_stream::ReadQuotedStringList(std::vector<const char *> &ListOut)
{
	//NOTE: This one makes a copy of the strings.
	ExpectToken(TokenType_OpenBrace);
	while(true)
	{
		token Token = ReadToken();
		
		if(Token.Type == TokenType_QuotedString)
		{
			ListOut.push_back(Token.StringValue.Copy().Data);
		}
		else if(Token.Type == TokenType_CloseBrace)
		{
			break;
		}
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
*/

void
token_stream::ReadDoubleSeries(std::vector<double> &ListOut)
{
	while(true)
	{
		token Token = PeekToken();
		if(Token.Type != TokenType_Numeric) break;
		ListOut.push_back(ExpectDouble());
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
			if(Token.Type != TokenType_Numeric) break;
			Value.ValDouble = ExpectDouble();
			ListOut.push_back(Value);
		}
	}
	else if(Spec.Type == ParameterType_UInt)
	{
		while(true)
		{
			token Token = PeekToken();
			if(Token.Type != TokenType_Numeric) break;
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
			if(Token.Type != TokenType_UnquotedString) break;
			token_string Val = ExpectUnquotedString();
			auto Find = Spec.EnumNameToValue.find(Val);
			if(Find == Spec.EnumNameToValue.end())
				FatalError("ERROR: The parameter \"", Spec.Name, "\" does not have a possible enum value called \"", Val, "\".\n");
			Value.ValUInt = Find->second;
			ListOut.push_back(Value);
		}
	}
	else assert(0);  //NOTE: This should be caught by the library implementer. Signifies that this was called with possibly a new type that is not handled yet?
}
