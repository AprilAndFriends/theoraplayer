#include <algorithm>
#include <math.h>
#include <map>
#include "TheoraUtil.h"
#include "TheoraException.h"

std::string str(int i)
{
    char s[32];
    sprintf(s,"%d",i);
    return std::string(s);
}

int str_to_int(std::string s)
{
    int i;
    sscanf(s.c_str(),"%d",&i);
    return i;
}

float str_to_float(std::string s)
{
    float f;
    sscanf(s.c_str(),"%f",&f);
    return f;
}

int hexstr_to_int(std::string s)
{
    int i;
    sscanf(s.c_str(),"%x",&i);
    return i;
}

std::string str_toupper(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), toupper);
    return s;
}

bool str_split(std::string s,std::string splitter,std::string& out_left,std::string& out_right)
{
    int index=s.find(splitter);
    if (index < 0) return 0;
    
    out_left=s.substr(0,index);
    out_right=s.substr(index+splitter.size(),1000);
    
    return 1;
}

std::vector<std::string> str_split(std::string s,std::string splitter)
{
	std::vector<std::string> lst;
	int index=0,last=0;
	for (;;)
	{
		index=s.find(splitter,last);
		if (index < 0) break;
		lst.push_back(s.substr(last,index-last));
		last=index+splitter.size();
	}
	if (last < s.size()) lst.push_back(s.substr(last,s.size()));
	
	return lst;
}

std::string remove_spaces(std::string s)
{
    std::string out="";
    for(std::string::iterator it=s.begin();it!=s.end();it++)
    {
        if (*it != ' ') out+=*it;
    }
    return out;
}

bool startswith(std::string s,std::string with_what)
{
    return (s.substr(0,with_what.size()) == with_what);
}

bool endswith(std::string s,std::string with_what)
{
	int size=with_what.size();
    return (s.substr(s.size()-size,size) == with_what);
}

std::string pathGetFilename(std::string path,bool with_suffix)
{
    int index1=path.rfind("/"); if (index1 < 0) index1=path.rfind("\\");
    int index2=(with_suffix) ? path.size() : path.rfind(".");
    std::string name=path.substr(index1+1,index2-index1-1);
    return name;
}

std::string pathGetBaseDir(std::string path)
{
	int index=path.rfind("/");  if (index < 0) index=path.rfind("\\");
	return path.substr(0,index);
}

float sign(float number)
{
	if (number >= 0) return 1;
	else             return -1;
}

float Sin(float angle)
{
	return sin(angle/57.295779513f);
}

float Cos(float angle)
{
	return cos(angle/57.295779513f);
}

float clamp(float value,float min_value,float max_value)
{
	if      (value < min_value) value=min_value;
	else if (value > max_value) value=max_value;
	return value;
}
