/*
 * CoverageHeaderReader.cpp
 *
 *  Created on: Sep 17, 2014
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#include "CoverageHeaderReader.h"
#include "FilePath.h"

std::string CoverageHeaderReader::getFn(char const * const outDir)
    {
    FilePath outFn(outDir, FP_Dir);
    outFn.appendDir("covLib");
    ensurePathExists(outFn.c_str());
    outFn.appendFile("OovCoverage.h");
    return outFn;
    }

void CoverageHeaderReader::insertBufToMap(std::string const &buf)
    {
    size_t pos = 0;
    mInstrDefineMap.clear();
    int lineCounter = 0;	// Base 1
    while(pos != std::string::npos)
	{
	lineCounter++;
	size_t endPos = buf.find('\n', pos);
	std::string line(buf, pos, endPos-pos);

	if(lineCounter > 5)
	    {
	    // Insert line
	    char keyword[10];
	    char fnDef[250];
	    char commentChars[10];
	    int count;
	    int offset;
	    int numItems = sscanf(line.c_str(), "%10s %250s %d %10s %d", keyword, fnDef,
		    &offset, commentChars, &count);
	    if(numItems >= 5)
		{
		mInstrDefineMap.insert(std::pair<std::string, int>(fnDef, count));
		}
	    }
	else if(lineCounter == 4)
	    {
	    char keyword[10];
	    char def[100];
	    mNumInstrumentedLines = 0;
	    sscanf(line.c_str(), "%10s %100s %d", keyword, def, &mNumInstrumentedLines);
	    }

	pos = endPos;
	if(pos != std::string::npos)
	    pos++;
	}
    }

void CoverageHeaderReader::read(SharedFile &outDefFile)
    {
    std::string buf(outDefFile.getSize(), 0);
    int actualSize;
    bool success = outDefFile.read(&buf[0], buf.size(), actualSize);
    if(success)
	{
	insertBufToMap(buf);
	}
    }

