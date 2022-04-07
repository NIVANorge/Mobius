

/*
	Very simple queue that allows you to peek ahead of the first position.
	No advanced C++, so no move semantics or things like that supported. It was only made for a single purpose of having a token queue for file parsing.
*/

template<typename T>
struct peek_queue
{
	T      *Data;
	size_t  Capacity;
	
	s64 CursorFirst;
	s64 CursorEnd;
	
	// Data[(CursorFirst + Idx) % Capacity]  is item number Idx in the queue, Idx=0 being the first
	// Data[(CursorEnd-1) % Capacity]        is the last item in the queue
	
	peek_queue();
	~peek_queue();
	
	void      Advance();
	void      Reserve(s64 ReserveCapacity);
	s64       MaxPeek();
	const T & Peek(s64 PeekAhead);
	      T & Append();
};

template<typename T>
peek_queue<T>::peek_queue()
{
	Capacity = 16;
	Data     = AllocClearedArray(T, Capacity);
	
	CursorFirst = 0;
	CursorEnd   = 0;
}

template<typename T>
peek_queue<T>::~peek_queue()
{
	free(Data);
}

template<typename T>
void peek_queue<T>::Advance()
{
	if(CursorFirst < CursorEnd)
		CursorFirst++;
	else
		FatalError("ERROR(internal): Advanced a peek_queue beyond end.\n");
}

template<typename T>
void peek_queue<T>::Reserve(s64 ReserveCapacity)
{
	if(ReserveCapacity > (s64)Capacity)
	{
		// Make capacity a power of 2 just because.
		size_t NewCapacity = Capacity*2;
		while(ReserveCapacity > (s64)NewCapacity) NewCapacity *= 2;
		
		T *NewData = AllocClearedArray(T, NewCapacity);
		
		// Copy over data to new buffer
		for(s64 Idx = 0; Idx < (s64)Capacity; ++Idx)
			NewData[(CursorFirst + Idx) % NewCapacity] = Data[(CursorFirst + Idx) % Capacity];
		
		free(Data);
		Data     = NewData;
		Capacity = NewCapacity;
	}
}

template<typename T>
s64 peek_queue<T>::MaxPeek()
{
	return (CursorEnd-1) - CursorFirst;
}

template<typename T>
const T & peek_queue<T>::Peek(s64 PeekAt)
{
	if(PeekAt < 0)
		FatalError("ERROR (internal): Peeked backwards in a queue.\n");
	
	if(CursorFirst + PeekAt >= CursorEnd)
		FatalError("ERROR (internal): Peeked too far ahead in a queue.\n");
	
	return Data[(CursorFirst + PeekAt) % Capacity];
}

template<typename T>
T & peek_queue<T>::Append()
{
	s64 NeedCapacity = CursorEnd - CursorFirst + 1;
	if(NeedCapacity > (s64)Capacity)
		FatalError("ERROR (internal): Appended to queue without reserving enough capacity first.\n");
	
	return Data[(CursorEnd++) % Capacity];
}