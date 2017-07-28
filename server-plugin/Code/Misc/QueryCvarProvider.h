/*
Copyright 2012 - Le Padellec Sylvain

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http ://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef QUERYCVARPROVIDER_H
#define QUERYCVARPROVIDER_H

#include "Interfaces/InterfacesProxy.h"

class QueryCvarProvider
{
private:
	SourceSdk::QueryCvarCookie_t * m_engine_cvar_cookie;

	void FixQueryCvarCookie();

public:
	QueryCvarProvider();
	~QueryCvarProvider();
		
	void InitCookie();

	SourceSdk::QueryCvarCookie_t StartQueryCvarValue(SourceSdk::edict_t *pEntity, const char *pName);
};

extern QueryCvarProvider g_QueryCvarProvider;

#endif // QUERYCVARPROVIDER_H
