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
	static size_t memory_used;

protected:
	Singleton()
	{
	}

	virtual ~Singleton()
	{
	}

public:
	static bool IsCreated ()
	{
		return hClass::instance != nullptr;
	}

	static void CreateInstance()
	{
		Assert( !IsCreated () );
		void* ptr = HeapMemoryManager::AllocateMemory ( sizeof ( C ), hClass::memory_used, 16 );
		hClass::instance = new(ptr) C();
	}

	static void Required()
	{
		if ( !IsCreated () )
			CreateInstance();
	}

	inline static C * const GetInstance();

	static void DestroyInstance()
	{
		if( IsCreated () )
		{
			hClass::instance->~C ();
			HeapMemoryManager::FreeMemory ( hClass::instance, hClass::memory_used );
			hClass::instance = nullptr;
		}
	}
};

template <class C>
C* Singleton<C>::instance(nullptr);

template <class C>
size_t Singleton<C>::memory_used(0);

template <class C>
inline C * const Singleton<C>::GetInstance()
{
	Assert( IsCreated () );
	return hClass::instance;
}

#endif // SINGLETON_H
