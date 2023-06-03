#include <stdlib.h>
#include <string.h>

#include <efs_device.h>
#include <efs_file.h>
#include <efs_fs.h>
#include <efs.h>

#include "efs_util.h"


eris_int32_t eris_strcmp(const char *cs, const char *ct)
{
    while (*cs && *cs == *ct)
    {
        cs++;
        ct++;
    }

    return (*cs - *ct);
}

void *eris_memcpy(void *dst, const void *src, eris_ubase_t count)
{
#ifdef Eris_KSERVICE_USING_TINY_SIZE
    char *tmp = (char *)dst, *s = (char *)src;
    eris_ubase_t len = 0;

    if (tmp <= s || tmp > (s + count))
    {
        while (count--)
            *tmp ++ = *s ++;
    }
    else
    {
        for (len = count; len > 0; len --)
            tmp[len - 1] = s[len - 1];
    }

    return dst;
#else

#define UNALIGNED(X, Y) \
    (((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))
#define BIGBLOCKSIZE    (sizeof (long) << 2)
#define LITTLEBLOCKSIZE (sizeof (long))
#define TOO_SMALL(LEN)  ((LEN) < BIGBLOCKSIZE)

    char *dst_ptr = (char *)dst;
    char *src_ptr = (char *)src;
    long *aligned_dst = Eris_NULL;
    long *aligned_src = Eris_NULL;
    eris_ubase_t len = count;

    /* If the size is small, or either SRC or DST is unaligned,
    then punt into the byte copy loop.  This should be rare. */
    if (!TOO_SMALL(len) && !UNALIGNED(src_ptr, dst_ptr))
    {
        aligned_dst = (long *)dst_ptr;
        aligned_src = (long *)src_ptr;

        /* Copy 4X long words at a time if possible. */
        while (len >= BIGBLOCKSIZE)
        {
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            *aligned_dst++ = *aligned_src++;
            len -= BIGBLOCKSIZE;
        }

        /* Copy one long word at a time if possible. */
        while (len >= LITTLEBLOCKSIZE)
        {
            *aligned_dst++ = *aligned_src++;
            len -= LITTLEBLOCKSIZE;
        }

        /* Pick up any residual with a byte copier. */
        dst_ptr = (char *)aligned_dst;
        src_ptr = (char *)aligned_src;
    }

    while (len--)
        *dst_ptr++ = *src_ptr++;

    return dst;
#undef UNALIGNED
#undef BIGBLOCKSIZE
#undef LITTLEBLOCKSIZE
#undef TOO_SMALL
#endif /* Eris_KSERVICE_USING_TINY_SIZE */
}



eris_err_t eris_memheap_init(struct eris_memheap *memheap,
                         const char        *name,
                         void              *start_addr,
                         eris_size_t         size)
{
    struct BlockLink_t *item;

    Eris_ASSERT(memheap != Eris_NULL);

    /* initialize pool object */
    //eris_object_init(&(memheap->parent), Eris_Object_Class_MemHeap, name);

    memheap->start_addr                     = start_addr;
    memheap->pool_size                      = Eris_ALIGN_DOWN(size, Eris_ALIGN_SIZE);
    memheap->xFreeBytesRemaining            = memheap->pool_size - (2 * xHeapStructSize);
    memheap->max_used_size                  = memheap->pool_size - memheap->xFreeBytesRemaining;
    memheap->xMinimumEverFreeBytesRemaining = memheap->xFreeBytesRemaining;
    memheap->xStart                         = *(memheap->start_addr);

    /* initialize the free list header */
    
    item                        = &(memheap->xStart);
    item->magic                 = (Eris_MEMHEAP_MAGIC | Eris_MEMHEAP_FREED);
    item->pool_ptr              = memheap;
    item->pxNextFreeBlock       = (BlockLink_t*)((eris_uint8_t *)item + xHeapStructSize)

    /* set the free list to free list header */
    memheap->free_list = item;
    item = item->pxNextFreeBlock;
    /* initialize the first big memory block */
    pxEnd                       = (BlockLink_t*)
                ((eris_uint8_t *)item + memheap->available_size + xHeapStructSize);
    item                        = (struct eris_memheap_item *)start_addr;
    item->magic                 = (Eris_MEMHEAP_MAGIC | Eris_MEMHEAP_FREED);
    item->pool_ptr              = memheap;
    item->pxNextFreeBlock       = pxEnd;
    item->xBlockSize            = memheap->xFreeBytesRemaining;

#ifdef Eris_USING_MEMTRACE
    eris_memset(item->owner_thread_name, ' ', sizeof(item->owner_thread_name));
#endif /* Eris_USING_MEMTRACE */

    /* block list header */
    memheap->block_list = item;

    /* initialize semaphore lock */
    memheap->lock = xSemaphoreCreateMutex();
    memheap->locked = Eris_FALSE;
 
    Eris_DEBUG_LOG(Eris_DEBUG_MEMHEAP,
                 ("memory memheap: start addr 0x%08x, size %d, free list header 0x%08x\n",
                  start_addr, size, &(memheap->free_header)));

    return Eris_EOK;
}

eris_weak void *eris_memset(void *s, int c, eris_ubase_t count)
{
#ifdef Eris_KSERVICE_USING_TINY_SIZE
    char *xs = (char *)s;

    while (count--)
        *xs++ = c;

    return s;
#else
#define LBLOCKSIZE      (sizeof(eris_ubase_t))
#define UNALIGNED(X)    ((long)X & (LBLOCKSIZE - 1))
#define TOO_SMALL(LEN)  ((LEN) < LBLOCKSIZE)

    unsigned int i = 0;
    char *m = (char *)s;
    unsigned long buffer = 0;
    unsigned long *aligned_addr = RT_NULL;
    unsigned char d = (unsigned int)c & (unsigned char)(-1);  /* To avoid sign extension, copy C to an
                                unsigned variable. (unsigned)((char)(-1))=0xFF for 8bit and =0xFFFF for 16bit: word independent */

    Eris_ASSERT(LBLOCKSIZE == 2 || LBLOCKSIZE == 4 || LBLOCKSIZE == 8);

    if (!TOO_SMALL(count) && !UNALIGNED(s))
    {
        /* If we get this far, we know that count is large and s is word-aligned. */
        aligned_addr = (unsigned long *)s;

        /* Store d into each char sized location in buffer so that
         * we can set large blocks quickly.
         */
        for (i = 0; i < LBLOCKSIZE; i++)
        {
            *(((unsigned char *)&buffer)+i) = d;
        }

        while (count >= LBLOCKSIZE * 4)
        {
            *aligned_addr++ = buffer;
            *aligned_addr++ = buffer;
            *aligned_addr++ = buffer;
            *aligned_addr++ = buffer;
            count -= 4 * LBLOCKSIZE;
        }

        while (count >= LBLOCKSIZE)
        {
            *aligned_addr++ = buffer;
            count -= LBLOCKSIZE;
        }

        /* Pick up the remainder with a bytewise loop. */
        m = (char *)aligned_addr;
    }

    while (count--)
    {
        *m++ = (char)d;
    }

    return s;

#undef LBLOCKSIZE
#undef UNALIGNED
#undef TOO_SMALL
#endif /* RT_KSERVICE_USING_TINY_SIZE */
}



void * pvPortMalloc_efs( size_t xWantedSize , struct eris_memheap * memheap)
{
    BlockLink_t * pxBlock;
    BlockLink_t * pxPreviousBlock;
    BlockLink_t * pxNewBlockLink;
    void * pvReturn = NULL;
    size_t xAdditionalRequiredSize;
    size_t xUsed;

    /* The memheap must be initialised before the first call to
     * prvPortMalloc(). */
    configASSERT( memheap->pxEnd );

    xSemaphoreTake( memheap->lock, portMAX_DELAY );;
    {
        if( xWantedSize > 0 )
        {
            /* The wanted size must be increased so it can contain a BlockLink_t
             * structure in addition to the requested amount of bytes. Some
             * additional increment may also be needed for alignment. */
            xAdditionalRequiredSize = xHeapStructSize + portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK );

            if( heapADD_WILL_OVERFLOW( xWantedSize, xAdditionalRequiredSize ) == 0 )
            {   
                xUsed = xWantedSize;
                xWantedSize += xAdditionalRequiredSize;
            }
            else
            {
                xWantedSize = 0;
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        /* Check the block size we are trying to allocate is not so large that the
         * top bit is set.  The top bit of the block size member of the BlockLink_t
         * structure is used to determine who owns the block - the application or
         * the kernel, so it must be free. */
        if( heapBLOCK_SIZE_IS_VALID( xWantedSize ) != 0 )
        {
            if( ( xWantedSize > 0 ) && ( xWantedSize <= memheap->xFreeBytesRemaining ) )
            {
                /* Traverse the list from the start (lowest address) block until
                 * one of adequate size is found. */
                pxPreviousBlock = &(memheap->xStart);
                pxBlock = memheap->xStart.pxNextFreeBlock;

                while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
                {
                    pxPreviousBlock = pxBlock;
                    pxBlock = pxBlock->pxNextFreeBlock;
                }

                /* If the end marker was reached then a block of adequate size
                 * was not found. */
                if( pxBlock != memheap->pxEnd )
                {
                    /* Return the memory space pointed to - jumping over the
                     * BlockLink_t structure at its start. */
                    pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

                    /* This block is being returned for use so must be taken out
                     * of the list of free blocks. */
                    pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                    pxBlock->xUsedBlockSize = xUsed;
                    /* If the block is larger than required it can be split into
                     * two. */
                    if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
                    {
                        /* This block is to be split into two.  Create a new
                         * block following the number of bytes requested. The void
                         * cast is used to prevent byte alignment warnings from the
                         * compiler. */
                        pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );

                        /* Calculate the sizes of two blocks split from the
                         * single block. */
                        
                        pxNewBlockLink->pool_ptr = memheap;
                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
                        pxNewBlockLink->magic         = (Eris_MEMHEAP_MAGIC | Eris_MEMHEAP_FREED);
                        pxBlock->xBlockSize = xWantedSize;

                        /* Insert the new block into the list of free blocks. */
                        prvInsertBlockIntoFreeList_efs( ( pxNewBlockLink ) ,memheap);
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    memheap->xFreeBytesRemaining -= pxBlock->xBlockSize;

                    if( memheap->xFreeBytesRemaining < memheap->xMinimumEverFreeBytesRemaining )
                    {
                        memheap->xMinimumEverFreeBytesRemaining = memheap->xFreeBytesRemaining;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    /* The block is being returned - it is allocated and owned
                     * by the application and has no "next" block. */
                    heapALLOCATE_BLOCK( pxBlock );
                    pxBlock->pxNextFreeBlock = NULL;
                    memheap->max_used_size=memheap->pool_size - memheap->xFreeBytesRemaining;
                    memheap->xNumberOfSuccessfulAllocations++;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        traceMALLOC( pvReturn, xWantedSize );
    }
    ( void ) xSemaphoreGive( memheap->lock);

    #if ( configUSE_MALLOC_FAILED_HOOK == 1 )
    {
        if( pvReturn == NULL )
        {
            vApplicationMallocFailedHook();
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
    #endif /* if ( configUSE_MALLOC_FAILED_HOOK == 1 ) */

    return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFree_efs( void * pv , struct eris_memheap *memheap)
{
    uint8_t * puc = ( uint8_t * ) pv;
    BlockLink_t * pxLink;

    if( pv != NULL )
    {
        /* The memory being freed will have an BlockLink_t structure immediately
         * before it. */
        puc -= memheap->xHeapStructSize;

        /* This casting is to keep the compiler from issuing warnings. */
        pxLink = ( void * ) puc;

        configASSERT( heapBLOCK_IS_ALLOCATED( pxLink ) != 0 );
        configASSERT( pxLink->pxNextFreeBlock == NULL );

        if( heapBLOCK_IS_ALLOCATED( pxLink ) != 0 )
        {
            if( pxLink->pxNextFreeBlock == NULL )
            {
                /* The block is being returned to the memheap - it is no longer
                 * allocated. */
                heapFREE_BLOCK( pxLink );
                #if ( configHEAP_CLEAR_MEMORY_ON_FREE == 1 )
                {
                    ( void ) memset( puc + memheap->xHeapStructSize, 0, pxLink->xBlockSize - memheap->xHeapStructSize );
                }
                #endif

                xSemaphoreTake( memheap->lock, portMAX_DELAY );;
                {
                    /* Add this block to the list of free blocks. */
                    memheap->xFreeBytesRemaining += pxLink->xBlockSize;
                    traceFREE( pv, pxLink->xBlockSize );
                    prvInsertBlockIntoFreeList_efs( ( ( BlockLink_t * ) pxLink ) ,memheap);
                    memheap->xNumberOfSuccessfulFrees++;
                }
                ( void ) xSemaphoreGive( memheap->lock);
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
    }
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize_efs( struct eris_memheap *memheap )
{
    return memheap->xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize_efs( struct eris_memheap *memheap )
{
    return memheap->xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

void * pvPortCalloc_efs( size_t xNum,
                     size_t xSize,
                     struct eris_memheap *memheap )
{
    void * pv = NULL;

    if( heapMULTIPLY_WILL_OVERFLOW( xNum, xSize ) == 0 )
    {
        pv = pvPortMalloc_efs( xNum * xSize , memheap);

        if( pv != NULL )
        {
            ( void ) memset( pv, 0, xNum * xSize );
        }
    }

    return pv;
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList_efs( BlockLink_t * pxBlockToInsert ,struct eris_memheap * memheap)
{
    BlockLink_t * pxIterator;
    uint8_t * puc;

    /* Iterate through the list until a block is found that has a higher address
     * than the block being inserted. */
    for( pxIterator = &(memheap->xStart); pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
    {
        /* Nothing to do here, just iterate to the right position. */
    }

    /* Do the block being inserted, and the block it is being inserted after
     * make a contiguous block of memory? */
    puc = ( uint8_t * ) pxIterator;

    if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
    {
        pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
        pxBlockToInsert = pxIterator;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }

    /* Do the block being inserted, and the block it is being inserted before
     * make a contiguous block of memory? */
    puc = ( uint8_t * ) pxBlockToInsert;

    if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
    {
        if( pxIterator->pxNextFreeBlock != memheap->pxEnd )
        {
            /* Form one big block from the two blocks. */
            pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
            pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
        }
        else
        {
            pxBlockToInsert->pxNextFreeBlock = memheap->pxEnd;
        }
    }
    else
    {
        pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
    }

    /* If the block being inserted plugged a gab, so was merged with the block
     * before and the block after, then it's pxNextFreeBlock pointer will have
     * already been set, and should not be set here as that would make it point
     * to itself. */
    if( pxIterator != pxBlockToInsert )
    {
        pxIterator->pxNextFreeBlock = pxBlockToInsert;
    }
    else
    {
        mtCOVERAGE_TEST_MARKER();
    }
}
/*-----------------------------------------------------------*/

//void vPortDefineHeapRegions( const HeapRegion_t * const pxHeapRegions )
//{
//    BlockLink_t * pxFirstFreeBlockInRegion = NULL;
//    BlockLink_t * pxPreviousFreeBlock;
//    portPOINTER_SIZE_TYPE xAlignedHeap;
//    size_t xTotalRegionSize, xTotalHeapSize = 0;
//    BaseType_t xDefinedRegions = 0;
//    portPOINTER_SIZE_TYPE xAddress;
//    const HeapRegion_t * pxHeapRegion;
//
//    /* Can only call once! */
//    configASSERT( pxEnd == NULL );
//
//    pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );
//
//    while( pxHeapRegion->xSizeInBytes > 0 )
//    {
//        xTotalRegionSize = pxHeapRegion->xSizeInBytes;
//
//        /* Ensure the memheap region starts on a correctly aligned boundary. */
//        xAddress = ( portPOINTER_SIZE_TYPE ) pxHeapRegion->pucStartAddress;
//
//        if( ( xAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
//        {
//            xAddress += ( portBYTE_ALIGNMENT - 1 );
//            xAddress &= ~portBYTE_ALIGNMENT_MASK;
//
//            /* Adjust the size for the bytes lost to alignment. */
//            xTotalRegionSize -= ( size_t ) ( xAddress - ( portPOINTER_SIZE_TYPE ) pxHeapRegion->pucStartAddress );
//        }
//
//        xAlignedHeap = xAddress;
//
//        /* Set xStart if it has not already been set. */
//        if( xDefinedRegions == 0 )
//        {
//            /* xStart is used to hold a pointer to the first item in the list of
//             *  free blocks.  The void cast is used to prevent compiler warnings. */
//            xStart.pxNextFreeBlock = ( BlockLink_t * ) xAlignedHeap;
//            xStart.xBlockSize = ( size_t ) 0;
//        }
//        else
//        {
//            /* Should only get here if one region has already been added to the
//             * memheap. */
//            configASSERT( pxEnd != NULL );
//
//            /* Check blocks are passed in with increasing start addresses. */
//            configASSERT( xAddress > ( size_t ) pxEnd );
//        }
//
//        /* Remember the location of the end marker in the previous region, if
//         * any. */
//        pxPreviousFreeBlock = pxEnd;
//
//        /* pxEnd is used to mark the end of the list of free blocks and is
//         * inserted at the end of the region space. */
//        xAddress = xAlignedHeap + xTotalRegionSize;
//        xAddress -= xHeapStructSize;
//        xAddress &= ~( ( size_t ) portBYTE_ALIGNMENT_MASK );
//        pxEnd = ( BlockLink_t * ) xAddress;
//        pxEnd->xBlockSize = 0;
//        pxEnd->pxNextFreeBlock = NULL;
//
//        /* To start with there is a single free block in this region that is
//         * sized to take up the entire memheap region minus the space taken by the
//         * free block structure. */
//        pxFirstFreeBlockInRegion = ( BlockLink_t * ) xAlignedHeap;
//        pxFirstFreeBlockInRegion->xBlockSize = ( size_t ) ( xAddress - ( portPOINTER_SIZE_TYPE ) pxFirstFreeBlockInRegion );
//        pxFirstFreeBlockInRegion->pxNextFreeBlock = pxEnd;
//
//        /* If this is not the first region that makes up the entire memheap space
//         * then link the previous region to this region. */
//        if( pxPreviousFreeBlock != NULL )
//        {
//            pxPreviousFreeBlock->pxNextFreeBlock = pxFirstFreeBlockInRegion;
//        }
//
//        xTotalHeapSize += pxFirstFreeBlockInRegion->xBlockSize;
//
//        /* Move onto the next HeapRegion_t structure. */
//        xDefinedRegions++;
//        pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );
//    }
//
//    xMinimumEverFreeBytesRemaining = xTotalHeapSize;
//    xFreeBytesRemaining = xTotalHeapSize;
//
//    /* Check something was actually defined before it is accessed. */
//    configASSERT( xTotalHeapSize );
//}
///*-----------------------------------------------------------*/

void *pvPortRealloc_efs(struct eris_memheap *memheap, void *ptr, eris_size_t newsize)
{
    BlockLink_t * pxBlock;
    BlockLink_t * pxPreviousBlock;
    BlockLink_t * pxOldBlock;
    BlockLink_t * pxNewBlockLink;
    eris_size_t oldsize;
    eris_size_t xAdditionalRequiredSize;
    eris_size_t xUsed;
    eris_bool_t flag = 0;

    void * pvReturn = NULL;
    uint8_t * puc = ( uint8_t * ) ptr;
    if(newsize <= 0)
    {
        vPortFree_efs(ptr,memheap);

        return Eris_NULL;
    }
    if(ptr == NULL)
    {
        return pvPortMalloc_efs(newsize,memheap);
    }
    puc -= memheap->xHeapStructSize;
    pxBlock = (BlockLink_t *) puc;

    //存在检测(未完成)
    oldsize = pxBlock->xUsedBlockSize;
    pxOldBlock = pxBlock;
    xSemaphoreTake( memheap->lock, portMAX_DELAY );;
    {
        if(newsize > 0 )
        {
            xAdditionalRequiredSize = memheap->xHeapStructSize + portBYTE_ALIGNMENT - ( newsize & portBYTE_ALIGNMENT_MASK );

            if( heapADD_WILL_OVERFLOW( newsize, xAdditionalRequiredSize ) == 0 )
            {   
                xUsed = newsize;
                newsize += xAdditionalRequiredSize;
            }
            else
            {
                newsize = 0;
            }
        }

        
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }

        if( heapBLOCK_SIZE_IS_VALID( newsize ) != 0 )
        {
            if(pxBlock->xBlockSize >= newsize )
            {   
                memheap->xFreeBytesRemaining += pxOldBlock->xBlockSize;
                pvReturn = ( ( ( uint8_t * ) pxBlock ) + memheap->xHeapStructSize);
                pxBlock->xUsedBlockSize = xUsed;

                    if( ( pxBlock->xBlockSize - newsize ) > heapMINIMUM_BLOCK_SIZE )
                    {
                        pxNewBlockLink = ( BlockLink_t * ) ( ( ( uint8_t * ) pxBlock ) + newsize );

                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - newsize;
                        pxNewBlockLink->xUsedBlockSize = 0L;
                        pxNewBlockLink->magic = (Eris_MEMHEAP_MAGIC | Eris_MEMHEAP_FREED);
                        pxNewBlockLink->pool_ptr = memheap;
                        pxBlock->xBlockSize = newsize;

                        /* Insert the new block into the list of free blocks. */
                        prvInsertBlockIntoFreeList_efs( ( pxNewBlockLink ) ,memheap);
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    memheap->xFreeBytesRemaining -= pxBlock->xBlockSize;

                    if( memheap->xFreeBytesRemaining < memheap->xMinimumEverFreeBytesRemaining )
                    {
                        memheap->xMinimumEverFreeBytesRemaining = memheap->xFreeBytesRemaining;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    heapALLOCATE_BLOCK( pxBlock );
                    pxBlock->pxNextFreeBlock = NULL;
                    memheap->xNumberOfSuccessfulAllocations++;
            }
            else if( newsize <= memheap->xFreeBytesRemaining)
            {
                pxPreviousBlock = &xStart;
                pxBlock = xStart.pxNextFreeBlock;


                while( ( pxBlock->xBlockSize < newsize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
                {
                    pxPreviousBlock = pxBlock;
                    pxBlock = pxBlock->pxNextFreeBlock;
                }

                
                if( pxBlock != pxEnd )
                {
                    pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

                    pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

                    pxBlock->xUsedBlockSize = xUsed;

                    if( ( pxBlock->xBlockSize - newsize ) > heapMINIMUM_BLOCK_SIZE )
                    {
                        pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + newsize );
                        pxNewBlockLink->pool_ptr = memheap;
                        pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - newsize;
                        pxNewBlockLink->xUsedBlockSize = 0L;
                        pxNewBlockLink->magic = (Eris_MEMHEAP_MAGIC | Eris_MEMHEAP_FREED);
                        pxBlock->xBlockSize = newsize;

                        /* Insert the new block into the list of free blocks. */
                        prvInsertBlockIntoFreeList_efs( ( pxNewBlockLink ),memheap );
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }
                    memheap->xFreeBytesRemaining -= pxBlock->xBlockSize;

                    if( memheap->xFreeBytesRemaining < memheap->xMinimumEverFreeBytesRemaining )
                    {
                        memheap->xMinimumEverFreeBytesRemaining = memheap->xFreeBytesRemaining;
                    }
                    else
                    {
                        mtCOVERAGE_TEST_MARKER();
                    }

                    heapALLOCATE_BLOCK( pxBlock );
                    
                    pxBlock->pxNextFreeBlock = NULL;
                    memheap->xNumberOfSuccessfulAllocations++;
                    eris_memcpy( pvReturn , ptr,(eris_ubase_t)oldsize);
                    flag = 1;
                }
                else
                {
                    mtCOVERAGE_TEST_MARKER();
                }
            }
            else
            {
                mtCOVERAGE_TEST_MARKER();
            }
        }
        else
        {
            mtCOVERAGE_TEST_MARKER();
        }
        memheap->max_used_size = memheap->pool_size - memheap->xFreeBytesRemaining;
    }
    ( void ) xSemaphoreGive( memheap->lock);
    if(flag)
    {
        vPortfree((void *)ptr,memheap);
    }
    return pvReturn;
}