#include "SerialPort.h"
#include "FreeTimer.h"
using namespace std;
using namespace GC20;

int main()
{
    SerialPortConfig cfg(SerialPortConfig::SpMode::RS232, 1152009);
    SerialPort rs232("RS232", cfg);
    rs232.Open();
    FreeTimer tmr;
    int sec;
    uint8_t buf[256];
    for (int x = 0; x < 10; x++)
    {
        while (!tmr.IsExpired())
            ;
        int n = rs232.Read(buf, 255);
        buf[n] = '\0';
        printf("\n[%d]Got %d bytes:%s", x, n, buf);
        tmr.SetMs(1000);
    }
    rs232.Close();
}
