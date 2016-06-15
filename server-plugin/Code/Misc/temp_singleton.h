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

#ifdef GNUC
#	include <mm_malloc.h>
#endif

class NoCopy
{
protected:
	NoCopy() {}
	virtual ~NoCopy() {}

private:
	NoCopy(NoCopy const &) = delete;
	NoCopy& operator=(NoCopy const &) = delete;
};

template <class C>
class Singleton : protected NoCopy
{
	typedef Singleton<C> hClass;

private:
	static C* instance;

protected:
	Singleton() : NoCopy()
	{
	}

	virtual ~Singleton() override
	{
	}

public:
	static void CreateInstance()
	{
		//Assert(hClass::instance == nullptr);
		void* ptr = _mm_malloc(sizeof(C), 16);
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
		if (hClass::instance)
		{
			hClass::instance->~C();
			_mm_free(hClass::instance);
		}
		hClass::instance = nullptr;
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
