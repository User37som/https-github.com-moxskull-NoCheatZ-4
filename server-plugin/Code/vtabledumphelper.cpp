#include "Interfaces/InterfacesProxy.h"

#include "Misc/Helpers.h"

#ifdef WIN32
#	define DbgBreak() __debugbreak()
#else
#	include <signal.h>
#	define DbgBreak() raise(SIGTRAP)
#endif

/*
	(gdb) set print demangle on
	(gdb) set $i = 0
	(gdb) while $i < 100
     >print $i
     >p /a (*pcur)[$i]
     >set $i = $i + 1
     >end
*/

void HelpMeeee()
{
	using namespace SourceSdk::InterfacesProxy;
	using namespace Helpers;

	// engine callbacks first

	void *** pcur;

	printf("------------------------------\nDumping virtual classes pointers for game ID %d\n", m_game);

	printf("pcur -> m_servergamedll %d\n", m_servergamedll_version);
	pcur = (void ***)m_servergamedll;

	DbgBreak();

	printf("pcur -> m_playerinfomanager %d\n", m_playerinfomanager_version);
	pcur = (void ***)m_playerinfomanager;

	DbgBreak();

	printf("pcur -> m_servergameents %d\n", m_servergameents_version);
	pcur = (void ***)m_servergameents;

	DbgBreak();

	printf("pcur -> m_servergameclients %d\n", m_servergameclients_version);
	pcur = (void ***)m_servergameclients;

	DbgBreak();

	printf("pcur -> m_engineserver %d\n", m_engineserver_version);
	pcur = (void ***)m_engineserver;

	DbgBreak();

	printf("pcur -> m_gameeventmanager %d\n", m_gameeventmanager_version);
	pcur = (void ***)m_gameeventmanager;

	DbgBreak();

	printf("pcur -> m_serverpluginhelpers %d\n", m_serverpluginhelpers_version);
	pcur = (void ***)m_serverpluginhelpers;

	DbgBreak();

	printf("pcur -> m_enginetrace %d\n", m_enginetrace_version);
	pcur = (void ***)m_enginetrace;

	DbgBreak();

	printf("pcur -> m_cvar %d\n", m_enginecvar_version);
	pcur = (void ***)m_cvar;

	DbgBreak();

	if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
	{
		SourceSdk::edict_t_csgo * cur(PEntityOfEntIndex(0));
		SourceSdk::edict_t_csgo * const max_cur(cur + MAX_EDICTS);
		do
		{
#undef GetClassName
			if (cur->GetClassName() != nullptr && cur->GetClassName()[0] != 0)
			{
				printf("pcur -> Entity %s m_pUnk\n", cur->GetClassName());
				pcur = (void ***)cur->m_pUnk;
				DbgBreak();
			}
		} while (++cur <= max_cur);
	}
	else
	{
		SourceSdk::edict_t * cur(PEntityOfEntIndex(0));
		SourceSdk::edict_t * const max_cur(cur + MAX_EDICTS);
		do
		{
#undef GetClassName
			if (cur->GetClassName() != nullptr && cur->GetClassName()[0] != 0)
			{
				printf("pcur -> Entity %s m_pUnk\n", cur->GetClassName());
				pcur = (void ***)cur->m_pUnk;
				DbgBreak();
			}
		} while (++cur <= max_cur);
	}
	
	printf("%X\n", (size_t)pcur);
}
