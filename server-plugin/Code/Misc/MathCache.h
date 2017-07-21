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

#ifndef MATHCACHE_H
#define MATHCACHE_H

#include <cstdint>

#include "Maths/Vector.h"

#include "temp_singleton.h"

struct MathInfo
{
	SourceSdk::Vector m_abs_origin;
	SourceSdk::Vector m_eyepos;
	SourceSdk::QAngle m_eyeangles;
	SourceSdk::Vector m_velocity;
	SourceSdk::Vector m_abs_velocity;
	SourceSdk::Vector m_mins;
	SourceSdk::Vector m_maxs;
};

class MathCache :
	public Singleton
{
private:
	struct CacheInfo
	{
		MathInfo m_info;
		bool m_is_not_expired;
	};

	CacheInfo m_cache[ 66 ];

public:
	MathCache ();
	virtual ~MathCache () final;

	/*
		This will update internal cache if it is expired or if force is specified
	*/
	MathInfo & RT_GetCachedMaths ( int const player_index, bool const force_update = false );

	/*
		Set all cache as expired, this is done once per tick
	*/
	inline void RT_SetCacheExpired ();
};

inline void MathCache::RT_SetCacheExpired ()
{
	memset ( m_cache, 0, ( uint8_t* ) ( &m_cache[ 66 ] ) - ( uint8_t* ) ( m_cache ) );
}

extern MathCache g_MathCache;

#endif
