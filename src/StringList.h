/*
 * Simple string list
 */

#pragma once

class StringList
{
public:
	StringList();
	~StringList();

	StringList& operator=(StringList &rhs);

	int getNumStrings();
	
	bool addString(const char *s);
	const char *getString(int index);
	bool removeString(int index);
	void clearList();
	int findString(const char *s);

private:
	bool growList(int new_size);
	char* newStrDup(const char *src);

	int _maxStrings;
	int _numStrings;
	char ** _list;
};
