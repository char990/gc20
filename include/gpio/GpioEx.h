#pragma once
class GpioEx
{
public:
    enum DIR { OUTPUT, INPUT };
    GpioEx(unsigned int pin,int inout);
    ~GpioEx(void);
    int SetDir(int inout);
    int SetValue(bool value);
    int GetValue();             // -1:falied, 0:low, 1:high
    int SetEdge(char *edge);
    unsigned int Pin(){return _pin;};

private:
    int OpenFd(void);
    int GetFd(void);
    int CloseFd(void);
    unsigned int _pin;
    int _fd;
    int _ex;
    int _dir;
    int Export();
    int Unexport();
};


