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

#ifndef HOOK_H
#define HOOK_H

#include <limits>

#ifdef WIN32
#	include "Misc/include_windows_headers.h"
#	define HOOKFN_EXT __thiscall
#	define HOOKFN_INT __fastcall
#else
#	include <sys/mman.h>
#	include <unistd.h>
#	define HOOKFN_EXT
#	define HOOKFN_INT __attribute__((cdecl))
#endif

#include "Interfaces/iserverunknown.h"

#include "Players/NczPlayerManager.h"

/* 
	Used to replace a pointer in one virtual table.
	Returns the old function pointer or 0 in case of error.
*/
DWORD VirtualTableHook(DWORD* classptr, const int vtable, const DWORD newInterface, const DWORD expectedInterface = 0 );
void MoveVirtualFunction(DWORD const * const from, DWORD * const to);

class CBaseEntity;

/*
	HookInfo will store data about one hooked instance.
	It is often used in a list when we need to hook thousands of classes that don't really have the same virtual table.
*/
template <class C = SourceSdk::CBaseEntity>
struct HookInfo
{
	C* origEnt;
	DWORD* pInterface;
	DWORD oldFn;
	
	HookInfo()
	{
		origEnt = nullptr;
		pInterface = nullptr;
		oldFn = 0;
	};
	HookInfo(const HookInfo& other)
	{
		origEnt = other.origEnt;
		pInterface = other.pInterface;
		oldFn = other.oldFn;
	};
	HookInfo operator=(const HookInfo& other)
	{
		return HookInfo(other);
	};
	HookInfo(DWORD* iface)
	{
		pInterface = iface;
	};

	bool operator== (const HookInfo& other) const
	{
		return (pInterface == other.pInterface);
	};
};

template <class C = SourceSdk::CBaseEntity>
class HookList
{
	typedef HookInfo<C> inner_type;
public:
	struct elem_t
	{
		inner_type* m_value;
		elem_t* m_next;

		~elem_t()
		{
			delete m_value;
		}
	};

private:

	elem_t* m_first;

public:

	HookList()
	{
		m_first = nullptr;
	}
	~HookList()
	{
		while (m_first != nullptr)
		{
			Remove(m_first);
		}
	}

	elem_t* GetFirst() const
	{
		return m_first;
	}

	/*
		Add an element to front. The value is already allocated using new before calling Add.
	*/
	elem_t* Add(inner_type* value)
	{
		if (m_first != nullptr)
		{
			elem_t* const old_first = m_first;
			m_first = new elem_t;
			m_first->m_next = old_first;
			m_first->m_value = value;
		}
		else
		{
			m_first = new elem_t;
			m_first->m_next = nullptr;
			m_first->m_value = value;
		}
		return m_first;
	}

	/*
		Find this exact pointer, call destructor and remove it from list
	*/
	void Remove(inner_type* value)
	{
		elem_t* iterator = m_first;
		elem_t* prev = nullptr;
		while (iterator != nullptr)
		{
			if (iterator->m_value == value)
			{
				elem_t* to_remove = iterator;
				if (prev == nullptr)
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
		Find this iterator and remove it from list
	*/
	elem_t* Remove(elem_t* it)
	{
		elem_t* iterator = m_first;
		elem_t* prev = nullptr;
		elem_t* return_value = nullptr;
		while (iterator != nullptr)
		{
			if (iterator == it)
			{
				return_value = iterator->m_next;
				if (prev == nullptr)
				{
					m_first = iterator->m_next;
				}
				else
				{
					prev->m_next = iterator->m_next;
				}
				delete it;
				return return_value;
			}
			prev = iterator;
			iterator = iterator->m_next;
		}

		return return_value;
	}

	/*
		Returns the HookInfo by vtable pointer
	*/
	elem_t* const FindByVtable(DWORD const * const vtable) const
	{
		elem_t* iterator = m_first;
		while (iterator != nullptr)
		{
			if (iterator->m_value->pInterface == vtable)
			{
				return iterator;
			}
			iterator = iterator->m_next;
		}
		return nullptr;
	}

	/*
		Returns the HookInfo by instance
	*/
	elem_t* const FindByInstance(C const * const instance) const
	{
		elem_t* iterator = m_first;
		while (iterator != nullptr)
		{
			if (iterator->m_value->origEnt == instance)
			{
				return iterator;
			}
			iterator = iterator->m_next;
		}
		return nullptr;
	}

	/*
	Returns the HookInfo by function
	*/
	elem_t* const FindByFunction(DWORD const fn, elem_t const * const exclude_me = nullptr) const
	{
		elem_t* iterator = m_first;
		while (iterator != nullptr)
		{
			if (iterator != exclude_me)
			{
				if (iterator->m_value->oldFn == fn)
				{
					return iterator;
				}
			}
			iterator = iterator->m_next;
		}
		return nullptr;
	}

	/*
		Find this exact pointer
	*/
	elem_t* const Find(inner_type const * const value) const
	{
		elem_t* iterator = m_first;
		while (iterator != nullptr)
		{
			if (iterator->m_value == value)
			{
				return iterator;
			}
			iterator = iterator->m_next;
		}
		return nullptr;
	}

	/*
		Find this value
	*/
	elem_t* const Find(inner_type const & value) const
	{
		elem_t* iterator = m_first;
		while (iterator != nullptr)
		{
			if (*(iterator->m_value) == value)
			{
				return iterator;
			}
			iterator = iterator->m_next;
		}
		return nullptr;
	}
};

/*
	Get the virtual table pointer of class.
	x is a pointer to the target class.
*/


/*
	Some testers need to receive callbacks in a specific order against other testers.
	This is used to save some operations when a tester detected something.
*/
template <class C>
struct SortedListener
{
	C* listener;
	size_t priority;
	SlotStatus filter;
};

template <class C>
class HookListenersList
{
	typedef SortedListener<C> inner_type;
public:
	struct elem_t
	{
		elem_t()
		{
			m_next = nullptr;
		}

		inner_type m_value;
		elem_t* m_next;
	};

private:

	elem_t* m_first;

public:

	HookListenersList()
	{
		m_first = nullptr;
	}
	~HookListenersList()
	{
		while (m_first != nullptr)
		{
			Remove(m_first->m_value.listener);
		}
	}

	elem_t* GetFirst() const
	{
		return m_first;
	}

	/*
		Add a listener sorted by priority.
	*/
	elem_t* Add(C * const listener, size_t const priority = 0, SlotStatus const filter = PLAYER_IN_TESTS)
	{
		if (m_first == nullptr)
		{
			m_first = new elem_t();
			m_first->m_value.listener = listener;
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
				if (priority <= iterator->m_value.priority)
				{
					// Insert here

					if (prev == nullptr) // iterator == m_first
					{
						elem_t* const old_first = m_first;
						m_first = new elem_t();
						m_first->m_next = old_first;
						m_first->m_value.listener = listener;
						m_first->m_value.priority = priority;
						m_first->m_value.filter = filter;
						return m_first;
					}
					else
					{
						prev->m_next = new elem_t();
						prev->m_next->m_next = iterator;
						prev->m_next->m_value.listener = listener;
						prev->m_next->m_value.priority = priority;
						prev->m_next->m_value.filter = filter;
						return prev->m_next;
					}
				}

				prev = iterator;
				iterator = iterator->m_next;
			}
			while (iterator != nullptr);

			prev->m_next = new elem_t();
			prev->m_next->m_value.listener = listener;
			prev->m_next->m_value.priority = priority;
			prev->m_next->m_value.filter = filter;
			return prev->m_next;
		}
	}

	/*
		Find this listener and remove it from list
	*/
	void Remove(C const * const listener)
	{
		elem_t* iterator = m_first;
		elem_t* prev = nullptr;
		while (iterator != nullptr)
		{
			if (iterator->m_value.listener == listener)
			{
				elem_t* to_remove = iterator;
				if (prev == nullptr)
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
	elem_t* const FindByListener(C const * const listener, C const * const exclude_me = nullptr) const
	{
		elem_t* iterator = m_first;
		while (iterator != nullptr)
		{
			if (iterator->m_value.listener != exclude_me)
			{
				if (iterator->m_value.listener == listener)
				{
					return iterator;
				}
			}
			iterator = iterator->m_next;
		}
		return nullptr;
	}
};

#endif
