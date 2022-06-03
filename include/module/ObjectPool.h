#pragma once
#include <vector>

template <class T>
class ObjectPool;

template <class T>
class Poolable
{
public:
    ObjectPool<T> *ParentPool()
    {
        return mOwner;
    }

    virtual void PopClean() = 0;
    virtual void PushClean() = 0;

private:
    friend class ObjectPool<T>;
    ObjectPool<T> *mOwner{nullptr};
};

template <class T>
class ObjectPool
{
public:
    ObjectPool(int size) : size(size), freeCnt(size), pool(size), freeObj(size), busyObj(size)
    {
        for (int i = 0; i < size; i++)
        {
            pool[i] = new T;
            freeObj[i] = pool[i];
            busyObj[i] = nullptr;
            if (std::is_base_of<Poolable<T>, T>::value)
            {
                ((Poolable<T> *)pool[i])->mOwner = this;
            }
        }
    }

    ~ObjectPool()
    {
        for (auto &s : pool)
        {
            delete s;
        }
    }

    /// \brief  Pop an object form pool
    /// \return object pointer; nullptr: no valid object, all used
    T *Pop()
    {
        T *pObj = nullptr;
        if (freeCnt > 0)
        {
            freeCnt--;
            if (std::is_base_of<Poolable<T>, T>::value)
            {
                ((Poolable<T> *)freeObj[freeCnt])->PopClean();
            }
            print_status();
            pObj = freeObj[freeCnt];
            for(int i=0;i<size;i++)
            {
                if(busyObj[i]==nullptr)
                {
                    busyObj[i]=pObj;
                    break;
                }
            }
        }
        return pObj;
    }

    /// \brief  Push an object into pool
    /// \param  object pointer
    /// \return int : valid objects in pool; -1 : failed
    int Push(T *pObj)
    {
        if (freeCnt >= size)
        {
            throw std::overflow_error("Pool is full, can't push a new obj in it");
        }
        if (pObj == nullptr)
        {
            throw std::runtime_error("pObj is nullptr, can't push it into the pool");
        }
        if (std::is_base_of<Poolable<T>, T>::value)
        {
            ((Poolable<T> *)pObj)->PushClean();
        }
        freeObj[freeCnt++] = pObj;
        for(int i=0;i<size;i++)
        {
            if(busyObj[i]==pObj)
            {
                busyObj[i]=nullptr;
                break;
            }
        }
        print_status();
        return freeCnt;
    }

    std::vector<T *> &Pool() { return pool; };
    std::vector<T *> &FreeObj() { return freeObj; };
    std::vector<T *> &BusyObj() { return busyObj; };
    int Size() { return size; }
    int FreeCnt() { return freeCnt; }
    int BusyCnt() { return size-freeCnt; }

private:
    /// \brief  pool size
    int size;
    /// \brief  free objects counter
    int freeCnt;

    std::vector<T *> pool;      // hold all new objects
    std::vector<T *> freeObj;      // free objects
    std::vector<T *> busyObj;      // busy objects

    void print_status()
    {
        Pdebug("Pool status: size:%d, busy:%d, free:%d", size, size-freeCnt, freeCnt);
    }
};
