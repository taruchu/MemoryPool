#pragma once

#include "MemoryPool.h"

	bool MemoryPool::AllocateRawMemoryArray(void)
	{
		try
		{
			size_t allocationSize = sizeof(unsigned char**) * m_memArraySize;
			m_ppRawMemoryArray = (unsigned char**)malloc(allocationSize);
			return (m_ppRawMemoryArray != nullptr);
		}
		catch (bad_alloc& ex)
		{
			cout << ex.what() << endl;
		}
		catch (exception& ex)
		{
			cout << ex.what() << endl;
		}
	}

	bool MemoryPool::Init(unsigned int chunkSize, unsigned int numChunks)// chunkSize = (sizeof(unsigned char) or 1 byte) * N;  numChunks = 128; which is 32 bytes of usable block, without the 4 byte header pointer overhead per chunk
	{
		m_chunkSize = chunkSize;
		m_numChunks = numChunks;

		m_isMemPoolReady = GrowMemoryArray(m_memArraySize);
		return  m_isMemPoolReady;
	}

	bool MemoryPool::GrowMemoryArray(int initialMemoryArraySize)
	{
		try
		{
			if (m_ppRawMemoryArray)
			{
				//Allocate a new array
				size_t allocationSize = sizeof(unsigned char*) * ((initialMemoryArraySize == 0) ? (m_memArraySize + 1) : initialMemoryArraySize);
				unsigned char** ppNewMemArray = (unsigned char**)malloc(allocationSize);

				//Make sure the allocation succeded
				if (!ppNewMemArray)
					return false;

				//Copy any existing memeory pointers over
				for (unsigned int i = 0; i < m_memArraySize; i++)
				{
					ppNewMemArray[i] = m_ppRawMemoryArray[i];
				}

				//Allocate a new block of memory.  Indexing m_memArraySize here is 
				//safe because we haven't incremented it yet to reflect the new size
				ppNewMemArray[m_memArraySize] = AllocateNewMemoryBlock();

				//Attach the block to the end of the current memory list
				unsigned char* pCurr = (unsigned char*)m_pHead;//Cast here, we will still pass over the correct memory address
				unsigned char* pNext = GetNext((unsigned char*)m_pHead);
				while (pNext)
				{
					pCurr = pNext;
					pNext = GetNext(pNext);
				}

				SetNext(pCurr, ppNewMemArray[m_memArraySize]);
				pCurr = nullptr;
				pNext = nullptr;

				//Deallocate the old memory array by first setting it's elements to null because the new array is pointing to
				//those locations.
				if (m_ppRawMemoryArray)
				{
					for (unsigned int i = 0; i < m_memArraySize; i++)
					{
						m_ppRawMemoryArray[i] = nullptr;
					}
					//Now free the heap memory allocated for the old memory array.
					Destroy();
				}
				
				//Assign the raw memory pointer to the new allocation
				m_ppRawMemoryArray = ppNewMemArray;
				ppNewMemArray = nullptr;//Set the temporary pointer to null now.
				m_pHead = (unsigned char**)m_ppRawMemoryArray[m_memArraySize];//Head must point directly to the first free chunk
			}
			else
			{
				AllocateRawMemoryArray();
				for (unsigned int i = 0; i < m_memArraySize; i++)
				{
					m_ppRawMemoryArray[i] = AllocateNewMemoryBlock();
					if (i > 0)
					{
						//Need to connect each new block to the previously allocated block.
						unsigned char* pCurr = m_ppRawMemoryArray[i - 1];
						unsigned char* pNext = GetNext(m_ppRawMemoryArray[i - 1]);
						while (pNext)
						{
							pCurr = pNext;
							pNext = GetNext(pNext);
						}
						SetNext(pCurr, m_ppRawMemoryArray[i]);
						pCurr = nullptr;
						pNext = nullptr;
					}
				}
				m_pHead = (unsigned char**)m_ppRawMemoryArray[0];//Set head to the first chunk
			}

			//Increment the size count if this is not the initialization
			m_memArraySize = (initialMemoryArraySize == 0) ? m_memArraySize + 1 : initialMemoryArraySize;
			return true;
		}
		catch (bad_alloc& ex)
		{
			cout << ex.what() << endl;
		}
		catch (exception& ex)
		{
			cout << ex.what() << endl;
		}
	}

	unsigned char* MemoryPool::AllocateNewMemoryBlock(void)
	{
		try
		{
			//Calculate the size of each block and the size of the actual memory allocation
			size_t miniBlockSize = m_chunkSize + CHUNK_HEADER_SIZE;// chunk + linked list overhead
			size_t trueSize = miniBlockSize * m_numChunks;

			//Allocate the memory
			unsigned char* pNewMem = (unsigned char*)malloc(trueSize);
			if (!pNewMem)
				return nullptr;

			//Turn the memory into a linked list of chunks
			unsigned char* pEnd = pNewMem + trueSize;
			unsigned char* pCurr = pNewMem;
			while (pCurr < pEnd)
			{
				//Calculate the next pointer position
				unsigned char* pNext = pCurr + miniBlockSize;

				//Set the next pointer
				unsigned char** ppChunkHeader = (unsigned char**)pCurr;
				ppChunkHeader[0] = (pNext < pEnd ? pNext : nullptr);

				//Move to the next mini-block
				pCurr += miniBlockSize;
				ppChunkHeader = nullptr;
				pNext = nullptr;
			}
			pEnd = nullptr;
			pCurr = nullptr;
			return pNewMem;
		}
		catch (exception& ex)
		{
			cout << ex.what() << endl;
		}
	}
	 
	void MemoryPool::Destroy(void)
	{
		if (m_ppRawMemoryArray)
		{
			 
			free(m_ppRawMemoryArray);
			m_ppRawMemoryArray = nullptr;
			 
		}
		return;
	}

	void* MemoryPool::Alloc(void)
	{
		try
		{
			//If we're out of memory chunks, grow the pool. This is very expensive. Remember, head points to the next free chunk.
			//So if this is null, we are out of free chunks.
			if (!(m_pHead))
			{
				//If we don't allow resizes or the max resize limit is reached, return NULL
				if (!m_toAllowResize || m_memArraySize >= m_memArrayMaxSize)
				{
					return nullptr;
				}
				//This is where we would grow the memory repeatedly as needed.
				if (!GrowMemoryArray())
					return nullptr; //Couldn't allocate anymore memory
			}

			//Grab the first chunk from the list and move to the next chunk
			unsigned char* pAlloc = (unsigned char*)m_pHead;
			m_pHead = (unsigned char**)GetNext((unsigned char*)m_pHead);
			
			return (pAlloc + CHUNK_HEADER_SIZE); //Make sure we return a pointer to the data section only. (Seek up to data section)
		}
		catch (exception& ex)
		{
			cout << ex.what() << endl;
		}
	}

	void MemoryPool::Free(void* pMem)
	{
		try
		{

			//Calling Free() on a NULL pointer is perfectly valid C++ so we have to check for it.
			if (pMem)
			{
				
				//The pointer we get back is just to the data section of the chunk. 
				//This gets us the full chunk. (Seek backwards past the chunk header)
				unsigned char* pBlock = ((unsigned char*)pMem) - CHUNK_HEADER_SIZE; 
				
				//Push the chunk to the front of the list
				SetNext(pBlock, (unsigned char*)m_pHead);
				m_pHead = (unsigned char**)pBlock;
				return;
			}
		}
		catch (exception& ex)
		{
			cout << ex.what() << endl;
		}
	}

	void MemoryPool::Reset()
	{
		try
		{
			m_memArraySize = MEMORY_ARRAY_SIZE;
			m_memArrayMaxSize = MAX_MEMORY_ARRAY_SIZE;
			m_pHead = nullptr;
			if (m_ppRawMemoryArray)
			{
				Destroy();
			}
			GrowMemoryArray(m_memArraySize);
			return;
		}
		catch (exception& ex)
		{
			cout << ex.what() << endl;
		}
	}
	unsigned char* MemoryPool::GetNext(unsigned char* pBlock)
	{
		try
		{
			if (pBlock)
			{
				return ((unsigned char**)pBlock)[0];
			}
			else
				return nullptr;
		}
		catch (exception& ex)
		{
			cout << ex.what() << endl;
		}
	}
	void MemoryPool::SetNext(unsigned char* pBlockToChange, unsigned char* pNewNext)
	{
		try
		{
			if (pBlockToChange)
			{
				((unsigned char**)pBlockToChange)[0] = pNewNext;
			}
		}
		catch (exception& ex)
		{
			cout << ex.what() << endl;
		}
	}

	
	

	bool MemoryPoolManager::AllocateChunk(void *& ptr, size_t allocSize)
	{
		MemoryPool* p_memPool = nullptr;
		MemPoolMangrContext* p_context = nullptr;
		void* p_Alloc = nullptr;

		m_ValidationMapIter = m_ValidationMap.find(&ptr);		
		if (m_ValidationMapIter != m_ValidationMap.end())
		{
			//Get the current context
			p_context = &(m_ValidationMapIter->second);

			//Return false if they are trying to allocate memory they already have.
			if (p_context->m_MemoryChunkSize == allocSize && p_context->p_MemoryAddress == ptr)
				return false;

			//If this context contains any kind of allocated memory we must free it first.
			if (p_context->p_MemoryAddress)
			{ 
				m_MainMapIter = m_MainMap.find(p_context->m_MemoryChunkSize);
				if(m_MainMapIter != m_MainMap.end())
					m_MainMapIter->second.Free(p_context->p_MemoryAddress);
				p_context->m_MemoryChunkSize = 0;
				p_context->p_MemoryAddress = nullptr;
				m_MainMapIter = m_MainMap.end();
			}

			//Delete the old entry and start fresh
			m_ValidationMap.erase(m_ValidationMapIter);
			p_context = nullptr;
		}
		 
		//Is this memory pool already in the map?  
		m_MainMapIter = m_MainMap.find(allocSize);
		//MemoryPool obj_memPool;
		if (m_MainMapIter != m_MainMap.end())
		{
			p_memPool = &(m_MainMapIter->second); 
		}
		else
		{
			MemoryPool obj_memPool;
			m_MainMap[allocSize] = obj_memPool; //Do this first before "Init", because map stores a copy of the object
												//and it will duplicate the allocated "memorypool raw pointer". Don't want free to
												//get called multiple times on the same pointer in memorypool destructor.
			if (m_MainMap[allocSize].Init(allocSize, BLOCK_SIZE_TIER1))
				p_memPool = &m_MainMap[allocSize];
			else
				return false;
		}
		 
		//Allocate the requested memory. I need to turn off the "allow resize" if garbage collection is on 
		//so that the collection has a chance to run before the memory pool is extended.
		bool b_InitialResizeState = p_memPool->GetAllowResize();
		if (m_IsGarbageCollectionOn)
			p_memPool->SetAllowResize(false);

		p_Alloc = p_memPool->Alloc();
		if (!p_Alloc && m_IsGarbageCollectionOn)
		{
			CollectAbandonedMemory();
			p_memPool->SetAllowResize(b_InitialResizeState);
			//Retry the allocation
			p_Alloc = p_memPool->Alloc();
		}
		if (!p_Alloc)
		{
			//No more memory to give out :)
			ptr = nullptr;
			return false;
		}
		
		 
		//Cool I got a chunk. Now I'll create a validation mapping
		MemPoolMangrContext obj_context(allocSize, p_Alloc);
		m_ValidationMap[&ptr] = obj_context;
		ptr =  p_Alloc;   
		 
		m_ValidationMapIter = m_ValidationMap.end();
		m_MainMapIter = m_MainMap.end();
		return true;
	}

	bool MemoryPoolManager::DeallocateChunk(void*& ptr)
	{
		//Is this pointer valid? 
		m_ValidationMapIter = m_ValidationMap.find(&ptr);
		bool valid = (m_ValidationMapIter != m_ValidationMap.end()) && (ptr == m_ValidationMapIter->second.p_MemoryAddress);
		 
		if (!valid)
			return false;

		//It is Cool :), now I'll free it's memory....if I need to :)
		m_MainMapIter = m_MainMap.find(m_ValidationMapIter->second.m_MemoryChunkSize);
		if (m_MainMapIter != m_MainMap.end())
			m_MainMapIter->second.Free(ptr);

		//Remove it from the validation mapping
		m_ValidationMap.erase(m_ValidationMapIter); 

		//Set the caller's pointer to NULL
		ptr = nullptr; 

		m_ValidationMapIter = m_ValidationMap.end();
		m_MainMapIter = m_MainMap.end();
		return true;
	}

	void MemoryPoolManager::CollectAbandonedMemory(void)
	{
		//For each pointer in the validation mapping, see if it is abandoned. If so free the memory
		ValidationMappingTypeIter iter;
		for (iter = m_ValidationMap.begin(); iter != m_ValidationMap.end();)
		{
			if (*iter->first != iter->second.p_MemoryAddress)
			{
				FreeAbandonedMemory(iter->second);
				//Remove the validation mapping
				m_ValidationMap.erase(iter++);
			}
			else
			{
				++iter;
			}
		}
		return;
	}

	void MemoryPoolManager::FreeAbandonedMemory(MemPoolMangrContext& context)
	{
		MainMappingTypeIter itr = m_MainMap.find(context.m_MemoryChunkSize);
		if (itr != m_MainMap.end())
			itr->second.Free(context.p_MemoryAddress); 
		return;
	}

