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

#include <ACETypes.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AudioSystemControl_wwise.h>

#include <IXml.h>

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
        const AZStd::string& GetLocalizationFolder() const;

    private:
        void LoadSoundBanks(const AZStd::string_view sRootFolder, const AZStd::string_view sSubPath, bool bLocalised);
        void LoadControlsInFolder(const AZStd::string_view sFolderPath);
        void LoadControl(XmlNodeRef root);
        void ExtractControlsFromXML(XmlNodeRef root, EWwiseControlTypes type, const AZStd::string_view controlTag, const AZStd::string_view controlNameAttribute);

    private:
        AZStd::string m_sLocalizationFolder;

        CAudioSystemEditor_wwise* m_pAudioSystemImpl;
    };
} // namespace AudioControls
