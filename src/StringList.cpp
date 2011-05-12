/*
 * Simple string list
 */

#include <stdlib.h>
#include <string.h>

#include "StringList.h"

StringList::StringList()
{
	_maxStrings = 0;
	_numStrings = 0;
	_list = NULL;
}

StringList::~StringList()
{
	clearList();
}

void StringList::clearList()
{
	if (_list) {
		for (int i = 0; i < _numStrings; i++) {
			if (_list[i]) {
				delete [] _list[i];
				_list[i] = NULL;
			}	
		}

		delete [] _list;
		_list = NULL;
	}

	_numStrings = 0;
	_maxStrings = 0;
}

StringList& StringList::operator=(StringList &rhs)
{
	if (this != &rhs) {
		clearList();

		for (int i = 0; i < rhs._numStrings; i++) {
			addString(rhs._list[i]);
		}
	}

	return *this;
}

int StringList::getNumStrings()
{
	return _numStrings;
}

bool StringList::addString(const char *s)
{
	char *temp;

	if (!s || !*s) {
		return false;
	}

	if (_numStrings == _maxStrings) {
		if (!growList(_maxStrings + 8)) {
			return false;
		}
	}

	temp = newStrDup(s);

	if (!temp) {
		return false;
	}

	int len = strlen(temp);

	if (len > 1) {
		if (temp[len-1] == '\r') {
			temp[len-1] = 0;
		}
	}

	_list[_numStrings] = temp;	
	_numStrings++;

	return true;
}

const char *StringList::getString(int index)
{
	if (index < 0 || index >= _numStrings) {
		return NULL;
	}

	return _list[index];
}

bool StringList::removeString(int index)
{
	if (index < 0 || index >= _numStrings) {
		return false;
	}	

	if (_list[index]) {
		delete [] _list[index];
		_list[index] = NULL;
	}

	for (int i = index + 1; i < _numStrings; i++) {
		_list[i-1] = _list[i];
		_list[i] = NULL;
	}

	_numStrings--;

	return true;
}

int StringList::findString(const char *s)
{
	if (s  && *s) {
		for (int i = 0; i < _numStrings; i++) {
			if (!strcmp(_list[i], s)) {
				return i;
			}
		}
	}

	return -1;
}

bool StringList::growList(int new_size)
{
	int i;
	char **temp;

	if (new_size < _maxStrings) {
		return false;
	}

	temp = new char *[new_size];

	if (!temp) {
		return false;
	}

	for (i = 0; i < _numStrings; i++) {
		temp[i] = _list[i];
	}

	for (; i < new_size; i++) {
		temp[i] = NULL;
	}

	if (_list) {
		delete [] _list;
	}

	_list = temp;
	_maxStrings = new_size;

	return true;
}

char* StringList::newStrDup(const char *src)
{
	if (!src) {
		return NULL;
	}

	int len = 1 + strlen(src);

	char *dst = new char[len];

	if (dst) {
		strncpy_s(dst, len, src, _TRUNCATE);
	}

	return dst;
}
