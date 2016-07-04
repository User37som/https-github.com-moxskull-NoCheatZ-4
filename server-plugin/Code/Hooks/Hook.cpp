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

#include "Hook.h"

int HookCompare ( HookInfo const * a, HookInfo const * b )
{
	if( a->vf_entry > b->vf_entry ) return 1;
	else if( a->vf_entry < b->vf_entry ) return -1;
	else return 0;
}

void MoveVirtualFunction ( DWORD const * const from, DWORD * const to )
{
#ifdef WIN32
	DWORD dwOld;
	if( !VirtualProtect ( to, sizeof ( DWORD ), PAGE_EXECUTE_READWRITE, &dwOld ) )
	{
		return;
	}
#else // LINUX
	uint32_t psize ( sysconf ( _SC_PAGESIZE ) );
	void *p = ( void * ) ( ( DWORD ) ( to ) & ~( psize - 1 ) );
	if( mprotect ( p, sizeof ( DWORD ) & ( psize - 1 ), PROT_READ | PROT_WRITE | PROT_EXEC ) < 0 )
	{
		return;
	}
#endif // WIN32
	DebugMessage ( Helpers::format ( "MoveVirtualFunction : function 0x%X replaced by 0x%X.", *to, *from ) );
	*to = *from;
#ifdef WIN32
	VirtualProtect ( to, sizeof ( DWORD ), dwOld, &dwOld );
#else // LINUX
	mprotect ( p, sizeof ( DWORD ) & ( psize - 1 ), PROT_READ | PROT_EXEC );
#endif // WIN32
}
