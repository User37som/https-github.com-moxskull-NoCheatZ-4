/*
Copyright 2012 - Le Padellec Sylvain

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef SINGLETON_H
#define SINGLETON_H

#include <new>

#include "SdkPreprocessors.h"

#include "ClassSpecifications.h"
#include "HeapMemoryManager.h"

template <class C>
class Singleton :
	protected NoCopy,
	protected NoMove
{
	typedef Singleton<C> hClass;

private:
	static C* instance;

protected:
	Singleton() : NoCopy()
	{
	}

	virtual ~Singleton()
	{
	}

public:
	static void CreateInstance()
	{
		Assert(hClass::instance == nullptr);
		void* ptr = HeapMemoryManager::AllocateMemory ( sizeof ( C ), static_cast< HeapMemoryManager::OverrideNew<16>* >( hClass::instance )->m_hmm_capacity, 16 );
		hClass::instance = new(ptr) C();
	}

	static void Required()
	{
		if (hClass::instance == nullptr)
			CreateInstance();
	}

	inline static C * const GetInstance();

	static void DestroyInstance()
	{
		if( hClass::instance )
		{
			hClass::instance->~C ();
			HeapMemoryManager::FreeMemory ( hClass::instance, static_cast< HeapMemoryManager::OverrideNew<16>* >( hClass::instance )->m_hmm_capacity );
			hClass::instance = nullptr;
		}
	}
};

template <class C>
C* Singleton<C>::instance = nullptr;

template <class C>
inline C * const Singleton<C>::GetInstance()
{
	Assert(hClass::instance);
	return hClass::instance;
}

#endif // SINGLETON_H
