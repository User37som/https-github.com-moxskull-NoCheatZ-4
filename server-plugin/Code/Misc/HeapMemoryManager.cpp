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

#include "HeapMemoryManager.h"

#ifdef GNUC
#	include <mm_malloc.h>
#endif
#include <algorithm>

#include "SdkPreprocessors.h"

namespace HeapMemoryManager
{
	FreeMemoryHolder m_free_memory[ HMM_MAX_FREE_OBJECTS ];
	bool m_memory_init ( false );

	void* AllocateMemory ( size_t bytes, size_t & new_capacity, size_t align_of /* = 4U */ )
	{
		if( !m_memory_init ) InitPool ();
		FreeMemoryHolder* it ( m_free_memory );
		FreeMemoryHolder * const it_end ( &( m_free_memory[ HMM_MAX_FREE_OBJECTS ] ) );
		do
		{
			if( it->m_ptr == nullptr ) // If this pointer is not valid, then we quit and call standard alloc, because array is sorted.
			{
				break;
			}
			else
			{
				if( it->m_capacity >= bytes ) // Is there enough capacity here ?
				{
					if( size_t ( it->m_ptr ) % align_of == 0 && it->m_capacity % align_of == 0 ) // Try to see if the pointer fits in the alignement requirements
					{
						new_capacity = it->m_capacity;
						void * ret ( it->m_ptr );

						// move other containers down by 1 index

						FreeMemoryHolder* it2(it);
						while (++it2 != it_end)
						{
							it->Copy(it2);
							it2->Zero();
							++it;
						}

						return ret;
					}
				}
			}
		}
		while( ++it != it_end );

		new_capacity = align_of;
		while( new_capacity < bytes /*|| ( new_capacity % align_of != 0) */ ) new_capacity <<= 1;

		void * nptr ( _mm_malloc ( new_capacity, align_of ) );

		return nptr;
	}

	bool IsPoolFull()
	{
		return (m_free_memory + HMM_MAX_FREE_OBJECTS - 1)->m_ptr != nullptr;
	}

	void FreeMemory ( void * ptr, size_t capacity )
	{
		if( !m_memory_init ) InitPool ();
		else if (IsPoolFull())
		{
			_mm_free(ptr);
		}

		// find the best place (insertion sort)

		FreeMemoryHolder* it(m_free_memory);
		FreeMemoryHolder * const it_end(&(m_free_memory[HMM_MAX_FREE_OBJECTS]));
		do
		{
			if (it->m_ptr == nullptr || it->m_capacity > capacity)
			{
				break;
			}
		} while (++it != it_end);

		// move other containers up by 1 index

		FreeMemoryHolder* it2(it_end);
		FreeMemoryHolder* it3(it2 - 2);
		while (--it2 != it)
		{
			it2->Copy(it3);
			it3->Zero();
			--it3;
		}
		
		it->m_ptr = ptr;
		it->m_capacity = capacity;
	}

	void InitPool ()
	{
		FreeMemoryHolder * it ( m_free_memory );
		FreeMemoryHolder const * const it_end ( &( m_free_memory[ HMM_MAX_FREE_OBJECTS ] ) );
		do
		{
			it->Zero();
		}
		while( ++it != it_end );
		m_memory_init = true;
	}

	void FreePool ()
	{
		if( !m_memory_init ) return;
		FreeMemoryHolder * it ( m_free_memory );
		FreeMemoryHolder const * const it_end ( &( m_free_memory[ HMM_MAX_FREE_OBJECTS ] ) );
		do
		{
			if( it->m_ptr != nullptr )
			{
				_mm_free ( it->m_ptr );
				it->Zero();
			}
			else
			{
				return;
			}
		}
		while( ++it != it_end );
	}
}
