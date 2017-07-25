#ifndef SIGSCAN_H
#define SIGSCAN_H

#include <string.h>

#include "Misc/ClassSpecifications.h"

#ifdef WIN32
#	include <Windows.h>
#	include <Psapi.h>
#else

#endif

typedef unsigned char mem_byte;
typedef unsigned int mem_dword;

struct sig_ctx :
	private NoCopy,
	private NoMove
{
	size_t const m_siglength;
	mem_byte const * const m_code;
	mem_byte const * const m_mask;
	mem_byte * m_codestrip;
	size_t const m_addr_offset;
	mem_byte * m_out;

	sig_ctx(mem_byte const * const code, mem_byte const * const mask, size_t len, size_t offset = 0) :
		m_siglength(len),
		m_code(code),
		m_mask(mask),
		m_codestrip(new mem_byte[len]),
		m_addr_offset(offset),
		m_out( nullptr )
	{
		for (size_t x = 0; x < len; ++x)
		{
			m_codestrip[x] = m_code[x] & m_mask[x];
		}
	}

	~sig_ctx()
	{
		delete[] m_codestrip;
	}
};

bool TestSig(mem_byte const * const start, sig_ctx const * ctx);
void ScanMemoryRegion(mem_byte * start, mem_byte const * const end, sig_ctx * ctx);

#endif