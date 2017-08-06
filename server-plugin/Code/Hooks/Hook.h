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
#include <iostream>
#include <algorithm>

#ifdef WIN32
#	include "Misc/include_windows_headers.h"
#	define HOOKFN_EXT __thiscall
#	define HOOKFN_INT __fastcall
#else
#	include <sys/mman.h>
#	include <unistd.h>
#	define HOOKFN_EXT
#	define HOOKFN_INT __attribute__((cdecl))
#	include <errno.h>
#endif

#include "SdkPreprocessors.h"
#include "Interfaces/iserverunknown.h"

#include "Preprocessors.h"
#include "Players/NczPlayerManager.h"
#include "Misc/HeapMemoryManager.h"
#include "Systems/Logger.h"
#include "Misc/Helpers.h"

/*
	Used to replace a pointer in one virtual table.
	Returns the old function pointer or 0 in case of error.
*/

basic_string GetModuleNameFromMemoryAddress ( DWORD ptr );

void MoveVirtualFunction ( DWORD const * const from, DWORD * const to );
DWORD ReplaceVirtualFunctionByFakeVirtual(DWORD const replace_by, DWORD * const replace_here);

class RAII_MemoryProtectDword
{
private:
	DWORD * m_addr;
	DWORD m_dwold;
	bool * m_errored;
	static uint32_t m_pagesize;
	void * a;
	size_t b;

#ifdef GNUC
	void SetPagesize();
#endif
public:
	RAII_MemoryProtectDword(DWORD * addr, bool * err);

	~RAII_MemoryProtectDword();
};

class CBaseEntity;

/*
	HookInfo will store data about one hooked instance.
	It is often used in a list when we need to hook thousands of classes that don't really have the same virtual table.
*/

struct HookInfo
{
	DWORD oldFn; // Old function address that we replace in the vtable
	void* origEnt; // Address of the class used to determine virtual table base
	DWORD* pInterface; // Virtual table base
	DWORD* vf_entry; // Pointer to the entry in the virtual table
	DWORD newFn; // New function address that will be at *vf_entry after hook
	char const * debugname;
	bool ishooked; // Is used to tell us if the function is actually hooked or not. This is used when we try to revert the changes and rehook.

	HookInfo ()
	{
		memset ( this, 0, sizeof ( HookInfo ) );
	}
	HookInfo ( const HookInfo& other )
	{
		memcpy ( this, &other, sizeof ( HookInfo ) );
	}
	HookInfo& operator=( const HookInfo& other )
	{
		memcpy ( this, &other, sizeof ( HookInfo ) );
		return *this;
	}
	HookInfo ( void* class_ptr, int vfid, DWORD new_fn ) :
		oldFn ( 0 ),
		origEnt( class_ptr ),
		pInterface ( ( DWORD* )*( DWORD* ) origEnt ),
		vf_entry ( &( pInterface[ vfid ] ) ),
		newFn ( new_fn ),
		debugname( nullptr ),
		ishooked( false )
	{}
	HookInfo ( void* class_ptr, int vfid ) : // This constructor is used only to create a reference for comparison
		//origEnt ( class_ptr ),
		//pInterface ( ( DWORD* )*( DWORD* ) origEnt ),
		vf_entry ( (*(DWORD**)class_ptr) + vfid )//,
		//debugname( nullptr ),
		//ishooked( false )
	{}

	inline bool operator== (const HookInfo& other) const;
};

inline bool HookInfo::operator== (const HookInfo& other) const
{
	return (vf_entry == other.vf_entry);
}

typedef CUtlVector<HookInfo> hooked_list_t;

extern int HookCompare ( HookInfo const * a, HookInfo const * b );

template <class CallerTicket>
class HookGuard :
	public Singleton
{
private:
	hooked_list_t m_list;

public:
	HookGuard () : Singleton ()
	{}
	virtual ~HookGuard () final
	{};

	void VirtualTableHook ( HookInfo& info, char const * debugname = "", bool force = false )
	{
		if( m_list.HasElement ( info ) && !force ) return;

		if (info.debugname)
			debugname = info.debugname;

		bool err = false;
		RAII_MemoryProtectDword z(info.vf_entry, &err);
		if (!err)
		{
			bool can_hook = true;

			if (info.oldFn && info.oldFn != SafePtrDeref(info.vf_entry))
			{
				g_Logger.Msg<MSG_WARNING>(Helpers::format("VirtualTableHook %s: Unexpected virtual table value in VirtualTableHook. Module %s might be in conflict.", debugname, GetModuleNameFromMemoryAddress(SafePtrDeref(info.vf_entry)).c_str()));
				can_hook = force;
			}
			else if (info.newFn == *info.vf_entry)
			{
				if (!m_list.HasElement(info))
				{
					g_Logger.Msg<MSG_WARNING>(Helpers::format("VirtualTableHook %s: Virtual function pointer 0x%X was the same but not registered as hooked ...", debugname, info.newFn));
					info.ishooked = true;
					info.debugname = debugname;
					m_list.AddToTail(info);
					m_list.Sort(HookCompare);
					can_hook = false;
				}
			}

			if (can_hook)
			{
				info.oldFn = *info.vf_entry;
				*info.vf_entry = info.newFn;
				DebugMessage(Helpers::format("VirtualTableHook %s: function 0x%X at 0x%X in %s replaced by 0x%X from %s.", debugname, info.oldFn, info.vf_entry, GetModuleNameFromMemoryAddress(info.oldFn).c_str(), info.newFn, GetModuleNameFromMemoryAddress(info.newFn).c_str()));

				if (!m_list.HasElement(info))
				{
					info.ishooked = true;
					info.debugname = debugname;
					m_list.AddToTail(info);
					m_list.Sort(HookCompare);
				}
				else
				{
					info.ishooked = !info.ishooked;
				}
			}
		}
	}

	// Find by virtual table entry address
	inline DWORD RT_GetOldFunction ( void* class_ptr, int vfid ) const
	{
		int it ( m_list.Find ( HookInfo ( class_ptr, vfid ) ) );
		return m_list[it].oldFn; // let it crash because it's not supposed to be not found anyway. just saved a tiny branch-miss.

		/*if( it != -1 )
		{
			return m_list[ it ].oldFn;
		}
		else
		{
			return 0;
		}*/
	}

	// Only find by virtual table base (Remove need to call ConfigManager), class_ptr is converted to virtual table base
	DWORD RT_GetOldFunctionByVirtualTable ( void* class_ptr ) const
	{
#ifndef DEBUG
		DWORD* vt ( ( ( DWORD* )*( DWORD* ) class_ptr ) );
		for( hooked_list_t::const_iterator it ( m_list.begin () ); it != m_list.end (); ++it )
		{
			if( it->pInterface == vt )
			{
				return it->oldFn;
			}
		}
		return 0;
#else
		DWORD* vt(((DWORD*)*(DWORD*)class_ptr));
		DWORD oldfn ( 0 );
		bool found ( false );
		for (hooked_list_t::const_iterator it(m_list.begin()); it != m_list.end(); ++it)
		{
			if (it->pInterface == vt)
			{
				// Do not find another one with a different function, please ...
				Assert(found == false || oldfn == it->oldFn);
				oldfn = it->oldFn;
				found = true;
			}
		}
		return oldfn;
#endif
	}

	// Only find by virtual table base (Remove need to call ConfigManager), class_ptr is the original instance
	DWORD RT_GetOldFunctionByInstance ( void* class_ptr ) const
	{
#ifndef DEBUG
		for( hooked_list_t::const_iterator it ( m_list.begin () ); it != m_list.end (); ++it )
		{
			if( it->origEnt == class_ptr )
			{
				return it->oldFn;
			}
		}
		return 0;
#else
		DWORD oldfn ( 0 );
		bool found ( false );
		for (hooked_list_t::const_iterator it(m_list.begin()); it != m_list.end(); ++it)
		{
			if (it->origEnt == class_ptr)
			{
				// Do not find another one with a different function, please ...
				Assert(found == false || oldfn == it->oldFn);
				oldfn = it->oldFn;
				found = true;
			}
		}
		return oldfn;
#endif
	}

	void RevertAll()
	{
		for (hooked_list_t::iterator it(m_list.begin()); it != m_list.end(); ++it)
		{
			if (it->ishooked)
			{
				std::swap(it->newFn, it->oldFn);

				// The target function might have changed.
				it->oldFn = 0;

				VirtualTableHook(*it, "Invert", true);
			}
		}
	}

	void RehookAll()
	{
		for (hooked_list_t::iterator it(m_list.begin()); it != m_list.end(); ++it)
		{
			if (!it->ishooked)
			{
				std::swap(it->newFn, it->oldFn);

				// The target function might have changed.
				it->oldFn = 0;

				VirtualTableHook(*it, "Rehook", true);
			}
		}
	}

	void UnhookAll ()
	{
		RevertAll();
		m_list.RemoveAll ();
	}
};

/*
	Some testers need to receive callbacks in a specific order against other testers.
	This is used to save some operations when a tester detected something.
*/
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
	struct elem_t :
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

	HookListenersList ()
	{
		m_first = nullptr;
	}
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

#endif
