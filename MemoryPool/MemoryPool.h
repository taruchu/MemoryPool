#pragma once
 

#include<map> 
#include <iostream>
#include <exception>
#include <memory>
using namespace std;

	/*
		(1)
		This class creates a memory pool using a linked list which is defined using memory locations. Each memory location is defined as a chunk.
		A chunk has a 4 byte header section that is responsible for storing a pointer to the next chunk's memory address.
		This is how the link is created.

		(2)
		The linked list is stored in memory using a char** pointer. This pointer points to an array of char* chunks. Each 
		chunk is represented as a char* because it's header is a char*. But the rest of the chunk is used for user data. The function
		Alloc(void) will return a void* to the start of the data section of the next availiable chunk. The free list is found
		by referencing the m_pHead pointer. The head pointer points to the start of the free list. When we free a chunk by calling
		Free(void* memPtr), this chunk is inserted in the front of the list and the head pointer is assigned to point to it.
		Anytime we try to allocate, and m_pHead is pointing to NULL, which means "no more in free list", the list must grow if allowed.

	*/
	class MemoryPool
	{
		unsigned char** m_ppRawMemoryArray; // An array of memory blocks, each split up into chunks
		unsigned char** m_pHead;			// The front of the memory chunk linked list
		unsigned int m_chunkSize, m_numChunks;// The size of each chunk and number of chunks per array
		unsigned int m_memArraySize, m_memArrayMaxSize;		// The number elements in the memory array, and the max allowed
		bool m_toAllowResize;				// True if we resize the memory pool when it fills
		bool m_isMemPoolReady;
		const static size_t CHUNK_HEADER_SIZE = (sizeof(unsigned char*));
		const static int MEMORY_ARRAY_SIZE = 1;
		const static int MAX_MEMORY_ARRAY_SIZE = 20;

	public:
		//Construction
		MemoryPool(void)
		{
			m_memArraySize = MEMORY_ARRAY_SIZE;
			m_memArrayMaxSize = MAX_MEMORY_ARRAY_SIZE;
			m_ppRawMemoryArray = nullptr;
			m_pHead = nullptr;
			m_isMemPoolReady = false;
			m_toAllowResize = true;
			return;
		}
		MemoryPool(bool resize)
		{
			m_memArraySize = MEMORY_ARRAY_SIZE;
			m_memArrayMaxSize = MAX_MEMORY_ARRAY_SIZE;
			m_ppRawMemoryArray = nullptr;
			m_pHead = nullptr;
			m_isMemPoolReady = false;
			m_toAllowResize = resize;
			return;
		} 
		~MemoryPool(void)
		{
			Destroy();
			return;
		}
		//chunkSize = (sizeof(unsigned char) or 1 byte) * N {where N = 4};  numChunks = 128 {which is the size of 1 block of chuncks}; 
		//This is 32 bytes of usable chunck, without the 4 byte header pointer overhead per chunk.
		//The raw memory array stores the blocks of chunks.
		bool Init(unsigned int chunkSize, unsigned int numChunks);
		bool AllocateRawMemoryArray(void);
		

		//Allocaton functions
		void* Alloc(void); //This returns a chunk, not a block
		void Free(void* pMem);// This frees a chunk, not a block
		unsigned int GetChunkSize(void) const { return m_chunkSize; }

		//Settings
		bool GetReadyStatus() { return m_isMemPoolReady; }
		bool GetAllowResize() { return m_toAllowResize; }
		void SetAllowResize(bool resize) { m_toAllowResize = resize; return; }

	private:
		//Resets internal vars
		void Reset(void);

		//Internal memory allocation helpers
		bool GrowMemoryArray(int initialMemoryArraySize = 0);//When this runs the allocated memory to the program is actually increased twice as much + 1. And I guess this prevents having to call this too many times.
		unsigned char* AllocateNewMemoryBlock(void);
		void Destroy(void);

		//Internal linked list management
		unsigned char* GetNext(unsigned char* pBlock);
		void SetNext(unsigned char* pBlockToChange, unsigned char* pNewNext);

		//Don't allow copy constructor
		MemoryPool(const MemoryPool& memPool) {}
	};




	/*
		(1)
		This defines a base class that all memory managed classes must inherit from. This will prevent
		those classes from begin able to use new/new[] or delete/delete[].
	*/
	class MemoryPoolManagedClass
	{
	public:
		MemoryPoolManagedClass() {}
		virtual ~MemoryPoolManagedClass() {};
		void* operator new(size_t sz)
		{ 
			throw exception("Cannot use new with a MemoryPoolManagedClass.");
			return nullptr;
		}
		void* operator new[](size_t sz)
		{  
			throw exception("Cannot use new[] with a MemoryPoolManagedClass.");
			return nullptr;
		}
		void operator delete(void* p)
		{
			throw exception("Cannot use delete with a MemoryPoolManagedClass.");
			return;
		}
		void operator delete[](void* p)
		{
			throw exception("Cannot use delete[] with a MemoryPoolManagedClass.");
			return;
		}	
	};

	/*
		(1)
		This defines a context module for the memory pool manager.
	*/
	class MemPoolMangrContext : MemoryPoolManagedClass
	{
	public:
		MemPoolMangrContext() {}
		MemPoolMangrContext(size_t size, void* memory, size_t hash_code = 0)
		{
			m_MemoryChunkSize = size;
			p_MemoryAddress = memory;
			m_typeinfo_hash_code = hash_code;
			return;
		} 
		~MemPoolMangrContext() override
		{
			p_MemoryAddress = nullptr;
			return;
		}
		size_t m_MemoryChunkSize;
		void* p_MemoryAddress;
		size_t m_typeinfo_hash_code;//NOTE:(Optional) This may come in handy later for querying a list of these context objects for a specific type.
		//For example: using a SetVector<MemPoolMangrContext> setv;  then use the select feature to filter according to this hash code.
		//Once you know the type of the object, you could reconstruct the memory values for the that object by casting p_MemoryAddress to a pointer of the type.
		//Now that I pray on it, I could use this for an awsome recovery system. What if I needed to change the state of all objects of type A in memory in response to 
		//a system mistake? I could do this live during runtime using a Lua script. Cool :)
		
	private:
		
		MemPoolMangrContext(const MemPoolMangrContext& mangr){} //Prevent the copy constructor
	};


	/*
		This class is a memory pool manager. It is un-managed by default, meaning you must call the deallocation method before
		you assign your pointer to something else or it will cause a memory leak in the Memory Pool. However, it can be extended
		to become a managed garbage collected version if needed --- just by flipping a bool. This manager will keep a Main std::map(no duplicate keys) of
		size_t objects as the key and MemoryPool objects as the value. This will allow a memory pool for a certain
		chunk size to stay in the map as long as it is needed, to be re-used. The allocation caller will give a pointer,
		and an allocation size to the manager when allocation is needed. The manager will create a MemoryPool object(if none already exist for that chunk size)
		and then put the size_t inside the map as the key and the MemoryPool object inside of the map as it's value.
		Then the manager will use this MemoryPool object to allocate the requested memory and will assign the pointer to it.
		There will also be another Validation std::map for keeping track of the pointer and it's allocated memory. This map will be
		"set up" by the allocation method and then used by the deallocation method to double check the pointer sent to it by the caller. If the caller sends
		a pointer to the deallocation method that is in this map and that is still pointing to the memory in the map associated with it, the deallocater
		will use the pointer to call MemoryPool.Free(void* ptr). 
			This is cool because if we wanted to turn this manager into a garbage collected version. I could create a custom eviction policy that 
		parses the validation map when the memory pool memory runs out. And use this instead of just
		extending the memory pool. For example, I could first find all pointers in the validation map's keys that are no longer pointing to the memory address
		in it's map value(Context), and call MemoryPool.Free(void* ptr) passing in a brand new pointer that is assigned to the "abandoned memory" just for the purposes of freeing that memory.
		Also, we will know which MemoryPool object to use based on the sizeof(abandoned memory) that was abandoned. I will use a context for storing this. The
		size is the chunk size_t that is used as the key in the Main std:map of MemoryPool objects. Amen thank you God :-)
			The basic deallocation will be as follows. The caller will again give the manager a pointer. If the pointer is found in the
		Validation map as a key and still pointing to the memory address in the value, the deallocation will proceed, if not it will simply return false.
		Returning true/false will tell the caller that they	can either: reuse the pointer, or investigate what went wrong with the deallocation.
		To deallocate, the manager will	find the pointer's MemoryPool object using the sizeof(the allocated memory) found in it's validation std::map value(Context),
		call MemoryPool.Free(pointer), set the pointer to NULL, and remove it's presence from the Validation std::map. When the memory pool manager's object 
		goes out of scope and is deallocated by the OS, the Main std:map, Validation std::map, and all the Memory Pools will be deallocated gracefully. 
		The memory pool manager's destructor will make sure that all pointers in the Validation std::map are assigned to NULL before the maps are deallocated
		to the OS.
	*/
	class MemoryPoolManager : MemoryPoolManagedClass
	{
	public:
		MemoryPoolManager()
		{ 
			 m_ValidationMapIter = m_ValidationMap.end();
			 m_MainMapIter = m_MainMap.end();
			 m_IsGarbageCollectionOn = false;
			return;
		}
		~MemoryPoolManager() override
		{
			//Make sure all pointers still in the validation Map are set to NULL so that their owners are aware
			//of the memory being reclaimed by the OS.
			for (m_ValidationMapIter = m_ValidationMap.begin(); m_ValidationMapIter != m_ValidationMap.end(); m_ValidationMapIter++)
			{
				*m_ValidationMapIter->first = nullptr;
			}
			return;
		}

		//Management
		bool AllocateChunk(void*& ptr, size_t allocSize);
		bool DeallocateChunk(void*& ptr);
		bool m_IsGarbageCollectionOn;		

	private:  
		typedef map<size_t, MemoryPool> MainMappingType;
		typedef map<size_t, MemoryPool>::iterator MainMappingTypeIter;
		typedef map<void**, MemPoolMangrContext> ValidationMappingType;
		typedef map<void**, MemPoolMangrContext>::iterator ValidationMappingTypeIter;
		MainMappingType m_MainMap;
		MainMappingTypeIter m_MainMapIter;
		map<void**, MemPoolMangrContext>  m_ValidationMap;
		ValidationMappingTypeIter  m_ValidationMapIter;

		//The number of chuncks in each allocated block. The MemoryPool class is defaulted to 1 block initially, then it will extend if needed.
		const unsigned int BLOCK_SIZE_TIER1 = 1;
		const unsigned int BLOCK_SIZE_TIER2 = 50;
		const unsigned int BLOCK_SIZE_TIER3 = 100;


		void CollectAbandonedMemory(void);
		void FreeAbandonedMemory(MemPoolMangrContext& context);

		//Don't allow a copy constructor.
		MemoryPoolManager(const MemoryPoolManager& manager){}
	};



