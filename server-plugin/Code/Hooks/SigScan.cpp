#include "SigScan.h"
#include <cassert>

bool TestSig(mem_byte const * const start, sig_ctx const * ctx)
{
	mem_byte const * mem_iter( start );
	mem_byte const * const end ( start + ctx->m_siglength );
	mem_byte const * mask_iter( ctx->m_mask );
	mem_byte const * prep_iter( ctx->m_codestrip );

	do
	{
		if ((*mem_iter & *mask_iter) != *prep_iter)
		{
			return false;
		}
		++mask_iter;
		++prep_iter;
	} while (++mem_iter < end);

	return true;
}

void ScanMemoryRegion(mem_byte * start, mem_byte const * const end, sig_ctx * ctx)
{
#ifdef DEBUG
	bool found(false);
	do
	{
		if (TestSig(start, ctx))
		{
			ctx->m_out = start + ctx->m_addr_offset;
			assert(found == false); // The scan hits multiple locations ...
			found = true;
		}
		++start;
	} while ( start + ctx->m_siglength <= end );
#else
	do
	{
		if (TestSig(start, ctx))
		{
			ctx->m_out = start + ctx->m_addr_offset;
			return;
		}
		++start;
	} while (start + ctx->m_siglength <= end);
#endif
}
