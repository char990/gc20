#ifndef __DBHELPER_H__
#define __DBHELPER_H__

class DbHelper
{
private:
    DbHelper() {}

public:
    DbHelper(DbHelper const &) = delete;
    void operator=(DbHelper const &) = delete;
    static DbHelper &Instance()
    {
        static DbHelper instance;
        return instance;
    }

    void Init();
};

#endif
