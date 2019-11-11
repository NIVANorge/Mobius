


/*
	This is an allocator useful for doing temporary allocation of memory, where you don't want to deallocate until you are finished with everything.
	
	It does not use multiple bucket sizes like a proper "bucket allocator", I just didn't find any other good name for it.
	It is usually best to set the bucket size so high that you don't need to allocate many buckets in the common use case.
	
	Note, this is a very memory-inefficient implementation if it overflows the buckets a lot, as it does not go back to earlier buckets to see if they have space for a smaller allocation if they have overflowed once. So you typically want large bucket sizes compared to the allocation sizes.
*/

struct memory_bucket
{
	u8 *Data;
	memory_bucket *Next;
};

struct bucket_allocator
{
	size_t BucketSize;
	size_t CurrentUsed;

	memory_bucket *First;
	memory_bucket *Current;
	
	bucket_allocator() : BucketSize(0), CurrentUsed(0), First(nullptr), Current(nullptr)
	{
	}
	
	void
	Initialize(size_t BucketSize)
	{
		if(this->BucketSize != 0)
		{
			MOBIUS_FATAL_ERROR("ERROR (internal): Tried to initialize a bucket allocator twice.\n");
		}
		
		//TODO: Round up to multiple of 8?
		
		this->BucketSize = BucketSize;
	}
	
	template<typename T> T*
	Allocate(size_t Count)
	{
		if(Count == 0) return nullptr;
		
		if(BucketSize == 0)
		{
			MOBIUS_FATAL_ERROR("ERROR (internal): Tried to allocate from an uninitialized bucket allocator.\n");
		}
		
		size_t RequestedSize = sizeof(T) * Count;
		//NOTE: Pad to 64-bit aligned just to be safe. Not sure how important this is though.
		size_t Lack = 8 - RequestedSize % 8;
		if(Lack != 8) RequestedSize += Lack;
		
		
		if(RequestedSize > BucketSize)
		{
			//TODO: Should probably allow for having a larger bucket in this case instead of giving an error?
			MOBIUS_FATAL_ERROR("ERROR (internal): Tried to do a bucket allocation that was larger than the bucket size.\n");
		}
		
		if(!Current || CurrentUsed + RequestedSize > BucketSize)
		{
			memory_bucket *NewBucket;
			if(Current && Current->Next)
			{
				// This happens if we have called Flush(). Reuse earlier buckets.
				NewBucket = Current->Next;
				memset(NewBucket->Data, 0, BucketSize);
			}
			else
			{
				// Otherwise, create a new bucket
				size_t AllocSize = sizeof(memory_bucket) + BucketSize;
				u8 *Memory = AllocClearedArray(u8, AllocSize);     //TODO: Should clearing be optional?

				NewBucket = (memory_bucket *)Memory;
				NewBucket->Data = Memory + sizeof(memory_bucket);
				NewBucket->Next = nullptr;
			}
			
			if(Current)	Current->Next = NewBucket;
			else        First         = NewBucket;
			
			Current = NewBucket;
			CurrentUsed = 0;
		}
		
		T *Result = (T *)(Current->Data + CurrentUsed);
		CurrentUsed += RequestedSize;
		
		return Result;
	}
	
	template<typename T> T *
	Copy(const T* Source, size_t Count)
	{
		if(Count == 0) return nullptr;
		T *Result = Allocate<T>(Count);
		memcpy(Result, Source, Count*sizeof(T));
		return Result;
	}
	
	char *
	CopyString(const char *Source)
	{
		//NOTE: This is for copying 0-terminated strings only
		return Copy(Source, strlen(Source)+1);
	}
	
	void
	Flush()
	{
		Current = First;
		memset(Current->Data, 0, BucketSize);
		CurrentUsed = 0;
	}
	
	void
	DeallocateAll()
	{
		if(!First) return;
		
		memory_bucket *Bucket = First;
		while(Bucket)
		{
			memory_bucket *Next = Bucket->Next;
			free(Bucket);
			Bucket = Next;
		}
		First   = nullptr;
		Current = nullptr;
		CurrentUsed = 0;
	}
};




//Array that does not own its data, but instead lives in "bucket memory".

template<typename T>
struct array
{
	T      *Data;
	size_t Count;
	
	array() : Data(nullptr), Count(0) {};
	
	void
	Allocate(bucket_allocator *Allocator, size_t Count)
	{
		Data = Allocator->Allocate<T>(Count);
		this->Count = Count;
	}
	
	array<T>
	Copy(bucket_allocator *Allocator) const
	{
		array<T> Result;
		Result.Count = Count;
		Result.Data  = Allocator->Copy(Data, Count);
		return Result;
	}
	
	inline T& operator[](size_t Index) { return Data[Index]; }
	inline const T& operator[](size_t Index) const { return Data[Index]; }
	
	//NOTE: For C++ - style iteration
	inline T* begin() { return Data; }
	inline T* end()   { return Data + Count; }
	inline const T* begin() const { return Data; }
	inline const T* end()   const { return Data + Count; }
};

template<typename T>
array<T> CopyDataToArray(bucket_allocator *Allocator, const T *Data, size_t Count)
{
	array<T> Result;
	Result.Count = Count;
	Result.Data  = Allocator->Copy(Data, Count);
	return Result;
}


