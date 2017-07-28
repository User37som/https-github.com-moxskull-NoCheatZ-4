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

#ifndef TIER0LINKER_H
#define TIER0LINKER_H

namespace Tier0
{
	void LinkTier0Functions();

	typedef void(*MsgFunc_t) (const char* pMsg, ...);
	typedef double(*Plat_FloatTime_t)(void);

	extern MsgFunc_t Msg;
	extern Plat_FloatTime_t Plat_FloatTime;
}

#endif // TIER0LINKER_H
