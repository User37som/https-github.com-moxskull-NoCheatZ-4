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

#ifndef HOOKLISTENERSLIST_H
#define HOOKLISTENERSLIST_H

#include "Misc/HeapMemoryManager.h" // OverrideNew
#include "Players/ProcessFilter.h" // SlotStatus

#include "Misc/ForwardedCommonDefinitions.h" // For other headers that includes this file

template <class C>
struct SortedListener
{
	mutable C * listener;
	size_t priority;
	SlotStatus filter;
};

template <class C>
class HookListenersList
{
	typedef SortedListener<C> inner_type;
public:
	struct alignas( 8 ) elem_t :
		HeapMemoryManager::OverrideNew<8>
	{
		elem_t ()
		{
			m_next = nullptr;
		}

		inner_type m_value;
		elem_t* m_next;
	};

private:

	elem_t* m_first;

public:

	HookListenersList () :
		m_first( nullptr )
	{
	
	};

	~HookListenersList ()
	{
		while( m_first != nullptr )
		{
			Remove ( m_first->m_value.listener );
		}
	}

	elem_t* GetFirst () const
	{
		return m_first;
	}

	/*
	Add a listener sorted by priority.
	*/
	elem_t* Add ( C const * const listener, size_t const priority = 0, SlotStatus const filter = SlotStatus::PLAYER_IN_TESTS )
	{
		if( m_first == nullptr )
		{
			m_first = new elem_t ();
			__assume ( m_first != nullptr );
			m_first->m_value.listener = const_cast< C * const >( listener );
			m_first->m_value.priority = priority;
			m_first->m_value.filter = filter;
			return m_first;
		}
		else
		{
			elem_t* iterator = m_first;
			elem_t* prev = nullptr;
			do
			{
				if( priority <= iterator->m_value.priority )
				{
					// Insert here

					if( prev == nullptr ) // iterator == m_first
					{
						elem_t* const old_first = m_first;
						m_first = new elem_t ();
						__assume ( m_first != nullptr );
						m_first->m_next = old_first;
						m_first->m_value.listener = const_cast< C * const >( listener );
						m_first->m_value.priority = priority;
						m_first->m_value.filter = filter;
						return m_first;
					}
					else
					{
						prev->m_next = new elem_t ();
						prev->m_next->m_next = iterator;
						prev->m_next->m_value.listener = const_cast< C * const >( listener );
						prev->m_next->m_value.priority = priority;
						prev->m_next->m_value.filter = filter;
						return prev->m_next;
					}
				}

				prev = iterator;
				iterator = iterator->m_next;
			}
			while( iterator != nullptr );

			prev->m_next = new elem_t ();
			__assume ( prev->m_next != nullptr );
			prev->m_next->m_value.listener = const_cast< C * const >( listener );
			prev->m_next->m_value.priority = priority;
			prev->m_next->m_value.filter = filter;
			return prev->m_next;
		}
	}

	/*
	Find this listener and remove it from list
	*/
	void Remove ( C const * const listener )
	{
		elem_t* iterator = m_first;
		elem_t* prev = nullptr;
		while( iterator != nullptr )
		{
			if( iterator->m_value.listener == listener )
			{
				elem_t* to_remove = iterator;
				if( prev == nullptr )
				{
					m_first = iterator->m_next;
				}
				else
				{
					prev->m_next = iterator->m_next;
				}
				iterator = iterator->m_next;
				delete to_remove;
				return;
			}
			prev = iterator;
			iterator = iterator->m_next;
		}
	}

	/*
	Find by listener
	*/
	elem_t* const FindByListener ( C const * const listener, C const * const exclude_me = nullptr ) const
	{
		elem_t* iterator = m_first;
		while( iterator != nullptr )
		{
			if( iterator->m_value.listener != exclude_me )
			{
				if( iterator->m_value.listener == listener )
				{
					return iterator;
				}
			}
			iterator = iterator->m_next;
		}
		return nullptr;
	}
};

#endif // HOOKLISTENERSLIST_H
