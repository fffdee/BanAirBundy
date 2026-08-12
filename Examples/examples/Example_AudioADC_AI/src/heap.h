/***
 *@fn		heap.h
 *@auther	d(^_^)d
 *@version	V1.1.7
 *@date		2018-05-21
 *@brief	use heap to write code.
 */

#ifndef _HEAP_H
#define _HEAP_H

#include "string.h"

#define TONY_API


typedef struct _HeapBlockLink
{
	struct _HeapBlockLink *HeapNextFreeBlock;
	size_t BlockSize;
}HeapBlockLink_t;

#define HEAP_MAX_SIZE		   512
#define TBYTE_ALIGNMENT		   8
#define TBITS_PER_BYTE		 ( ( size_t ) 8 )

#if TBYTE_ALIGNMENT == 32
	#define TBYTE_ALIGNMENT_MASK ( 0x001f )
#endif

#if TBYTE_ALIGNMENT == 16
	#define TBYTE_ALIGNMENT_MASK ( 0x000f )
#endif

#if TBYTE_ALIGNMENT == 8
	#define TBYTE_ALIGNMENT_MASK ( 0x0007 )
#endif

#if TBYTE_ALIGNMENT == 4
	#define TBYTE_ALIGNMENT_MASK	( 0x0003 )
#endif

#if TBYTE_ALIGNMENT == 2
	#define TBYTE_ALIGNMENT_MASK	( 0x0001 )
#endif

#if TBYTE_ALIGNMENT == 1
	#define TBYTE_ALIGNMENT_MASK	( 0x0000 )
#endif

//API
//
/***
 * @func    T_PortMalloc
 * @brief	get mem in heap
 *          malloc函数是按TBYTE_ALIGNMENT对齐的
 *          比如：TBYTE_ALIGNMENT=8，当WantedSize%8！=0时会将它补充为TBYTE_ALIGNMENT的倍数，导致free size按TBYTE_ALIGNMENT倍数计算
 * @param
 * 		    WantedSize :　 want size for bytes
 * @return
 *          1.malloc addr
 *          2.(return==NULL) malloc fail!
 */
TONY_API void *T_PortMalloc( size_t WantedSize );
TONY_API void T_PortFree( void *pv );
TONY_API size_t T_PortGetFreeHeapSize( void );//return bytes
TONY_API size_t T_PortGetMinimumEverFreeHeapSize( void );//return bytes

#endif
