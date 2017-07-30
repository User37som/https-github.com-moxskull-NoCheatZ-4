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

#ifndef HEAPMEMORYMANAGER_H
#define HEAPMEMORYMANAGER_H

#include <array>
#include <algorithm>
#include <new>

#include "ClassSpecifications.h"
#include "Containers/utlvector.h"

/*
	The purpose of this file is to simply reduce the amount of re-allocations of any type
*/

#define HMM_MAX_FREE_OBJECTS 512
#define HMM_MAX_SINGLE_OBJECT_SIZE 4096 // bytes

namespace HeapMemoryManager
{
	struct FreeMemoryHolder
	{
		void * m_ptr;
		size_t m_capacity;

		FreeMemoryHolder () :
			m_ptr(nullptr),
			m_capacity(0)
		{
		}

		void Copy(FreeMemoryHolder const * other)
		{
			m_ptr = other->m_ptr;
			m_capacity = other->m_capacity;
		}

		void Zero()
		{
			m_ptr = nullptr;
			m_capacity = 0;
		}
	};

	void* AllocateMemory ( size_t bytes, size_t & new_capacity, size_t align_of = 4U );
	void FreeMemory ( void * ptr, size_t capacity);

	void InitPool ();
	void FreePool ();

	bool IsPoolFull();

	template <size_t align>
	struct OverrideNew
	{
		size_t m_hmm_capacity;

		void SetHMMCapacity ( size_t capacity )
		{
			m_hmm_capacity = capacity;
		}

		void* operator new( size_t i ) noexcept
		{
			size_t cap;
			void * pthis (HeapMemoryManager::AllocateMemory ( i, cap, align ));
			static_cast< OverrideNew* >( pthis )->SetHMMCapacity ( cap );
			return pthis;
		}

		void* operator new( size_t i, void* ptr ) noexcept
		{
			return ptr;
		}

		void operator delete( void * ptr ) noexcept
		{
			HeapMemoryManager::FreeMemory ( ptr, static_cast< OverrideNew* >( ptr )->m_hmm_capacity );
		}

		void operator delete( void * ptr, void * place ) noexcept
		{
		}
	};
}

#endif // HEAPMEMORYMANAGER_H
