/*
 * NameValueFile.h
 *
 *  Created on: Aug 27, 2013
 *  \copyright 2013 DCBlaha.  Distributed under the GPL.
 */

#ifndef NAMEVALUEFILE_H_
#define NAMEVALUEFILE_H_

// This file format borrows slightly from json.
//
// The reserved characters are :{} and end of line
// 	Colon separates name from value
//	{} identifies the begin and end of a record.
//	end of line is end of a name value pair
//
// Differences from json are:
//	name values are not quoted
//	no array support

#include <map>
#include <string>
#include <vector>
#include <set>

class CompoundValueRef
    {
    public:
	static void addArg(char const * const arg, std::vector<std::string> &vec)
	    { vec.push_back(arg); }
	/// The delimiter is \n for editing in an editor, and a semicolon
	/// for passing on the command line or saved in the file.
	static std::string getAsString(const std::vector<std::string> &vec,
		char delimiter=';');
	static std::string getAsString(const std::set<std::string> &stdset,
		char delimiter=';');
	static std::vector<std::string> parseString(char const * const str,
		char delimiter=';');
	static void parseStringRef(char const * const str, std::vector<std::string> &vec,
		char delimiter=';');
    };

/// This builds compound string arguments delimited with a semicolon.
/// This can help build options such as:
/// CppArgs|-I/mingw/include;-IC:/Program Files/GTK+-Bundle-3.6.1/include
class CompoundValue:public std::vector<std::string>
    {
    public:
	static const int npos = -1;
	void addArg(char const * const arg)
	    { push_back(arg); }
	/// The delimiter is \n for editing in an editor, and a semicolon
	/// for passing on the command line or saved in the file.
	std::string getAsString(char delimiter=';') const
	    { return CompoundValueRef::getAsString(*this, delimiter); }
	void parseString(char const * const str, char delimiter=';')
	    { CompoundValueRef::parseStringRef(str, *this, delimiter); }
	// Returns index to found string, else npos.
	size_t find(char const * const str);
	/// WARNING - This should not be saved to the options file.
	/// This is for passing command line arguments in Windows/DOS.
	/// This can only be used if the switch (ex: -I) is already stored in the argument.
	void quoteAllArgs();
	static void quoteCommandLineArg(std::string &str);
    };

class NameValueRecord
    {
    public:
	NameValueRecord():
	    mSaveNullValues(false)
	    {}
	static char const mapDelimiter = '|';
	void clear()
	    { mNameValues.clear(); }
	void setNameValue(char const * const optionName, char const * const value);
	std::string getValue(char const * const optionName) const;
	void setNameValueBool(char const * const optionName, bool val);
	bool getValueBool(char const * const optionName) const;
	const std::map<std::string, std::string> &getNameValues() const
	    { return mNameValues; }
	void write(FILE *fp);
	void read(FILE *fp);
	void saveNullValues(bool save)
	    { mSaveNullValues = save; }

    private:
	bool mSaveNullValues;
	std::map<std::string, std::string> mNameValues;
	bool getLine(FILE *fp, std::string &str);
    };

/// This can store a bunch of options in a file. Each option is on a separate line
/// and has a name and value separated by a colon.  Anything after the first
/// colon is a value, which means the value can contain colons.
class NameValueFile:public NameValueRecord
    {
    public:
	NameValueFile(char const * const fn = NULL)
	    {
	    if(fn)
		setFilename(fn);
	    }
	void setFilename(char const * const fn)
	    { mFilename = fn; }
	void setFilename(std::string const &fn)
	    { mFilename = fn.c_str(); }
	bool writeFile();
	bool readFile();

    private:
	std::string mFilename;
	const std::string &getFilename() const
	    { return mFilename; }
    };


#endif /* NAMEVALUEFILE_H_ */
