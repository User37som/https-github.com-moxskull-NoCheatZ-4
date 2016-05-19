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

#include <cstring> // memcpy
#include <algorithm> // find

#include "RayTraceCache.h"
#include "../Misc/IFaceManager.h"

RayTraceInfo::RayTraceInfo(){};

RayTraceInfo::~RayTraceInfo(){};

ComparisonHelper::ComparisonHelper(unsigned int fMask, Ray_t const & ray)
{
	m_fMask = fMask;
	memcpy(&m_ray, &ray, sizeof(Ray_t));
}

ComparisonHelper::ComparisonHelper(ComparisonHelper const & other)
{
	memcpy(this, &other, sizeof(ComparisonHelper));
}

bool ComparisonHelper::operator==(ComparisonHelper const & other) const
{
	// FIXME : Should use SSE but I'm unsure of the bytes fill up value made by the padding.
	// FIXME : Try to implement a reversed ray testing

	if(m_fMask != other.m_fMask) return false;

	if(m_ray.m_IsRay != other.m_ray.m_IsRay) return false;
	if(m_ray.m_IsSwept != other.m_ray.m_IsSwept) return false;
	if(m_ray.m_Start != other.m_ray.m_Start) return false;
	if(m_ray.m_Delta != other.m_ray.m_Delta) return false;
	if(m_ray.m_StartOffset != other.m_ray.m_StartOffset) return false;
	if(m_ray.m_Extents != other.m_ray.m_Extents) return false;

	return true;
}

bool ComparisonHelper::operator==(RayTraceInfo const & other) const
{
	return other.inner_info == *this;
}

RayTraceInfo::RayTraceInfo(ComparisonHelper_t const * const ch, trace_t const * const tr)
{
	memcpy(&inner_info, ch, sizeof(ComparisonHelper_t));
	memcpy(&m_result, tr, sizeof(trace_t));
}

RayTraceInfo::RayTraceInfo(RayTraceInfo const & other)
{
	memcpy(this, &other, sizeof(RayTraceInfo));
}

void RayTraceInfo::PutResult(trace_t* result) const
{
	memcpy(result, &m_result, sizeof(trace_t));
}

RayTraceCache::RayTraceCache()
{

}

RayTraceCache::~RayTraceCache()
{
	Unload();
}

void RayTraceCache::Load()
{
	m_cache.reserve(2048);
}

void RayTraceCache::HookTraceRay()
{
	/* Thanksfully we have the declaration of IEngineTrace so we can grab the
		vtable offset of TraceRay using the commented code below
		with the debugger in ASM mode :

		CSGO : (mov eax, edx+38h) = (56/4) = 14 for Windows
		CSS : (mov eax, edx+30h) = (48/4) = 12 for Windows
		
		g_IFaceManager.GetItrace()->TraceRay ...
	*/

#	ifdef DEBUG
	printf("RayTraceCache::HookTraceRay()\n- Offset = %d\n- enginetrace = %X\n- pdwInterface = %X\n", DEFAULT_TRACERAY_OFFSET, g_IFaceManager.GetItrace(), IFACE_PTR(g_IFaceManager.GetItrace()));
#	
	endif
	m_hook.pInterface = IFACE_PTR(g_IFaceManager.GetItrace());
	m_hook.origEnt = g_IFaceManager.GetItrace();
	m_hook.oldFn = VirtualTableHook(hook.pInterface, DEFAULT_TRACERAY_OFFSET, ( uint32_t )nTraceRay );
}

void RayTraceCache::ResetCache()
{
	m_cache.clear();
}

void RayTraceCache::UnhookTraceRay()
{
	VirtualTableHook(m_hook.pInterface, DEFAULT_TRACERAY_OFFSET, (uint32_t)m_hook.oldFn, ( uint32_t )nTraceRay);

	m_hook = HookInfo<IEngineTrace>();
}

void RayTraceCache::Unload()
{
	InnerCache_t empty;
	std::swap(empty, m_cache);
}

#ifdef GNUC
void HOOKFN_INT RayTraceCache::nTraceRay(Ray_t const & ray, unsigned int fMask, ITraceFilter * pTraceFilter, trace_t * pTrace)
#else
void HOOKFN_INT RayTraceCache::nTraceRay(Ray_t const & ray, void*, unsigned int fMask, ITraceFilter * pTraceFilter, trace_t * pTrace)
#endif
{
	ComparisonHelper_t seek(fMask, ray);

	InnerCache_t::reverse_iterator it = std::find(m_cache.rbegin(), m_cache.rend(), seek);

	if(it != m_cache.rend())
	{
		it->PutResult(pTrace);
	}
	else
	{
		TraceRay_t gpOldFn;
		*(uint32_t*)&(gpOldFn) = m_hook.oldFn;
		gpOldFn(ray, fMask, pTraceFilter, pTrace);

		m_cache.insert(m_cache.end(), RayTraceInfo(&seek, pTrace));
	}
}

