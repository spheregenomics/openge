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
: output_stream(&cout)
, open(false)
{
    
}

bool SamWriter::Open(const string& filename,
                     const string& samHeaderText,
                     const RefVector& referenceSequences) {
    this->filename = filename;

    if(filename != "stdout") {
        file.open(filename.c_str(), ios::out);
        output_stream = &file;
    
        if(file.fail()) {
            cerr << "Failed to open SAM output file " << filename << endl;
            return false;
        }
    }
    
    *output_stream << samHeaderText;

    references = referenceSequences;
    
    open = true;
    
    return true;
}

bool SamWriter::Close() {
    if(file.is_open())
        file.close();
    
    open = false;
    
    return true;
}

bool SamWriter::SaveAlignment(BamTools::BamAlignment & a) {
    // tab-delimited
    // <QNAME> <FLAG> <RNAME> <POS> <MAPQ> <CIGAR> <MRNM> <MPOS> <ISIZE> <SEQ> <QUAL> [ <TAG>:<VTYPE>:<VALUE> [...] ]
    
    a.BuildCharData();
    assert(open);
    ostream & m_out = *output_stream;
    
    // write name & alignment flag
    m_out << a.getName() << "\t" << a.getAlignmentFlag() << "\t";
    
    // write reference name
    if ( (a.getRefID() >= 0) && (a.getRefID() < (int)references.size()) ) 
        m_out << references[a.getRefID()].RefName << "\t";
    else 
        m_out << "*\t";
    
    // write position & map quality
    m_out << a.getPosition()+1 << "\t" << a.getMapQuality() << "\t";
    
    // write CIGAR
    const vector<CigarOp>& cigarData = a.getCigarData();
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
    if ( a.IsPaired() && (a.getMateRefID() >= 0) && (a.getMateRefID() < (int)references.size()) ) {
        if ( a.getMateRefID() == a.getRefID() )
            m_out << "=\t";
        else
            m_out << references[a.getMateRefID()].RefName << "\t";
        m_out << a.getMatePosition()+1 << "\t" << a.getInsertSize() << "\t";
    } 
    else
        m_out << "*\t0\t0\t";
    
    // write sequence
    if ( a.getQueryBases().empty() )
        m_out << "*\t";
    else
        m_out << a.getQueryBases() << "\t";
    
    // write qualities
    if ( a.getQualities().empty() )
        m_out << "*";
    else
        m_out << a.getQualities();
    
    // write tag data
    const char* tagData = a.getTagData().c_str();
    const size_t tagDataLength = a.getTagData().length();
    
    size_t index = 0;
    while ( index < tagDataLength ) {
        
        // write tag name   
        string tagName = a.getTagData().substr(index, 2);
        m_out << "\t" << tagName << ":";
        index += 2;
        
        // get data type
        char type = a.getTagData().at(index);
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
