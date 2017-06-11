#pragma once

#include<iostream>
#include<exception>
#include<memory>
#include<list>

//using namespace std::tr1;
using namespace std;
 

class IPrintable
{
public:
	virtual void VPrint() = 0;
};

class CPrintable : public IPrintable
{ 
	char* m_Name;
public:
	CPrintable(char* name) 
	{ 		
		m_Name = name;
		printf("create %s\n", m_Name);
		return;
	}
	virtual ~CPrintable()
	{
		printf("delete %s\n", m_Name);
		return;
	}
	void VPrint()
	{
		printf("print %s\n", m_Name);
		return;
	}
};

shared_ptr<CPrintable> CreateAnObject(char* name);

void ProcessObject(shared_ptr<CPrintable> o);

void TestSharedPointers(void);
 