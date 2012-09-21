#ifndef OGE_SEQ_READER_CACHE_H
#define OGE_SEQ_READER_CACHE_H

/*********************************************************************
 *
 * bam_deserializer.h: Read data from an uncompressed bam stream.
 * Open Genomics Engine
 *
 * Author: Lee C. Baker, VBI
 * Last modified: 28 August 2012
 *
 *********************************************************************
 *
 * This file is released under the Virginia Tech Non-Commercial
 * Purpose License. A copy of this license has been provided in
 * the openge/ directory.
 *
 *********************************************************************/

#include "read_stream_reader.h"
#include "thread_pool.h"

template <class reader_t>
class SequentialReaderCache : public ReadStreamReader {
public:
    SequentialReaderCache()
    : thread_job(this)
    {}
    virtual bool open(const std::string & filename) { read_finished = false; return reader.open(filename); }
    virtual const BamTools::SamHeader & getHeader() const { return reader.getHeader(); };
    virtual void close() { return reader.close(); }
    virtual BamTools::BamAlignment * read();
    virtual bool is_open() { return reader.is_open(); }
protected:
    reader_t reader;
    SynchronizedBlockingQueue<BamTools::BamAlignment *> read_queue;
    
    class PrefetchJob : public ThreadJob {
        SequentialReaderCache * cache;
    public:
        PrefetchJob(SequentialReaderCache * cache)
        : cache(cache)
        , is_running(false)
        {}

        bool is_running;
        virtual void runJob() ;
    };
    PrefetchJob thread_job;
    bool read_finished;
};

template <class reader_t>
BamTools::BamAlignment * SequentialReaderCache<reader_t>::read() {
    if(read_queue.size() < 5000 && (read_queue.empty() || read_queue.back() != NULL) && thread_job.is_running == false && !read_finished) {
        ThreadPool::sharedPool()->addJob(&thread_job);
        thread_job.is_running = true;
    }

    BamTools::BamAlignment * ret = read_queue.pop();
    
    if(NULL == ret)
        read_finished = true;
    
    return ret;
}

template <class reader_t>
void SequentialReaderCache<reader_t>::PrefetchJob::runJob() {
    while(cache->read_queue.size() < 10000 && (cache->read_queue.empty() || cache->read_queue.back() != NULL)) {
        BamTools::BamAlignment * al = cache->reader.read();
        cache->read_queue.push(al);
    }
    is_running = false;
}

#endif