
FILE *OpenFile(const char *Filename, const char *Mode)
{
	// Wrapper to allow for non-ascii names on Windows. Assumes Filename is UTF8 formatted.
	
	FILE *File;
#ifdef _WIN32
	std::u16string Filename16 = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(Filename);
	std::u16string Mode16     = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(Mode);
	File = _wfopen((wchar_t *)Filename16.data(), (wchar_t *)Mode16.data());
#else
	File = fopen(Filename, Mode);
#endif

	if(!File)
		FatalError("ERROR: Tried to open file \"", Filename, "\", but was not able to.\n");

	return File;
}

static token_string
ReadEntireFile(const char *Filename)
{
	token_string FileData = {};
	
	FILE *File = OpenFile(Filename, "rb");

	fseek(File, 0, SEEK_END);
	FileData.Length = ftell(File);
	fseek(File, 0, SEEK_SET);
	
	if(FileData.Length == 0)
	{
		fclose(File);
		FatalError("ERROR: File ", Filename, " has 0 length.\n");
	}
	
	char *Data = (char *)malloc(FileData.Length + 1);
	
	if(!Data)
		FatalError("Unable to allocate enough memory to read in file ", Filename, '\n');

	size_t ReadSize = fread((void *)Data, 1, FileData.Length, File); // NOTE: Casting away constness, but it doesn't matter since we just allocated it ourselves.
	fclose(File);
	
	if(ReadSize != FileData.Length)
	{
		free((void *)Data);
		FatalError("ERROR: Was unable to read the entire file ", Filename);
	}
	
	Data[FileData.Length] = '\0';    // Zero-terminate it in case we want to interface with C libraries.
	FileData.Data = Data;
	
	return FileData;
}