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

#pragma once

// include the required headers
#include "EMotionFXConfig.h"
#include "BaseObject.h"
#include <MCore/Source/UnicodeString.h>
#include <MCore/Source/Array.h>


namespace EMotionFX
{
    class EMFX_API AnimGraphGameControllerSettings
        : public BaseObject
    {
        MCORE_MEMORYOBJECTCATEGORY(AnimGraphGameControllerSettings, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_GAMECONTROLLER);

    public:
        enum ParameterMode
        {
            PARAMMODE_STANDARD                          = 0,
            PARAMMODE_ZEROTOONE                         = 1,
            PARAMMODE_PARAMRANGE                        = 2,
            PARAMMODE_POSITIVETOPARAMRANGE              = 3,
            PARAMMODE_NEGATIVETOPARAMRANGE              = 4,
            PARAMMODE_ROTATE_CHARACTER                  = 5
        };

        enum ButtonMode
        {
            BUTTONMODE_NONE                             = 0,
            BUTTONMODE_SWITCHSTATE                      = 1,
            BUTTONMODE_TOGGLEBOOLEANPARAMETER           = 2,
            BUTTONMODE_ENABLEBOOLWHILEPRESSED           = 3,
            BUTTONMODE_DISABLEBOOLWHILEPRESSED          = 4,
            BUTTONMODE_ENABLEBOOLFORONLYONEFRAMEONLY    = 5
        };

        struct EMFX_API ParameterInfo
        {
            MCORE_MEMORYOBJECTCATEGORY(AnimGraphGameControllerSettings::ParameterInfo, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_GAMECONTROLLER);
            ParameterInfo(const char* parameterName);

            ParameterInfo* Clone() const;

            MCore::String           mParameterName;
            ParameterMode           mMode;
            bool                    mInvert;
            bool                    mEnabled;
            uint8                   mAxis;
        };

        struct EMFX_API ButtonInfo
        {
            MCORE_MEMORYOBJECTCATEGORY(AnimGraphGameControllerSettings::ButtonInfo, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_GAMECONTROLLER);
            ButtonInfo(uint32 buttonIndex);

            ButtonInfo* Clone() const;

            uint32                  mButtonIndex;
            ButtonMode              mMode;
            MCore::String           mString;                /**< Mostly used to store the attribute or parameter name to which this button belongs to. */
            bool                    mOldIsPressed;
            bool                    mEnabled;
        };

        class EMFX_API Preset
            : public BaseObject
        {
            MCORE_MEMORYOBJECTCATEGORY(AnimGraphGameControllerSettings::Preset, EMFX_DEFAULT_ALIGNMENT, EMFX_MEMCATEGORY_ANIMGRAPH_GAMECONTROLLER);

        public:
            static Preset* Create(const char* name);

            // try to find the parameter or button info and create a new one for the given name or index in case no existing one has been found
            ParameterInfo* FindParameterInfo(const char* parameterName);
            ButtonInfo* FindButtonInfo(uint32 buttonIndex);

            /**
             * Check if the parameter with the given name is being controlled by the gamepad.
             * This assumes that the mString member from the ButtonInfo contains the parameter name.
             * @param[in] stringName The name to compare against the mString member of the button infos.
             * @result True in case a button info with the given string name doesn't have BUTTONMODE_NONE assigned, false in the other case.
             */
            bool CheckIfIsParameterButtonControlled(const char* stringName);

            /**
             * Check if any of the button infos that are linked to the given string name is enabled.
             * This assumes that the mString member from the ButtonInfo contains the parameter name.
             * @param[in] stringName The name to compare against the mString member of the button infos.
             * @result True in case any of the button infos with the given string name is enabled.
             */
            bool CheckIfIsButtonEnabled(const char* stringName);

            /**
             * Set all button infos that are linked to the given string name to the enabled flag.
             * This assumes that the mString member from the ButtonInfo contains the parameter name.
             * @param[in] stringName The name to compare against the mString member of the button infos.
             * @param[in] isEnabled True in case the button infos shall be enabled, false if they shall become disabled.
             */
            void SetButtonEnabled(const char* stringName, bool isEnabled);

            void Clear();

            void SetName(const char* name)                              { mName = name; }
            const char* GetName() const                                 { return mName.AsChar(); }
            const MCore::String& GetNameString() const                  { return mName; }

            void SetNumParamInfos(uint32 numParamInfos)                 { mParameterInfos.Resize(numParamInfos); }
            void SetParamInfo(uint32 index, ParameterInfo* paramInfo)   { mParameterInfos[index] = paramInfo; }
            uint32 GetNumParamInfos() const                             { return mParameterInfos.GetLength(); }
            ParameterInfo* GetParamInfo(uint32 index) const             { return mParameterInfos[index]; }

            void SetNumButtonInfos(uint32 numButtonInfos)               { mButtonInfos.Resize(numButtonInfos); }
            void SetButtonInfo(uint32 index, ButtonInfo* buttonInfo)    { mButtonInfos[index] = buttonInfo; }
            uint32 GetNumButtonInfos() const                            { return mButtonInfos.GetLength(); }
            ButtonInfo* GetButtonInfo(uint32 index) const               { return mButtonInfos[index]; }

            Preset* Clone() const;

        private:
            MCore::Array<ParameterInfo*>    mParameterInfos;
            MCore::Array<ButtonInfo*>       mButtonInfos;
            MCore::String                   mName;

            Preset(const char* name);
            ~Preset();
        };

        static AnimGraphGameControllerSettings* Create();

        void AddPreset(Preset* preset);
        void RemovePreset(uint32 index);
        void SetNumPresets(uint32 numPresets);
        void SetPreset(uint32 index, Preset* preset);
        void Clear();

        void OnParameterNameChange(const char* oldName, const char* newName);

        Preset* GetPreset(uint32 index) const;
        uint32 GetNumPresets() const;

        uint32 GetActivePresetIndex() const;
        Preset* GetActivePreset() const;
        void SetActivePreset(Preset* preset);

        uint32 FindPresetIndexByName(const char* presetName) const;
        uint32 FindPresetIndex(Preset* preset) const;

        AnimGraphGameControllerSettings* Clone() const;

    private:
        MCore::Array<Preset*>   mPresets;
        uint32                  mActivePresetIndex;

        AnimGraphGameControllerSettings();
        ~AnimGraphGameControllerSettings();
    };
}   // namespace EMotionFX
