
#pragma once
 
#include "SharedPointers.h"

 
shared_ptr<CPrintable> CreateAnObject(char* name)
{
	return shared_ptr<CPrintable>(new CPrintable(name));
}

void ProcessObject(shared_ptr<CPrintable> o)
{
	printf("(print from a function) ");
	o->VPrint();
	return;
}

void TestSharedPointers(void)
{
	shared_ptr<CPrintable> ptr1(new CPrintable("1")); //NOTE: create object 1
	shared_ptr<CPrintable> ptr2(new CPrintable("2")); //NOTE: create object 2

	ptr1 = ptr2; //NOTE: destry object 1
	ptr2 = CreateAnObject("3"); //NOTE: used as a return value
	ProcessObject(ptr1);  //NOTE: call a function

						  //THOU SHALL NOT DO USAGE EXAMPLES...
						  //
	CPrintable o1("notdo");
	//ptr1 = &o1;  //NOTE: Syntax error! It's on the stack...not the Heap
	//
	CPrintable *o2 = new CPrintable("notdo2");
	//ptr1 = o2;  //NOTE: Syntax error ! Use the next line to do this...
	ptr1 = shared_ptr<CPrintable>(o2);

	//NOTE: You can even use shared_ptr on ints!

	shared_ptr<int> a(new int);
	shared_ptr<int> b(new int);

	*a = 5;
	*b = 6;

	const int *q = a.get();  //NOTE: use this for reading in multi-threaded code..

							 //NOTE: this is expecially cool - you can alos use it in lists.
	list<shared_ptr<int>> intList;
	list<shared_ptr<IPrintable>> printableList;
	for (int i = 0; i < 100; ++i)
	{
		intList.push_back(shared_ptr<int>(new int(rand())));
		printableList.push_back(shared_ptr<IPrintable>(new CPrintable("list")));
	}


	//NOTE:  No leaks!!!!! Isn't that cool....
}