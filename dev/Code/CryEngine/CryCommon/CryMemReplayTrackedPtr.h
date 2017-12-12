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

#ifndef CRYINCLUDE_CRYCOMMON_CRYMEMREPLAYTRACKEDPTR_H
#define CRYINCLUDE_CRYCOMMON_CRYMEMREPLAYTRACKEDPTR_H
#pragma once


#if CAPTURE_REPLAY_LOG

template <class _I>
class MemReplayTrackedSmartPtr
{
private:
    _I* p;
public:
    MemReplayTrackedSmartPtr()
        : p(NULL) {}
    MemReplayTrackedSmartPtr(int Null)
        : p(NULL) {}
    MemReplayTrackedSmartPtr(_I* p_)
    {
        p = p_;
        if (p)
        {
            p->AddRef();
            CryGetIMemReplay()->AddAllocReference(p, this);
        }
    }

    MemReplayTrackedSmartPtr(const MemReplayTrackedSmartPtr& p_)
    {
        p = p_.p;
        if (p)
        {
            p->AddRef();
            CryGetIMemReplay()->AddAllocReference(p, this);
        }
    }
    ~MemReplayTrackedSmartPtr()
    {
        if (p)
        {
            CryGetIMemReplay()->RemoveAllocReference(this);
            p->Release();
        }
    }
    operator _I*() const {
        return p;
    }
    operator const _I*() const {
        return p;
    }
    _I& operator*() const { return *p; }
    _I* operator->(void) const { return p; }
    _I* get() const { return p; }
    MemReplayTrackedSmartPtr&  operator=(_I* newp)
    {
        if (p)
        {
            CryGetIMemReplay()->RemoveAllocReference(this);
        }
        if (newp)
        {
            newp->AddRef();
        }
        if (p)
        {
            p->Release();
        }
        if (newp)
        {
            CryGetIMemReplay()->AddAllocReference(newp, this);
        }
        p = newp;
        return *this;
    }

    void reset()
    {
        MemReplayTrackedSmartPtr<_I>().swap(*this);
    }

    void reset(_I* p)
    {
        MemReplayTrackedSmartPtr<_I>(p).swap(*this);
    }

    MemReplayTrackedSmartPtr&  operator=(const MemReplayTrackedSmartPtr& newp)
    {
        if (p)
        {
            CryGetIMemReplay()->RemoveAllocReference(this);
        }
        if (newp.p)
        {
            newp.p->AddRef();
        }
        if (p)
        {
            p->Release();
        }
        if (newp.p)
        {
            CryGetIMemReplay()->AddAllocReference(newp.p, this);
        }
        p = newp.p;
        return *this;
    }
    operator bool() const
    {
        return p != NULL;
    };
    bool operator !() const
    {
        return p == NULL;
    };
    bool  operator ==(const _I* p2) const
    {
        return p == p2;
    };
    bool  operator ==(_I* p2) const
    {
        return p == p2;
    };
    bool  operator ==(const MemReplayTrackedSmartPtr<_I>& rhs) const
    {
        return p == rhs.p;
    }
    bool  operator !=(const _I* p2) const
    {
        return p != p2;
    };
    bool  operator !=(_I* p2) const
    {
        return p != p2;
    };
    bool  operator !=(const MemReplayTrackedSmartPtr& p2) const
    {
        return p != p2.p;
    };
    bool  operator <(const _I* p2) const
    {
        return p < p2;
    };
    bool  operator >(const _I* p2) const
    {
        return p > p2;
    };
    // set our contained pointer to null, but don't call Release()
    // useful for when we want to do that ourselves, or when we know that
    // the contained pointer is invalid
    friend _I* ReleaseOwnership(MemReplayTrackedSmartPtr<_I>& ptr)
    {
        _I* ret = ptr.p;
        if (ret)
        {
            CryGetIMemReplay()->RemoveAllocReference(&ptr);
        }
        ptr.p = 0;
        return ret;
    }
    void swap(MemReplayTrackedSmartPtr<_I>& other)
    {
        std::swap(*this, other);
    }

    AUTO_STRUCT_INFO
};

template <typename T>
ILINE void swap(MemReplayTrackedSmartPtr<T>& a, MemReplayTrackedSmartPtr<T>& b)
{
    a.swap(b);
}

template <class _I>
inline bool operator ==(const MemReplayTrackedSmartPtr<_I>& p1, int null)
{
    assert(!null);
    return !(bool)p1;
}
template <class _I>
inline bool operator !=(const MemReplayTrackedSmartPtr<_I>& p1, int null)
{
    assert(!null);
    return (bool)p1;
}
template <class _I>
inline bool operator ==(int null, const MemReplayTrackedSmartPtr<_I>& p1)
{
    assert(!null);
    return !(bool)p1;
}
template <class _I>
inline bool operator !=(int null, const MemReplayTrackedSmartPtr<_I>& p1)
{
    assert(!null);
    return (bool)p1;
}

template <typename T>
class MemReplayTracingPointer
{
public:
    MemReplayTracingPointer() {}

    MemReplayTracingPointer(T* ptr)
        : m_ptr(ptr)
    {
        CryGetIMemReplay()->AddAllocReference(ptr, this);
    }

    MemReplayTracingPointer(const MemReplayTracingPointer<T>& other)
        : m_ptr(other.m_ptr)
    {
        CryGetIMemReplay()->AddAllocReference(other.m_ptr, this);
    }

    MemReplayTracingPointer<T>& operator = (const MemReplayTracingPointer<T>& other)
    {
        CryGetIMemReplay()->AddAllocReference(m_ptr, this);
        m_ptr = other.m_ptr;
        CryGetIMemReplay()->AddAllocReference(other.m_ptr, this);
        return *this;
    }

    template <typename O>
    MemReplayTracingPointer(O* ptr)
        : m_ptr(ptr)
    {
        CryGetIMemReplay()->AddAllocReference(ptr, this);
    }

    template <typename O>
    MemReplayTracingPointer<T>& operator = (const MemReplayTracingPointer<O>& other)
    {
        CryGetIMemReplay()->AddAllocReference(m_ptr, this);
        m_ptr = other.m_ptr;
        CryGetIMemReplay()->AddAllocReference(other.m_ptr, this);
        return *this;
    }

    ~MemReplayTracingPointer()
    {
        CryGetIMemReplay()->AddAllocReference(m_ptr, this);
    }

    T& operator * () const { return *m_ptr; }
    T* operator -> () const { return m_ptr; }

    operator bool () const {
        return m_ptr != 0;
    }
    operator T* () const {
        return m_ptr;
    }

private:
    T* m_ptr;
};

#else

#define MemReplayTrackedSmartPtr _smart_ptr

#endif

#endif // CRYINCLUDE_CRYCOMMON_CRYMEMREPLAYTRACKEDPTR_H
