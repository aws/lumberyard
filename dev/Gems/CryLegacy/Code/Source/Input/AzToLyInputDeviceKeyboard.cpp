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
#include "CryLegacy_precompiled.h"

#include "AzToLyInputDeviceKeyboard.h"

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>

using namespace AzFramework;

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInputDeviceKeyboard::AzToLyInputDeviceKeyboard(IInput& input)
    : AzToLyInputDevice(input, eIDT_Keyboard, "keyboard", InputDeviceKeyboard::Id)
{
    MapSymbol(InputDeviceKeyboard::Key::Escape.GetNameCrc32(), eKI_Escape, "escape");
    MapSymbol(InputDeviceKeyboard::Key::Alphanumeric1.GetNameCrc32(), eKI_1, "1");
    MapSymbol(InputDeviceKeyboard::Key::Alphanumeric2.GetNameCrc32(), eKI_2, "2");
    MapSymbol(InputDeviceKeyboard::Key::Alphanumeric3.GetNameCrc32(), eKI_3, "3");
    MapSymbol(InputDeviceKeyboard::Key::Alphanumeric4.GetNameCrc32(), eKI_4, "4");
    MapSymbol(InputDeviceKeyboard::Key::Alphanumeric5.GetNameCrc32(), eKI_5, "5");
    MapSymbol(InputDeviceKeyboard::Key::Alphanumeric6.GetNameCrc32(), eKI_6, "6");
    MapSymbol(InputDeviceKeyboard::Key::Alphanumeric7.GetNameCrc32(), eKI_7, "7");
    MapSymbol(InputDeviceKeyboard::Key::Alphanumeric8.GetNameCrc32(), eKI_8, "8");
    MapSymbol(InputDeviceKeyboard::Key::Alphanumeric9.GetNameCrc32(), eKI_9, "9");
    MapSymbol(InputDeviceKeyboard::Key::Alphanumeric0.GetNameCrc32(), eKI_0, "0");
    MapSymbol(InputDeviceKeyboard::Key::PunctuationHyphen.GetNameCrc32(), eKI_Minus, "minus");
    MapSymbol(InputDeviceKeyboard::Key::PunctuationEquals.GetNameCrc32(), eKI_Equals, "equals");
    MapSymbol(InputDeviceKeyboard::Key::EditBackspace.GetNameCrc32(), eKI_Backspace, "backspace");
    MapSymbol(InputDeviceKeyboard::Key::EditTab.GetNameCrc32(), eKI_Tab, "tab");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericQ.GetNameCrc32(), eKI_Q, "q");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericW.GetNameCrc32(), eKI_W, "w");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericE.GetNameCrc32(), eKI_E, "e");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericR.GetNameCrc32(), eKI_R, "r");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericT.GetNameCrc32(), eKI_T, "t");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericY.GetNameCrc32(), eKI_Y, "y");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericU.GetNameCrc32(), eKI_U, "u");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericI.GetNameCrc32(), eKI_I, "i");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericO.GetNameCrc32(), eKI_O, "o");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericP.GetNameCrc32(), eKI_P, "p");
    MapSymbol(InputDeviceKeyboard::Key::PunctuationBracketL.GetNameCrc32(), eKI_LBracket, "lbracket");
    MapSymbol(InputDeviceKeyboard::Key::PunctuationBracketR.GetNameCrc32(), eKI_RBracket, "rbracket");
    MapSymbol(InputDeviceKeyboard::Key::EditEnter.GetNameCrc32(), eKI_Enter, "enter");
    MapSymbol(InputDeviceKeyboard::Key::ModifierCtrlL.GetNameCrc32(), eKI_LCtrl, "lctrl", SInputSymbol::Button, eMM_LCtrl);
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericA.GetNameCrc32(), eKI_A, "a");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericS.GetNameCrc32(), eKI_S, "s");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericD.GetNameCrc32(), eKI_D, "d");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericF.GetNameCrc32(), eKI_F, "f");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericG.GetNameCrc32(), eKI_G, "g");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericH.GetNameCrc32(), eKI_H, "h");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericJ.GetNameCrc32(), eKI_J, "j");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericK.GetNameCrc32(), eKI_K, "k");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericL.GetNameCrc32(), eKI_L, "l");
    MapSymbol(InputDeviceKeyboard::Key::PunctuationSemicolon.GetNameCrc32(), eKI_Semicolon, "semicolon");
    MapSymbol(InputDeviceKeyboard::Key::PunctuationApostrophe.GetNameCrc32(), eKI_Apostrophe, "apostrophe");
    MapSymbol(InputDeviceKeyboard::Key::PunctuationTilde.GetNameCrc32(), eKI_Tilde, "tilde");
    MapSymbol(InputDeviceKeyboard::Key::ModifierShiftL.GetNameCrc32(), eKI_LShift, "lshift", SInputSymbol::Button, eMM_LShift);
    MapSymbol(InputDeviceKeyboard::Key::PunctuationBackslash.GetNameCrc32(), eKI_Backslash, "backslash");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericZ.GetNameCrc32(), eKI_Z, "z");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericX.GetNameCrc32(), eKI_X, "x");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericC.GetNameCrc32(), eKI_C, "c");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericV.GetNameCrc32(), eKI_V, "v");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericB.GetNameCrc32(), eKI_B, "b");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericN.GetNameCrc32(), eKI_N, "n");
    MapSymbol(InputDeviceKeyboard::Key::AlphanumericM.GetNameCrc32(), eKI_M, "m");
    MapSymbol(InputDeviceKeyboard::Key::PunctuationComma.GetNameCrc32(), eKI_Comma, "comma");
    MapSymbol(InputDeviceKeyboard::Key::PunctuationPeriod.GetNameCrc32(), eKI_Period, "period");
    MapSymbol(InputDeviceKeyboard::Key::PunctuationSlash.GetNameCrc32(), eKI_Slash, "slash");
    MapSymbol(InputDeviceKeyboard::Key::ModifierShiftR.GetNameCrc32(), eKI_RShift, "rshift", SInputSymbol::Button, eMM_RShift);
    MapSymbol(InputDeviceKeyboard::Key::NumPadMultiply.GetNameCrc32(), eKI_NP_Multiply, "np_multiply");
    MapSymbol(InputDeviceKeyboard::Key::ModifierAltL.GetNameCrc32(), eKI_LAlt, "lalt", SInputSymbol::Button, eMM_LAlt);
    MapSymbol(InputDeviceKeyboard::Key::EditSpace.GetNameCrc32(), eKI_Space, "space");
    MapSymbol(InputDeviceKeyboard::Key::EditCapsLock.GetNameCrc32(), eKI_CapsLock, "capslock", SInputSymbol::Toggle, eMM_CapsLock);
    MapSymbol(InputDeviceKeyboard::Key::Function01.GetNameCrc32(), eKI_F1, "f1");
    MapSymbol(InputDeviceKeyboard::Key::Function02.GetNameCrc32(), eKI_F2, "f2");
    MapSymbol(InputDeviceKeyboard::Key::Function03.GetNameCrc32(), eKI_F3, "f3");
    MapSymbol(InputDeviceKeyboard::Key::Function04.GetNameCrc32(), eKI_F4, "f4");
    MapSymbol(InputDeviceKeyboard::Key::Function05.GetNameCrc32(), eKI_F5, "f5");
    MapSymbol(InputDeviceKeyboard::Key::Function06.GetNameCrc32(), eKI_F6, "f6");
    MapSymbol(InputDeviceKeyboard::Key::Function07.GetNameCrc32(), eKI_F7, "f7");
    MapSymbol(InputDeviceKeyboard::Key::Function08.GetNameCrc32(), eKI_F8, "f8");
    MapSymbol(InputDeviceKeyboard::Key::Function09.GetNameCrc32(), eKI_F9, "f9");
    MapSymbol(InputDeviceKeyboard::Key::Function10.GetNameCrc32(), eKI_F10, "f10");
    MapSymbol(InputDeviceKeyboard::Key::NumLock.GetNameCrc32(), eKI_NumLock, "numlock", SInputSymbol::Toggle, eMM_NumLock);
    MapSymbol(InputDeviceKeyboard::Key::WindowsSystemScrollLock.GetNameCrc32(), eKI_ScrollLock, "scrolllock", SInputSymbol::Toggle, eMM_ScrollLock);
    MapSymbol(InputDeviceKeyboard::Key::NumPad7.GetNameCrc32(), eKI_NP_7, "np_7");
    MapSymbol(InputDeviceKeyboard::Key::NumPad8.GetNameCrc32(), eKI_NP_8, "np_8");
    MapSymbol(InputDeviceKeyboard::Key::NumPad9.GetNameCrc32(), eKI_NP_9, "np_9");
    MapSymbol(InputDeviceKeyboard::Key::NumPadSubtract.GetNameCrc32(), eKI_NP_Substract, "np_subtract");
    MapSymbol(InputDeviceKeyboard::Key::NumPad4.GetNameCrc32(), eKI_NP_4, "np_4");
    MapSymbol(InputDeviceKeyboard::Key::NumPad5.GetNameCrc32(), eKI_NP_5, "np_5");
    MapSymbol(InputDeviceKeyboard::Key::NumPad6.GetNameCrc32(), eKI_NP_6, "np_6");
    MapSymbol(InputDeviceKeyboard::Key::NumPadAdd.GetNameCrc32(), eKI_NP_Add, "np_add");
    MapSymbol(InputDeviceKeyboard::Key::NumPad1.GetNameCrc32(), eKI_NP_1, "np_1");
    MapSymbol(InputDeviceKeyboard::Key::NumPad2.GetNameCrc32(), eKI_NP_2, "np_2");
    MapSymbol(InputDeviceKeyboard::Key::NumPad3.GetNameCrc32(), eKI_NP_3, "np_3");
    MapSymbol(InputDeviceKeyboard::Key::NumPad0.GetNameCrc32(), eKI_NP_0, "np_0");
    MapSymbol(InputDeviceKeyboard::Key::NumPadDecimal.GetNameCrc32(), eKI_NP_Period, "np_period");
    MapSymbol(InputDeviceKeyboard::Key::Function11.GetNameCrc32(), eKI_F11, "f11");
    MapSymbol(InputDeviceKeyboard::Key::Function12.GetNameCrc32(), eKI_F12, "f12");
    MapSymbol(InputDeviceKeyboard::Key::Function13.GetNameCrc32(), eKI_F13, "f13");
    MapSymbol(InputDeviceKeyboard::Key::Function14.GetNameCrc32(), eKI_F14, "f14");
    MapSymbol(InputDeviceKeyboard::Key::Function15.GetNameCrc32(), eKI_F15, "f15");
    //MapSymbol(???, eKI_Colon, "colon");
    //MapSymbol(???, eKI_Underline, "underline");
    MapSymbol(InputDeviceKeyboard::Key::NumPadEnter.GetNameCrc32(), eKI_NP_Enter, "np_enter");
    MapSymbol(InputDeviceKeyboard::Key::ModifierCtrlR.GetNameCrc32(),  eKI_RCtrl, "rctrl", SInputSymbol::Button, eMM_RCtrl);
    MapSymbol(InputDeviceKeyboard::Key::NumPadDivide.GetNameCrc32(), eKI_NP_Divide, "np_divide");
    MapSymbol(InputDeviceKeyboard::Key::WindowsSystemPrint.GetNameCrc32(), eKI_Print, "print");
    MapSymbol(InputDeviceKeyboard::Key::ModifierAltR.GetNameCrc32(), eKI_RAlt,  "ralt", SInputSymbol::Button, eMM_RAlt);
    MapSymbol(InputDeviceKeyboard::Key::WindowsSystemPause.GetNameCrc32(), eKI_Pause,  "pause");
    MapSymbol(InputDeviceKeyboard::Key::NavigationHome.GetNameCrc32(), eKI_Home,  "home");
    MapSymbol(InputDeviceKeyboard::Key::NavigationArrowUp.GetNameCrc32(), eKI_Up,  "up");
    MapSymbol(InputDeviceKeyboard::Key::NavigationPageUp.GetNameCrc32(), eKI_PgUp,  "pgup");
    MapSymbol(InputDeviceKeyboard::Key::NavigationArrowLeft.GetNameCrc32(), eKI_Left,  "left");
    MapSymbol(InputDeviceKeyboard::Key::NavigationArrowRight.GetNameCrc32(), eKI_Right,  "right");
    MapSymbol(InputDeviceKeyboard::Key::NavigationEnd.GetNameCrc32(), eKI_End,  "end");
    MapSymbol(InputDeviceKeyboard::Key::NavigationArrowDown.GetNameCrc32(), eKI_Down,  "down");
    MapSymbol(InputDeviceKeyboard::Key::NavigationPageDown.GetNameCrc32(), eKI_PgDn,  "pgdn");
    MapSymbol(InputDeviceKeyboard::Key::NavigationInsert.GetNameCrc32(), eKI_Insert,  "insert");
    MapSymbol(InputDeviceKeyboard::Key::NavigationDelete.GetNameCrc32(), eKI_Delete,  "delete");
    //MapSymbol(InputDeviceKeyboard::Key::ModifierSuperL.GetNameCrc32(), eKI_LWin,  "lwin");
    //MapSymbol(InputDeviceKeyboard::Key::ModifierSuperR.GetNameCrc32(), eKI_RWin,  "rwin");
    //MapSymbol(???, eKI_Apps,  "apps");
    MapSymbol(InputDeviceKeyboard::Key::SupplementaryISO.GetNameCrc32(), eKI_OEM_102, "oem_102");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
AzToLyInputDeviceKeyboard::~AzToLyInputDeviceKeyboard()
{
}
