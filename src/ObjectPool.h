#ifndef __OBJECTPOOL_H__
#define __OBJECTPOOL_H__

template <class Object, int N>
class ObjectPool
{
public:
    /// \brief  Constructor, init pool with nullptr
    ObjectPool():size(0)
    {
        for(int i=0;i<N;i++)
        {
            pool[i]=nullptr;
        }
    }

    /// \brief  Pop an object form pool
    /// \return object pointer; nullptr: no valid object, all used
    Object *Pop()
    {
        if(size<=0)
        {
            return nullptr;
        }
        size--;
        return pool[size];
    }

    /// \brief  Push an object into pool
    /// \param  object pointer
    /// \return int : valid objects in pool; -1 : failed
    int Push(Object *pObj)
    {
        if(size>=N || pObj == nullptr)
        {
            return -1;
        }
        pool[size++] = pObj;
        return size;
    }

    /// \brief  valid objects in pool
    int GetSize() { return size; }

private:
    int size;
    Object * pool[N];
};

#endif
