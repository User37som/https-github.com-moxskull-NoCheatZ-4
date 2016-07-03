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

	inline int SortMemPool ( FreeMemoryHolder const * a, FreeMemoryHolder const * b )
	{
		if( a->m_capacity > b->m_capacity ) return 1;
		else if( a->m_capacity < b->m_capacity ) return -1;
		else return 0;
	}

	int SortMemPool_wrap ( void const * a, void const * b )
	{
		return SortMemPool ( static_cast< FreeMemoryHolder const * >( a ), static_cast< FreeMemoryHolder const * >( b ) );
	}

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
					if( size_t( it->m_ptr ) % align_of == 0 && it->m_capacity % align_of == 0 ) // Try to see if the pointer fits in the alignement requirements
					{
						new_capacity = it->m_capacity;
						void * ret ( it->m_ptr );
						it->m_capacity = std::numeric_limits<size_t>::max();
						it->m_ptr = nullptr;

						std::qsort ( m_free_memory, HMM_MAX_FREE_OBJECTS, sizeof( FreeMemoryHolder ), SortMemPool_wrap );

                        //printf("reuse %p (%u, %u)\n", ret, new_capacity, align_of);

						return ret;
					}
				}
			}
		}
		while( ++it != it_end );

		new_capacity = align_of;
		while( new_capacity < bytes /*|| ( new_capacity % align_of != 0) */) new_capacity <<= 1;

        void * nptr(_mm_malloc ( new_capacity, align_of ));
        //printf("alloc %p (%u, %u)\n", nptr, new_capacity, align_of);
		return nptr;
	}

	void FreeMemory ( void * ptr, size_t capacity )
	{
		if( !m_memory_init ) InitPool ();
		Assert ( capacity && (capacity & 0xFF) != 0xCC && "Stop using memset everywhere ..." );
		FreeMemoryHolder* it ( m_free_memory + HMM_MAX_FREE_OBJECTS - 1 );
		if( it->m_ptr != nullptr || capacity > HMM_MAX_SINGLE_OBJECT_SIZE ) // Pool is full or memory too big
		{
            //printf("free %p (%u)\n", ptr, capacity);
			_mm_free ( ptr );
		}
		else
		{
			it->m_ptr = ptr;
			it->m_capacity = capacity;

			//printf("stored %p (%u)\n", ptr, capacity);

			std::qsort ( m_free_memory, HMM_MAX_FREE_OBJECTS, sizeof ( FreeMemoryHolder ), SortMemPool_wrap );
		}
	}

	void InitPool ()
	{
		FreeMemoryHolder * it ( m_free_memory );
		FreeMemoryHolder const * const it_end ( &( m_free_memory[ HMM_MAX_FREE_OBJECTS ] ) );
		do
		{
			it->m_ptr = nullptr;
			it->m_capacity = std::numeric_limits<size_t>::max ();
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
                //printf("free %p (%u)\n", it->m_ptr, it->m_capacity);
				_mm_free ( it->m_ptr );
				it->m_ptr = nullptr;
				it->m_capacity = std::numeric_limits<size_t>::max ();
			}
			else
			{
				return;
			}
		}
		while( ++it != it_end );
	}
}
