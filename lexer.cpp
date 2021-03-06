
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
	TokenType_EOF,
};

//NOTE: WARNING: This has to match the token_type enum!!!
const char *TokenNames[9] =
{
	"(unknown)",
	"unquoted string",
	"quoted string",
	":",
	"{",
	"}",
	"number",
	"boolean",
	"(end of file)",
};

struct token
{
	//TODO: putting these in a union breaks something. Can we find out what?
	token_type Type;
	token_string StringValue;
	//union
	//{
	u64 UIntValue;
	double DoubleValue;
	bool BoolValue;
	//};
	bool IsUInt;
};

struct token_stream
{
	token_stream(const char *Filename)
	{
		FileData = 0;
		FileDataLength = 0;
		this->Filename = Filename;
		FILE *File = fopen(Filename, "rb");
		if(!File)
		{
			MOBIUS_FATAL_ERROR("ERROR: Tried to open file " << Filename << ", but was not able to.");
		}
		fseek(File, 0, SEEK_END);
		FileDataLength = ftell(File);
		fseek(File, 0, SEEK_SET);
		
		if(FileDataLength == 0)
		{
			fclose(File);
			MOBIUS_FATAL_ERROR("ERROR: File " << Filename << " has 0 length.");
		}
		
		FileData = (char *)malloc(FileDataLength + 1);
		if(FileData)
		{
			size_t ReadSize = fread(FileData, 1, FileDataLength, File);
			if(ReadSize != FileDataLength)
			{
				MOBIUS_FATAL_ERROR("ERROR: Was unable to read the entire file " << Filename);
			}
			FileData[FileDataLength] = '\0';
		}
		fclose(File);
		
		if(!FileData)
		{
			MOBIUS_FATAL_ERROR("Unable to allocate enough memory to read in file " << Filename << std::endl);
		}
		
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
	
	//NOTE! Any pointer to a previously read token may be invalidated if you read or peek a new one.
	token ReadToken();
	token PeekToken(size_t PeekAhead = 1);
	token ExpectToken(token_type);
	
	double   ExpectDouble();
	u64      ExpectUInt();
	bool     ExpectBool();
	datetime ExpectDate();
	token_string ExpectQuotedString();
	token_string ExpectUnquotedString();
	
	void ReadQuotedStringList(std::vector<token_string> &ListOut);
	void ReadQuotedStringList(std::vector<const char *> &ListOut);
	void ReadDoubleSeries(std::vector<double> &ListOut);
	void ReadParameterSeries(std::vector<parameter_value> &ListOut, parameter_type Type);
	
	void PrintErrorHeader(bool CurrentColumn=false);

	const char *Filename;

private:
	s32 StartLine;
	s32 StartColumn;
	s32 Line;
	s32 Column;
	s32 PreviousColumn;
	
	bool AtEnd = false;
	
	//FILE *File;
	char   *FileData;
	size_t FileDataLength;
	s64    AtChar;
	
	std::vector<token> Tokens;
	s64 AtToken;
	
	void ReadTokenInternal_();
	
};

void
token_stream::PrintErrorHeader(bool CurrentColumn)
{
	u32 Col = StartColumn;
	if(CurrentColumn) Col = Column;
	MOBIUS_PARTIAL_ERROR("ERROR: In file " << Filename << " line " << (StartLine+1) << " column " << (Col) << ": ");
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
		MOBIUS_PARTIAL_ERROR("Expected a token of type " << TokenNames[Type] << ", got a(n) " << TokenNames[Token.Type]);
		if(Token.Type == TokenType_QuotedString || Token.Type == TokenType_UnquotedString)
		{
			MOBIUS_PARTIAL_ERROR(" (" << Token.StringValue << ")");
		}
		MOBIUS_FATAL_ERROR(std::endl);
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
	
	//assert(Token.StringValue.Length == 0);
	
	s32 NumericPos = 0;
	bool IsNegative  = false;
	bool HasComma    = false;
	bool HasExponent = false;
	//bool IsNAN       = false;
	bool ExponentIsNegative = false;
	u64 BeforeComma = 0;
	u64 AfterComma = 0;
	u64 Exponent = 0;
	u64 DigitsAfterComma = 0;
	
	
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
				MOBIUS_FATAL_ERROR("Found a token of unknown type" << std::endl);
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
			else if (c != '"')
			{
				if(c == '\n')
				{
					PrintErrorHeader();
					MOBIUS_FATAL_ERROR("Newline within quoted string." << std::endl);
				}
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
				if( (HasComma && !HasExponent) || (IsNegative && !HasExponent) || NumericPos != 0)
				{
					PrintErrorHeader();
					MOBIUS_FATAL_ERROR("Misplaced minus in numeric literal." << std::endl);
				}
				
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
			else if(c == '+')
			{
				if(!HasExponent || NumericPos != 0)
				{
					PrintErrorHeader();
					MOBIUS_FATAL_ERROR("Misplaced plus in numeric literal." << std::endl);
				}
				//ignore the plus.
			}
			else if(c == '.')
			{
				if(HasExponent)
				{
					PrintErrorHeader();
					MOBIUS_FATAL_ERROR("Comma in exponent in numeric literal." << std::endl);
				}
				if(HasComma)
				{
					PrintErrorHeader();
					MOBIUS_FATAL_ERROR("More than one comma in a numeric literal." << std::endl);
				}
				NumericPos = 0;
				HasComma = true;
			}
			else if(c == 'e' || c == 'E')
			{
				if(HasExponent)
				{
					PrintErrorHeader();
					MOBIUS_FATAL_ERROR("More than one exponent sign ('e' or 'E') in a numeric literal." << std::endl);
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
						MOBIUS_FATAL_ERROR("Too large exponent in numeric literal" << std::endl);
					}
				}
				else if(HasComma)
				{
					if(!MultiplyByTenAndAdd(&AfterComma, (u64)(c - '0')))
					{
						PrintErrorHeader();
						MOBIUS_FATAL_ERROR("Numeric overflow after comma in numeric literal (too many digits)." << std::endl);
					}
					DigitsAfterComma++;
				}
				else
				{
					if(!MultiplyByTenAndAdd(&BeforeComma, (u64)(c - '0')))
					{
						PrintErrorHeader();
						MOBIUS_FATAL_ERROR("Numeric overflow in numeric literal (too many digits). If this is a double, try to use scientific notation instead." << std::endl);
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
	
	//std::cout << StartLine << ": " << Token.StringValue << std::endl;
	
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
		MOBIUS_FATAL_ERROR("Got a signed value when expecting an unsigned integer." << std::endl);
	}
	return Token.UIntValue;
}

bool token_stream::ExpectBool()
{
	token Token = ExpectToken(TokenType_Bool);
	return Token.BoolValue;
}

datetime token_stream::ExpectDate()
{
	token_string DateStr = ExpectQuotedString();
	bool ParseSuccess;
	datetime Date(DateStr.Data, &ParseSuccess);
	if(!ParseSuccess)
	{
		PrintErrorHeader();
		MOBIUS_FATAL_ERROR("Unrecognized date format \"" << DateStr << "\". Supported format: Y-m-d" << std::endl);
	}
	return Date;
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
			MOBIUS_FATAL_ERROR("End of file before list was ended." << std::endl);
		}
		else
		{
			PrintErrorHeader();
			MOBIUS_FATAL_ERROR("Unexpected token." << std::endl);
		}
	}
}

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
			MOBIUS_FATAL_ERROR("End of file before list was ended." << std::endl);
		}
		else
		{
			PrintErrorHeader();
			MOBIUS_FATAL_ERROR("Unexpected token." << std::endl);
		}
	}
}

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
token_stream::ReadParameterSeries(std::vector<parameter_value> &ListOut, parameter_type Type)
{
	parameter_value Value;
	
	if(Type == ParameterType_Double)
	{
		while(true)
		{
			token Token = PeekToken();
			if(Token.Type != TokenType_Numeric) break;
			Value.ValDouble = ExpectDouble();
			ListOut.push_back(Value);
		}
	}
	else if(Type == ParameterType_UInt)
	{
		while(true)
		{
			token Token = PeekToken();
			if(Token.Type != TokenType_Numeric) break;
			Value.ValUInt = ExpectUInt();
			ListOut.push_back(Value);
		}
	}
	else if(Type == ParameterType_Bool)
	{
		while(true)
		{
			token Token = PeekToken();
			if(Token.Type != TokenType_Bool) break;
			Value.ValBool = ExpectBool();
			ListOut.push_back(Value);
		}
	}
	else assert(0);  //NOTE: This should be caught by the library implementer. Signifies that this was called with either a Type==ParameterType_Time or a possibly new type that is not handled yet?
	
	//NOTE: Date values have to be handled separately since we can't distinguish them from quoted strings...
	// TODO: Make separate format for dates?
}
