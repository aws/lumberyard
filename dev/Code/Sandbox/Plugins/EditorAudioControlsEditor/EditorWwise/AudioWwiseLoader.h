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

#include <IXml.h>
#include "ACETypes.h"
#include "AudioSystemControl_wwise.h"

namespace AudioControls
{
    class CAudioSystemEditor_wwise;

    //-------------------------------------------------------------------------------------------//
    class CAudioWwiseLoader
    {
    public:
        CAudioWwiseLoader()
            : m_pAudioSystemImpl(nullptr)
        {
        }
        void Load(CAudioSystemEditor_wwise* pAudioSystemImpl);
        string GetLocalizationFolder() const;

    private:
        void LoadSoundBanks(const string& sRootFolder, const string& sSubPath, bool bLocalised);
        void LoadControlsInFolder(const string& sFolderPath);
        void LoadControl(XmlNodeRef root);
        void ExtractControlsFromXML(XmlNodeRef root, EWwiseControlTypes type, const string& controlTag, const string& controlNameAttribute);

    private:
        static const char* ms_sGameParametersFolder;
        static const char* ms_sGameStatesPath;
        static const char* ms_sSwitchesFolder;
        static const char* ms_sEventsFolder;
        static const char* ms_sEnvironmentsFolder;
        static const char* ms_sSoundBanksPath;

        string m_sLocalizationFolder;

        CAudioSystemEditor_wwise* m_pAudioSystemImpl;
    };
} // namespace AudioControls
