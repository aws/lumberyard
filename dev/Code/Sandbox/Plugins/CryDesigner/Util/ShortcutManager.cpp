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
#include "ShortcutManager.h"
#include "Serialization/Enum.h"
#include "MainWindow.h"
#include "KeyboardCustomizationSettings.h"
#include "Core/BrushHelper.h"
#include "Tools/ToolCommon.h"

using namespace CD;

namespace CD
{
    bool HasShortcutRegistered(const SShortCutItem& shortcut)
    {
        static const std::unordered_map<ESecondKey, Qt::Key> s_secondKeys =
        {
            { ESKey_Tab, Qt::Key_Tab },
            { ESKey_Return, Qt::Key_Return },
            { ESKey_Backspace, Qt::Key_Backspace },
            { ESKey_Space, Qt::Key_Space },
            { ESKey_PageUp, Qt::Key_PageUp },
            { ESKey_PageDown, Qt::Key_PageDown },
            { ESKey_End, Qt::Key_End },
            { ESKey_Home, Qt::Key_Home },
            { ESKey_Left, Qt::Key_Left },
            { ESKey_Up, Qt::Key_Up },
            { ESKey_Right, Qt::Key_Right },
            { ESKey_Down, Qt::Key_Down },
            { ESKey_Insert, Qt::Key_Insert },
            { ESKey_Delete, Qt::Key_Delete },

            { ESKey_0, Qt::Key_0 },
            { ESKey_1, Qt::Key_1 },
            { ESKey_2, Qt::Key_2 },
            { ESKey_3, Qt::Key_3 },
            { ESKey_4, Qt::Key_4 },
            { ESKey_5, Qt::Key_5 },
            { ESKey_6, Qt::Key_6 },
            { ESKey_7, Qt::Key_7 },
            { ESKey_8, Qt::Key_8 },
            { ESKey_9, Qt::Key_9 },

            { ESKey_A, Qt::Key_A },
            { ESKey_B, Qt::Key_B },
            { ESKey_C, Qt::Key_C },
            { ESKey_D, Qt::Key_D },
            { ESKey_E, Qt::Key_E },
            { ESKey_F, Qt::Key_F },
            { ESKey_G, Qt::Key_G },
            { ESKey_H, Qt::Key_H },
            { ESKey_I, Qt::Key_I },
            { ESKey_J, Qt::Key_J },
            { ESKey_K, Qt::Key_K },
            { ESKey_L, Qt::Key_L },
            { ESKey_M, Qt::Key_M },
            { ESKey_N, Qt::Key_N },
            { ESKey_O, Qt::Key_O },
            { ESKey_P, Qt::Key_P },
            { ESKey_Q, Qt::Key_Q },
            { ESKey_R, Qt::Key_R },
            { ESKey_S, Qt::Key_S },
            { ESKey_T, Qt::Key_T },
            { ESKey_U, Qt::Key_U },
            { ESKey_V, Qt::Key_V },
            { ESKey_W, Qt::Key_W },
            { ESKey_X, Qt::Key_X },
            { ESKey_Y, Qt::Key_Y },
            { ESKey_Z, Qt::Key_Z },

            { ESKey_F1, Qt::Key_F1 },
            { ESKey_F2, Qt::Key_F2 },
            { ESKey_F3, Qt::Key_F3 },
            { ESKey_F4, Qt::Key_F4 },
            { ESKey_F5, Qt::Key_F5 },
            { ESKey_F6, Qt::Key_F6 },
            { ESKey_F7, Qt::Key_F7 },
            { ESKey_F8, Qt::Key_F8 },
            { ESKey_F9, Qt::Key_F9 },
            { ESKey_F10, Qt::Key_F10 },
            { ESKey_F11, Qt::Key_F11 },
            { ESKey_F12, Qt::Key_F12 },
        };

        auto pShortcutMgr = MainWindow::instance()->GetShortcutManager();
        if( !pShortcutMgr )
            return false;

        int key = 0;

        switch (shortcut.firstKey)
        {
        case EFKey_Ctrl:
            key = Qt::CTRL;
            break;
        case EFKey_Shift:
            key = Qt::SHIFT;
            break;
        case EFKey_CtrlShift:
            key = Qt::CTRL + Qt::SHIFT;
            break;
        }

        auto it = s_secondKeys.find(shortcut.secondKey);
        if (it != std::end(s_secondKeys))
        {
            key += it->second;
        }

        return pShortcutMgr->FindActionForShortcut(QKeySequence(key)) != nullptr;
    }
}

ShortcutManager::ShortcutManager()
{
    Load();
}

void ShortcutManager::Save()
{
    CD::SaveSettings(Serialization::SStruct(*this), "Shortcuts");
}

void ShortcutManager::Load()
{
    m_ShortCutItems.clear();

    if (CD::LoadSettings(Serialization::SStruct(*this), "Shortcuts"))
    {
        int toolCount = Serialization::getEnumDescription<CD::EDesignerTool>().count();
        int shortcutCount = m_ShortCutItems.size();

        for (int i = 0; i < shortcutCount; ++i)
        {
            if (m_ShortCutItems[i].function != CD::eDesigner_Invalid)
            {
                continue;
            }
            int k = 0;
            for (k = 0; k < toolCount; ++k)
            {
                int tool = Serialization::getEnumDescription<CD::EDesignerTool>().valueByIndex(k);
                if (!HasFunction(tool))
                {
                    break;
                }
            }
            if (k < toolCount)
            {
                m_ShortCutItems[i].function = (CD::EDesignerTool)Serialization::getEnumDescription<CD::EDesignerTool>().valueByIndex(k);
            }
        }
    }
    else
    {
        for (int i = 0, iCount(Serialization::getEnumDescription<CD::EDesignerTool>().count()); i < iCount; ++i)
        {
            EDesignerTool tool = (EDesignerTool)Serialization::getEnumDescription<CD::EDesignerTool>().valueByIndex(i);
            if (tool == eDesigner_Invalid)
            {
                continue;
            }
            Add(SShortCutItem(tool, EFKey_None, ESKey_None));
        }

        SetShortcut(SShortCutItem(eDesigner_Vertex, EFKey_None, ESKey_Insert));
        SetShortcut(SShortCutItem(eDesigner_Edge, EFKey_None, ESKey_Home));
        SetShortcut(SShortCutItem(eDesigner_Face, EFKey_None, ESKey_PageUp));

        SetShortcut(SShortCutItem(eDesigner_Box, EFKey_Shift, ESKey_B));
        SetShortcut(SShortCutItem(eDesigner_AllNone, EFKey_Ctrl, ESKey_Q));
        SetShortcut(SShortCutItem(eDesigner_Extrude, EFKey_CtrlShift, ESKey_E));
        SetShortcut(SShortCutItem(eDesigner_Remove, EFKey_None, ESKey_Delete));
        SetShortcut(SShortCutItem(eDesigner_Pivot2Bottom, EFKey_None, ESKey_Tab));

        Save();
    }

    CheckDuplicatedShortcuts();
}

void ShortcutManager::Serialize(Serialization::IArchive& ar)
{
    if (ar.IsEdit())
    {
        const auto& desc = Serialization::getEnumDescription<EDesignerTool>();
        for (size_t i = 0; i < m_ShortCutItems.size(); ++i)
        {
            ar(m_ShortCutItems[i], "Shortcut", desc.name(m_ShortCutItems[i].function));
        }
    }
    else
    {
        ar(m_ShortCutItems, "Shortcuts", "Shortcuts");
    }
}

bool ShortcutManager::Process(uint32 nChar)
{
    bool bPressCTRL = GetKeyState(VK_CONTROL) & (1 << 15);
    bool bPressSHIFT = GetKeyState(VK_SHIFT) & (1 << 15);

    for (int i = 0, iCount(m_ShortCutItems.size()); i < iCount; ++i)
    {
        if (m_ShortCutItems[i].secondKey == ESKey_None)
        {
            continue;
        }

        if (bPressSHIFT && bPressCTRL && m_ShortCutItems[i].firstKey != EFKey_CtrlShift)
        {
            continue;
        }

        if (bPressCTRL && !bPressSHIFT && m_ShortCutItems[i].firstKey != EFKey_Ctrl)
        {
            continue;
        }

        if (!bPressCTRL && bPressSHIFT && m_ShortCutItems[i].firstKey != EFKey_Shift)
        {
            continue;
        }

        if (!bPressSHIFT && !bPressCTRL && m_ShortCutItems[i].firstKey != EFKey_None)
        {
            continue;
        }

        if (m_ShortCutItems[i].secondKey == nChar)
        {
            CD::RunTool(m_ShortCutItems[i].function);
            return true;
        }
    }

    return false;
}

bool ShortcutManager::CheckDuplicatedShortcuts()
{
    std::vector<string> duplicatedShorts1;
    std::vector<std::pair<string, string> > duplicatedShorts2;

    for (int i = 0, iCount(m_ShortCutItems.size()); i < iCount; ++i)
    {
        const SShortCutItem& sc = m_ShortCutItems[i];
        if (sc.secondKey == ESKey_None)
        {
            continue;
        }
        if (HasShortcutRegistered(sc))
        {
            const char* name = Serialization::getEnumDescription<CD::EDesignerTool>().name(sc.function);
            if (name)
            {
                duplicatedShorts1.push_back(name);
            }
        }
    }
    for (int i = 0, iCount(m_ShortCutItems.size()); i < iCount; ++i)
    {
        const SShortCutItem& sc = m_ShortCutItems[i];
        for (int k = i + 1; k < iCount; ++k)
        {
            if (!m_ShortCutItems[i].IsSameKeySetting(m_ShortCutItems[k]))
            {
                continue;
            }

            const char* name0 = Serialization::getEnumDescription<CD::EDesignerTool>().name(m_ShortCutItems[i].function);
            const char* name1 = Serialization::getEnumDescription<CD::EDesignerTool>().name(m_ShortCutItems[k].function);
            if (name0 && name1)
            {
                duplicatedShorts2.push_back(std::pair<string, string>(name0, name1));
            }
        }
    }

    if (!duplicatedShorts1.empty() || !duplicatedShorts2.empty())
    {
        CD::Log("- CryDesigner Shortcut Duplication Check Result -");
        for (int i = 0, iCount(duplicatedShorts1.size()); i < iCount; ++i)
        {
            CD::Log("[%s]'s Shortcut has already existed in Editor so it should be re-assigned.", duplicatedShorts1[i].c_str());
        }
        for (int i = 0, iCount(duplicatedShorts2.size()); i < iCount; ++i)
        {
            CD::Log("[%s] and [%s] have the same key combination so at least one of them should be re-assigned.", duplicatedShorts2[i].first.c_str(), duplicatedShorts2[i].second.c_str());
        }
        CD::Log("------------------------------------------------------------------------------------------------\n");
        return true;
    }
    return false;
}

void ShortcutManager::SetShortcut(CD::SShortCutItem& sc)
{
    for (int i = 0, iCount(m_ShortCutItems.size()); i < iCount; ++i)
    {
        if (m_ShortCutItems[i].function == sc.function)
        {
            m_ShortCutItems[i] = sc;
            return;
        }
    }
    Add(sc);
}

SERIALIZATION_ENUM_BEGIN(EFirstKey, "FirstKey")
SERIALIZATION_ENUM_LABEL(EFKey_None, "")
SERIALIZATION_ENUM_LABEL(EFKey_Ctrl, "CTRL")
SERIALIZATION_ENUM_LABEL(EFKey_Shift, "SHIFT")
SERIALIZATION_ENUM_LABEL(EFKey_CtrlShift, "CTRL+SHIFT")
SERIALIZATION_ENUM_END()

SERIALIZATION_ENUM_BEGIN(ESecondKey, "SecondKey")
SERIALIZATION_ENUM_LABEL(ESKey_None, "")

SERIALIZATION_ENUM_LABEL(ESKey_Tab, "TAB")
SERIALIZATION_ENUM_LABEL(ESKey_Return, "RETURN")
SERIALIZATION_ENUM_LABEL(ESKey_Backspace, "BACKSPACE")
SERIALIZATION_ENUM_LABEL(ESKey_Space, "SPACE")
SERIALIZATION_ENUM_LABEL(ESKey_PageUp, "PAGEUP")
SERIALIZATION_ENUM_LABEL(ESKey_PageDown, "PAGEDOWN")
SERIALIZATION_ENUM_LABEL(ESKey_End, "END")
SERIALIZATION_ENUM_LABEL(ESKey_Home, "HOME")
SERIALIZATION_ENUM_LABEL(ESKey_Left, "LEFT")
SERIALIZATION_ENUM_LABEL(ESKey_Up, "UP")
SERIALIZATION_ENUM_LABEL(ESKey_Right, "RIGHT")
SERIALIZATION_ENUM_LABEL(ESKey_Down, "DOWN")
SERIALIZATION_ENUM_LABEL(ESKey_Insert, "INSERT")
SERIALIZATION_ENUM_LABEL(ESKey_Delete, "DELETE")

SERIALIZATION_ENUM_LABEL(ESKey_0, "0")
SERIALIZATION_ENUM_LABEL(ESKey_1, "1")
SERIALIZATION_ENUM_LABEL(ESKey_2, "2")
SERIALIZATION_ENUM_LABEL(ESKey_3, "3")
SERIALIZATION_ENUM_LABEL(ESKey_4, "4")
SERIALIZATION_ENUM_LABEL(ESKey_5, "5")
SERIALIZATION_ENUM_LABEL(ESKey_6, "6")
SERIALIZATION_ENUM_LABEL(ESKey_7, "7")
SERIALIZATION_ENUM_LABEL(ESKey_8, "8")
SERIALIZATION_ENUM_LABEL(ESKey_9, "9")

SERIALIZATION_ENUM_LABEL(ESKey_A, "A")
SERIALIZATION_ENUM_LABEL(ESKey_B, "B")
SERIALIZATION_ENUM_LABEL(ESKey_C, "C")
SERIALIZATION_ENUM_LABEL(ESKey_D, "D")
SERIALIZATION_ENUM_LABEL(ESKey_E, "E")
SERIALIZATION_ENUM_LABEL(ESKey_F, "F")
SERIALIZATION_ENUM_LABEL(ESKey_G, "G")
SERIALIZATION_ENUM_LABEL(ESKey_H, "H")
SERIALIZATION_ENUM_LABEL(ESKey_I, "I")
SERIALIZATION_ENUM_LABEL(ESKey_J, "J")
SERIALIZATION_ENUM_LABEL(ESKey_K, "K")
SERIALIZATION_ENUM_LABEL(ESKey_L, "L")
SERIALIZATION_ENUM_LABEL(ESKey_M, "M")
SERIALIZATION_ENUM_LABEL(ESKey_N, "N")
SERIALIZATION_ENUM_LABEL(ESKey_O, "O")
SERIALIZATION_ENUM_LABEL(ESKey_P, "P")
SERIALIZATION_ENUM_LABEL(ESKey_Q, "Q")
SERIALIZATION_ENUM_LABEL(ESKey_R, "R")
SERIALIZATION_ENUM_LABEL(ESKey_S, "S")
SERIALIZATION_ENUM_LABEL(ESKey_T, "T")
SERIALIZATION_ENUM_LABEL(ESKey_U, "U")
SERIALIZATION_ENUM_LABEL(ESKey_V, "V")
SERIALIZATION_ENUM_LABEL(ESKey_W, "W")
SERIALIZATION_ENUM_LABEL(ESKey_X, "X")
SERIALIZATION_ENUM_LABEL(ESKey_Y, "Y")
SERIALIZATION_ENUM_LABEL(ESKey_Z, "Z")

SERIALIZATION_ENUM_LABEL(ESKey_F1, "F1")
SERIALIZATION_ENUM_LABEL(ESKey_F2, "F2")
SERIALIZATION_ENUM_LABEL(ESKey_F3, "F3")
SERIALIZATION_ENUM_LABEL(ESKey_F4, "F4")
SERIALIZATION_ENUM_LABEL(ESKey_F5, "F5")
SERIALIZATION_ENUM_LABEL(ESKey_F6, "F6")
SERIALIZATION_ENUM_LABEL(ESKey_F7, "F7")
SERIALIZATION_ENUM_LABEL(ESKey_F8, "F8")
SERIALIZATION_ENUM_LABEL(ESKey_F9, "F9")
SERIALIZATION_ENUM_LABEL(ESKey_F10, "F10")
SERIALIZATION_ENUM_LABEL(ESKey_F11, "F11")
SERIALIZATION_ENUM_LABEL(ESKey_F12, "F12")
SERIALIZATION_ENUM_END()
