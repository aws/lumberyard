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

#include <AudioWwiseLoader.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <IAudioSystemControl.h>
#include <IAudioSystemEditor.h>
#include <AudioSystemEditor_wwise.h>

#include <ISystem.h>
#include <CryFile.h>
#include <CryPath.h>
#include <Util/PathUtil.h>

using namespace PathUtil;

namespace AudioControls
{
    namespace WwiseStrings
    {
        // Files / Folders
        static constexpr const char* GameParametersFolder = "Game Parameters";
        static constexpr const char* GameStatesFolder = "States";
        static constexpr const char* SwitchesFolder = "Switches";
        static constexpr const char* EventsFolder = "Events";
        static constexpr const char* EnvironmentsFolder = "Master-Mixer Hierarchy";
        static constexpr const char* SoundBanksFolder = "sounds/wwise";
        static constexpr const char* SoundBankExt = ".bnk";
        static constexpr const char* InitBankName = "init.bnk";

        // Xml
        static constexpr const char* GameParameterTag = "GameParameter";
        static constexpr const char* EventTag = "Event";
        static constexpr const char* AuxBusTag = "AuxBus";
        static constexpr const char* SwitchGroupTag = "SwitchGroup";
        static constexpr const char* StateGroupTag = "StateGroup";
        static constexpr const char* ChildrenListTag = "ChildrenList";
        static constexpr const char* NameAttribute = "Name";

    } // namespace WwiseStrings

    //-------------------------------------------------------------------------------------------//
    void CAudioWwiseLoader::Load(CAudioSystemEditor_wwise* pAudioSystemImpl)
    {
        m_pAudioSystemImpl = pAudioSystemImpl;
        const AZStd::string wwiseProjectFullPath(m_pAudioSystemImpl->GetDataPath());
        LoadControlsInFolder(wwiseProjectFullPath + WwiseStrings::GameParametersFolder);
        LoadControlsInFolder(wwiseProjectFullPath + WwiseStrings::GameStatesFolder);
        LoadControlsInFolder(wwiseProjectFullPath + WwiseStrings::SwitchesFolder);
        LoadControlsInFolder(wwiseProjectFullPath + WwiseStrings::EventsFolder);
        LoadControlsInFolder(wwiseProjectFullPath + WwiseStrings::EnvironmentsFolder);
        LoadSoundBanks(WwiseStrings::SoundBanksFolder, "", false);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioWwiseLoader::LoadSoundBanks(const AZStd::string_view sRootFolder, const AZStd::string_view sSubPath, bool bLocalized)
    {
        _finddata_t fd;
        ICryPak* pCryPak = gEnv->pCryPak;
        AZStd::string search;
        AzFramework::StringFunc::Path::Join(sRootFolder.data(), sSubPath.data(), search);
        AzFramework::StringFunc::Path::Join(search.c_str(), "*.*", search, false, true, false);
        intptr_t handle = pCryPak->FindFirst(search.c_str(), &fd);
        if (handle != -1)
        {
            bool bLocalisedLoaded = bLocalized;
            const AZStd::string ignoreFilename = "init.bnk";
            do
            {
                AZStd::string sName = fd.name;
                if (!sName.empty() && sName != "." && sName != "..")
                {
                    if (fd.attrib & _A_SUBDIR)
                    {
                        if (!bLocalisedLoaded)
                        {
                            // each sub-folder represents a different language,
                            // we load only one as all of them should have the
                            // same content (in the future we want to have a
                            // consistency report to highlight if this is not the case)
                            m_sLocalizationFolder = sName;
                            LoadSoundBanks(sRootFolder, m_sLocalizationFolder, true);
                            bLocalisedLoaded = true;
                        }
                    }
                    else if (AzFramework::StringFunc::Find(sName.c_str(), ".bnk") != AZStd::string::npos
                        && !AzFramework::StringFunc::Equal(sName.c_str(), ignoreFilename.c_str()))
                    {
                        m_pAudioSystemImpl->CreateControl(SControlDef(sName, eWCT_WWISE_SOUND_BANK, bLocalized, nullptr, sSubPath));
                    }
                }
            }
            while (pCryPak->FindNext(handle, &fd) >= 0);
            pCryPak->FindClose(handle);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioWwiseLoader::LoadControlsInFolder(const AZStd::string_view sFolderPath)
    {
        _finddata_t fd;
        ICryPak* pCryPak = gEnv->pCryPak;
        AZStd::string search;
        AzFramework::StringFunc::Path::Join(sFolderPath.data(), "*.*", search, false, true, false);
        intptr_t handle = pCryPak->FindFirst(search.c_str(), &fd);
        if (handle != -1)
        {
            do
            {
                AZStd::string sName = fd.name;
                if (!sName.empty() && sName != "." && sName != "..")
                {
                    AZStd::string nextSearch;
                    AzFramework::StringFunc::Path::Join(sFolderPath.data(), sName.c_str(), nextSearch);

                    if (fd.attrib & _A_SUBDIR)
                    {
                        LoadControlsInFolder(nextSearch);
                    }
                    else
                    {
                        XmlNodeRef pRoot = GetISystem()->LoadXmlFromFile(nextSearch.c_str());
                        if (pRoot)
                        {
                            LoadControl(pRoot);
                        }
                    }
                }
            }
            while (pCryPak->FindNext(handle, &fd) >= 0);
            pCryPak->FindClose(handle);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioWwiseLoader::ExtractControlsFromXML(XmlNodeRef root, EWwiseControlTypes type, const AZStd::string_view controlTag, const AZStd::string_view controlNameAttribute)
    {
        AZStd::string_view xmlTag(root->getTag());
        if (xmlTag.compare(controlTag) == 0)
        {
            AZStd::string name = root->getAttr(controlNameAttribute.data());
            m_pAudioSystemImpl->CreateControl(SControlDef(name, type));
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioWwiseLoader::LoadControl(XmlNodeRef pRoot)
    {
        if (pRoot)
        {
            ExtractControlsFromXML(pRoot, eWCT_WWISE_RTPC, "GameParameter", "Name");
            ExtractControlsFromXML(pRoot, eWCT_WWISE_EVENT, "Event", "Name");
            ExtractControlsFromXML(pRoot, eWCT_WWISE_AUX_BUS, "AuxBus", "Name");

            // special case for switches
            AZStd::string_view tag(pRoot->getTag());
            bool bIsSwitch = tag.compare("SwitchGroup") == 0;
            bool bIsState = tag.compare("StateGroup") == 0;
            if (bIsSwitch || bIsState)
            {
                const AZStd::string sParent(pRoot->getAttr("Name"));
                IAudioSystemControl* pGroup = m_pAudioSystemImpl->GetControlByName(sParent);
                if (!pGroup)
                {
                    pGroup = m_pAudioSystemImpl->CreateControl(SControlDef(sParent, bIsSwitch ? eWCT_WWISE_SWITCH_GROUP : eWCT_WWISE_GAME_STATE_GROUP, false));
                }

                const XmlNodeRef pChildren = pRoot->findChild("ChildrenList");
                if (pChildren)
                {
                    const int size = pChildren->getChildCount();
                    for (int i = 0; i < size; ++i)
                    {
                        const XmlNodeRef pChild = pChildren->getChild(i);
                        if (pChild)
                        {
                            m_pAudioSystemImpl->CreateControl(SControlDef(pChild->getAttr("Name"), bIsSwitch ? eWCT_WWISE_SWITCH : eWCT_WWISE_GAME_STATE, false, pGroup));
                        }
                    }
                }
            }

            int size = pRoot->getChildCount();
            for (int i = 0; i < size; ++i)
            {
                LoadControl(pRoot->getChild(i));
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    const AZStd::string& CAudioWwiseLoader::GetLocalizationFolder() const
    {
        return m_sLocalizationFolder;
    }

} // namespace AudioControls
