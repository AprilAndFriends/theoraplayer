#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <string>

class _TheoraGenericException
{
public:
    std::string mErrText,mFile,mType;
	int mLineNumber;

	_TheoraGenericException(const std::string& errorText,std::string type="",std::string file="",int line=0);
    virtual ~_TheoraGenericException() {}
    
	virtual std::string repr();
    
	void writeOutput();
	
	virtual const std::string& getErrorText() { return mErrText; }
    
	const std::string getType(){ return mType; }
};

#define TheoraGenericException(msg) _TheoraGenericException(msg,"TheoraGenericException",__FILE__,__LINE__)


#define exception_cls(name) class name : public _TheoraGenericException \
{ \
public: \
	name(const std::string& errorText,std::string type="",std::string file="",int line=0) : \
	  _TheoraGenericException(errorText,type,file,line){} \
}

exception_cls(_KeyException);


#endif
