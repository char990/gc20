#include <sign/GroupApp.h>
#include <sign/Scheduler.h>

GroupApp::GroupApp()
{
}

GroupApp::~GroupApp()
{
}

int GroupApp::Rx(uint8_t *data, int len)
{
    switch (data[1])
    {
    case 0x06:
        SlaveStatusRpl(data, len);
        break;
    case 0x08:
        SlaveExtStatusRpl(data, len);
        break;
    default:
        return -1;
    }
    return 0;
}

void GroupApp::Clean()
{
    
}

void GroupApp::SlaveStatusRpl(uint8_t * data, int len) 
{
    if(len!=14) return;
}

void GroupApp::SlaveExtStatusRpl(uint8_t * data, int len) 
{
    if(len<22)return;
}

