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

#ifndef RAYTRACECACHE
#define RAYTRACECACHE

#include "Misc/minmax_on.h"
#include "edict.h"
#include "iservernetworkable.h"
#include "IEngineTrace.h"
#include "Misc/minmax_off.h"

#include <vector>
#include <cstdint>

#include "Hook.h"

/*
	Used to avoid calling traces with the same parameters, because traces are expensive.
	With this method we can also listen to the traces made by the engine and others plugins, so this will decrease the CPU usage a lot.
*/

// The function declaration we will cache
typedef void (HOOKFN_EXT *TraceRay_t)(Ray_t const & ray, unsigned int fMask, ITraceFilter * pTraceFilter, trace_t * pTrace);

#ifdef NCZ_CSS
#	ifdef GNUC
#		define DEFAULT_TRACERAY_OFFSET 13
#	else
#		define DEFAULT_TRACERAY_OFFSET 12
#	endif
#endif
#ifdef NCZ_CSGO
#	ifdef GNUC
#		define DEFAULT_TRACERAY_OFFSET 15
#	else
#		define DEFAULT_TRACERAY_OFFSET 14
#	endif
#endif

class RayTraceInfo;

typedef struct ALIGN16 ComparisonHelper
{
	uint32_t m_crc;
	uint32_t m_fMask;
	Ray_t m_ray;

	ComparisonHelper(){};
	inline ComparisonHelper(unsigned int fMask, Ray_t const & ray);
	inline ComparisonHelper(ComparisonHelper const & other);

	bool operator==(ComparisonHelper const & other) const;
	bool operator==(RayTraceInfo const & other) const;
} ALIGN16_POST ComparisonHelper_t;

class RayTraceInfo
{
	friend ComparisonHelper;
private:

	ComparisonHelper_t inner_info;
	//ITraceFilter* m_filter; // Not really usefull
	trace_t m_result;

public:
	RayTraceInfo();
	inline RayTraceInfo(ComparisonHelper_t const * const ch, trace_t const * const tr);
	inline RayTraceInfo(RayTraceInfo const & other);
	~RayTraceInfo();

	inline void PutResult(trace_t* result) const;
};

typedef std::vector<RayTraceInfo> InnerCache_t;

class RayTraceCache
{
public:
	RayTraceCache();
	~RayTraceCache();

	static void Load();
	static void HookTraceRay();
	static void ResetCache();
	static void UnhookTraceRay();
	static void Unload();

private:
#	ifdef GNUC
	static void HOOKFN_INT nTraceRay(Ray_t const & ray, unsigned int fMask, ITraceFilter * pTraceFilter, trace_t * pTrace);
#	else
	static void HOOKFN_INT nTraceRay(Ray_t const & ray, void*, unsigned int fMask, ITraceFilter * pTraceFilter, trace_t * pTrace);
#	endif

	static InnerCache_t m_cache;
	static HookInfo<IEngineTrace> m_hook;
};

#endif
