/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRY3DENGINE_PARTICLELIST_H
#define CRYINCLUDE_CRY3DENGINE_PARTICLELIST_H
#pragma once

#include "ParticleMemory.h"

////////////////////////////////////////////////////////////////////////
#define for_all_ptrs(Type, p, cont) \
    for (Type* p = (cont).begin(), * p##_next; p && ((p##_next = (cont).next(p)) || true); p = p##_next)

#define for_rev_all_ptrs(Type, p, cont) \
    for (Type* p = (cont).rbegin(), * p##_prev; p && ((p##_prev = (cont).prev(p)) || true); p = p##_prev)

////////////////////////////////////////////////////////////////////////
// Bidirectional list

template< class T>
class ParticleList
{
public:

    typedef T value_type;

    // Derived type with intrusive links after T, to preserve alignment
    struct Node
        : T
    {
        Node*   pNext;
        Node*   pPrev;
    };

    ParticleList()
    { reset(); }

    ~ParticleList()
    { clear(); }

    bool operator +() const
    { return m_pHead != NULL; }
    bool operator !() const
    { return m_pHead == NULL; }
    bool empty() const
    { return m_pHead == NULL; }
    size_t size() const
    { return m_nSize; }

    CONST_VAR_FUNCTION(T & front(),
        { assert(!empty());
          return *m_pHead;
        })
    CONST_VAR_FUNCTION(T & back(),
        { assert(!empty());
          return *m_pTail;
        })
    ILINE void PrefetchLink(const T* link) const
    { PrefetchLine(link, sizeof(T));    }

    //
    // Iteration
    //
    CONST_VAR_FUNCTION(T * begin(),
        { return m_pHead;
        })
    CONST_VAR_FUNCTION(T * end(),
        { return NULL;
        })
    ILINE static T * next(const T * p)
    {
        return p ? get_node(p)->pNext : NULL;
    }

    CONST_VAR_FUNCTION(T * rbegin(),
        { return m_pTail;
        })
    CONST_VAR_FUNCTION(T * rend(),
        { return NULL;
        })
    static T * prev(const T * p)
    {
        return p ? get_node(p)->pPrev : NULL;
    }

    //
    // Add elements
    //
    void* push_back_new()
    {
        Node* pNode = allocate();
        if (pNode)
        {
            insert(NULL, pNode);
        }
        return pNode;
    }
    T* push_back()
    {
        Node* pNode = allocate();
        if (pNode)
        {
            new(pNode) T();
            insert(NULL, pNode);
        }
        return pNode;
    }
    T* push_back(const T& obj)
    {
        Node* pNode = allocate();
        if (pNode)
        {
            new(pNode) T(obj);
            insert(NULL, pNode);
        }
        return pNode;
    }

    void* push_front_new()
    {
        Node* pNode = allocate();
        if (pNode)
        {
            insert(m_pHead, pNode);
        }
        return pNode;
    }
    T* push_front()
    {
        Node* pNode = allocate();
        if (pNode)
        {
            new(pNode) T();
            insert(m_pHead, pNode);
        }
        return pNode;
    }
    T* push_front(const T& obj)
    {
        Node* pNode = allocate();
        if (pNode)
        {
            new(pNode) T(obj);
            insert(m_pHead, pNode);
        }
        return pNode;
    }

    void* insert_new(const T* pNext)
    {
        Node* pNode = allocate();
        if (pNode)
        {
            insert(get_node(pNext), pNode);
        }
        return pNode;
    }
    T* insert(const T* pNext)
    {
        Node* pNode = allocate();
        if (pNode)
        {
            new(pNode) T();
            insert(get_node(pNext), pNode);
        }
        return pNode;
    }
    T* insert(const T* pNext, const T& obj)
    {
        Node* pNode = allocate();
        if (pNode)
        {
            new(pNode) T(obj);
            insert(get_node(pNext), pNode);
        }
        return pNode;
    }

    //
    // Remove elements
    //
    void erase(T* p)
    {
        assert(p);
        assert(!empty());

        Node* pNode = get_node(p);
        remove(pNode);
        destroy(pNode);
        validate();
    }

    void pop_back()
    {
        erase(m_pTail);
    }

    void pop_front()
    {
        erase(m_pHead);
    }

    void clear()
    {
        // Destroy all elements, in reverse order
        while (m_pTail)
        {
            erase(m_pTail);
        }
        reset();
    }

    //
    // Move element.
    //
    void move(const T* pDestObj, const T* pSrcObj)
    {
        assert(pDestObj && pSrcObj && pDestObj != pSrcObj);
        Node* pSrcNode = get_node(pSrcObj);

        remove(pSrcNode);
        insert(get_node(pDestObj), pSrcNode);
    }

    template<class TSizer>
    void GetMemoryUsagePlain(TSizer* pSizer) const
    {
        pSizer->AddObject(this, size() * sizeof(Node));
    }

    template<class TSizer>
    void GetMemoryUsage(TSizer* pSizer) const
    {
        for_all_ptrs (const T, p, *this)
        {
            if (pSizer->AddObject(p, sizeof(Node)))
            {
                p->GetMemoryUsage(pSizer);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////
    class const_iterator
    {
    public:
        const_iterator(ParticleList& list);

        ~const_iterator()
        {
            m_pList->validate();
        }

        operator bool() const
        {
            return MainPointer() != NULL;
        }

        void operator++()
        {
            assert(MainPointer());
            Node* pOldElem = m_pNode;
            m_pNode = m_pNode->pNext;
            FlushLine128(pOldElem, 0);
            if (m_pNode)
            {
                PrefetchLine(m_pNode->pNext, 0);
            }
        }

        const T& operator*() const
        { return *LocalPointer(); }
        const T* operator->() const
        { return LocalPointer(); }
        operator const T*() const
        {
            return MainPointer();
        }

    protected:
        const_iterator(const const_iterator&);
        const_iterator& operator=(const const_iterator&);

        ILINE Node* MainPointer() const
        { return m_pNode; }
        ILINE const Node* LocalPointer() const
        { return m_pNode; }

        Node*                       m_pNode;                // Current element pointer.

        ParticleList*       m_pList;
    };

    ////////////////////////////////////////////////////////////////////////
    class iterator
        : public const_iterator
    {
        // These types are imported to help gcc with type deduction
        using const_iterator::m_pList;
        using const_iterator::LocalPointer;
        using const_iterator::MainPointer;
        using const_iterator::m_pNode;

    public:
        iterator(ParticleList& list)
            : const_iterator(list) {}

        const T& operator*() const
        { return *LocalPointer(); }
        T& operator*()
        { return non_const(*LocalPointer()); }

        const T* operator->() const
        { return LocalPointer(); }
        T* operator->()
        { return &non_const(*LocalPointer()); }

        operator const T*() const
        {
            return MainPointer();
        }
        operator T*()
        {
            return MainPointer();
        }

        void erase();
    };

protected:

    Node*       m_pHead;
    Node*       m_pTail;
    uint32                                  m_nSize;

protected:

    void reset()
    {
        m_pHead = m_pTail = NULL;
        m_nSize = 0;
    }

    static Node* get_node(const T* p)
    { return &non_const(*alias_cast<const Node*>(p)); }

    Node* allocate()
    {
        Node* pNew = (Node*)ParticleObjectAllocator().Allocate(sizeof(Node));
        if (pNew)
        {
            m_nSize++;
        }
        return pNew;
    }
    void destroy(Node* pNode)
    {
        assert(pNode);
        pNode->~Node();
        ParticleObjectAllocator().Deallocate(pNode, sizeof(Node));
        m_nSize--;
    }

    void insert(Node* pNext, Node* pNode)
    {
        assert(pNode);

        Node*& pPrev = pNext ? pNext->pPrev : m_pTail;
        pNode->pPrev = pPrev;
        pNode->pNext = pNext;

        if (pPrev)
        {
            pPrev->pNext = pNode;
        }
        else
        {
            m_pHead = pNode;
        }

        pPrev = pNode;

        validate();
    }

    void remove(Node* pNode)
    {
        assert(pNode);
        assert(!empty());

        if (!pNode->pPrev)
        {
            assert(pNode == m_pHead);
            m_pHead = pNode->pNext;
        }
        else
        {
            pNode->pPrev->pNext = pNode->pNext;
        }

        if (!pNode->pNext)
        {
            assert(pNode == m_pTail);
            m_pTail = pNode->pPrev;
        }
        else
        {
            pNode->pNext->pPrev = pNode->pPrev;
        }
    }

    void validate() const
    {
#if defined(_DEBUG)
        if (empty())
        {
            assert(!m_nSize && !m_pHead && !m_pTail);
        }
        else
        {
            assert(m_pHead && !m_pHead->pPrev);
            assert(m_pTail && !m_pTail->pNext);
            if (m_nSize == 1)
            {
                assert(m_pHead == m_pTail);
            }
        }

        Node* pPrev = NULL;
        int nCount = 0;
        for (Node* p = m_pHead; p; p = p->pNext)
        {
            assert(p->pPrev == pPrev);
            assert(p->pNext || p == m_pTail);
            pPrev = p;
            nCount++;
        }
        assert(nCount == size());
#endif // _DEBUG
    }
};

///////////////////////////////////////////////////////////////////////////////
// iterator implementation

template<typename T>
inline ParticleList<T>::const_iterator::const_iterator(ParticleList& list)
    : m_pList(&list)
{
    m_pList->validate();
    m_pNode = list.m_pHead;
}

template<typename T>
void ParticleList<T>::iterator::erase()
{
    assert(MainPointer());
    Node* pToErase = m_pNode;
    m_pNode = m_pNode->pNext;
    m_pList->erase(pToErase);
}

#endif // CRYINCLUDE_CRY3DENGINE_PARTICLELIST_H
