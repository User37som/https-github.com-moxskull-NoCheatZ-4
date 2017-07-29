// ... could we work at the same time in the same server without killing it ?

#ifndef DEARMETAMOD_H
#define DEARMETAMOD_H

/*
	Concept solution 1 :
	- Hook SourceHook.
	- Unhook us everytime before sourcehook wants to hook/unhook something we already hooked.
	- Rehook us after.

	Case 1 : We hooked the function at first before MM.
	Event										| Current hook sequence         
	- MM wants to hook, we intercept it.        | Server -> NCZ -> Server
	- Revert everything.                        | Server -> Server
	- Let MM hook it.							| Server -> MM -> Server
	- Rehook us.								| Server -> NCZ -> MM -> Server
	- MM wants to unhook, we intercept it.      | Server -> NCZ -> MM -> Server
	- Revert everything.                        | Server -> MM -> Server
	- Let MM unhook.                            | Server -> Server
	- Rehook.                                   | Server -> NCZ -> Server

	Case 2 : We are unaware that MM hooked a function before us.
	Event										| Current hook sequence
	- Initial state.                            | Server -> MM -> Server
	- We hook the function.                     | Server -> NCZ -> MM -> Server
	- MM wants to unhook, we intercept it.      | Server -> NCZ -> MM -> Server
	- Revert everything.                        | Server -> MM -> Server
	- Let MM unhook it.							| Server -> Server
	- Rehook us.								| Server -> NCZ -> Server
	- Now similar as case 1


*/

#include "Preprocessors.h"
#include "Hook.h"

class ISourceHook_Skeleton
{
public:
	enum AddHookMode
	{
		Hook_Normal,
		Hook_VP,
		Hook_DVP
	};
	typedef int(*HookManagerPubFunc)(bool store, void *hi);

	virtual int GetIfaceVersion() = 0;
	virtual int GetImplVersion() = 0;
	virtual int AddHook(int plug, AddHookMode mode, void *iface, int thisptr_offs, HookManagerPubFunc myHookMan, void *handler, bool post) = 0;
	virtual bool RemoveHook(int plug, void *iface, int thisptr_offs, HookManagerPubFunc myHookMan, void *handler, bool post) = 0;
	virtual bool RemoveHookByID(int hookid) = 0;
	virtual bool PauseHookByID(int hookid) = 0;
	virtual bool UnpauseHookByID(int hookid) = 0;
	virtual void SetRes(int res) = 0;
	virtual int GetPrevRes() = 0;
	virtual int GetStatus() = 0;
	virtual const void *GetOrigRet() = 0;
	virtual const void *GetOverrideRet() = 0;
	virtual void *GetIfacePtr() = 0;
	virtual void *GetOverrideRetPtr() = 0;
	virtual void RemoveHookManager(int plug, HookManagerPubFunc pubFunc) = 0;
	virtual void SetIgnoreHooks(void *vfnptr) = 0;
	virtual void ResetIgnoreHooks(void *vfnptr) = 0;
	virtual void *GetOrigVfnPtrEntry(void *vfnptr) = 0;
	virtual void DoRecall() = 0;
	virtual void *SetupHookLoop(void *hi, void *vfnptr, void *thisptr, void **origCallAddr, int *statusPtr, int *prevResPtr, int *curResPtr, const void *origRetPtr, void *overrideRetPtr) = 0;
	virtual void EndContext(void *pCtx) = 0;
};

class SourceHookSafety :
	public Singleton
{
private:
	ISourceHook_Skeleton * g_SourceHook;
	static bool m_reverted;

public:
	SourceHookSafety();
	~SourceHookSafety();

	void TryHookMMSourceHook();
	void UnhookMMSourceHook();

	static void ProcessRevertAll();
	static void ProcessRehootAll();

#ifdef GNUC
	static int HOOKFN_INT my_AddHook(void* isourcehook, int plug, ISourceHook_Skeleton::AddHookMode mode, void *iface, int thisptr_offs, ISourceHook_Skeleton::HookManagerPubFunc myHookMan, void *handler, bool post);
	static bool HOOKFN_INT my_RemoveHook(void* isourcehook, int plug, void *iface, int thisptr_offs, ISourceHook_Skeleton::HookManagerPubFunc myHookMan, void *handler, bool post);
	static bool HOOKFN_INT my_RemoveHookById(void* isourcehook, int id);
#else
	static int HOOKFN_INT my_AddHook(void* isourcehook, void* weak, int plug, ISourceHook_Skeleton::AddHookMode mode, void *iface, int thisptr_offs, ISourceHook_Skeleton::HookManagerPubFunc myHookMan, void *handler, bool post);
	static bool HOOKFN_INT my_RemoveHook(void* isourcehook, void* weak, int plug, void *iface, int thisptr_offs, ISourceHook_Skeleton::HookManagerPubFunc myHookMan, void *handler, bool post);
	static bool HOOKFN_INT my_RemoveHookById(void* isourcehook, void* weak, int id);
#endif
};

extern SourceHookSafety g_SourceHookSafety;

#endif // DEARMETAMOD_H
