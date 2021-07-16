#ifndef __DBHELPER_H__
#define __DBHELPER_H__

class DbHelper
{
private:
    DbHelper() {} // Constructor? (the {} brackets) are needed here.

public:
    DbHelper(DbHelper const &) = delete;
    void operator=(DbHelper const &) = delete;
    static DbHelper &Instance()
    {
        static DbHelper instance; // Guaranteed to be destroyed. Instantiated on first use.
        return instance;
    }

    void Init();
};

#endif
