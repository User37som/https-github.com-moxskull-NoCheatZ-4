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
	FreeMemoryList_t m_free_memory;
	size_t m_memlist_elem_count(0);

	bool SortMemPool ( FreeMemoryHolder const & a, FreeMemoryHolder const & b )
	{
		if( a.m_capacity == 0 )
		{
			return false;
		}
		else if( b.m_capacity == 0 )
		{
			return true;
		}
		else
		{
			return a.m_capacity < b.m_capacity;
		}
	}

	void* AllocateMemory ( size_t bytes, size_t & new_capacity, size_t align_of /* = 4U */ )
	{
		FreeMemoryList_t::const_iterator aend ( m_free_memory.cend () );
		for( FreeMemoryList_t::iterator it ( m_free_memory.begin ()); it != aend; ++it )
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
						it->m_capacity = 0;
						it->m_ptr = nullptr;

						--m_memlist_elem_count;

						FreeMemoryList_t::iterator t ( it + 1 );
						if( t != aend )
						{
							std::rotate ( it, t, m_free_memory.begin () + m_memlist_elem_count );
						}
						return ret;
					}
				}
			}
		}

		new_capacity = align_of;
		while( new_capacity < bytes /*|| ( new_capacity % align_of != 0) */) new_capacity <<= 1;

		return _mm_malloc ( new_capacity, align_of );
	}

	void FreeMemory ( void * ptr, size_t capacity )
	{
		Assert ( capacity && "Stop using memset everywhere ..." );
		FreeMemoryList_t::reverse_iterator it ( m_free_memory.rbegin () );
		if( it->m_ptr != nullptr || capacity > HMM_MAX_SINGLE_OBJECT_SIZE ) // Pool is full or memory too big
		{
			_mm_free ( ptr );
		}
		else
		{
			it->m_ptr = ptr;
			it->m_capacity = capacity;

			std::sort ( m_free_memory.begin (), m_free_memory.end (), SortMemPool );

			++m_memlist_elem_count;
		}
	}

	void FreePool ()
	{
		for( FreeMemoryList_t::iterator it ( m_free_memory.begin () ); it != m_free_memory.cend (); ++it )
		{
			if( it->m_ptr != nullptr )
			{
				_mm_free ( it->m_ptr );
				it->m_ptr = nullptr;
				it->m_capacity = 0;
				--m_memlist_elem_count;
			}
			else
			{
				return;
			}
		}
	}
}
