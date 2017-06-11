

#pragma once;

#include<iostream>
#include "MemoryPool.h"
#include <exception>
//#include "SharedPointers.h"

using namespace std;
//using namespace memorypool;

 

int main(void)
{
	try
	{
		void* ptr = nullptr;
		void* ptr2 = nullptr;
		 
		MemoryPoolManager manager;
		manager.m_IsGarbageCollectionOn = true; //turn garbage collection on or off
		if(manager.AllocateChunk(ptr, sizeof(int)))
		{

			*((int*)ptr) = 5;

			cout << *((int*)ptr) << endl;

			ptr = nullptr;  //(garbage collection must be on) Set this to null to cause garbage collection rather than extending the allocation pool
							// Adjust constant called "BLOCK_SIZE_TIER1" in MemoryPool.h to increase the number of chuncks allowed before running out of memory.
		}
		  
		if(manager.AllocateChunk(ptr2, sizeof(int)))
		{ 
			*((int*)ptr2) = 6;

			cout << *((int*)ptr2) << endl;			
		}

		if(ptr)
			manager.DeallocateChunk(ptr);
		if(ptr2)
			manager.DeallocateChunk(ptr2);
		
	}
	catch (exception& ex)
	{
		cout << ex.what() << endl;
	}

	return 0;
}