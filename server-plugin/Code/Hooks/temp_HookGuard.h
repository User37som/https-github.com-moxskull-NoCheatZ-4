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

#ifndef HOOKGUARD_H
#define HOOKGUARD_H

#include "Players/ProcessFilter.h" // SlotStatus
#include "Systems/Logger.h" // Msg, DebugMessage
#include "Misc/Helpers.h" // format
#include "Hook.h" // GetModuleNameFromMemoryAddress, commondef

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

	HookInfo ();
	HookInfo ( const HookInfo& other );
	HookInfo& operator=( const HookInfo& other );
	HookInfo ( void* class_ptr, int vfid, DWORD new_fn );
	HookInfo ( void* class_ptr, int vfid );

	bool operator== ( const HookInfo& other ) const;
};

typedef CUtlVector<HookInfo> hooked_list_t;

extern int HookCompare ( HookInfo const * a, HookInfo const * b );

template <class CallerTicket>
class HookGuard :
	public Singleton< HookGuard < CallerTicket > >
{
private:
	typedef Singleton< HookGuard < CallerTicket > > singleton_class;

	hooked_list_t m_list;

public:
	HookGuard () :
		singleton_class ()
	{

	}

	virtual ~HookGuard () final
	{

	}

	void VirtualTableHook ( HookInfo& info, bool force = false )
	{
		if( m_list.HasElement ( info ) && !force ) return;

		//info.oldFn = 0;
#ifdef WIN32
		DWORD dwOld;
		if( !VirtualProtect ( info.vf_entry, 2 * sizeof ( DWORD* ), PAGE_EXECUTE_READWRITE, &dwOld ) )
		{
			Logger::GetInstance ()->Msg<MSG_ERROR> ( Helpers::format ( "VirtualTableHook : VirtualProtect error -> Cannot hook function in %s", GetModuleNameFromMemoryAddress ( *info.vf_entry ).c_str () ) );
			return;
		}
#else // LINUX
		uint32_t psize ( sysconf ( _SC_PAGESIZE ) );
		void *p ( ( void * ) ( ( DWORD ) ( info.vf_entry ) & ~( psize - 1 ) ) );
		if( mprotect ( p, ( ( 2 * sizeof ( void * ) ) + ( ( DWORD ) ( info.vf_entry ) & ( psize - 1 ) ) ), PROT_READ | PROT_WRITE | PROT_EXEC ) < 0 )
		{
			Logger::GetInstance ()->Msg<MSG_ERROR> ( Helpers::format ( "VirtualTableHook : mprotect error -> Cannot hook function in %s", GetModuleNameFromMemoryAddress ( *info.vf_entry ).c_str () ) );
			return;
		}
#endif // WIN32

		bool can_hook = true;

		if( info.oldFn && info.oldFn != *info.vf_entry )
		{
			Logger::GetInstance ()->Msg<MSG_WARNING> ( Helpers::format ( "VirtualTableHook : Unexpected virtual table value in VirtualTableHook. Module %s might be in conflict.", GetModuleNameFromMemoryAddress ( *info.vf_entry ).c_str () ) );
			can_hook = false;
		}
		else if( info.newFn == *info.vf_entry )
		{
			if( !m_list.HasElement ( info ) )
			{
				Logger::GetInstance ()->Msg<MSG_WARNING> ( "VirtualTableHook : Virtual function pointer was the same but not registered as hooked ..." );
				m_list.AddToTail ( info );
				m_list.Sort ( HookCompare );
				can_hook = false;
			}
		}

		if( can_hook )
		{
			info.oldFn = *info.vf_entry;
			*info.vf_entry = info.newFn;
			DebugMessage ( Helpers::format ( "VirtualTableHook : function 0x%X at 0x%X in %s replaced by 0x%X from %s.", info.oldFn, info.vf_entry, GetModuleNameFromMemoryAddress ( info.oldFn ).c_str (), info.newFn, GetModuleNameFromMemoryAddress ( info.newFn ).c_str () ) );

			if( !m_list.HasElement ( info ) )
			{
				m_list.AddToTail ( info );
				m_list.Sort ( HookCompare );
			}
		}

#ifdef WIN32
		VirtualProtect ( info.vf_entry, 2 * sizeof ( DWORD* ), dwOld, &dwOld );
#else // LINUX
		mprotect ( p, ( ( 2 * sizeof ( void * ) ) + ( ( DWORD ) ( info.vf_entry ) & ( psize - 1 ) ) ), PROT_READ | PROT_EXEC );
#endif // WIN32
	}

	// Find by virtual table entry address
	DWORD RT_GetOldFunction ( void* class_ptr, int vfid ) const
	{
		int const it ( m_list.Find ( HookInfo ( class_ptr, vfid ) ) );
		if( it != -1 )
		{
			return m_list[ it ].oldFn;
		}
		else
		{
			return 0;
		}
	}

	// Only find by virtual table base (Remove need to call ConfigManager), class_ptr is converted to virtual table base
	DWORD RT_GetOldFunctionByVirtualTable ( void* class_ptr ) const
	{
		DWORD* const vt ( ( ( DWORD* )*( DWORD* ) class_ptr ) );
		for( hooked_list_t::const_iterator it ( m_list.begin () ); it != m_list.end (); ++it )
		{
			if( it->pInterface == vt )
			{
				return it->oldFn;
			}
		}
		return 0;
	}

	// Only find by virtual table base (Remove need to call ConfigManager), class_ptr is the original instance
	DWORD RT_GetOldFunctionByInstance ( void* const class_ptr ) const
	{
		for( hooked_list_t::const_iterator it ( m_list.begin () ); it != m_list.end (); ++it )
		{
			if( it->origEnt == class_ptr )
			{
				return it->oldFn;
			}
		}
		return 0;
	}

	void UnhookAll ()
	{
		for( hooked_list_t::iterator it ( m_list.begin () ); it != m_list.end (); ++it )
		{
			std::swap ( it->newFn, it->oldFn );
			VirtualTableHook ( *it, true ); // unhook
		}
		m_list.RemoveAll ();
	}
};

#endif // HOOKGUARD_H
