// DQueue.cpp: implementation of the CDQueue class.
//
//////////////////////////////////////////////////////////////////////

#include "publicfunc.h"
#include "DQueue.h"
#include "AutoCS.h"



// Ô¤·ÖÅä¿Õ¼ä p_uBlockSize*p_chBlocks 
void ST_CHUNK::Init(size_t p_uBlockSize,unsigned char p_chBlocks)
{
	
    pData = new unsigned char[p_uBlockSize*p_chBlocks];
    chFirstAvailableBlock = 0;
    chBlocksAvailable = p_chBlocks;
    unsigned char i = 0;
    unsigned char * p = pData;
    for(;i != p_chBlocks; p += p_uBlockSize)
    {
        *p = ++i;
    }
}
void * ST_CHUNK:: Allocate(size_t p_uBlockSize)
{
	CAutoCriticalRegion ac(&csLock);
    if(!chBlocksAvailable)
	{
        return 0;
	}
    unsigned char * pResult = pData + (chFirstAvailableBlock * p_uBlockSize);
    chFirstAvailableBlock = *pResult;
    -- chBlocksAvailable;
    return pResult;
}
void ST_CHUNK::Deallocate(void* p_pBlock,size_t p_uBlockSize)
{
	CAutoCriticalRegion ac(&csLock);
    ASSERT(p_pBlock >= pData);
    unsigned  char * toRelease = static_cast<unsigned char *>(p_pBlock);
    ASSERT((toRelease - pData) % p_uBlockSize == 0);
    *toRelease = chFirstAvailableBlock;
    chFirstAvailableBlock = static_cast<unsigned char>((toRelease - pData) / p_uBlockSize);
    ASSERT(chFirstAvailableBlock == (toRelease - pData) / p_uBlockSize);
    ++ chBlocksAvailable;
} 

