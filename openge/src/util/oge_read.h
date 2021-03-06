/*********************************************************************
 *
 * oge_read.h:  Main class for storing read data.
 * Open Genomics Engine
 *
 * Author: Lee C. Baker, VBI
 * Last modified: 17 Oct 2012
 *
 *********************************************************************
 *
 * This file is released under the Virginia Tech Non-Commercial
 * Purpose License. A copy of this license has been provided in
 * the openge/ directory.
 *
 *********************************************************************
 *
 * This file extends BamTool's BamAlignment class, and adds allocate/
 * deallocate functions that cache allocated BamAlignments for 
 * peformance reasons.
 *
 *********************************************************************/

#ifndef OGE_READ_H
#define OGE_READ_H

#include "bamtools/BamAlignment.h"

#include "thread_pool.h"

class OGERead : public BamTools::BamAlignment {
public:
    // cached allocator functions
    static OGERead * allocate();
    static void deallocate(OGERead * al);
    static void clearCachedAllocations();
    static Spinlock allocate_lock;
protected:
    static SynchronizedQueue<OGERead *> cached_allocations;
    static SynchronizedQueue<OGERead *> cached_allocations_cleared;
    static bool clean_thread_running;
};

#endif