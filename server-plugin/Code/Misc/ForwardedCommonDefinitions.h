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

#ifndef FORWARDEDCOMMONDEFINITIONS_H

#ifndef DWORD
#	define DWORD unsigned long
#endif

#ifdef WIN32
#	define HOOKFN_EXT __thiscall
#	define HOOKFN_INT __fastcall
#else
#	define HOOKFN_EXT
#	define HOOKFN_INT __attribute__((cdecl))
#endif

class BaseDetection;
class BaseSystem;
class NczPlayer;
class NczPlayerManager;
class CBaseEntity;

typedef int offset_t;

namespace SourceSdk
{
	struct datamap_t;
	struct edict_t;
	typedef edict_t* CCheckTransmitInfo;
	class CBaseEntity;
}

namespace Helpers
{
	struct game_tm;
}

#endif // FORWARDEDCOMMONDEFINITIONS_H