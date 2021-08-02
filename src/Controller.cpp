#include "Controller.h"

void Controller::Init()
{
    displayTimeout.Clear();
}

void Controller::RefreshDispTime()
{
    displayTimeout.Setms(100000);
}

void Controller::SessionLed(uint8_t v)
{
    sessionLed = v;
}
