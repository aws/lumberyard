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

#include "StdAfx.h"
#include "AudioWwiseLoader.h"
#include "CryFile.h"
#include "ISystem.h"
#include "CryPath.h"
#include "Util/PathUtil.h"
#include "IAudioSystemEditor.h"
#include <IAudioSystemControl.h>
#include "AudioSystemEditor_wwise.h"

using namespace PathUtil;

namespace AudioControls
{
    const char* CAudioWwiseLoader::ms_sGameParametersFolder = "Game Parameters";
    const char* CAudioWwiseLoader::ms_sGameStatesPath = "States";
    const char* CAudioWwiseLoader::ms_sSwitchesFolder = "Switches";
    const char* CAudioWwiseLoader::ms_sEventsFolder = "Events";
    const char* CAudioWwiseLoader::ms_sEnvironmentsFolder = "Master-Mixer Hierarchy";
    const char* CAudioWwiseLoader::ms_sSoundBanksPath = "sounds/wwise";

    //-------------------------------------------------------------------------------------------//
    void CAudioWwiseLoader::Load(CAudioSystemEditor_wwise* pAudioSystemImpl)
    {
        m_pAudioSystemImpl = pAudioSystemImpl;
        const string wwiseProjectFullPath(m_pAudioSystemImpl->GetDataPath());
        LoadControlsInFolder(wwiseProjectFullPath + ms_sGameParametersFolder);
        LoadControlsInFolder(wwiseProjectFullPath + ms_sGameStatesPath);
        LoadControlsInFolder(wwiseProjectFullPath + ms_sSwitchesFolder);
        LoadControlsInFolder(wwiseProjectFullPath + ms_sEventsFolder);
        LoadControlsInFolder(wwiseProjectFullPath + ms_sEnvironmentsFolder);
        LoadSoundBanks(ms_sSoundBanksPath, "", false);
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioWwiseLoader::LoadSoundBanks(const string& sRootFolder, const string& sSubPath, bool bLocalised)
    {
        _finddata_t fd;
        ICryPak* pCryPak = gEnv->pCryPak;
        intptr_t handle = pCryPak->FindFirst(sRootFolder + "/" + sSubPath + "/*.*", &fd);
        if (handle != -1)
        {
            bool bLocalisedLoaded = bLocalised;
            const string ignoreFilename = "Init.bnk";
            do
            {
                string sName = fd.name;
                if (sName != "." && sName != ".." && !sName.empty())
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
                    else if (sName.find(".bnk") != string::npos && sName.compareNoCase(ignoreFilename) != 0)
                    {
                        m_pAudioSystemImpl->CreateControl(SControlDef(sName, eWCT_WWISE_SOUND_BANK, bLocalised, nullptr, sSubPath));
                    }
                }
            }
            while (pCryPak->FindNext(handle, &fd) >= 0);
            pCryPak->FindClose(handle);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioWwiseLoader::LoadControlsInFolder(const string& sFolderPath)
    {
        _finddata_t fd;
        ICryPak* pCryPak = gEnv->pCryPak;
        intptr_t handle = pCryPak->FindFirst(sFolderPath + "/*.*", &fd);
        if (handle != -1)
        {
            do
            {
                string sName = fd.name;
                if (sName != "." && sName != ".." && !sName.empty())
                {
                    if (fd.attrib & _A_SUBDIR)
                    {
                        LoadControlsInFolder(sFolderPath + "/" + sName);
                    }
                    else
                    {
                        string sFilename = sFolderPath + "/" + sName;
                        XmlNodeRef pRoot = GetISystem()->LoadXmlFromFile(sFilename);
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
    void CAudioWwiseLoader::ExtractControlsFromXML(XmlNodeRef root, EWwiseControlTypes type, const string& controlTag, const string& controlNameAttribute)
    {
        string xmlTag = root->getTag();
        if (xmlTag.compare(controlTag) == 0)
        {
            string name = root->getAttr(controlNameAttribute);
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
            string tag = pRoot->getTag();
            bool bIsSwitch = tag.compare("SwitchGroup") == 0;
            bool bIsState = tag.compare("StateGroup") == 0;
            if (bIsSwitch || bIsState)
            {
                const string sParent = pRoot->getAttr("Name");
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
    string CAudioWwiseLoader::GetLocalizationFolder() const
    {
        return m_sLocalizationFolder;
    }

} // namespace AudioControls
