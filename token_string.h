



//NOTE: This is a length-based string that does not have ownership of its data. Good for making substrings without having to reallocate.

struct token_string
{
	token_string() : Data(0), Length(0) {};
	token_string(const char *);
	
	const char *Data;
	size_t Length;
	
	bool Equals(const char *) const;
	token_string Copy() const;
};

token_string::token_string(const char *DataIn)
{
	Length = strlen(DataIn);
	Data = DataIn;
}

std::ostream& operator<<(std::ostream& Os, const token_string& Str)
{
	for(size_t At = 0; At < Str.Length; ++At)
	{
		Os << Str.Data[At];
	}
	return Os;
}

bool operator==(const token_string &StrA, const token_string &StrB)
{
	if(StrA.Length != StrB.Length) return false;
	for(size_t At = 0; At < StrA.Length; ++At)
	{
		if(StrA.Data[At] != StrB.Data[At]) return false;
	}
	return true;
}

bool token_string::Equals(const char *Str) const
{
	for(size_t At = 0; At < Length; ++At)
	{
		if(Str[At] == 0) return false;
		if(Str[At] != Data[At]) return false;
	}
	if(Str[Length] != 0) return false;
	return true;
}

token_string token_string::Copy() const
{
	token_string Result;
	char *NewData = (char *)malloc(Length + 1);
	Result.Data = NewData;
	Result.Length = Length;
	NewData[Length] = '\0'; //NOTE: In case people want a 0-terminated string
	for(size_t At = 0; At < Length; ++At)
	{
		NewData[At] = Data[At];
	}
	return Result;
}

//TODO: Borrowed hash function from https://stackoverflow.com/questions/20649864/c-unordered-map-with-char-as-key . We should look into it more..
struct token_string_hash_function
{
    //BKDR Hash algorithm
    int operator()(const token_string &Str) const
    {
        int Seed = 131;//31  131 1313 13131131313 etc//
        int Hash = 0;
		for(size_t At = 0; At < Str.Length; ++At)
        {
            Hash = (Hash * Seed) + Str.Data[At];
        }
        return Hash & (0x7FFFFFFF);
    }
};