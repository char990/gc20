#ifndef __OBJECTPOOL_H__
#define __OBJECTPOOL_H__

template <class Object>
class ObjectPool
{
public:
    ObjectPool(int size):size(size),cnt(size)
    {
        pool = new Object [size];
        unused = new Object * [size];
        for(int i=0;i<size;i++)
        {
            unused[i]=&pool[i];
        }
    }

    ~ObjectPool()
    {
        delete [] pool;
        delete [] unused;
    }

    /// \brief  Pop an object form pool
    /// \return object pointer; nullptr: no valid object, all used
    Object * Pop()
    {
        if(cnt<=0)
        {
            return nullptr;
        }
        cnt--;
        return &unused[cnt];
    }

    /// \brief  Push an object into pool
    /// \param  object pointer
    /// \return int : valid objects in pool; -1 : failed
    int Push(Object *pObj)
    {
        if(cnt>=size || pObj == nullptr)
        {
            return -1;
        }
        unused[cnt] = pObj;
        cnt++;
        return cnt;
    }

    Object * Pool() { return pool; }
    int Size() { return size; }
    int Cnt() { return cnt; }

private:
    /// \brief  pool size
    int size;
    /// \brief  unused object counter
    int cnt;
    Object * pool;
    Object ** unused;
};

#endif
