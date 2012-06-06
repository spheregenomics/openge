/*********************************************************************
 *
 * sam_writer.cpp: A simple SAM writer.
 * Open Genomics Engine
 *
 * Author: Lee C. Baker, VBI
 * Last modified: 16 March 2012
 *
 *********************************************************************
 *
 * This file is released under the Virginia Tech Non-Commercial 
 * Purpose License. A copy of this license has been provided in 
 * the openge/ directory.
 *
 *********************************************************************
 *
 * The SaveAlignment function in this file was ported from bamtools' 
 * bamtools_convert.cpp, and was originially released under the 
 * following license (MIT):
 *
 * (c) 2010 Derek Barnett, Erik Garrison
 * Marth Lab, Department of Biology, Boston College
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <iostream>
#include <sstream>
#include <cassert>
#include "sam_writer.h"

using namespace std;
using namespace BamTools;

SamWriter::SamWriter() 
: open(false)
{
    
}

bool SamWriter::Open(const string& filename,
                     const string& samHeaderText,
                     const RefVector& referenceSequences) {
    this->filename = filename;
    file.open(filename.c_str(), ios::out);
    
    if(file.fail()) {
        cerr << "Failed to open SAM output file " << filename << endl;
        return false;
    }
    
    file << samHeaderText << endl;

    references = referenceSequences;
    
    open = true;
    
    return true;
}

bool SamWriter::Close() {
    file.close();
    
    open = false;
    
    return true;
}

bool SamWriter::SaveAlignment(const BamTools::BamAlignment & a) {
    // tab-delimited
    // <QNAME> <FLAG> <RNAME> <POS> <MAPQ> <CIGAR> <MRNM> <MPOS> <ISIZE> <SEQ> <QUAL> [ <TAG>:<VTYPE>:<VALUE> [...] ]
    
    assert(open);
    ofstream & m_out = file;
    
    // write name & alignment flag
    m_out << a.Name << "\t" << a.AlignmentFlag << "\t";
    
    // write reference name
    if ( (a.RefID >= 0) && (a.RefID < (int)references.size()) ) 
        m_out << references[a.RefID].RefName << "\t";
    else 
        m_out << "*\t";
    
    // write position & map quality
    m_out << a.Position+1 << "\t" << a.MapQuality << "\t";
    
    // write CIGAR
    const vector<CigarOp>& cigarData = a.CigarData;
    if ( cigarData.empty() ) m_out << "*\t";
    else {
        vector<CigarOp>::const_iterator cigarIter = cigarData.begin();
        vector<CigarOp>::const_iterator cigarEnd  = cigarData.end();
        for ( ; cigarIter != cigarEnd; ++cigarIter ) {
            const CigarOp& op = (*cigarIter);
            m_out << op.Length << op.Type;
        }
        m_out << "\t";
    }
    
    // write mate reference name, mate position, & insert size
    if ( a.IsPaired() && (a.MateRefID >= 0) && (a.MateRefID < (int)references.size()) ) {
        if ( a.MateRefID == a.RefID )
            m_out << "=\t";
        else
            m_out << references[a.MateRefID].RefName << "\t";
        m_out << a.MatePosition+1 << "\t" << a.InsertSize << "\t";
    } 
    else
        m_out << "*\t0\t0\t";
    
    // write sequence
    if ( a.QueryBases.empty() )
        m_out << "*\t";
    else
        m_out << a.QueryBases << "\t";
    
    // write qualities
    if ( a.Qualities.empty() )
        m_out << "*";
    else
        m_out << a.Qualities;
    
    // write tag data
    const char* tagData = a.TagData.c_str();
    const size_t tagDataLength = a.TagData.length();
    
    size_t index = 0;
    while ( index < tagDataLength ) {
        
        // write tag name   
        string tagName = a.TagData.substr(index, 2);
        m_out << "\t" << tagName << ":";
        index += 2;
        
        // get data type
        char type = a.TagData.at(index);
        ++index;
        switch ( type ) {
            case (Constants::BAM_TAG_TYPE_ASCII) :
                m_out << "A:" << tagData[index];
                ++index;
                break;
                
            case (Constants::BAM_TAG_TYPE_INT8)  :
            case (Constants::BAM_TAG_TYPE_UINT8) :
                m_out << "i:" << (int)tagData[index];
                ++index;
                break;
                
            case (Constants::BAM_TAG_TYPE_INT16) :
                m_out << "i:" << BamTools::UnpackSignedShort(&tagData[index]);
                index += sizeof(int16_t);
                break;
                
            case (Constants::BAM_TAG_TYPE_UINT16) :
                m_out << "i:" << BamTools::UnpackUnsignedShort(&tagData[index]);
                index += sizeof(uint16_t);
                break;
                
            case (Constants::BAM_TAG_TYPE_INT32) :
                m_out << "i:" << BamTools::UnpackSignedInt(&tagData[index]);
                index += sizeof(int32_t);
                break;
                
            case (Constants::BAM_TAG_TYPE_UINT32) :
                m_out << "i:" << BamTools::UnpackUnsignedInt(&tagData[index]);
                index += sizeof(uint32_t);
                break;
                
            case (Constants::BAM_TAG_TYPE_FLOAT) :
                m_out << "f:" << BamTools::UnpackFloat(&tagData[index]);
                index += sizeof(float);
                break;
                
            case (Constants::BAM_TAG_TYPE_HEX)    :
            case (Constants::BAM_TAG_TYPE_STRING) :
                m_out << type << ":";
                while (tagData[index]) {
                    m_out << tagData[index];
                    ++index;
                }
                ++index;
                break;
        }
        
        if ( tagData[index] == '\0') 
            break;
    }
    
    m_out << endl;
    
    return true;
}
