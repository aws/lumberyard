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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_DYNAMICPOPUPMENU_H
#define CRYINCLUDE_EDITOR_CONTROLS_DYNAMICPOPUPMENU_H
#pragma once

class CDynamicPopupMenu;
class CPopupMenuItem0;
template<class Arg1>
class CPopupMenuItem1;
template<class Arg1, class Arg2>
class CPopupMenuItem2;
template<class Arg1, class Arg2, class Arg3>
class CPopupMenuItem3;
template<class Arg1, class Arg2, class Arg3, class Arg4>
class CPopupMenuItem4;

class SANDBOX_API CPopupMenuItem
    : public _reference_target<int>
{
public:
    friend CDynamicPopupMenu;
    typedef std::vector<_smart_ptr<CPopupMenuItem> > Children;

    CPopupMenuItem(const char* text = "")
        : m_id(0)
        , m_parent(0)
        , m_text(text)
        , m_checked(false)
        , m_enabled(true)
        , m_default(false)
    {}

    virtual ~CPopupMenuItem();

    CPopupMenuItem& Check(bool checked = true){ m_checked = checked; return *this; }
    bool IsChecked() const{ return m_checked; }

    CPopupMenuItem& Enable(bool enabled = true){ m_enabled = enabled; return *this; }
    bool IsEnabled() const{ return m_enabled; }

    // CPopupMenuItem& SetHotkey(KeyPress key);
    // KeyPress Hotkey() const{ return m_hotkey; }

    void SetDefault(bool defaultItem = true){ m_default = defaultItem; }
    bool IsDefault() const{ return m_default; }

    const char* Text() { return m_text.c_str(); }
    CPopupMenuItem& AddSeparator();
    CPopupMenuItem0& Add(const char* text);

    CPopupMenuItem0& Add(const char* text, const Functor0& functor);

    template<class Arg1>
    CPopupMenuItem1<Arg1>& Add(const char* text, const Functor1<Arg1>& functor, const Arg1& arg1);

    template<class Arg1, class Arg2>
    CPopupMenuItem2<Arg1, Arg2>& Add(const char* text, const Functor2<Arg1, Arg2>& functor, const Arg1& arg1, const Arg2& arg2);

    template<class Arg1, class Arg2, class Arg3>
    CPopupMenuItem3<Arg1, Arg2, Arg3>& Add(const char* text, const Functor3<Arg1, Arg2, Arg3>& functor, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3);

    template<class Arg1, class Arg2, class Arg3, class Arg4>
    CPopupMenuItem4<Arg1, Arg2, Arg3, Arg4>& Add(const char* text, const Functor4<Arg1, Arg2, Arg3, Arg4>& functor, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4);

    CPopupMenuItem* Find(const char* text);

    CPopupMenuItem* GetParent() { return m_parent; }
    Children& GetChildren() { return m_children; };
    const Children& GetChildren() const { return m_children; }
    bool Empty() const { return m_children.empty(); }
private:
    void AddChildren(CPopupMenuItem* item)
    {
        m_children.reserve(128);
        m_children.push_back(item);
        item->m_parent = this;
    }

    HMENU MenuHandle() const
    {
        assert(!m_children.empty());
        return HMENU(m_id);
    }
    void SetMenuHandle(HMENU menu)
    {
        assert(!m_children.empty());
        m_id = (unsigned int)(menu);
    }
    unsigned int MenuID() const
    {
        assert(m_children.empty());
        return m_id;
    }
    void SetMenuID(unsigned int id)
    {
        assert(m_children.empty());
        m_id = id;
    }

    virtual void Call() = 0;

    unsigned int m_id;
    bool m_default;
    bool m_checked;
    bool m_enabled;
    string m_text;
    CPopupMenuItem* m_parent;
    Children m_children;
    //KeyPress m_hotkey;
};

class CPopupMenuItem0
    : public CPopupMenuItem
{
public:
    CPopupMenuItem0(const char* text = "", const Functor0& functor = Functor0())
        : CPopupMenuItem(text)
        , m_functor(functor)
    {}

    void Call()
    {
        if (m_functor)
        {
            m_functor();
        }
    }

protected:
    Functor0 m_functor;
};


template<class Arg1>
class CPopupMenuItem1
    : public CPopupMenuItem
{
public:
    CPopupMenuItem1(const char* text, const Functor1<Arg1>& functor, const Arg1& arg1)
        : CPopupMenuItem(text)
        , m_functor(functor)
        , m_arg1(arg1)
    {}

    virtual void Call()
    {
        if (m_functor)
        {
            m_functor(m_arg1);
        }
    }

protected:
    Functor1<Arg1> m_functor;
    Arg1 m_arg1;
};

template<class Arg1, class Arg2>
class CPopupMenuItem2
    : public CPopupMenuItem
{
public:
    CPopupMenuItem2(const char* text, const Functor2<Arg1, Arg2>& functor, const Arg1& arg1, const Arg2& arg2)
        : CPopupMenuItem(text)
        , m_functor(functor)
        , m_arg1(arg1)
        , m_arg2(arg2)
    {}

    virtual void Call()
    {
        if (m_functor)
        {
            m_functor(m_arg1, m_arg2);
        }
    }

protected:
    Functor2<Arg1, Arg2> m_functor;
    Arg1 m_arg1;
    Arg2 m_arg2;
};

template<class Arg1, class Arg2, class Arg3>
class CPopupMenuItem3
    : public CPopupMenuItem
{
public:
    CPopupMenuItem3(const char* text, const Functor3<Arg1, Arg2, Arg3>& functor, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
        : CPopupMenuItem(text)
        , m_functor(functor)
        , m_arg1(arg1)
        , m_arg2(arg2)
        , m_arg3(arg3)
    {}

    virtual void Call()
    {
        if (m_functor)
        {
            m_functor(m_arg1, m_arg2, m_arg3);
        }
    }

protected:
    Functor3<Arg1, Arg2, Arg3> m_functor;
    Arg1 m_arg1;
    Arg2 m_arg2;
    Arg3 m_arg3;
};

template<class Arg1, class Arg2, class Arg3, class Arg4>
class CPopupMenuItem4
    : public CPopupMenuItem
{
public:
    CPopupMenuItem4(const char* text, const Functor4<Arg1, Arg2, Arg3, Arg4>& functor, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4)
        : CPopupMenuItem(text)
        , m_functor(functor)
        , m_arg1(arg1)
        , m_arg2(arg2)
        , m_arg3(arg3)
        , m_arg4(arg4)
    {}

    virtual void Call()
    {
        if (m_functor)
        {
            m_functor(m_arg1, m_arg2, m_arg3, m_arg4);
        }
    }

protected:
    Functor4<Arg1, Arg2, Arg3, Arg4> m_functor;
    Arg1 m_arg1;
    Arg2 m_arg2;
    Arg3 m_arg3;
    Arg4 m_arg4;
};

template<class Arg1>
CPopupMenuItem1<Arg1>& CPopupMenuItem::Add(const char* text, const Functor1<Arg1>& functor, const Arg1& arg1)
{
    CPopupMenuItem1<Arg1>* item = new CPopupMenuItem1<Arg1>(text, functor, arg1);
    AddChildren(item);
    return *item;
}

template<class Arg1, class Arg2>
CPopupMenuItem2<Arg1, Arg2>& CPopupMenuItem::Add(const char* text, const Functor2<Arg1, Arg2>& functor, const Arg1& arg1, const Arg2& arg2)
{
    CPopupMenuItem2<Arg1, Arg2>* item = new CPopupMenuItem2<Arg1, Arg2>(text, functor, arg1, arg2);
    AddChildren(item);
    return *item;
}

template<class Arg1, class Arg2, class Arg3>
CPopupMenuItem3<Arg1, Arg2, Arg3>& CPopupMenuItem::Add(const char* text, const Functor3<Arg1, Arg2, Arg3>& functor, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
{
    CPopupMenuItem3<Arg1, Arg2, Arg3>* item = new CPopupMenuItem3<Arg1, Arg2, Arg3>(text, functor, arg1, arg2, arg3);
    AddChildren(item);
    return *item;
}

template<class Arg1, class Arg2, class Arg3, class Arg4>
CPopupMenuItem4<Arg1, Arg2, Arg3, Arg4>& CPopupMenuItem::Add(const char* text, const Functor4<Arg1, Arg2, Arg3, Arg4>& functor, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4)
{
    CPopupMenuItem4<Arg1, Arg2, Arg3, Arg4>* item = new CPopupMenuItem4<Arg1, Arg2, Arg3, Arg4>(text, functor, arg1, arg2, arg3, arg4);
    AddChildren(item);
    return *item;
}

class CDynamicPopupMenu
{
public:
    CDynamicPopupMenu();
    CPopupMenuItem0& GetRoot() { return m_root; };
    const CPopupMenuItem0& GetRoot() const { return m_root; };

    void Spawn(HWND atWindow);
    void Spawn(int x, int y, HWND parent);
    void Clear();

private:
    CPopupMenuItem* NextItem(CPopupMenuItem* item) const;
    CPopupMenuItem0 m_root;

    void AssignIDs();
};


#endif // CRYINCLUDE_EDITOR_CONTROLS_DYNAMICPOPUPMENU_H
