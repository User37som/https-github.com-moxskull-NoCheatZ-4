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

#ifndef ENTPROPS_H
#define ENTPROPS_H

#include "temp_basiclist.h"
#include "temp_basicstring.h"
#include "temp_singleton.h"

#undef GetClassName
#include "SdkPreprocessors.h"
#include "Interfaces/edict.h"
#include "Interfaces/server_class.h"
#include "Interfaces/datamap.h"

#include "Helpers.h"
#include "Systems/Logger.h"

typedef int offset_t;

class CBaseEntity;

typedef struct PropertyCacheS
{
	bool prop_type; // false = SendTable, true = datamap
	basic_string prop_name;
	offset_t prop_offset;
	size_t comp_pred;

	PropertyCacheS()
	{
		prop_type = false;
		prop_offset = 0;
		comp_pred = 0;
	};
	PropertyCacheS(const PropertyCacheS& other)
	{
		prop_type = other.prop_type;
		prop_name = other.prop_name;
		prop_offset = other.prop_offset;
		comp_pred = other.comp_pred;
	};
	PropertyCacheS(const basic_string& name, offset_t offset, bool type)
	{
		prop_type = type;
		prop_name = name;
		prop_offset = offset;
		comp_pred = name.find_last_of("_.")+1;
	};
} PropertyCacheT;

typedef CUtlVector<PropertyCacheT> PropsListT;

SourceSdk::datamap_t* GetDataDescMap(SourceSdk::edict_t* const pEntity);

class EntityProps : public Singleton<EntityProps>
{
private:
	/* I believe offsets are consistents so don't need to reset the cache during runtime */
	PropsListT m_cache;

	void GetPropOffset(const basic_string& name, offset_t* pOffset);

	bool GetDataOffset(SourceSdk::datamap_t* dt, const basic_string& data_name, offset_t* offset);

	bool FindInCache(const basic_string &path, offset_t* offset, bool type = false);

	EntityProps(const EntityProps& other){};
	EntityProps& operator=(EntityProps const &  other) {};

public:
	EntityProps(){};
	~EntityProps(){};

	template<typename T>
	T* GetPropValue(const basic_string &path, SourceSdk::edict_t * const pEdict, bool type = false)
	{
		SourceSdk::CBaseEntity * const pBase = pEdict->GetUnknown()->GetBaseEntity();
		offset_t offset = 0;
		if (!FindInCache(path, &offset, type))
		{
			if (!type)
			{
				GetPropOffset(path, &offset);
			}
			else
			{
				SourceSdk::datamap_t* const dt = GetDataDescMap(pEdict);
				GetDataOffset(dt, path, &offset);
			}
		}
		Assert(offset > 0);
		return reinterpret_cast<T *>(reinterpret_cast<uint8_t *>(pBase) + offset);
	};
};

#endif
