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
#include "AudioSystemEditor_wwise.h"
#include "AudioSystemControl_wwise.h"
#include <CryFile.h>
#include <ISystem.h>
#include "CryCrc32.h"
#include <ACETypes.h>
#include <CryPath.h>
#include <StlUtils.h>
#include <Util/PathUtil.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    TImplControlType TagToType(const string& tag)
    {
        if (tag == "WwiseSwitch")
        {
            return eWCT_WWISE_SWITCH_GROUP;
        }
        else if (tag == "WwiseState")
        {
            return eWCT_WWISE_GAME_STATE_GROUP;
        }
        else if (tag == "WwiseFile")
        {
            return eWCT_WWISE_SOUND_BANK;
        }
        else if (tag == "WwiseRtpc")
        {
            return eWCT_WWISE_RTPC;
        }
        else if (tag == "WwiseEvent")
        {
            return eWCT_WWISE_EVENT;
        }
        else if (tag == "WwiseAuxBus")
        {
            return eWCT_WWISE_AUX_BUS;
        }
        return eWCT_INVALID;
    }

    //-------------------------------------------------------------------------------------------//
    string TypeToTag(const TImplControlType type)
    {
        switch (type)
        {
        case eWCT_WWISE_EVENT:
            return "WwiseEvent";
        case eWCT_WWISE_RTPC:
            return "WwiseRtpc";
        case eWCT_WWISE_SWITCH:
            return "WwiseValue";
        case eWCT_WWISE_AUX_BUS:
            return "WwiseAuxBus";
        case eWCT_WWISE_SOUND_BANK:
            return "WwiseFile";
        case eWCT_WWISE_GAME_STATE:
            return "WwiseValue";
        case eWCT_WWISE_SWITCH_GROUP:
            return "WwiseSwitch";
        case eWCT_WWISE_GAME_STATE_GROUP:
            return "WwiseState";
        }
        return "";
    }

    //-------------------------------------------------------------------------------------------//
    CAudioSystemEditor_wwise::CAudioSystemEditor_wwise()
    {
        Reload();
    }

    //-------------------------------------------------------------------------------------------//
    CAudioSystemEditor_wwise::~CAudioSystemEditor_wwise()
    {
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioSystemEditor_wwise::Reload()
    {
        // set all the controls as placeholder as we don't know if
        // any of them have been removed but still have connections to them
        TControlMap savedControls;
        TControlMap::const_iterator it = m_controls.begin();
        TControlMap::const_iterator end = m_controls.end();
        for (; it != end; ++it)
        {
            TControlPtr pControl = it->second;
            if (pControl)
            {
                pControl->SetPlaceholder(true);
            }
        }

        // reload data
        m_loader.Load(this);

        m_connectionsByID.clear();
        UpdateConnectedStatus();
    }

    //-------------------------------------------------------------------------------------------//
    IAudioSystemControl* CAudioSystemEditor_wwise::CreateControl(const SControlDef& controlDefinition)
    {
        string sFullname = controlDefinition.sName;
        IAudioSystemControl* pParent = controlDefinition.pParent;
        if (pParent)
        {
            sFullname = controlDefinition.pParent->GetName() + "/" + sFullname;
        }

        if (!controlDefinition.sPath.empty())
        {
            sFullname = controlDefinition.sPath + "/" + sFullname;
        }

        CID nID = GetID(sFullname);

        IAudioSystemControl* pControl = GetControl(nID);
        if (pControl)
        {
            if (pControl->IsPlaceholder())
            {
                pControl->SetPlaceholder(false);
                if (pParent && pParent->IsPlaceholder())
                {
                    pParent->SetPlaceholder(false);
                }
            }
            return pControl;
        }
        else
        {
            TControlPtr pNewControl = std::make_shared<IAudioSystemControl_wwise>(controlDefinition.sName, nID, controlDefinition.eType);
            if (pParent == nullptr)
            {
                pParent = &m_rootControl;
            }

            if (controlDefinition.eType == eWCT_WWISE_SOUND_BANK)
            {
                pNewControl->SetName(sFullname);
            }

            pParent->AddChild(pNewControl.get());
            pNewControl->SetParent(pParent);
            pNewControl->SetLocalised(controlDefinition.bLocalised);
            m_controls[nID] = pNewControl;
            return pNewControl.get();
        }
    }

    //-------------------------------------------------------------------------------------------//
    IAudioSystemControl* CAudioSystemEditor_wwise::GetControl(CID id) const
    {
        if (id != ACE_INVALID_CID)
        {
            return stl::find_in_map(m_controls, id, TControlPtr()).get();
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    IAudioSystemControl* CAudioSystemEditor_wwise::GetControlByName(const string& sName, bool bIsLocalised, IAudioSystemControl* pParent) const
    {
        string sFullName = sName;
        if (pParent)
        {
            sFullName = pParent->GetName() + "/" + sFullName;
        }
        if (bIsLocalised)
        {
            sFullName = m_loader.GetLocalizationFolder() + "/" + sFullName;
        }
        return GetControl(GetID(sFullName));
    }

    //-------------------------------------------------------------------------------------------//
    TConnectionPtr CAudioSystemEditor_wwise::CreateConnectionToControl(EACEControlType eATLControlType, IAudioSystemControl* pMiddlewareControl)
    {
        if (pMiddlewareControl)
        {
            pMiddlewareControl->SetConnected(true);
            ++m_connectionsByID[pMiddlewareControl->GetId()];

            if (pMiddlewareControl->GetType() == eWCT_WWISE_RTPC)
            {
                switch (eATLControlType)
                {
                    case EACEControlType::eACET_RTPC:
                    {
                        return std::make_shared<CRtpcConnection>(pMiddlewareControl->GetId());
                    }
                    case EACEControlType::eACET_SWITCH_STATE:
                    {
                        return std::make_shared<CStateToRtpcConnection>(pMiddlewareControl->GetId());
                    }
                }
            }

            return std::make_shared<IAudioConnection>(pMiddlewareControl->GetId());
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    TConnectionPtr CAudioSystemEditor_wwise::CreateConnectionFromXMLNode(XmlNodeRef pNode, EACEControlType eATLControlType)
    {
        if (pNode)
        {
            const string sTag = pNode->getTag();
            TImplControlType type = TagToType(sTag);
            if (type != AUDIO_IMPL_INVALID_TYPE)
            {
                string sName = pNode->getAttr("wwise_name");
                string sLocalised = pNode->getAttr("wwise_localised");
                bool bLocalised = (sLocalised.compareNoCase("true") == 0);

                // If control not found, create a placeholder.
                // We want to keep that connection even if it's not in the middleware.
                // The user could be using the engine without the wwise project
                IAudioSystemControl* pControl = GetControlByName(sName, bLocalised);
                if (pControl == nullptr)
                {
                    pControl = CreateControl(SControlDef(sName, type));
                    if (pControl)
                    {
                        pControl->SetPlaceholder(true);
                        pControl->SetLocalised(bLocalised);
                    }
                }

                // If it's a switch we actually connect to one of the states within the switch
                if (type == eWCT_WWISE_SWITCH_GROUP || type == eWCT_WWISE_GAME_STATE_GROUP)
                {
                    if (pNode->getChildCount() == 1)
                    {
                        pNode = pNode->getChild(0);
                        if (pNode)
                        {
                            string sChildName = pNode->getAttr("wwise_name");

                            IAudioSystemControl* pChildControl = GetControlByName(sChildName, false, pControl);
                            if (pChildControl == nullptr)
                            {
                                pChildControl = CreateControl(SControlDef(sChildName, type == eWCT_WWISE_SWITCH_GROUP ? eWCT_WWISE_SWITCH : eWCT_WWISE_GAME_STATE, false, pControl));
                            }
                            pControl = pChildControl;
                        }
                    }
                    else
                    {
                        CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "Audio Controls Editor (Wwise): Error reading connection to Wwise control %s", sName);
                    }
                }

                if (pControl)
                {
                    pControl->SetConnected(true);
                    ++m_connectionsByID[pControl->GetId()];

                    if (type == eWCT_WWISE_RTPC)
                    {
                        switch (eATLControlType)
                        {
                            case EACEControlType::eACET_RTPC:
                            {
                                TRtpcConnectionPtr pConnection = std::make_shared<CRtpcConnection>(pControl->GetId());

                                float mult = 1.0f;
                                float shift = 0.0f;
                                if (pNode->haveAttr("atl_mult"))
                                {
                                    const string sProperty = pNode->getAttr("atl_mult");
                                    mult = (float)std::atof(sProperty.c_str());
                                }
                                if (pNode->haveAttr("atl_shift"))
                                {
                                    const string sProperty = pNode->getAttr("atl_shift");
                                    shift = (float)std::atof(sProperty.c_str());
                                }
                                pConnection->fMult = mult;
                                pConnection->fShift = shift;
                                return pConnection;
                            }
                            case EACEControlType::eACET_SWITCH_STATE:
                            {
                                TStateConnectionPtr pConnection = std::make_shared<CStateToRtpcConnection>(pControl->GetId());

                                float value = 0.0f;
                                if (pNode->haveAttr("wwise_value"))
                                {
                                    const string sProperty = pNode->getAttr("wwise_value");
                                    value = (float)std::atof(sProperty.c_str());
                                }
                                pConnection->fValue = value;
                                return pConnection;
                            }
                        }
                    }
                    else
                    {
                        return std::make_shared<IAudioConnection>(pControl->GetId());
                    }
                }
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    XmlNodeRef CAudioSystemEditor_wwise::CreateXMLNodeFromConnection(const TConnectionPtr pConnection, const EACEControlType eATLControlType)
    {
        const IAudioSystemControl* pControl = GetControl(pConnection->GetID());
        if (pControl)
        {
            switch (pControl->GetType())
            {
                case AudioControls::eWCT_WWISE_SWITCH:
                case AudioControls::eWCT_WWISE_SWITCH_GROUP:
                case AudioControls::eWCT_WWISE_GAME_STATE:
                case AudioControls::eWCT_WWISE_GAME_STATE_GROUP:
                {
                    const IAudioSystemControl* pParent = pControl->GetParent();
                    if (pParent)
                    {
                        XmlNodeRef pSwitchNode = GetISystem()->CreateXmlNode(TypeToTag(pParent->GetType()));
                        pSwitchNode->setAttr("wwise_name", pParent->GetName());

                        XmlNodeRef pStateNode = pSwitchNode->createNode("WwiseValue");
                        pStateNode->setAttr("wwise_name", pControl->GetName());
                        pSwitchNode->addChild(pStateNode);

                        return pSwitchNode;
                    }
                    break;
                }

                case AudioControls::eWCT_WWISE_RTPC:
                {
                    XmlNodeRef pConnectionNode;
                    pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()));
                    pConnectionNode->setAttr("wwise_name", pControl->GetName());

                    if (eATLControlType == eACET_RTPC)
                    {
                        std::shared_ptr<const CRtpcConnection> pRtpcConnection = std::static_pointer_cast<const CRtpcConnection>(pConnection);
                        if (pRtpcConnection->fMult != 1.0f)
                        {
                            pConnectionNode->setAttr("atl_mult", pRtpcConnection->fMult);
                        }
                        if (pRtpcConnection->fShift != 0.0f)
                        {
                            pConnectionNode->setAttr("atl_shift", pRtpcConnection->fShift);
                        }
                    }
                    else if (eATLControlType == eACET_SWITCH_STATE)
                    {
                        std::shared_ptr<const CStateToRtpcConnection> pStateConnection = std::static_pointer_cast<const CStateToRtpcConnection>(pConnection);
                        pConnectionNode->setAttr("wwise_value", pStateConnection->fValue);
                    }
                    return pConnectionNode;
                }

                case AudioControls::eWCT_WWISE_EVENT:
                {
                    XmlNodeRef pConnectionNode;
                    pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()));
                    pConnectionNode->setAttr("wwise_name", pControl->GetName());
                    return pConnectionNode;
                }

                case AudioControls::eWCT_WWISE_AUX_BUS:
                {
                    XmlNodeRef pConnectionNode;
                    pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()));
                    pConnectionNode->setAttr("wwise_name", pControl->GetName());
                    return pConnectionNode;
                }

                case AudioControls::eWCT_WWISE_SOUND_BANK:
                {
                    XmlNodeRef pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()));
                    pConnectionNode->setAttr("wwise_name", pControl->GetName());
                    if (pControl->IsLocalised())
                    {
                        pConnectionNode->setAttr("wwise_localised", "true");
                    }
                    return pConnectionNode;
                }
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    string CAudioSystemEditor_wwise::GetTypeIcon(TImplControlType type) const
    {
        switch (type)
        {
        case eWCT_WWISE_EVENT:
            return ":/AudioControlsEditor/Wwise/Icons/event_nor.png";
        case eWCT_WWISE_RTPC:
            return ":/AudioControlsEditor/Wwise/Icons/gameparameter_nor.png";
        case eWCT_WWISE_SWITCH:
            return ":/AudioControlsEditor/Wwise/Icons/switch_nor.png";
        case eWCT_WWISE_AUX_BUS:
            return ":/AudioControlsEditor/Wwise/Icons/auxbus_nor.png";
        case eWCT_WWISE_SOUND_BANK:
            return ":/AudioControlsEditor/Wwise/Icons/soundbank_nor.png";
        case eWCT_WWISE_GAME_STATE:
            return ":/AudioControlsEditor/Wwise/Icons/state_nor.png";
        case eWCT_WWISE_SWITCH_GROUP:
            return ":/AudioControlsEditor/Wwise/Icons/switchgroup_nor.png";
        case eWCT_WWISE_GAME_STATE_GROUP:
            return ":/AudioControlsEditor/Wwise/Icons/stategroup_nor.png";
        default:
            // should make a "default"/empty icon...
            return ":/AudioControlsEditor/Wwise/Icons/switchgroup_nor.png";
        }
    }

    //-------------------------------------------------------------------------------------------//
    AudioControls::EACEControlType CAudioSystemEditor_wwise::ImplTypeToATLType(TImplControlType type) const
    {
        switch (type)
        {
        case eWCT_WWISE_EVENT:
            return eACET_TRIGGER;
        case eWCT_WWISE_RTPC:
            return eACET_RTPC;
        case eWCT_WWISE_SWITCH:
        case eWCT_WWISE_GAME_STATE:
            return eACET_SWITCH_STATE;
        case eWCT_WWISE_AUX_BUS:
            return eACET_ENVIRONMENT;
        case eWCT_WWISE_SOUND_BANK:
            return eACET_PRELOAD;
        case eWCT_WWISE_GAME_STATE_GROUP:
        case eWCT_WWISE_SWITCH_GROUP:
            return eACET_SWITCH;
        }
        return eACET_NUM_TYPES;
    }

    //-------------------------------------------------------------------------------------------//
    AudioControls::TImplControlTypeMask CAudioSystemEditor_wwise::GetCompatibleTypes(EACEControlType eATLControlType) const
    {
        switch (eATLControlType)
        {
        case eACET_TRIGGER:
            return eWCT_WWISE_EVENT;
        case eACET_RTPC:
            return eWCT_WWISE_RTPC;
        case eACET_SWITCH:
            return AUDIO_IMPL_INVALID_TYPE;
        case eACET_SWITCH_STATE:
            return (eWCT_WWISE_SWITCH | eWCT_WWISE_GAME_STATE | eWCT_WWISE_RTPC);
        case eACET_ENVIRONMENT:
            return (eWCT_WWISE_AUX_BUS | eWCT_WWISE_SWITCH | eWCT_WWISE_GAME_STATE | eWCT_WWISE_RTPC);
        case eACET_PRELOAD:
            return eWCT_WWISE_SOUND_BANK;
        }
        return AUDIO_IMPL_INVALID_TYPE;
    }

    //-------------------------------------------------------------------------------------------//
    AudioControls::CID CAudioSystemEditor_wwise::GetID(const string& sName) const
    {
        return CCrc32::Compute(sName);
    }

    //-------------------------------------------------------------------------------------------//
    string CAudioSystemEditor_wwise::GetName() const
    {
        return "Wwise";
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioSystemEditor_wwise::UpdateConnectedStatus()
    {
        TConnectionsMap::iterator it = m_connectionsByID.begin();
        TConnectionsMap::iterator end = m_connectionsByID.end();
        for (; it != end; ++it)
        {
            if (it->second > 0)
            {
                IAudioSystemControl* pControl = GetControl(it->first);
                if (pControl)
                {
                    pControl->SetConnected(true);
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioSystemEditor_wwise::ConnectionRemoved(IAudioSystemControl* pControl)
    {
        int nConnectionCount = m_connectionsByID[pControl->GetId()] - 1;
        if (nConnectionCount <= 0)
        {
            nConnectionCount = 0;
            pControl->SetConnected(false);
        }
        m_connectionsByID[pControl->GetId()] = nConnectionCount;
    }

    //-------------------------------------------------------------------------------------------//
    string CAudioSystemEditor_wwise::GetDataPath() const
    {
        return (Path::GetEditingGameDataFolder() + "/sounds/wwise_project/").c_str();
    }
} // namespace AudioControls
