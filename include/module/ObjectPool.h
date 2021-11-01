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
    ObjectPool(int size) : size(size), unusedCnt(size), pool(size), unusedObj(size)
    {
        for (int i = 0; i < size; i++)
        {
            pool[i] = new T;
            unusedObj[i] = pool[i];
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
        if (unusedCnt > 0)
        {
            unusedCnt--;
            if (std::is_base_of<Poolable<T>, T>::value)
            {
                ((Poolable<T> *)unusedObj[unusedCnt])->PopClean();
            }
            print_status();
            pObj = unusedObj[unusedCnt];
        }
        return pObj;
    }

    /// \brief  Push an object into pool
    /// \param  object pointer
    /// \return int : valid objects in pool; -1 : failed
    int Push(T *pObj)
    {
        if (unusedCnt >= size)
        {
            MyThrow("Pool is full, can't push a new obj in it");
        }
        if (pObj == nullptr)
        {
            MyThrow("pObj is nullptr, cna't push it into the pool");
        }
        if (std::is_base_of<Poolable<T>, T>::value)
        {
            ((Poolable<T> *)pObj)->PushClean();
        }
        unusedObj[unusedCnt++] = pObj;
        print_status();
        return unusedCnt;
    }

    std::vector<T *> &Pool() { return pool; };
    int Size() { return size; }
    int Unused() { return unusedCnt; }
    int Used() { return size - unusedCnt; }

private:
    /// \brief  pool size
    int size;
    /// \brief  unusedObj object counter
    int unusedCnt;

    std::vector<T *> pool;      // hold all new object
    std::vector<T *> unusedObj; // pop/push object pointer

    void print_status()
    {
        printf("Pool status: size:%d, used:%d, unused:%d\n", size, size - unusedCnt, unusedCnt);
    }
};
