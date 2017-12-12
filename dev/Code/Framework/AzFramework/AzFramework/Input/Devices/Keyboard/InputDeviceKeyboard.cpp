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

#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/Utils/ProcessRawInputEventQueues.h>

#include <AzCore/std/smart_ptr/make_shared.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDeviceId InputDeviceKeyboard::Id("keyboard");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Alphanumeric Keys
    const InputChannelId InputDeviceKeyboard::Key::Alphanumeric0("keyboard_key_alphanumeric_0");
    const InputChannelId InputDeviceKeyboard::Key::Alphanumeric1("keyboard_key_alphanumeric_1");
    const InputChannelId InputDeviceKeyboard::Key::Alphanumeric2("keyboard_key_alphanumeric_2");
    const InputChannelId InputDeviceKeyboard::Key::Alphanumeric3("keyboard_key_alphanumeric_3");
    const InputChannelId InputDeviceKeyboard::Key::Alphanumeric4("keyboard_key_alphanumeric_4");
    const InputChannelId InputDeviceKeyboard::Key::Alphanumeric5("keyboard_key_alphanumeric_5");
    const InputChannelId InputDeviceKeyboard::Key::Alphanumeric6("keyboard_key_alphanumeric_6");
    const InputChannelId InputDeviceKeyboard::Key::Alphanumeric7("keyboard_key_alphanumeric_7");
    const InputChannelId InputDeviceKeyboard::Key::Alphanumeric8("keyboard_key_alphanumeric_8");
    const InputChannelId InputDeviceKeyboard::Key::Alphanumeric9("keyboard_key_alphanumeric_9");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericA("keyboard_key_alphanumeric_A");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericB("keyboard_key_alphanumeric_B");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericC("keyboard_key_alphanumeric_C");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericD("keyboard_key_alphanumeric_D");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericE("keyboard_key_alphanumeric_E");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericF("keyboard_key_alphanumeric_F");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericG("keyboard_key_alphanumeric_G");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericH("keyboard_key_alphanumeric_H");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericI("keyboard_key_alphanumeric_I");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericJ("keyboard_key_alphanumeric_J");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericK("keyboard_key_alphanumeric_K");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericL("keyboard_key_alphanumeric_L");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericM("keyboard_key_alphanumeric_M");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericN("keyboard_key_alphanumeric_N");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericO("keyboard_key_alphanumeric_O");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericP("keyboard_key_alphanumeric_P");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericQ("keyboard_key_alphanumeric_Q");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericR("keyboard_key_alphanumeric_R");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericS("keyboard_key_alphanumeric_S");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericT("keyboard_key_alphanumeric_T");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericU("keyboard_key_alphanumeric_U");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericV("keyboard_key_alphanumeric_V");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericW("keyboard_key_alphanumeric_W");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericX("keyboard_key_alphanumeric_X");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericY("keyboard_key_alphanumeric_Y");
    const InputChannelId InputDeviceKeyboard::Key::AlphanumericZ("keyboard_key_alphanumeric_Z");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Edit (and escape) Keys
    const InputChannelId InputDeviceKeyboard::Key::EditBackspace("keyboard_key_edit_backspace");
    const InputChannelId InputDeviceKeyboard::Key::EditCapsLock("keyboard_key_edit_capslock");
    const InputChannelId InputDeviceKeyboard::Key::EditEnter("keyboard_key_edit_enter");
    const InputChannelId InputDeviceKeyboard::Key::EditSpace("keyboard_key_edit_space");
    const InputChannelId InputDeviceKeyboard::Key::EditTab("keyboard_key_edit_tab");
    const InputChannelId InputDeviceKeyboard::Key::Escape("keyboard_key_escape");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Function Keys
    const InputChannelId InputDeviceKeyboard::Key::Function01("keyboard_key_function_F01");
    const InputChannelId InputDeviceKeyboard::Key::Function02("keyboard_key_function_F02");
    const InputChannelId InputDeviceKeyboard::Key::Function03("keyboard_key_function_F03");
    const InputChannelId InputDeviceKeyboard::Key::Function04("keyboard_key_function_F04");
    const InputChannelId InputDeviceKeyboard::Key::Function05("keyboard_key_function_F05");
    const InputChannelId InputDeviceKeyboard::Key::Function06("keyboard_key_function_F06");
    const InputChannelId InputDeviceKeyboard::Key::Function07("keyboard_key_function_F07");
    const InputChannelId InputDeviceKeyboard::Key::Function08("keyboard_key_function_F08");
    const InputChannelId InputDeviceKeyboard::Key::Function09("keyboard_key_function_F09");
    const InputChannelId InputDeviceKeyboard::Key::Function10("keyboard_key_function_F10");
    const InputChannelId InputDeviceKeyboard::Key::Function11("keyboard_key_function_F11");
    const InputChannelId InputDeviceKeyboard::Key::Function12("keyboard_key_function_F12");
    const InputChannelId InputDeviceKeyboard::Key::Function13("keyboard_key_function_F13");
    const InputChannelId InputDeviceKeyboard::Key::Function14("keyboard_key_function_F14");
    const InputChannelId InputDeviceKeyboard::Key::Function15("keyboard_key_function_F15");
    const InputChannelId InputDeviceKeyboard::Key::Function16("keyboard_key_function_F16");
    const InputChannelId InputDeviceKeyboard::Key::Function17("keyboard_key_function_F17");
    const InputChannelId InputDeviceKeyboard::Key::Function18("keyboard_key_function_F18");
    const InputChannelId InputDeviceKeyboard::Key::Function19("keyboard_key_function_F19");
    const InputChannelId InputDeviceKeyboard::Key::Function20("keyboard_key_function_F20");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifier Keys
    const InputChannelId InputDeviceKeyboard::Key::ModifierAltL("keyboard_key_modifier_alt_l");
    const InputChannelId InputDeviceKeyboard::Key::ModifierAltR("keyboard_key_modifier_alt_r");
    const InputChannelId InputDeviceKeyboard::Key::ModifierCtrlL("keyboard_key_modifier_ctrl_l");
    const InputChannelId InputDeviceKeyboard::Key::ModifierCtrlR("keyboard_key_modifier_ctrl_r");
    const InputChannelId InputDeviceKeyboard::Key::ModifierShiftL("keyboard_key_modifier_shift_l");
    const InputChannelId InputDeviceKeyboard::Key::ModifierShiftR("keyboard_key_modifier_shift_r");
    const InputChannelId InputDeviceKeyboard::Key::ModifierSuperL("keyboard_key_modifier_super_l");
    const InputChannelId InputDeviceKeyboard::Key::ModifierSuperR("keyboard_key_modifier_super_r");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Navigation Keys
    const InputChannelId InputDeviceKeyboard::Key::NavigationArrowDown("keyboard_key_navigation_arrow_down");
    const InputChannelId InputDeviceKeyboard::Key::NavigationArrowLeft("keyboard_key_navigation_arrow_left");
    const InputChannelId InputDeviceKeyboard::Key::NavigationArrowRight("keyboard_key_navigation_arrow_right");
    const InputChannelId InputDeviceKeyboard::Key::NavigationArrowUp("keyboard_key_navigation_arrow_up");
    const InputChannelId InputDeviceKeyboard::Key::NavigationDelete("keyboard_key_navigation_delete");
    const InputChannelId InputDeviceKeyboard::Key::NavigationEnd("keyboard_key_navigation_end");
    const InputChannelId InputDeviceKeyboard::Key::NavigationHome("keyboard_key_navigation_home");
    const InputChannelId InputDeviceKeyboard::Key::NavigationInsert("keyboard_key_navigation_insert");
    const InputChannelId InputDeviceKeyboard::Key::NavigationPageDown("keyboard_key_navigation_page_down");
    const InputChannelId InputDeviceKeyboard::Key::NavigationPageUp("keyboard_key_navigation_page_up");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Numpad Keys
    const InputChannelId InputDeviceKeyboard::Key::NumLock("keyboard_key_num_lock");
    const InputChannelId InputDeviceKeyboard::Key::NumPad0("keyboard_key_numpad_0");
    const InputChannelId InputDeviceKeyboard::Key::NumPad1("keyboard_key_numpad_1");
    const InputChannelId InputDeviceKeyboard::Key::NumPad2("keyboard_key_numpad_2");
    const InputChannelId InputDeviceKeyboard::Key::NumPad3("keyboard_key_numpad_3");
    const InputChannelId InputDeviceKeyboard::Key::NumPad4("keyboard_key_numpad_4");
    const InputChannelId InputDeviceKeyboard::Key::NumPad5("keyboard_key_numpad_5");
    const InputChannelId InputDeviceKeyboard::Key::NumPad6("keyboard_key_numpad_6");
    const InputChannelId InputDeviceKeyboard::Key::NumPad7("keyboard_key_numpad_7");
    const InputChannelId InputDeviceKeyboard::Key::NumPad8("keyboard_key_numpad_8");
    const InputChannelId InputDeviceKeyboard::Key::NumPad9("keyboard_key_numpad_9");
    const InputChannelId InputDeviceKeyboard::Key::NumPadAdd("keyboard_key_numpad_add");
    const InputChannelId InputDeviceKeyboard::Key::NumPadDecimal("keyboard_key_numpad_decimal");
    const InputChannelId InputDeviceKeyboard::Key::NumPadDivide("keyboard_key_numpad_divide");
    const InputChannelId InputDeviceKeyboard::Key::NumPadEnter("keyboard_key_numpad_enter");
    const InputChannelId InputDeviceKeyboard::Key::NumPadMultiply("keyboard_key_numpad_multiply");
    const InputChannelId InputDeviceKeyboard::Key::NumPadSubtract("keyboard_key_numpad_subtract");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Punctuation Keys
    const InputChannelId InputDeviceKeyboard::Key::PunctuationApostrophe("keyboard_key_punctuation_apostrophe");
    const InputChannelId InputDeviceKeyboard::Key::PunctuationBackslash("keyboard_key_punctuation_backslash");
    const InputChannelId InputDeviceKeyboard::Key::PunctuationBracketL("keyboard_key_punctuation_bracket_l");
    const InputChannelId InputDeviceKeyboard::Key::PunctuationBracketR("keyboard_key_punctuation_bracket_r");
    const InputChannelId InputDeviceKeyboard::Key::PunctuationComma("keyboard_key_punctuation_comma");
    const InputChannelId InputDeviceKeyboard::Key::PunctuationEquals("keyboard_key_punctuation_equals");
    const InputChannelId InputDeviceKeyboard::Key::PunctuationHyphen("keyboard_key_punctuation_hyphen");
    const InputChannelId InputDeviceKeyboard::Key::PunctuationPeriod("keyboard_key_punctuation_period");
    const InputChannelId InputDeviceKeyboard::Key::PunctuationSemicolon("keyboard_key_punctuation_semicolon");
    const InputChannelId InputDeviceKeyboard::Key::PunctuationSlash("keyboard_key_punctuation_slash");
    const InputChannelId InputDeviceKeyboard::Key::PunctuationTilde("keyboard_key_punctuation_tilde");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Supplementary ISO Key
    const InputChannelId InputDeviceKeyboard::Key::SupplementaryISO("keyboard_key_supplementary_iso");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Windows System Keys
    const InputChannelId InputDeviceKeyboard::Key::WindowsSystemPause("keyboard_key_windows_system_pause");
    const InputChannelId InputDeviceKeyboard::Key::WindowsSystemPrint("keyboard_key_windows_system_print");
    const InputChannelId InputDeviceKeyboard::Key::WindowsSystemScrollLock("keyboard_key_windows_system_scroll_lock");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const AZStd::array<InputChannelId, 112> InputDeviceKeyboard::Key::All =
    {{
        // Alphanumeric Keys
        Alphanumeric0,
        Alphanumeric1,
        Alphanumeric2,
        Alphanumeric3,
        Alphanumeric4,
        Alphanumeric5,
        Alphanumeric6,
        Alphanumeric7,
        Alphanumeric8,
        Alphanumeric9,
        AlphanumericA,
        AlphanumericB,
        AlphanumericC,
        AlphanumericD,
        AlphanumericE,
        AlphanumericF,
        AlphanumericG,
        AlphanumericH,
        AlphanumericI,
        AlphanumericJ,
        AlphanumericK,
        AlphanumericL,
        AlphanumericM,
        AlphanumericN,
        AlphanumericO,
        AlphanumericP,
        AlphanumericQ,
        AlphanumericR,
        AlphanumericS,
        AlphanumericT,
        AlphanumericU,
        AlphanumericV,
        AlphanumericW,
        AlphanumericX,
        AlphanumericY,
        AlphanumericZ,

        // Edit (and escape) Keys
        EditBackspace,
        EditCapsLock,
        EditEnter,
        EditSpace,
        EditTab,
        Escape,

        // Function Keys
        Function01,
        Function02,
        Function03,
        Function04,
        Function05,
        Function06,
        Function07,
        Function08,
        Function09,
        Function10,
        Function11,
        Function12,
        Function13,
        Function14,
        Function15,
        Function16,
        Function17,
        Function18,
        Function19,
        Function20,

        // Modifier Keys
        ModifierAltL,
        ModifierAltR,
        ModifierCtrlL,
        ModifierCtrlR,
        ModifierShiftL,
        ModifierShiftR,
        ModifierSuperL,
        ModifierSuperR,

        // Navigation Keys
        NavigationArrowDown,
        NavigationArrowLeft,
        NavigationArrowRight,
        NavigationArrowUp,
        NavigationDelete,
        NavigationEnd,
        NavigationHome,
        NavigationInsert,
        NavigationPageDown,
        NavigationPageUp,

        // Numpad Keys
        NumLock,
        NumPad0,
        NumPad1,
        NumPad2,
        NumPad3,
        NumPad4,
        NumPad5,
        NumPad6,
        NumPad7,
        NumPad8,
        NumPad9,
        NumPadAdd,
        NumPadDecimal,
        NumPadDivide,
        NumPadEnter,
        NumPadMultiply,
        NumPadSubtract,

        // Punctuation Keys
        PunctuationApostrophe,
        PunctuationBackslash,
        PunctuationBracketL,
        PunctuationBracketR,
        PunctuationComma,
        PunctuationEquals,
        PunctuationHyphen,
        PunctuationPeriod,
        PunctuationSemicolon,
        PunctuationSlash,
        PunctuationTilde,

        // Supplementary ISO Key
        SupplementaryISO,

        // Windows System Keys
        WindowsSystemPause,
        WindowsSystemPrint,
        WindowsSystemScrollLock
    }};

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ModifierKeyMask GetCorrespondingModifierKeyMask(const InputChannelId& channelId)
    {
        if (channelId == InputDeviceKeyboard::Key::ModifierAltL) { return ModifierKeyMask::AltL; }
        if (channelId == InputDeviceKeyboard::Key::ModifierAltR) { return ModifierKeyMask::AltR; }
        if (channelId == InputDeviceKeyboard::Key::ModifierCtrlL) { return ModifierKeyMask::CtrlL; }
        if (channelId == InputDeviceKeyboard::Key::ModifierCtrlR) { return ModifierKeyMask::CtrlR; }
        if (channelId == InputDeviceKeyboard::Key::ModifierShiftL) { return ModifierKeyMask::ShiftL; }
        if (channelId == InputDeviceKeyboard::Key::ModifierShiftR) { return ModifierKeyMask::ShiftR; }
        if (channelId == InputDeviceKeyboard::Key::ModifierSuperL) { return ModifierKeyMask::SuperL; }
        if (channelId == InputDeviceKeyboard::Key::ModifierSuperR) { return ModifierKeyMask::SuperR; }
        return ModifierKeyMask::None;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboard::InputDeviceKeyboard()
        : InputDevice(Id)
        , m_modifierKeyStates(AZStd::make_shared<ModifierKeyStates>())
        , m_allChannelsById()
        , m_keyChannelsById()
        , m_pimpl()
        , m_implementationRequestHandler(*this)
    {
        // Create all key input channels
        for (const InputChannelId& channelId : Key::All)
        {
            const ModifierKeyMask modifierKeyMask = GetCorrespondingModifierKeyMask(channelId);
            InputChannelDigitalWithSharedModifierKeyStates* channel =
                aznew InputChannelDigitalWithSharedModifierKeyStates(channelId,
                                                                     *this,
                                                                     m_modifierKeyStates,
                                                                     modifierKeyMask);
            m_allChannelsById[channelId] = channel;
            m_keyChannelsById[channelId] = channel;
        }

        // Create the platform specific implementation
        m_pimpl.reset(Implementation::Create(*this));

        // Connect to the text entry request bus
        InputTextEntryRequestBus::Handler::BusConnect(GetInputDeviceId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboard::~InputDeviceKeyboard()
    {
        // Disconnect from the text entry request bus
        InputTextEntryRequestBus::Handler::BusDisconnect(GetInputDeviceId());

        // Destroy the platform specific implementation
        m_pimpl.reset();

        // Destroy all key input channels
        for (const auto& channelById : m_keyChannelsById)
        {
            delete channelById.second;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice::InputChannelByIdMap& InputDeviceKeyboard::GetInputChannelsById() const
    {
        return m_allChannelsById;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboard::IsSupported() const
    {
        return m_pimpl != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboard::IsConnected() const
    {
        return m_pimpl ? m_pimpl->IsConnected() : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceKeyboard::HasTextEntryStarted() const
    {
        return m_pimpl ? m_pimpl->HasTextEntryStarted() : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::TextEntryStart(const VirtualKeyboardOptions& options)
    {
        if (m_pimpl)
        {
            m_pimpl->TextEntryStart(options);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::TextEntryStop()
    {
        if (m_pimpl)
        {
            m_pimpl->TextEntryStop();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::TickInputDevice()
    {
        if (m_pimpl)
        {
            m_pimpl->TickInputDevice();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboard::Implementation::Implementation(InputDeviceKeyboard& inputDevice)
        : m_inputDevice(inputDevice)
        , m_rawKeyEventQueuesById()
        , m_rawTextEventQueue()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceKeyboard::Implementation::~Implementation()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::Implementation::QueueRawKeyEvent(const InputChannelId& inputChannelId,
                                                               bool rawKeyState)
    {
        // It should not (in theory) be possible to receive multiple raw key events with the same id
        // and state in succession; if it happens in practice for whatever reason this is still safe.
        m_rawKeyEventQueuesById[inputChannelId].push_back(rawKeyState);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::Implementation::QueueRawTextEvent(const AZStd::string& textUTF8)
    {
        m_rawTextEventQueue.push_back(textUTF8);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::Implementation::ProcessRawEventQueues()
    {
        // Process all raw input events that were queued since the last call to this function.
        // Text events should be processed first in case text input is disabled by a key event.
        ProcessRawInputTextEventQueue(m_rawTextEventQueue);
        ProcessRawInputEventQueues(m_rawKeyEventQueuesById, m_inputDevice.m_keyChannelsById);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceKeyboard::Implementation::ResetInputChannelStates()
    {
        m_inputDevice.ResetInputChannelStates();
    }
} // namespace AzFramework
