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

#include <MCore/Source/StandardHeaders.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include "EMStudioConfig.h"
#include <QWidget>


namespace EMStudio
{
    class EMSTUDIO_API MotionEventPreset
    {
        MCORE_MEMORYOBJECTCATEGORY(MotionEventPreset, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK);
    public:
        MotionEventPreset(const AZStd::string& eventType, const AZStd::string& eventParameter, const AZStd::string& mirrorType, uint32 color, uint32 eventID = MCORE_INVALIDINDEX32);
        ~MotionEventPreset() = default;

        const AZStd::string& GetEventType() const               { return mEventType; }
        const AZStd::string& GetMirrorType() const              { return mMirrorType; }
        const AZStd::string& GetEventParameter() const          { return mEventParameter; }
        uint32 GetEventColor() const                            { return mColor; }
        uint32 GetEventID() const                               { return mEventID; }
        bool GetIsDefault() const                               { return mIsDefault; }

        void SetEventType(const char* type)                     { mEventType = type; UpdateEventID(); }
        void SetMirrorType(const char* type)                    { mMirrorType = type; }
        void SetEventParameter(const char* parameter)           { mEventParameter = parameter; }
        void SetEventColor(uint32 color)                        { mColor = color; }
        void SetEventID(uint32 eventID)                         { mEventID = eventID; }
        void SetIsDefault(bool isDefault)                       { mIsDefault = isDefault; }

        void UpdateEventID();

    private:
        uint32              mEventID;
        AZStd::string       mEventType;
        AZStd::string       mMirrorType;
        AZStd::string       mEventParameter;
        uint32              mColor;
        bool                mIsDefault;
    };


    class EMSTUDIO_API MotionEventPresetManager
    {
        MCORE_MEMORYOBJECTCATEGORY(MotionEventPresetManager, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK);
    public:
        MotionEventPresetManager();
        ~MotionEventPresetManager();

        size_t GetNumPresets() const;
        bool IsEmpty() const;
        void AddPreset(MotionEventPreset* preset);
        void RemovePreset(uint32 index);
        MotionEventPreset* GetPreset(uint32 index) const;
        void Clear();

        void Load(const AZStd::string& filename);
        void Load()                                                             { Load(mFileName); }
        void LoadFromSettings();
        void SaveAs(const AZStd::string& filename, bool showNotification=true);
        void Save(bool showNotification=true)                                   { SaveAs(mFileName, showNotification); }

        bool GetIsDirty() const                                                 { return mDirtyFlag; }
        void SetDirtyFlag(bool isDirty)                                         { mDirtyFlag = isDirty; }
        const char* GetFileName() const                                         { return mFileName.c_str(); }
        const AZStd::string& GetFileNameString() const                          { return mFileName; }
        void SetFileName(const char* filename)                                  { mFileName = filename; }

        uint32 GetEventColor(const char* type, const char* parameter) const;
        uint32 GetEventColor(uint32 eventID) const;

    private:
        AZStd::vector<MotionEventPreset*>           mEventPresets;
        AZStd::string                               mFileName;
        bool                                        mDirtyFlag;
        static const AZ::u32                        m_unknownEventColor;

        void SaveToSettings();
        void CreateDefaultPresets();
        bool CheckIfHasPreset(const char* eventType) const;
    };
} // namespace EMStudio