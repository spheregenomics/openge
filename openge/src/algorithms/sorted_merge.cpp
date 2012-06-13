/*********************************************************************
 *
 * sorted_merge.cpp:  Merge multiple read streams, keeping them 
 *                    sorted.
 * Open Genomics Engine
 *
 * Author: Lee C. Baker, VBI
 * Last modified: 7 June 2012
 *
 *********************************************************************
 *
 * This file is released under the Virginia Tech Non-Commercial 
 * Purpose License. A copy of this license has been provided in 
 * the openge/ directory.
 *
 *********************************************************************/

#include "sorted_merge.h"

#include <algorithm>
#include <numeric>
#include <pthread.h>

#include <api/algorithms/Sort.h>

using namespace BamTools;
using namespace std;

SortedMerge::~SortedMerge()
{
    for( int i = 0; i < input_proxies.size(); i++)
        delete input_proxies[i];
}

SortedMerge::SortedMergeInputProxy::SortedMergeInputProxy(SortedMerge * parent) 
: merge_class(parent)
{
    int retval = pthread_mutex_init(&merge_complete, NULL);
    
	if(0 != retval) {
		perror("Error opening sortedmerge completion mutex");
        assert(0 == retval);
        exit(-1);
    }
    
    retval = pthread_mutex_lock(&merge_complete);    //hold this mutex until we are done.
    
	if(0 != retval) {
		perror("Error opening sortedmerge completion mutex");
        assert(0 == retval);
        exit(-1);
    }
}

int SortedMerge::SortedMergeInputProxy::runInternal()
{
    int retval = pthread_mutex_lock(&merge_complete);
    
	if(0 != retval) {
		perror("Error locking completion mutex in SortedMergeInputProxy class");
        assert(0 == retval);
        exit(-1);
    }
    
    retval = pthread_mutex_unlock(&merge_complete);
    
	if(0 != retval) {
		perror("Error locking completion mutex in SortedMergeInputProxy class");
        assert(0 == retval);
        exit(-1);
    }
    
    retval = pthread_mutex_destroy(&merge_complete);
    
	if(0 != retval) {
		perror("Error destroying completion mutex in SortedMergeInputProxy class");
        assert(0 == retval);
        exit(-1);
    }

    cerr << "Pipe done after " << read_count << " reads." << endl;

    return 0;
}

void SortedMerge::SortedMergeInputProxy::mergeDone()
{
    int retval = pthread_mutex_unlock(&merge_complete);
    
	if(0 != retval) {
		perror("Error unlocking completion mutex after merge completion");
        assert(0 == retval);
        exit(-1);
    }
}

SortedMerge::SortedMerge()
{
}

void SortedMerge::addSource(AlgorithmModule * source)
{
    SortedMergeInputProxy * proxy = new SortedMergeInputProxy(this);
    input_proxies.push_back(proxy);
    
    //set parent to be the first source, so parsing around the tree keeps working.
    if(input_proxies.size() == 1)
        proxy->addSink(this);

    source->addSink(proxy);
}

int SortedMerge::runInternal()
{
    ogeNameThread("am_merge_sorted");

    priority_queue<BamAlignment *, vector<BamAlignment *>, BamTools::Algorithms::Sort::ByPosition> reads;
    map<BamAlignment *, SortedMergeInputProxy * > read_sources;
    
    // first, get one read from each queue
    // make sure and deal with the case where one chain will never have any reads. TODO LCB

    for(int ctr = 0; ctr < input_proxies.size(); ctr++)
    {
        BamAlignment * read = input_proxies[ctr]->getInputAlignment();

        if(!read) {
            cerr << "Chain done." << endl;
            input_proxies[ctr]->mergeDone();
            continue;
        }

        reads.push(read);
        read_sources[read] = input_proxies[ctr];
    }

    //now handle the steady state situation. When sources are done, We
    // won't have a read any more in the reads pqueue.
    while(reads.size() > 0) {
        BamAlignment * read = reads.top() ;
        reads.pop();
        SortedMergeInputProxy * source = read_sources[read];

        read_sources.erase(read);

        putOutputAlignment(read);

        read = source->getInputAlignment();
        if(!read) {
            cerr << "Chain done." << endl;
            source->mergeDone();
            continue;
        }

        reads.push(read);
        read_sources[read] = source;
    }

    return 0;
}