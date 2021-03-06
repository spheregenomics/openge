/*********************************************************************
 *
 * command_help.cpp: Display help information.
 * Open Genomics Engine
 *
 * Author: Lee C. Baker, VBI
 * Last modified: 21 May 2012
 *
 *********************************************************************
 *
 * This file is released under the Virginia Tech Non-Commercial 
 * Purpose License. A copy of this license has been provided in 
 * the openge/ directory.
 *
 *********************************************************************/

#include "commands.h"
#include <openge_constants.h>

#include <iostream>
using namespace std;

namespace po = boost::program_options;

int VersionCommand::runCommand()
{
    cerr << "Open Genomics Engine " OPENGE_VERSION_STRING << endl;
    cerr << "Built on " __DATE__ " at " __TIME__ << endl;
    cerr << "(c) 2012 Virginia Bioinformatics Institute" << endl;
    cerr << "Compiled with boost version : "BOOST_LIB_VERSION << endl;
    
    return 0;
}
