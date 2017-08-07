#include "DearMetamodSource.h"
#include "SigScan.h"

#include "ConCommandHookListener.h"
#include "OnGroundHookListener.h"
#include "PlayerRunCommandHookListener.h"
#include "SetTransmitHookListener.h"
#include "ThinkPostHookListener.h"
#include "UserMessageHookListener.h"
#include "WeaponHookListener.h"

#ifdef GNUC
#	include "link.h"
#endif

HookGuard<SourceHookSafety> local_HookGuardSourceHookSafety;

bool SourceHookSafety::m_reverted(false);

SourceHookSafety::SourceHookSafety() :
	g_SourceHook(nullptr)
{
}

SourceHookSafety::~SourceHookSafety()
{
}

void SourceHookSafety::TryHookMMSourceHook()
{
	if (g_SourceHook) return;

	DebugMessage("Trying to locate metamod ...");

	if (SourceSdk::InterfacesProxy::ICvar_FindCommand("meta"))
	{
		DebugMessage("meta command found. Loading module address ...");
		basic_string gamedir;
		SourceSdk::InterfacesProxy::GetGameDir(gamedir);

		basic_string modulename("metamod.2.");

		if (SourceSdk::InterfacesProxy::m_game == SourceSdk::CounterStrikeGlobalOffensive)
		{
			modulename.append("csgo");
		}
		else
		{
			modulename.append("css");
		}

#ifdef WIN32
		modulename.append(".dll");

		HMODULE mm_module_handle(GetModuleHandleA(modulename.c_str()));
		if (mm_module_handle != NULL)
		{
			DebugMessage("Trying to hook sourcehook ( badass ) ...");

			mem_byte const metafactory_sig_code[48] = {
				0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x10, 0x83, 0xEC,
				0x08, 0x85, 0xC0, 0x74, 0x06, 0xC7, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x53, 0x8B, 0x5D, 0x08, 0x85,
				0xDB, 0x75, 0x09, 0x33, 0xC0, 0x5B, 0x8B, 0xE5,
				0x5D, 0xC2, 0x0C, 0x00, 0xB9, 0x54, 0x43, 0x02,
				0x10, 0x8B, 0xC3, 0xEB, 0x03, 0x00, 0x00, 0x00
			};

			mem_byte const metafactory_sig_mask[48] = {
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF,
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00,
				0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00
			};

			sig_ctx ctx(metafactory_sig_code, metafactory_sig_mask, 45, 0x67);

#else
		modulename.append(".so");

		basic_string relpath(Helpers::format("./%s/addons/metamod/bin/%s", gamedir.c_str(), modulename.c_str()));

		void ** modinfo = (void **)dlopen(relpath.c_str(), RTLD_NOW | RTLD_NOLOAD);
		void * mm_module_handle = nullptr;
		if (modinfo != NULL)
		{
			DebugMessage("Trying to hook sourcehook ( badass ) ...");

			//mm_module_handle = dlsym(modinfo, ".init_proc");
			// FIXME : Use link_map to get memory bounds of the module
			mm_module_handle = *modinfo;
			dlclose(modinfo);
		}
		if (mm_module_handle)
		{
			mem_byte const metafactory_sig_code[48] = {
				0x55, 0x53, 0x57, 0x56, 0x83, 0xEC, 0x2C, 0x8B,
				0x44, 0x24, 0x4C, 0x8B, 0x5C, 0x24, 0x44, 0x85,
				0xC0, 0x74, 0x06, 0xC7, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x31, 0xC0, 0x85, 0xDB, 0x0F, 0x84, 0x3D,
				0x01, 0x00, 0x00, 0x8B, 0x74, 0x24, 0x48, 0x89,
				0x1C, 0x24, 0xC7, 0x44, 0x24, 0x04, 0x3A, 0xFC
			};

			mem_byte const metafactory_sig_mask[48] = {
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00,
				0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
				0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00
			};

			sig_ctx ctx(metafactory_sig_code, metafactory_sig_mask, 45, 0x89);

#endif

			ScanMemoryRegion(reinterpret_cast<mem_byte *>(mm_module_handle), reinterpret_cast<mem_byte *>(mm_module_handle) + 0x30000, &ctx);

			if (ctx.m_out != nullptr)
			{
				g_SourceHook = reinterpret_cast<ISourceHook_Skeleton *>(*(reinterpret_cast<size_t**>(ctx.m_out)));

				if (g_SourceHook != nullptr)
				{
					DebugMessage(Helpers::format("g_SourceHook is at 0x%X (Interface Version %d, Impl Version %d)", g_SourceHook, g_SourceHook->GetIfaceVersion(), g_SourceHook->GetImplVersion()));
					HookInfo haddhook(g_SourceHook, 2, (DWORD)my_AddHook);
					HookInfo hremovehook(g_SourceHook, 3, (DWORD)my_RemoveHook);
					HookInfo hremovehookid(g_SourceHook, 4, (DWORD)my_RemoveHookById);

					local_HookGuardSourceHookSafety.VirtualTableHook(haddhook, "ISourceHook::AddHook");
					local_HookGuardSourceHookSafety.VirtualTableHook(hremovehook, "ISourceHook::RemoveHook");
					local_HookGuardSourceHookSafety.VirtualTableHook(hremovehookid, "ISourceHook::RemoveHookById");
				}
				else
				{
					g_Logger.Msg<MSG_ERROR>("Failed to get ISourceHook interface.");
				}
			}
			else
			{
				g_Logger.Msg<MSG_ERROR>("Sigscan failed for MetaFactory.");
			}
		}
		else
		{
			g_Logger.Msg<MSG_ERROR>("metamod module address not found.");
		}
	}
	else
	{
		DebugMessage("metamod module not loaded.");
	}
}

void SourceHookSafety::UnhookMMSourceHook()
{
	if (!g_SourceHook) return;

	local_HookGuardSourceHookSafety.UnhookAll();
}

void SourceHookSafety::ProcessRevertAll()
{
	if (!m_reverted) // In case it gets called multiple times in stack
	{
		g_Logger.SetBypassServerConsoleMsg(true);

		g_HookGuardConCommandHookListener.RevertAll();
		g_HookGuardOnGroundHookListener.RevertAll();
		g_HookGuardPlayerRunCommandHookListener.RevertAll();
		g_HookGuardSetTransmitHookListener.RevertAll();
		g_HookGuardThinkPostHookListener.RevertAll();
		g_HookGuardUserMessageHookListener.RevertAll();
		g_HookGuardWeaponHookListener.RevertAll();

		m_reverted = true;
	}
}

void SourceHookSafety::ProcessRehootAll()
{
	if (m_reverted)
	{
		g_HookGuardConCommandHookListener.RehookAll();
		g_HookGuardOnGroundHookListener.RehookAll();
		g_HookGuardPlayerRunCommandHookListener.RehookAll();
		g_HookGuardSetTransmitHookListener.RehookAll();
		g_HookGuardThinkPostHookListener.RehookAll();
		g_HookGuardUserMessageHookListener.RehookAll();
		g_HookGuardWeaponHookListener.RehookAll();

		g_Logger.SetBypassServerConsoleMsg(false);

		m_reverted = false;
	}
}

#ifdef GNUC
int HOOKFN_INT SourceHookSafety::my_AddHook(void * isourcehook, int plug, ISourceHook_Skeleton::AddHookMode mode, void * iface, int thisptr_offs, ISourceHook_Skeleton::HookManagerPubFunc myHookMan, void * handler, bool post)
#else
int HOOKFN_INT SourceHookSafety::my_AddHook(void * isourcehook, void * weak, int plug, ISourceHook_Skeleton::AddHookMode mode, void * iface, int thisptr_offs, ISourceHook_Skeleton::HookManagerPubFunc myHookMan, void * handler, bool post)
#endif
{
	typedef int (HOOKFN_EXT *AddHook_fn)(void* isourcehook, int plug, ISourceHook_Skeleton::AddHookMode mode, void *iface, int thisptr_offs, ISourceHook_Skeleton::HookManagerPubFunc myHookMan, void *handler, bool post);
	
	ProcessRevertAll();

	ST_W_STATIC AddHook_fn gpOldFn;
	*(DWORD*)&(gpOldFn) = local_HookGuardSourceHookSafety.RT_GetOldFunction(isourcehook, 2);
	int ret ( gpOldFn(isourcehook, plug, mode, iface, thisptr_offs, myHookMan, handler, post) );

	ProcessRehootAll();

	return ret;
}

#ifdef GNUC
bool HOOKFN_INT SourceHookSafety::my_RemoveHook(void * isourcehook, int plug, void * iface, int thisptr_offs, ISourceHook_Skeleton::HookManagerPubFunc myHookMan, void * handler, bool post)
#else
bool HOOKFN_INT SourceHookSafety::my_RemoveHook(void * isourcehook, void * weak, int plug, void * iface, int thisptr_offs, ISourceHook_Skeleton::HookManagerPubFunc myHookMan, void * handler, bool post)
#endif
{
	typedef int (HOOKFN_EXT *RemoveHook_fn)(void* isourcehook, int plug, void *iface, int thisptr_offs, ISourceHook_Skeleton::HookManagerPubFunc myHookMan, void *handler, bool post);

	ProcessRevertAll();

	ST_W_STATIC RemoveHook_fn gpOldFn;
	*(DWORD*)&(gpOldFn) = local_HookGuardSourceHookSafety.RT_GetOldFunction(isourcehook, 3);
	int ret( gpOldFn(isourcehook, plug, iface, thisptr_offs, myHookMan, handler, post));

	ProcessRehootAll();

	return ret;
}

#ifdef GNUC
bool HOOKFN_INT SourceHookSafety::my_RemoveHookById(void* isourcehook, int id)
#else
bool HOOKFN_INT SourceHookSafety::my_RemoveHookById(void* isourcehook, void* weak, int id)
#endif
{
	typedef int (HOOKFN_EXT *RemoveHookById_fn)(void* isourcehook, int id);

	ProcessRevertAll();

	ST_W_STATIC RemoveHookById_fn gpOldFn;
	*(DWORD*)&(gpOldFn) = local_HookGuardSourceHookSafety.RT_GetOldFunction(isourcehook, 4);
	bool ret(gpOldFn(isourcehook, id));

	ProcessRehootAll();

	return ret;
}

SourceHookSafety g_SourceHookSafety;
