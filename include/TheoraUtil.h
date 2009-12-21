#ifndef _TheoraUtil_h
#define _TheoraUtil_h

#include <string>
#include <vector>

#define foreach(type,lst) for (std::vector<type>::iterator it=lst.begin();it != lst.end(); it++)
#define foreach_r(type,lst) for (std::vector<type>::reverse_iterator it=lst.rbegin();it != lst.rend(); it++)

#define foreach_in_map(type,list) for (std::map<std::string,type>::iterator it=list.begin();it != list.end(); it++)

float sign(float number);
std::string str(int i);
std::string str_toupper(std::string s);
int str_to_int(std::string s);
float str_to_float(std::string s);
bool str_split(std::string s,std::string splitter,std::string& out_left,std::string& out_right);
std::vector<std::string> str_split(std::string s,std::string splitter);

bool startswith(std::string s,std::string with_what);
bool endswith(std::string s,std::string with_what);
std::string remove_spaces(std::string s);

std::string pathGetFilename(std::string path,bool with_suffix=1);
std::string pathGetBaseDir(std::string path);

float Sin(float angle);
float Cos(float angle);

float clamp(float value,float min_value,float max_value);

std::string generateName(std::string prefix);

void psleep(int milliseconds);
void writelog(std::string log_msg);
#endif