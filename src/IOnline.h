#ifndef __IONLINE_H__
#define __IONLINE_H__

class IOnline
{
public:
    virtual bool Online()=0;
    virtual void Online(bool v)=0;
};

#endif
