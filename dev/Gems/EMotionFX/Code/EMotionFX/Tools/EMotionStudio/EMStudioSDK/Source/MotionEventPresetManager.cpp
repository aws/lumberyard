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

#include "MotionEventPresetManager.h"
#include <AzCore/Math/Color.h>
#include "EMStudioManager.h"
#include "RenderPlugin/RenderOptions.h"
#include <MCore/Source/Color.h>
#include <MCore/Source/LogManager.h>
#include <EMotionFX/Source/EventManager.h>
#include <MysticQt/Source/MysticQtManager.h>
#include <QSettings>


namespace EMStudio
{
    MotionEventPreset::MotionEventPreset(const AZStd::string& eventType, const AZStd::string& eventParameter, const AZStd::string& mirrorType, uint32 color, uint32 eventID)
        : mEventType(eventType)
        , mMirrorType(mirrorType)
        , mEventParameter(eventParameter)
        , mEventID(eventID)
        , mColor(color)
        , mIsDefault(false)
    {
        UpdateEventID();
    }


    // update the event ID based on the name
    void MotionEventPreset::UpdateEventID()
    {
        mEventID = EMotionFX::GetEventManager().FindEventID(mEventType.c_str());

        // if its an unknown ID, see if we should register it and then try to get the updated ID
        if (mEventID == MCORE_INVALIDINDEX32)
        {
            if (!mEventType.empty())
            {
                mEventID = EMotionFX::GetEventManager().RegisterEventType(mEventType.c_str());
            }
        }
    }

    //-----------------------------------

    const uint32 MotionEventPresetManager::m_unknownEventColor = MCore::RGBA(193, 195, 196, 255);

    MotionEventPresetManager::MotionEventPresetManager()
        : mDirtyFlag(false)
    {
        mFileName = GetManager()->GetAppDataFolder() + "EMStudioDefaultEventPresets.cfg";
    }


    MotionEventPresetManager::~MotionEventPresetManager()
    {
        SaveToSettings();
        Save(false);
        Clear();
    }


    void MotionEventPresetManager::Clear()
    {
        for (MotionEventPreset* eventPreset : mEventPresets)
        {
            delete eventPreset;
        }

        mEventPresets.clear();
    }


    size_t MotionEventPresetManager::GetNumPresets() const
    {
        return mEventPresets.size();
    }


    bool MotionEventPresetManager::IsEmpty() const
    {
        return mEventPresets.empty(); 
    }


    void MotionEventPresetManager::AddPreset(MotionEventPreset* preset)
    {
        mEventPresets.push_back(preset);
        mDirtyFlag = true;
    }


    void MotionEventPresetManager::RemovePreset(uint32 index)
    {
        delete mEventPresets[index];
        mEventPresets.erase(mEventPresets.begin() + index);
        mDirtyFlag = true;
    }


    MotionEventPreset* MotionEventPresetManager::GetPreset(uint32 index) const
    {
        return mEventPresets[index];
    }


    void MotionEventPresetManager::CreateDefaultPresets()
    {
        const size_t startEvent = mEventPresets.size();
        mEventPresets.push_back(new MotionEventPreset("LeftFoot",    "",     "RightFoot",    MCore::RGBA(255, 0, 0)));
        mEventPresets.push_back(new MotionEventPreset("RightFoot",   "",     "LeftFoot",     MCore::RGBA(0, 255, 0)));

        // Mark the events as defaults.
        for (MotionEventPreset* eventPreset : mEventPresets)
        {
            eventPreset->SetIsDefault(true);
        }
    }
    
    void MotionEventPresetManager::Load(const AZStd::string& filename)
    {
        mFileName = filename;

        // Clear the old event presets.
        Clear();
        CreateDefaultPresets();

        QSettings settings(mFileName.c_str(), QSettings::IniFormat, GetManager()->GetMainWindow());

        const uint32 numMotionEventPresets = settings.value("numMotionEventPresets").toInt();
        if (numMotionEventPresets == 0)
        {
            return;
        }

        // Add the new motion event presets.
        AZStd::string eventType;
        AZStd::string mirrorType;
        AZStd::string eventParameter;
        for (uint32 i = 0; i < numMotionEventPresets; ++i)
        {
            settings.beginGroup(AZStd::string::format("%i", i).c_str());
            const MCore::RGBAColor color  = RenderOptions::StringToColor(settings.value("MotionEventPresetColor").toString());

            eventType       = settings.value("MotionEventPresetType").toString().toUtf8().data();
            mirrorType      = settings.value("MotionEventPresetMirrorType").toString().toUtf8().data();
            eventParameter  = settings.value("MotionEventPresetParameter").toString().toUtf8().data();

            settings.endGroup();
                
            MotionEventPreset* preset = new MotionEventPreset(eventType, eventParameter, mirrorType, color.ToInt());
            AddPreset(preset);
        }

        mDirtyFlag = false;

        // Update the default preset settings filename so that next startup the presets get auto-loaded.
        SaveToSettings();
    }


    void MotionEventPresetManager::SaveAs(const AZStd::string& filename, bool showNotification)
    {
        mFileName = filename;

        QSettings settings(mFileName.c_str(), QSettings::IniFormat, GetManager()->GetMainWindow());

        size_t numEventPresets = 0;

        // Remove the default ones
        for (MotionEventPreset* eventPreset : mEventPresets)
        {
            if (!eventPreset->GetIsDefault())
            {
                ++numEventPresets;
            }
        }
        {
            // QVariant doesnt like direct conversions from size_t in Mac
            QVariant vNumEventPresets;
            vNumEventPresets.setValue(numEventPresets);
            settings.setValue("numMotionEventPresets", vNumEventPresets);
        }
        
        // Loop trough all entries and save them to the settings file.
        size_t presetIndex = 0;
        for (MotionEventPreset* eventPreset : mEventPresets)
        {
            if (!eventPreset || eventPreset->GetIsDefault())
            {
                continue;
            }

            settings.beginGroup(AZStd::string::format("%i", presetIndex).c_str());
            settings.setValue("MotionEventPresetColor", RenderOptions::ColorToString(MCore::RGBAColor(eventPreset->GetEventColor())));
            settings.setValue("MotionEventPresetType", eventPreset->GetEventType().c_str());
            settings.setValue("MotionEventPresetMirrorType", eventPreset->GetMirrorType().c_str());
            settings.setValue("MotionEventPresetParameter", eventPreset->GetEventParameter().c_str());
            settings.endGroup();

            ++presetIndex;
        }

        // Sync to ensure the status is correct because qt delays the write to file.
        settings.sync();

        // Check if the settings correctly saved.
        if (settings.status() == QSettings::NoError)
        {
            mDirtyFlag = false;

            if (showNotification)
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS, "Motion event presets <font color=green>successfully</font> saved");
            }
        }
        else
        {
            if (showNotification)
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR, "Motion event presets <font color=red>failed</font> to save");
            }
        }

        // Update the default preset settings filename so that next startup the presets get auto-loaded.
        SaveToSettings();
    }


    void MotionEventPresetManager::SaveToSettings()
    {
        QSettings settings(GetManager()->GetMainWindow());
        settings.beginGroup("EMotionFX");
        settings.setValue("lastEventPresetFile", mFileName.c_str());
        settings.endGroup();
    }


    void MotionEventPresetManager::LoadFromSettings()
    {
        QSettings settings(GetManager()->GetMainWindow());
        settings.beginGroup("EMotionFX");
        mFileName = FromQtString(settings.value("lastEventPresetFile", QVariant("")).toString());
        settings.endGroup();
    }


    // Check if motion event with this configuration exists and return color.
    uint32 MotionEventPresetManager::GetEventColor(const char* type, const char* parameter) const
    {
        for (const MotionEventPreset* preset : mEventPresets)
        {
            // If the type and parameter are equal, return the color.
            if (preset->GetEventType() == type && preset->GetEventParameter() == parameter)
            {
                return preset->GetEventColor();
            }
        }

        // Use the same color for all events that are not from a preset.
        return m_unknownEventColor;
    }


    // Get the color for the first event with a given ID, regardless of its parameter.
    uint32 MotionEventPresetManager::GetEventColor(uint32 eventID) const
    {
        for (const MotionEventPreset* preset : mEventPresets)
        {
            if (preset->GetEventID() == eventID)
            {
                return preset->GetEventColor();
            }
        }

        // Use the same color for all events that are not from a preset.
        return m_unknownEventColor;
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MotionEventPresetManager.moc>