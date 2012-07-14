/*********************************************************************
 *
 * filter.cpp:  Algorithm module that filters a stream of reads.
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

#include "measure_coverage.h"

#include <iomanip>
#include <algorithm>
#include <numeric>
#include <cassert>

using namespace BamTools;
using namespace std;

MeasureCoverage::MeasureCoverage()
: verify_mapping(false)
, print_zero_cover_bases(false)
{ }

int MeasureCoverage::runInternal()
{
    SamHeader header = this->getHeader();
    int total_length = 0;
    int num_correct_maps = 0;
    int num_skipped_reads = 0;
    
    if(verbose) 
        cerr << "Setting up coverage counting structures" << endl;
    for(SamSequenceConstIterator i = header.Sequences.Begin(); i != header.Sequences.End(); i++) {
        int length = atoi(i->Length.c_str());
        total_length += length;

        coverage_map[i->Name].reserve(length);
        coverage_map[i->Name].insert(coverage_map[i->Name].begin(), length, 0);
        
        if(verify_mapping) {
            correctness_map[i->Name].reserve(length);
            correctness_map[i->Name].insert(correctness_map[i->Name].begin(), length, 0);
        }
    }
    
    if(verbose) 
        cerr << "Measuring coverage" << endl;
    while(true) {
        BamAlignment * al = getInputAlignment();
        
        if(!al)
            break;
        
        if(al->RefID == -1 || al->Position == -1) {
            num_skipped_reads++;
            putOutputAlignment(al);
            continue;
        }

        string & chr = header.Sequences[al->RefID].Name;
        assert(coverage_map.count(chr) > 0);
        vector<int> & chr_vector = coverage_map[chr];

        for(int i = al->Position; i <= al->Position + al->Length; i++)
            chr_vector[i]++;

        // we are scanning read names that look like 
        // "@chr1_80429992_80429614_1_0_0_0_0:0:0_0:0:0_2ed4"
        // and define correctness as chromosome name matching, and 
        // position being within 5 bases of either of the two numbers
        // in the name (8042xxxxx in this example
        if(verify_mapping) {
            char name[32] = {0};
            int n1, n2;
            const char * name_buffer = al->Name.c_str();
            const char * first_underscore = strchr(al->Name.c_str(), '_');
            strncpy(name, name_buffer, first_underscore - name_buffer);
            if( first_underscore ) {
                int ret = sscanf(first_underscore, "_%d_%d", &n1, &n2);
               if( 0 == strcmp(name, chr.c_str()) && ret == 2
               && (5 >= abs(n1 - al->Position) || 5 >= abs(n1 - al->Position))
               ) {
                
                    assert(correctness_map.count(chr) > 0);
                    vector<int> & correct_vector = correctness_map[chr];
                    for(int i = al->Position; i <= al->Position + al->Length; i++)
                        correct_vector[i]++;
                
                    num_correct_maps++;
                }
            }
        }
        
        putOutputAlignment(al);
    }

    if(num_skipped_reads)
        cerr << "Skipped " << num_skipped_reads << " unmapped reads." << endl;
    
    //now print coverage:
    cerr << "Average coverage:" << endl;
    for(map<string, vector<int> >::const_iterator vec = coverage_map.begin(); vec != coverage_map.end(); vec++) 
        cerr << "   " << setw(20) << vec->first << ": " << setw(8) << double( std::accumulate(vec->second.begin(), vec->second.end(), 0)) / vec->second.size() << "x" << endl;
    
    if(verbose && verify_mapping)
        cerr << "Found " << num_correct_maps << " / " << write_count << " (" << 100. * num_correct_maps / write_count << " %) reads were correctly mapped." << endl;

    if(verbose) {
        if(print_zero_cover_bases)
            cerr << "Writing coverage file (expect " << total_length << " lines)" << endl;
        else
            cerr << "Writing coverage file" << endl;
    }
    
    if(out_filename != "stdout") {

        ofstream outfile(out_filename.c_str());
        
        if(verify_mapping)
            outfile << "chromosome,position,coverage,correct_maps\n";
        else
            outfile << "chromosome,position,coverage\n";
        int write_ct = 0;
        for(map<string, vector<int> >::const_iterator vec = coverage_map.begin(); vec != coverage_map.end(); vec++) {
            const int * count_data = &(vec->second[0]);
            const int * correctness_data = &(correctness_map[vec->first][0]);
            
            for(int i = 0; i != vec->second.size(); i++) {
                if(print_zero_cover_bases || count_data[i] != 0) {
                    if(verify_mapping)
                        outfile << vec->first << "," << i+1 << "," << count_data[i] << "," << correctness_data[i] << "\n";
                    else
                        outfile << vec->first << "," << i+1 << "," << count_data[i] << "\n";
                }

                write_ct++;
                if(verbose && write_ct % 5000000 == 0)
                    cerr << "\rWriting " << 100. * write_ct / total_length << "% done";
            }
        }
        if(verbose)
            cerr << "\rWriting 100% done" << endl;
    }

    return 0;
}
