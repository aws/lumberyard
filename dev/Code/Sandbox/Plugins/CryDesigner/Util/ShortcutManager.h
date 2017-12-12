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

#pragma once

#include "ToolFactory.h"

namespace CD
{
    enum EFirstKey
    {
        EFKey_None = 0,
        EFKey_Ctrl = FCONTROL,
        EFKey_Shift = FSHIFT,
        EFKey_CtrlShift = FCONTROL | FSHIFT,
    };

    enum ESecondKey
    {
        ESKey_None               = 0,

        ESKey_Tab                = VK_TAB,
        ESKey_Return             = VK_RETURN,
        ESKey_Backspace          = VK_BACK,
        ESKey_Space              = VK_SPACE,
        ESKey_PageUp             = VK_PRIOR,
        ESKey_PageDown           = VK_NEXT,
        ESKey_End                = VK_END,
        ESKey_Home               = VK_HOME,
        ESKey_Left               = VK_LEFT,
        ESKey_Up                 = VK_UP,
        ESKey_Right              = VK_RIGHT,
        ESKey_Down               = VK_DOWN,
        ESKey_Insert             = VK_INSERT,
        ESKey_Delete             = VK_DELETE,

        ESKey_0                  = 0x30,
        ESKey_1,
        ESKey_2,
        ESKey_3,
        ESKey_4,
        ESKey_5,
        ESKey_6,
        ESKey_7,
        ESKey_8,
        ESKey_9,

        ESKey_A                  = 0x41,
        ESKey_B,
        ESKey_C,
        ESKey_D,
        ESKey_E,
        ESKey_F,
        ESKey_G,
        ESKey_H,
        ESKey_I,
        ESKey_J,
        ESKey_K,
        ESKey_L,
        ESKey_M,
        ESKey_N,
        ESKey_O,
        ESKey_P,
        ESKey_Q,
        ESKey_R,
        ESKey_S,
        ESKey_T,
        ESKey_U,
        ESKey_V,
        ESKey_W,
        ESKey_X,
        ESKey_Y,
        ESKey_Z,

        ESKey_F1                  = VK_F1,
        ESKey_F2,
        ESKey_F3,
        ESKey_F4,
        ESKey_F5,
        ESKey_F6,
        ESKey_F7,
        ESKey_F8,
        ESKey_F9,
        ESKey_F10,
        ESKey_F11,
        ESKey_F12,
    };

    struct SShortCutItem
    {
        SShortCutItem()
        {
            function = eDesigner_Invalid;
            firstKey = EFKey_None;
            secondKey = ESKey_None;
        }

        SShortCutItem(EDesignerTool _function, EFirstKey _firstKey, ESecondKey _secondKey)
            : function(_function)
            , firstKey(_firstKey)
            , secondKey(_secondKey)
        {
        }

        bool IsSameKeySetting(const SShortCutItem& shortcut) const
        {
            if (secondKey == ESKey_None)
            {
                return false;
            }
            return firstKey == shortcut.firstKey && secondKey == shortcut.secondKey;
        }

        EDesignerTool function;
        EFirstKey firstKey;
        ESecondKey secondKey;

        void Serialize(Serialization::IArchive& ar)
        {
            ar(function, "function", 0);
            ar(firstKey, "FirstKey", "^");
            ar(secondKey, "SecondKey", "^");
        }
    };
}

class ShortcutManager
{
public:

    void Serialize(Serialization::IArchive& ar);
    void Save();
    void Load();
    void Add(CD::SShortCutItem& sc){ m_ShortCutItems.push_back(sc); }
    void SetShortcut(CD::SShortCutItem& sc);
    bool Process(uint32 nChar);
    bool CheckDuplicatedShortcuts();

    static ShortcutManager& the()
    {
        static ShortcutManager shortcutMgr;
        return shortcutMgr;
    }

private:

    ShortcutManager();

    bool HasFunction(int function)
    {
        for (int i = 0, iCount(m_ShortCutItems.size()); i < iCount; ++i)
        {
            if (m_ShortCutItems[i].function == function)
            {
                return true;
            }
        }
        return false;
    }

    std::vector<CD::SShortCutItem> m_ShortCutItems;
};