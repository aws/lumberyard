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

#include "StdAfx.h"
#include "DynamicPopupMenu.h"
#include <algorithm>

CDynamicPopupMenu::CDynamicPopupMenu()
{
}


CPopupMenuItem* CDynamicPopupMenu::NextItem(CPopupMenuItem* item) const
{
    if (!item->GetChildren().empty())
    {
        return item->GetChildren().front();
    }
    else
    {
        if (item->GetParent() == 0)
        {
            return 0;
        }
        CPopupMenuItem0::Children& items = item->GetParent()->GetChildren();
        CPopupMenuItem0::Children::iterator it = std::find(items.begin(), items.end(), item);
        assert(it != items.end());
        while (item->GetParent() && *it == item->GetParent()->GetChildren().back())
        {
            item = item->GetParent();
            if (item->GetParent())
            {
                it = std::find(item->GetParent()->GetChildren().begin(), item->GetParent()->GetChildren().end(), item);
            }
        }
        if (item->GetParent())
        {
            ++it;
            item = *it;
            return item;
        }
        else
        {
            return 0;
        }
    }
}

void CDynamicPopupMenu::AssignIDs()
{
    DWORD currentID = 65535;
    CPopupMenuItem* current = &m_root;
    while (current)
    {
        if (current->GetChildren().empty())
        {
            current->m_id = currentID--;
        }
        else
        {
            current->m_id = 0;
        }

        current = NextItem(current);
    }
}

void CDynamicPopupMenu::Clear()
{
    m_root.GetChildren().clear();
}

void CDynamicPopupMenu::Spawn(HWND parent)
{
    POINT pt;
    GetCursorPos(&pt);
    Spawn(pt.x, pt.y, parent);
}

void CDynamicPopupMenu::Spawn(int x, int y, HWND parent)
{
    if (m_root.GetChildren().empty())
    {
        return;
    }

    AssignIDs();
    m_root.SetMenuHandle(::CreatePopupMenu());

    CPopupMenuItem* current = &m_root;
    while (current = NextItem(current))
    {
        if (current->GetChildren().empty())
        {
            UINT_PTR id = current->MenuID();
            string text = current->Text();
            if (text == "-")
            {
                AppendMenu(current->GetParent()->MenuHandle(), MF_SEPARATOR, id, "");
            }
            else
            {
                /*
                if (current->hotkey() != KeyPress()){
                    text += "\t";
                    text += current->hotkey().toString(true).c_str();
                }
                */
                AppendMenu(current->GetParent()->MenuHandle(),
                    MF_STRING | (current->IsChecked() ? MF_CHECKED : 0)
                    | (current->IsEnabled() ? 0 : MF_GRAYED)
                    | (current->IsDefault() ? MF_DEFAULT : 0),
                    id, text.c_str());
            }
        }
        else
        {
            HMENU handle = ::CreatePopupMenu();
            current->SetMenuHandle(handle);
            BOOL result = ::InsertMenu(current->GetParent()->MenuHandle(), -1, MF_BYPOSITION | MF_POPUP, UINT_PTR(handle), current->Text());
            assert(result == TRUE);
        }
    }

    current = &m_root;
    while (current = NextItem(current))
    {
        if (!current->GetChildren().empty())
        {
            UINT_PTR handle = UINT_PTR(current->MenuHandle());
        }
    }

    UINT id = ::TrackPopupMenu(m_root.MenuHandle(), TPM_LEFTBUTTON | TPM_LEFTALIGN | TPM_RETURNCMD, x, y, 0, parent, 0);

    if (m_root.GetChildren().empty())
    {
        return;
    }

    current = &m_root;
    while (current = NextItem(current))
    {
        if (current->GetChildren().empty())
        {
            UINT current_id = current->MenuID();
            if (current_id == id)
            {
                current->Call();
            }
        }
    }
}

// ---------------------------------------------------------------------------

CPopupMenuItem::~CPopupMenuItem()
{
    if (m_id && !m_children.empty())
    {
        ::DestroyMenu(MenuHandle());
    }
}


CPopupMenuItem0& CPopupMenuItem::Add(const char* text)
{
    CPopupMenuItem0* item = new CPopupMenuItem0(text, Functor0());
    AddChildren(item);
    return *item;
}

CPopupMenuItem0& CPopupMenuItem::Add(const char* text, const Functor0& functor)
{
    CPopupMenuItem0* item = new CPopupMenuItem0(text, functor);
    AddChildren(item);
    return *item;
}

/*
CPopupMenuItem& CPopupMenuItem::SetHotkey(KeyPress key)
{
    m_hotkey = key;
    return *this;
}
*/

CPopupMenuItem& CPopupMenuItem::AddSeparator()
{
    return Add("-");
}

CPopupMenuItem* CPopupMenuItem::Find(const char* text)
{
    for (Children::iterator it = m_children.begin(); it != m_children.end(); ++it)
    {
        CPopupMenuItem* item = *it;
        if (strcmp(item->Text(), text) == 0)
        {
            return item;
        }
    }
    return 0;
}
