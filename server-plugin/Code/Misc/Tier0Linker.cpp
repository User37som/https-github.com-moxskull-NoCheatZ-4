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

#include "Tier0Linker.h"

#include "include_windows_headers.h"
#include <iostream>

namespace Tier0
{
	MsgFunc_t Msg;
	Plat_FloatTime_t Plat_FloatTime;

#ifdef GNUC

#include <dlfcn.h>
#include <execinfo.h>

	void * GetModuleHandle(const char *name)
	{
		void *handle(dlopen(name, RTLD_NOW));

		if (handle == nullptr)
		{
			return nullptr;
		}

		dlclose(handle);
		return handle;
	}

#endif

	void LinkTier0Functions()
	{
#ifdef WIN32
		*(void **)&Msg = GetProcAddress(GetModuleHandleA("tier0.dll"), "Msg");
		*(void **)&Plat_FloatTime = GetProcAddress(GetModuleHandleA("tier0.dll"), "Plat_FloatTime");

#else
		void* pmodule(GetModuleHandle("libtier0_srv.so"));
		if (pmodule == nullptr)
		{
			pmodule = GetModuleHandle("tier0_srv.so");
		}
		if (pmodule == nullptr)
		{
			pmodule = GetModuleHandle("libtier0.so");
		}
		if (pmodule == nullptr)
		{
			pmodule = GetModuleHandle("tier0.so");
		}
		if (pmodule == nullptr)
		{
			std::cout << "Unable to locate any tier0 shared library.\n";
			return;
		}

		*(void **)&Msg = dlsym(pmodule, "Msg");
		*(void **)&Plat_FloatTime = dlsym(pmodule, "Plat_FloatTime");
#endif
	}
}
