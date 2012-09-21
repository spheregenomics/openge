#ifndef OGE_READ_STREAM_WRITER_H
#define OGE_READ_STREAM_WRITER_H

/*********************************************************************
 *
 * read_stream_writer.h: Interface for writing a BAM/SAM/CRAM file.
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

#include <api/SamHeader.h>
#include <api/BamAlignment.h>

class ReadStreamWriter {
    virtual bool open(const std::string & filename, const BamTools::SamHeader & header) = 0;
    virtual void close() = 0;
    virtual bool is_open() const = 0;
    virtual bool write(const BamTools::BamAlignment & alignment) = 0;
};

#endif