#ifndef OGE_ALGO_FILTER_H
#define OGE_ALGO_FILTER_H

/*********************************************************************
 *
 * filter.h:  Algorithm module that filters a stream of reads.
 * Open Genomics Engine
 *
 * Author: Lee C. Baker, VBI
 * Last modified: 22 May 2012
 *
 *********************************************************************
 *
 * This file is released under the Virginia Tech Non-Commercial 
 * Purpose License. A copy of this license has been provided in 
 * the openge/ directory.
 *
 *********************************************************************
 *
 * Filter algorithm module class. Filters a stream of reads by the 
 * region of the genome, or by count, or both. Some code based on 
 * Bamtools' merge code, from bamtools_merge.cpp.
 *
 *********************************************************************/

#include "algorithm_module.h"

#include "../util/bamtools/BamAux.h"

#include <string>

class Filter : public AlgorithmModule
{
public:
    Filter();
    void setRegion(std::string region) { region_string = region; has_region = true; }
    std::string getRegion() { return region_string; }
    void setCountLimit(int ct) { count_limit = ct;}
    size_t getCountLimit() { return count_limit;}
    void setQualityLimit(int mapq) { mapq_limit = mapq; }
    int getQualityLimit() { return mapq_limit;}
    void setMinimumReadLength(int min_length) { this->min_length = min_length; }
    void setMaximumReadLength(int max_length) { this->max_length = max_length; }
    void setTrimBeginLength(int length) { trim_begin_length = length; }
    void setTrimEndLength(int length) { trim_end_length = length; }
    bool setReadLengths(const std::string & length_string);
    static bool ParseRegionString(const std::string& regionString, BamTools::BamRegion& region, const BamSequenceRecords & sequences);
protected:
    virtual int runInternal();
    void trim(OGERead & al);
protected:
    std::string region_string;
    bool has_region;
    size_t count_limit;
    int mapq_limit;
    int min_length, max_length;
    int trim_begin_length, trim_end_length;
};

#endif