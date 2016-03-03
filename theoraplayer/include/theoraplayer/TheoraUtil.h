/// @file
/// @version 2.0
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause
/// 
/// @section DESCRIPTION
/// 
/// Provides some utility functions and macros.


// TODOth - should be moved to Utility.h probably
#ifndef THEORA_UTIL_H
#define THEORA_UTIL_H

#include <string>
#include <vector>

#ifndef THEORAUTIL_NOMACROS

#define foreach(type,lst) for (std::vector<type>::iterator it=lst.begin();it != lst.end(); ++it)
#define foreach_l(type,lst) for (std::list<type>::iterator it=lst.begin();it != lst.end(); ++it)
#define foreach_r(type,lst) for (std::vector<type>::reverse_iterator it=lst.rbegin();it != lst.rend(); ++it)
#define foreach_in_map(type,lst) for (std::map<std::string,type>::iterator it=lst.begin();it != lst.end(); ++it)

#endif

#define th_writelog(x) TheoraVideoManager::getSingleton().logMessage(x)


std::string str(int i);
std::string strf(float i);
void _psleep(int milliseconds);
int _nextPow2(int x);

#endif