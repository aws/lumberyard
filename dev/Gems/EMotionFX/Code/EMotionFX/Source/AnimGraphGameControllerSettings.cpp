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

// include required headers
#include "AnimGraphGameControllerSettings.h"


namespace EMotionFX
{
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // constructor
    AnimGraphGameControllerSettings::ParameterInfo::ParameterInfo(const char* parameterName)
    {
        mParameterName  = parameterName;
        mAxis           = MCORE_INVALIDINDEX8;
        mMode           = PARAMMODE_STANDARD;
        mInvert         = true;
        mEnabled        = true;
    }

    // clone parameter info
    AnimGraphGameControllerSettings::ParameterInfo* AnimGraphGameControllerSettings::ParameterInfo::Clone() const
    {
        ParameterInfo* result = new ParameterInfo(mParameterName.AsChar());
        result->mMode       = mMode;
        result->mInvert     = mInvert;
        result->mEnabled    = mEnabled;
        result->mAxis       = mAxis;
        return result;
    }


    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // constructor
    AnimGraphGameControllerSettings::ButtonInfo::ButtonInfo(uint32 buttonIndex)
    {
        mButtonIndex    = buttonIndex;
        mMode           = AnimGraphGameControllerSettings::BUTTONMODE_NONE;
        mOldIsPressed   = false;
        mEnabled        = true;
    }

    // clone button info
    AnimGraphGameControllerSettings::ButtonInfo* AnimGraphGameControllerSettings::ButtonInfo::Clone() const
    {
        ButtonInfo* result = new ButtonInfo(mButtonIndex);
        result->mMode           = mMode;
        result->mString         = mString;
        result->mOldIsPressed   = mOldIsPressed;
        result->mEnabled        = mEnabled;
        return result;
    }


    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AnimGraphGameControllerSettings::Preset* AnimGraphGameControllerSettings::Preset::Clone() const
    {
        Preset* result = new Preset(mName.AsChar());

        const uint32 numParameterInfos = mParameterInfos.GetLength();
        result->mParameterInfos.Resize(numParameterInfos);
        for (uint32 i = 0; i < numParameterInfos; ++i)
        {
            result->mParameterInfos[i] = mParameterInfos[i]->Clone();
        }

        const uint32 numButtonInfos = mButtonInfos.GetLength();
        result->mButtonInfos.Resize(numButtonInfos);
        for (uint32 i = 0; i < numButtonInfos; ++i)
        {
            result->mButtonInfos[i] = mButtonInfos[i]->Clone();
        }

        return result;
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // constructor
    AnimGraphGameControllerSettings::Preset::Preset(const char* name)
        : BaseObject()
    {
        mName = name;
        mParameterInfos.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_GAMECONTROLLER);
        mButtonInfos.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_GAMECONTROLLER);
    }


    // destructor
    AnimGraphGameControllerSettings::Preset::~Preset()
    {
        Clear();
    }


    // create
    AnimGraphGameControllerSettings::Preset* AnimGraphGameControllerSettings::Preset::Create(const char* name)
    {
        return new AnimGraphGameControllerSettings::Preset(name);
    }


    // get rid of all parameter infos
    void AnimGraphGameControllerSettings::Preset::Clear()
    {
        // get rid of the parameter infos
        const uint32 numParameterInfos = mParameterInfos.GetLength();
        for (uint32 i = 0; i < numParameterInfos; ++i)
        {
            delete mParameterInfos[i];
        }
        mParameterInfos.Clear();

        // get rid of the button infos
        const uint32 numButtonInfos = mButtonInfos.GetLength();
        for (uint32 i = 0; i < numButtonInfos; ++i)
        {
            delete mButtonInfos[i];
        }
        mButtonInfos.Clear();
    }


    // find parameter info by name and create one if it doesn't exist yet
    AnimGraphGameControllerSettings::ParameterInfo* AnimGraphGameControllerSettings::Preset::FindParameterInfo(const char* parameterName)
    {
        // get the number of parameter infos and iterate through them
        const uint32 numParameterInfos = mParameterInfos.GetLength();
        for (uint32 i = 0; i < numParameterInfos; ++i)
        {
            // check if we have already created a parameter info for the given parameter
            ParameterInfo* parameterInfo = mParameterInfos[i];
            if (parameterInfo->mParameterName.CheckIfIsEqual(parameterName))
            {
                return parameterInfo;
            }
        }

        // create a new parameter info
        ParameterInfo* parameterInfo = new ParameterInfo(parameterName);
        mParameterInfos.Add(parameterInfo);
        return parameterInfo;
    }


    // find the button info by index and create one if it doesn't exist
    AnimGraphGameControllerSettings::ButtonInfo* AnimGraphGameControllerSettings::Preset::FindButtonInfo(uint32 buttonIndex)
    {
        // get the number of buttons infos and iterate through them
        const uint32 numButtonInfos = mButtonInfos.GetLength();
        for (uint32 i = 0; i < numButtonInfos; ++i)
        {
            // check if we have already created a button info for the given button
            ButtonInfo* buttonInfo = mButtonInfos[i];
            if (mButtonInfos[i]->mButtonIndex == buttonIndex)
            {
                return buttonInfo;
            }
        }

        // create a new button info
        ButtonInfo* buttonInfo = new ButtonInfo(buttonIndex);
        mButtonInfos.Add(buttonInfo);
        return buttonInfo;
    }


    // check if the parameter with the given name is being controlled by the gamepad
    bool AnimGraphGameControllerSettings::Preset::CheckIfIsParameterButtonControlled(const char* stringName)
    {
        // get the number of buttons infos and iterate through them
        const uint32 numButtonInfos = mButtonInfos.GetLength();
        for (uint32 i = 0; i < numButtonInfos; ++i)
        {
            // check if the button info is linked to the given string
            ButtonInfo* buttonInfo = mButtonInfos[i];
            if (buttonInfo->mString.CheckIfIsEqual(stringName))
            {
                // return success in case this button info isn't set to the mode none
                if (buttonInfo->mMode != BUTTONMODE_NONE)
                {
                    return true;
                }
            }
        }

        // failure, not found
        return false;
    }


    // check if any of the button infos that are linked to the given string name is enabled
    bool AnimGraphGameControllerSettings::Preset::CheckIfIsButtonEnabled(const char* stringName)
    {
        // get the number of buttons infos and iterate through them
        const uint32 numButtonInfos = mButtonInfos.GetLength();
        for (uint32 i = 0; i < numButtonInfos; ++i)
        {
            // check if the button info is linked to the given string
            ButtonInfo* buttonInfo = mButtonInfos[i];
            if (buttonInfo->mString.CheckIfIsEqual(stringName))
            {
                // return success in case this button info is enabled
                if (buttonInfo->mEnabled)
                {
                    return true;
                }
            }
        }

        // failure, not found
        return false;
    }


    // set all button infos that are linked to the given string name to the enabled flag
    void AnimGraphGameControllerSettings::Preset::SetButtonEnabled(const char* stringName, bool isEnabled)
    {
        // get the number of buttons infos and iterate through them
        const uint32 numButtonInfos = mButtonInfos.GetLength();
        for (uint32 i = 0; i < numButtonInfos; ++i)
        {
            // check if the button info is linked to the given string and set the enabled flag in case it is linked to the given string name
            ButtonInfo* buttonInfo = mButtonInfos[i];
            if (buttonInfo->mString.CheckIfIsEqual(stringName))
            {
                buttonInfo->mEnabled = isEnabled;
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // constructor
    AnimGraphGameControllerSettings::AnimGraphGameControllerSettings()
        : BaseObject()
    {
        mPresets.SetMemoryCategory(EMFX_MEMCATEGORY_ANIMGRAPH_GAMECONTROLLER);
        mActivePresetIndex = MCORE_INVALIDINDEX32;
    }


    // destructor
    AnimGraphGameControllerSettings::~AnimGraphGameControllerSettings()
    {
        Clear();
    }


    // create
    AnimGraphGameControllerSettings* AnimGraphGameControllerSettings::Create()
    {
        return new AnimGraphGameControllerSettings();
    }


    // add a preset to the settings
    void AnimGraphGameControllerSettings::AddPreset(Preset* preset)
    {
        mPresets.Add(preset);
    }


    // remove the given preset
    void AnimGraphGameControllerSettings::RemovePreset(uint32 index)
    {
        mPresets.Remove(index);
    }


    // preallocate memory for the presets
    void AnimGraphGameControllerSettings::SetNumPresets(uint32 numPresets)
    {
        mPresets.Resize(numPresets);
    }


    // set the preset at the given index in the preset array
    void AnimGraphGameControllerSettings::SetPreset(uint32 index, Preset* preset)
    {
        mPresets[index] = preset;
    }


    // get rid of all presets
    void AnimGraphGameControllerSettings::Clear()
    {
        // get the number of presets and iterate through them
        const uint32 numPresets = mPresets.GetLength();
        for (uint32 i = 0; i < numPresets; ++i)
        {
            mPresets[i]->Destroy();
        }
        mPresets.Clear();
    }


    // find the preset by name
    uint32 AnimGraphGameControllerSettings::FindPresetIndexByName(const char* presetName) const
    {
        // get the number of presets and iterate through them
        const uint32 numPresets = mPresets.GetLength();
        for (uint32 i = 0; i < numPresets; ++i)
        {
            // compare the names and return the index in case they are equal
            if (mPresets[i]->GetNameString().CheckIfIsEqual(presetName))
            {
                return i;
            }
        }

        // return failure
        return MCORE_INVALIDINDEX32;
    }


    // find the index for a given preset
    uint32 AnimGraphGameControllerSettings::FindPresetIndex(Preset* preset) const
    {
        // get the number of presets and iterate through them
        const uint32 numPresets = mPresets.GetLength();
        for (uint32 i = 0; i < numPresets; ++i)
        {
            // compare the preset pointers and return the index in case they are equal
            if (mPresets[i] == preset)
            {
                return i;
            }
        }

        // return failure
        return MCORE_INVALIDINDEX32;
    }


    void AnimGraphGameControllerSettings::SetActivePreset(Preset* preset)
    {
        mActivePresetIndex = FindPresetIndex(preset);
    }


    uint32 AnimGraphGameControllerSettings::GetActivePresetIndex() const
    {
        if (mActivePresetIndex < mPresets.GetLength())
        {
            return mActivePresetIndex;
        }

        return MCORE_INVALIDINDEX32;
    }


    AnimGraphGameControllerSettings::Preset* AnimGraphGameControllerSettings::GetActivePreset() const
    {
        if (mActivePresetIndex < mPresets.GetLength())
        {
            return mPresets[mActivePresetIndex];
        }

        return nullptr;
    }


    void AnimGraphGameControllerSettings::OnParameterNameChange(const char* oldName, const char* newName)
    {
        // check if we have any preset to save
        const uint32 numPresets = mPresets.GetLength();
        if (numPresets == 0)
        {
            return;
        }

        // get the number of presets and iterate through them
        for (uint32 i = 0; i < numPresets; ++i)
        {
            // get the preset
            Preset* preset = mPresets[i];

            // loop trough all parameter infos
            const uint32 numParameterInfos = preset->GetNumParamInfos();
            for (uint32 j = 0; j < numParameterInfos; ++j)
            {
                ParameterInfo* parameterInfo = preset->GetParamInfo(j);
                if (parameterInfo == nullptr)
                {
                    continue;
                }

                // compare the names and replace them in case they are equal
                if (parameterInfo->mParameterName.CheckIfIsEqual(oldName))
                {
                    parameterInfo->mParameterName = newName;
                }
            }
        }
    }


    // clone the settings
    AnimGraphGameControllerSettings* AnimGraphGameControllerSettings::Clone() const
    {
        AnimGraphGameControllerSettings* result = new AnimGraphGameControllerSettings();

        // copy easy stuff
        result->mActivePresetIndex = mActivePresetIndex;

        // copy the presets
        const uint32 numPresets = mPresets.GetLength();
        result->mPresets.Resize(numPresets);
        for (uint32 i = 0; i < numPresets; ++i)
        {
            result->mPresets[i] = mPresets[i]->Clone();
        }

        return result;
    }


    AnimGraphGameControllerSettings::Preset* AnimGraphGameControllerSettings::GetPreset(uint32 index) const
    {
        return mPresets[index];
    }


    uint32 AnimGraphGameControllerSettings::GetNumPresets() const
    {
        return mPresets.GetLength();
    }
}   // namespace EMotionFX

