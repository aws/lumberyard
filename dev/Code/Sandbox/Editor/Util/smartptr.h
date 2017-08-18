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

#ifndef CRYINCLUDE_EDITOR_UTIL_SMARTPTR_H
#define CRYINCLUDE_EDITOR_UTIL_SMARTPTR_H
#pragma once


#if defined(WIN64) || defined(CRYINCLUDE_CRYCOMMON_SMARTPTR_H)

#define TSmartPtr _smart_ptr

#elif !defined(TSmartPtr)

///////////////////////////////////////////////////////////////////////////////
//
// Class TSmartPtr<T>.
// Smart pointer warper class that implements reference counting on IUnknown
// interface compatible class.
//
///////////////////////////////////////////////////////////////////////////////
template <class Type>
class TSmartPtr
{
    Type* p;
public:
    TSmartPtr()
        : p(NULL) {}
    TSmartPtr(Type* p_)
        : p(p_)
    {
        if (p)
        {
            (p)->AddRef();
        }
    }
    TSmartPtr(const TSmartPtr<Type>& p_)
        : p(p_.p)
    {
        if (p)
        {
            (p)->AddRef();
        }
    }                                                                         // Copy constructor.
    TSmartPtr(int Null)
        : p(NULL) {}
    ~TSmartPtr()
    {
        if (p)
        {
            (p)->Release();
        }
    }

    operator Type*() const {
        return p;
    }
    operator const Type*() const {
        return p;
    }
    Type& operator*() const { return *p; }
    Type* operator->(void) const { return p; }

    TSmartPtr&  operator=(Type* newp)
    {
        if (newp)
        {
            (newp)->AddRef();
        }
        if (p)
        {
            (p)->Release();
        }
        p = newp;
        return *this;
    }

    TSmartPtr&  operator=(const TSmartPtr<Type>& newp)
    {
        if (newp.p)
        {
            (newp.p)->AddRef();
        }
        if (p)
        {
            (p)->Release();
        }
        p = newp.p;
        return *this;
    }

    //Type* ptr() const { return p; };

    //operator bool() { return p != NULL; };
    operator bool() const {
        return p != NULL;
    };
    //bool  operator !() { return p == NULL; };
    bool  operator !() const { return p == NULL; };

    // Misc compare functions.
    bool  operator == (const Type* p2) const { return p == p2; };
    bool  operator != (const Type* p2) const { return p != p2; };
    bool  operator <  (const Type* p2) const { return p < p2; };
    bool  operator >  (const Type* p2) const { return p > p2; };

    bool operator == (const TSmartPtr<Type>& p2) const { return p == p2.p; };
    bool operator != (const TSmartPtr<Type>& p2) const { return p != p2.p; };
    bool operator < (const TSmartPtr<Type>& p2) const { return p < p2.p; };
    bool operator > (const TSmartPtr<Type>& p2) const { return p > p2.p; };

    friend bool operator == (const TSmartPtr<Type>& p1, int null);
    friend bool operator != (const TSmartPtr<Type>& p1, int null);
    friend bool operator == (int null, const TSmartPtr<Type>& p1);
    friend bool operator != (int null, const TSmartPtr<Type>& p1);
};

template <class T>
inline bool operator == (const TSmartPtr<T>& p1, int null)
{
    return p1.p == 0;
}

template <class T>
inline bool operator != (const TSmartPtr<T>& p1, int null)
{
    return p1.p != 0;
}

template <class T>
inline bool operator == (int null, const TSmartPtr<T>& p1)
{
    return p1.p == 0;
}

template <class T>
inline bool operator != (int null, const TSmartPtr<T>& p1)
{
    return p1.p != 0;
}
#endif //WIN64

/** Use this to define smart pointers of classes.
        For example:
        class CNode : public CRefCountBase {};
        SMARTPTRTypeYPEDEF( CNode );
        {
            CNodePtr node; // Smart pointer.
        }
*/

#define SMARTPTR_TYPEDEF(Class) typedef TSmartPtr<Class> Class##Ptr

#endif // CRYINCLUDE_EDITOR_UTIL_SMARTPTR_H
