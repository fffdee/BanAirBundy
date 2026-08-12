/***
 *@fn		heap.c
 *@auther	d(^_^)d
 *@version	V1.1.7
 *@date		2018-05-21
 *@brief	use heap to write code.
 */

#include "heap.h"
//#include "bb_api.h"
#include "type.h"
#include "debug.h"
//typedef unsigned char	uint8_t;

#define BB_MPU_START_ADDR  0x20040000//test only

#define HEAP_DBG(format, ...)			//printf(format, ##__VA_ARGS__)

static const size_t HeapStructSize	= ( sizeof( HeapBlockLink_t ) + ( ( size_t ) ( TBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) TBYTE_ALIGNMENT_MASK );
#define MINIMUM_BLOCK_SIZE		  ( ( size_t ) ( HeapStructSize << 1 ) )

static HeapBlockLink_t TStart, *pTEnd = NULL;

static size_t FreeBytesRemaining = 0U;
static size_t MinimumEverFreeBytesRemaining = 0U;

static size_t BlockAllocatedBit = 0;

//static unsigned char tony_heap[HEAP_MAX_SIZE];
static uint32_t gB1xSramEndAddr = BB_MPU_START_ADDR;

void T_HeapInit( void )
{
	extern char _end;
	HeapBlockLink_t *FirstFreeBlock;
	uint8_t *AlignedHeap;
	size_t TAddress;
	size_t TotalHeapSize = gB1xSramEndAddr - (uint32_t)&_end;//HEAP_MAX_SIZE;//buffer length of bytes

	//TAddress = ( size_t ) tony_heap;
	TAddress = (uint32_t)&_end;

	HEAP_DBG("&_end:0x%08lx\r\n",(uint32_t)&_end);

	if( ( TAddress & TBYTE_ALIGNMENT_MASK ) != 0 )
	{
		TAddress += ( TBYTE_ALIGNMENT - 1 );
		TAddress &= ~( ( size_t ) TBYTE_ALIGNMENT_MASK );
		TotalHeapSize -= TAddress - (uint32_t)&_end;
		//TotalHeapSize = gB1xSramEndAddr - (uint32_t)&_end;//HEAP_MAX_SIZE;
	}

	HEAP_DBG("AlignedHeap:0x%08lx\r\n",TAddress);
	AlignedHeap = ( uint8_t * ) TAddress;

	TStart.HeapNextFreeBlock = ( void * ) AlignedHeap;
	TStart.BlockSize = ( size_t ) 0;
	
	TAddress = ( ( size_t ) AlignedHeap ) + TotalHeapSize;
	TAddress -= HeapStructSize;
	TAddress &= ~( ( size_t ) TBYTE_ALIGNMENT_MASK );
	HEAP_DBG("TAddress:0x%08lx\r\n",TAddress);
	pTEnd = ( void * ) TAddress;
	pTEnd->BlockSize = 0;
	pTEnd->HeapNextFreeBlock = NULL;

	FirstFreeBlock = ( void * ) AlignedHeap;
	FirstFreeBlock->BlockSize = TAddress - ( size_t ) FirstFreeBlock;
	FirstFreeBlock->HeapNextFreeBlock = pTEnd;

	MinimumEverFreeBytesRemaining = (FirstFreeBlock->BlockSize);
	FreeBytesRemaining = (FirstFreeBlock->BlockSize);
	HEAP_DBG("free[%ld],%ld,%ld\r\n",FirstFreeBlock->BlockSize,sizeof(HeapBlockLink_t),HeapStructSize);

	BlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * TBITS_PER_BYTE ) - 1 );
}

static void T_InsertBlockIntoFreeList( HeapBlockLink_t *pxBlockToInsert )
{
	HeapBlockLink_t *pxIterator;
	uint8_t *puc;

	for( pxIterator = &TStart; pxIterator->HeapNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->HeapNextFreeBlock )
	{
		
	}
	puc = ( uint8_t * ) pxIterator;
	if( ( puc + pxIterator->BlockSize ) == ( uint8_t * ) pxBlockToInsert )
	{
		pxIterator->BlockSize += pxBlockToInsert->BlockSize;
		pxBlockToInsert = pxIterator;
	}
	puc = ( uint8_t * ) pxBlockToInsert;
	if( ( puc + pxBlockToInsert->BlockSize ) == ( uint8_t * ) pxIterator->HeapNextFreeBlock )
	{
		if( pxIterator->HeapNextFreeBlock != pTEnd )
		{
			pxBlockToInsert->BlockSize += pxIterator->HeapNextFreeBlock->BlockSize;
			pxBlockToInsert->HeapNextFreeBlock = pxIterator->HeapNextFreeBlock->HeapNextFreeBlock;
		}
		else
		{
			pxBlockToInsert->HeapNextFreeBlock = pTEnd;
		}
	}
	else
	{
		pxBlockToInsert->HeapNextFreeBlock = pxIterator->HeapNextFreeBlock;
	}
	if( pxIterator != pxBlockToInsert )
	{
		pxIterator->HeapNextFreeBlock = pxBlockToInsert;
	}
}

void *T_PortMalloc( size_t WantedSize )
{
	HeapBlockLink_t *ptBlock, *ptPreviousBlock, *ptNewBlockLink;
	void *ptReturn = NULL;

	if( pTEnd == NULL )
	{
		T_HeapInit();
	}

	if( ( WantedSize & BlockAllocatedBit ) == 0 )
	{
		if( WantedSize > 0 )
		{
			WantedSize += HeapStructSize;
			if( ( WantedSize & TBYTE_ALIGNMENT_MASK ) != 0x00 )
			{
				WantedSize += ( TBYTE_ALIGNMENT - ( WantedSize & TBYTE_ALIGNMENT_MASK ) );
			}
		}

		if( ( WantedSize > 0 ) && ( WantedSize <= FreeBytesRemaining ) )
		{
			ptPreviousBlock = &TStart;
			ptBlock = TStart.HeapNextFreeBlock;
			while( ( ptBlock->BlockSize < WantedSize ) && ( ptBlock->HeapNextFreeBlock != NULL ) )
			{
				ptPreviousBlock = ptBlock;
				ptBlock = ptBlock->HeapNextFreeBlock;
			}
			HEAP_DBG("ptBlock:0x%lx\r\n",(uint32_t)ptBlock);
			HEAP_DBG("ptPreviousBlock:0x%lx,0x%lx\r\n",(uint32_t)ptPreviousBlock,(uint32_t)ptPreviousBlock->HeapNextFreeBlock);
			if( ptBlock != pTEnd )
			{
				ptReturn = ( void * ) ( ( ( uint8_t * ) ptPreviousBlock->HeapNextFreeBlock ) + HeapStructSize );
				
				ptPreviousBlock->HeapNextFreeBlock = ptBlock->HeapNextFreeBlock;

				if( ( ptBlock->BlockSize - WantedSize ) > MINIMUM_BLOCK_SIZE )
				{
					ptNewBlockLink = ( void * ) ( ( ( uint8_t * ) ptBlock ) + WantedSize );

					ptNewBlockLink->BlockSize = ptBlock->BlockSize - WantedSize;
					ptBlock->BlockSize = WantedSize;
					HEAP_DBG("ptNewBlockLink[%ld]:0x%lx\r\n",ptBlock->BlockSize,(uint32_t)ptNewBlockLink);
					T_InsertBlockIntoFreeList( ptNewBlockLink );
				}
				HEAP_DBG("ptNewBlockLink_out[%ld]:0x%lx\r\n",ptBlock->BlockSize,(uint32_t)ptNewBlockLink);
				FreeBytesRemaining -= ptBlock->BlockSize;

				if( FreeBytesRemaining < MinimumEverFreeBytesRemaining )
				{
					MinimumEverFreeBytesRemaining = FreeBytesRemaining;
				}
				ptBlock->BlockSize |= BlockAllocatedBit;
				ptBlock->HeapNextFreeBlock = NULL;
			}
		}
	}
	return ptReturn;
}

void T_PortFree( void *pv )
{
	uint8_t *puc = ( uint8_t * ) pv;
	HeapBlockLink_t *ptLink;

	if( pv != NULL )
	{
		puc -= HeapStructSize;

		ptLink = ( void * ) puc;

		if( ( ptLink->BlockSize & BlockAllocatedBit ) != 0 )
		{
			if( ptLink->HeapNextFreeBlock == NULL )
			{
				ptLink->BlockSize &= ~BlockAllocatedBit;

				{
					FreeBytesRemaining += ptLink->BlockSize;
					T_InsertBlockIntoFreeList( ( ( HeapBlockLink_t * ) ptLink ) );
				}
			}
		}
	}
}


size_t T_PortGetFreeHeapSize( void )
{
	return FreeBytesRemaining;
}

size_t T_PortGetMinimumEverFreeHeapSize( void )
{
	return MinimumEverFreeBytesRemaining;
}

#define osPortMallocFromEnd 		osPortMalloc

void *osPortMalloc(uint32_t osWantedSize)
{
	void *ospvReturn = NULL;
	extern char _end;
	//vPortEnterCritical();
	DBG("Malloc:%d,%d\n ",(int)osWantedSize,(int)T_PortGetFreeHeapSize());
	ospvReturn=T_PortMalloc(osWantedSize);
	//vPortExitCritical();
#if 1
	if(ospvReturn != NULL)
	{
		DBG("Malloc:%08lx %d %d\n ",(uint32_t)ospvReturn, (int)osWantedSize, (int)T_PortGetFreeHeapSize());
	}
	else
	{
		DBG("NULL\n");
	}
#endif
	return ospvReturn;
}

void osPortFree(void *ospv)
{
	DBG("Add=%lx\n", (uint32_t)ospv);
	//vPortEnterCritical();
	T_PortFree(ospv);
	//vPortExitCritical();
}

uint32_t osPortRemainMem(void)
{
	return T_PortGetFreeHeapSize();
}
