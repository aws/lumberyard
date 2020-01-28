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

#include <AudioSystemEditor_wwise.h>

#include <AzCore/Math/Crc.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <ACETypes.h>
#include <AudioSystemControl_wwise.h>
#include <Common_wwise.h>

#include <ISystem.h>
#include <CryFile.h>
#include <CryPath.h>
#include <Util/PathUtil.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    TImplControlType TagToType(const AZStd::string_view tag)
    {
        if (tag == Audio::WwiseXmlTags::WwiseEventTag)
        {
            return eWCT_WWISE_EVENT;
        }
        else if (tag == Audio::WwiseXmlTags::WwiseRtpcTag)
        {
            return eWCT_WWISE_RTPC;
        }
        else if (tag == Audio::WwiseXmlTags::WwiseAuxBusTag)
        {
            return eWCT_WWISE_AUX_BUS;
        }
        else if (tag == Audio::WwiseXmlTags::WwiseFileTag)
        {
            return eWCT_WWISE_SOUND_BANK;
        }
        else if (tag == Audio::WwiseXmlTags::WwiseSwitchTag)
        {
            return eWCT_WWISE_SWITCH_GROUP;
        }
        else if (tag == Audio::WwiseXmlTags::WwiseStateTag)
        {
            return eWCT_WWISE_GAME_STATE_GROUP;
        }
        return eWCT_INVALID;
    }

    //-------------------------------------------------------------------------------------------//
    const AZStd::string_view TypeToTag(const TImplControlType type)
    {
        switch (type)
        {
        case eWCT_WWISE_EVENT:
            return Audio::WwiseXmlTags::WwiseEventTag;
        case eWCT_WWISE_RTPC:
            return Audio::WwiseXmlTags::WwiseRtpcTag;
        case eWCT_WWISE_SWITCH:
            return Audio::WwiseXmlTags::WwiseValueTag;
        case eWCT_WWISE_AUX_BUS:
            return Audio::WwiseXmlTags::WwiseAuxBusTag;
        case eWCT_WWISE_SOUND_BANK:
            return Audio::WwiseXmlTags::WwiseFileTag;
        case eWCT_WWISE_GAME_STATE:
            return Audio::WwiseXmlTags::WwiseValueTag;
        case eWCT_WWISE_SWITCH_GROUP:
            return Audio::WwiseXmlTags::WwiseSwitchTag;
        case eWCT_WWISE_GAME_STATE_GROUP:
            return Audio::WwiseXmlTags::WwiseStateTag;
        }
        return "";
    }

    //-------------------------------------------------------------------------------------------//
    CAudioSystemEditor_wwise::CAudioSystemEditor_wwise()
    {
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
        for (const auto& idControlPair : m_controls)
        {
            TControlPtr pControl = idControlPair.second;
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
        AZStd::string sFullname = controlDefinition.sName;
        IAudioSystemControl* pParent = controlDefinition.pParent;
        if (pParent)
        {
            AzFramework::StringFunc::Path::Join(controlDefinition.pParent->GetName().c_str(), sFullname.c_str(), sFullname);
        }

        if (!controlDefinition.sPath.empty())
        {
            AzFramework::StringFunc::Path::Join(controlDefinition.sPath.c_str(), sFullname.c_str(), sFullname);
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
            TControlPtr pNewControl = AZStd::make_shared<IAudioSystemControl_wwise>(controlDefinition.sName, nID, controlDefinition.eType);
            if (pParent == nullptr)
            {
                pParent = &m_rootControl;
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
            auto it = m_controls.find(id);
            if (it != m_controls.end())
            {
                return it->second.get();
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    IAudioSystemControl* CAudioSystemEditor_wwise::GetControlByName(AZStd::string sFullname, bool bIsLocalised, IAudioSystemControl* pParent) const
    {
        if (pParent)
        {
            AzFramework::StringFunc::Path::Join(pParent->GetName().c_str(), sFullname.c_str(), sFullname);
        }

        if (bIsLocalised)
        {
            AzFramework::StringFunc::Path::Join(m_loader.GetLocalizationFolder().c_str(), sFullname.c_str(), sFullname);
        }
        return GetControl(GetID(sFullname));
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
                        return AZStd::make_shared<CRtpcConnection>(pMiddlewareControl->GetId());
                    }
                    case EACEControlType::eACET_SWITCH_STATE:
                    {
                        return AZStd::make_shared<CStateToRtpcConnection>(pMiddlewareControl->GetId());
                    }
                }
            }

            return AZStd::make_shared<IAudioConnection>(pMiddlewareControl->GetId());
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    TConnectionPtr CAudioSystemEditor_wwise::CreateConnectionFromXMLNode(XmlNodeRef pNode, EACEControlType eATLControlType)
    {
        if (pNode)
        {
            const AZStd::string sTag(pNode->getTag());
            TImplControlType type = TagToType(sTag);
            if (type != AUDIO_IMPL_INVALID_TYPE)
            {
                AZStd::string sName(pNode->getAttr(Audio::WwiseXmlTags::WwiseNameAttribute));
                AZStd::string sLocalized(pNode->getAttr(Audio::WwiseXmlTags::WwiseLocalizedAttribute));
                bool bLocalized = AzFramework::StringFunc::Equal(sLocalized.c_str(), "true");

                // If control not found, create a placeholder.
                // We want to keep that connection even if it's not in the middleware.
                // The user could be using the engine without the wwise project
                IAudioSystemControl* pControl = GetControlByName(sName, bLocalized);
                if (pControl == nullptr)
                {
                    pControl = CreateControl(SControlDef(sName, type));
                    if (pControl)
                    {
                        pControl->SetPlaceholder(true);
                        pControl->SetLocalised(bLocalized);
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
                            AZStd::string sChildName(pNode->getAttr(Audio::WwiseXmlTags::WwiseNameAttribute));

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
                                TRtpcConnectionPtr pConnection = AZStd::make_shared<CRtpcConnection>(pControl->GetId());

                                float mult = 1.0f;
                                float shift = 0.0f;
                                if (pNode->haveAttr(Audio::WwiseXmlTags::WwiseMutiplierAttribute))
                                {
                                    const AZStd::string sProperty(pNode->getAttr(Audio::WwiseXmlTags::WwiseMutiplierAttribute));
                                    mult = AZStd::stof(sProperty);
                                }
                                if (pNode->haveAttr(Audio::WwiseXmlTags::WwiseShiftAttribute))
                                {
                                    const AZStd::string sProperty(pNode->getAttr(Audio::WwiseXmlTags::WwiseShiftAttribute));
                                    shift = AZStd::stof(sProperty);
                                }
                                pConnection->fMult = mult;
                                pConnection->fShift = shift;
                                return pConnection;
                            }
                            case EACEControlType::eACET_SWITCH_STATE:
                            {
                                TStateConnectionPtr pConnection = AZStd::make_shared<CStateToRtpcConnection>(pControl->GetId());

                                float value = 0.0f;
                                if (pNode->haveAttr(Audio::WwiseXmlTags::WwiseValueAttribute))
                                {
                                    const AZStd::string sProperty(pNode->getAttr(Audio::WwiseXmlTags::WwiseValueAttribute));
                                    value = AZStd::stof(sProperty);
                                }
                                pConnection->fValue = value;
                                return pConnection;
                            }
                        }
                    }
                    else
                    {
                        return AZStd::make_shared<IAudioConnection>(pControl->GetId());
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
                        XmlNodeRef pSwitchNode = GetISystem()->CreateXmlNode(TypeToTag(pParent->GetType()).data());
                        pSwitchNode->setAttr(Audio::WwiseXmlTags::WwiseNameAttribute, pParent->GetName().c_str());

                        XmlNodeRef pStateNode = pSwitchNode->createNode(Audio::WwiseXmlTags::WwiseValueTag);
                        pStateNode->setAttr(Audio::WwiseXmlTags::WwiseNameAttribute, pControl->GetName().c_str());
                        pSwitchNode->addChild(pStateNode);

                        return pSwitchNode;
                    }
                    break;
                }

                case AudioControls::eWCT_WWISE_RTPC:
                {
                    XmlNodeRef pConnectionNode;
                    pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()).data());
                    pConnectionNode->setAttr(Audio::WwiseXmlTags::WwiseNameAttribute, pControl->GetName().c_str());

                    if (eATLControlType == eACET_RTPC)
                    {
                        AZStd::shared_ptr<const CRtpcConnection> pRtpcConnection = AZStd::static_pointer_cast<const CRtpcConnection>(pConnection);
                        if (pRtpcConnection->fMult != 1.0f)
                        {
                            pConnectionNode->setAttr(Audio::WwiseXmlTags::WwiseMutiplierAttribute, pRtpcConnection->fMult);
                        }
                        if (pRtpcConnection->fShift != 0.0f)
                        {
                            pConnectionNode->setAttr(Audio::WwiseXmlTags::WwiseShiftAttribute, pRtpcConnection->fShift);
                        }
                    }
                    else if (eATLControlType == eACET_SWITCH_STATE)
                    {
                        AZStd::shared_ptr<const CStateToRtpcConnection> pStateConnection = AZStd::static_pointer_cast<const CStateToRtpcConnection>(pConnection);
                        pConnectionNode->setAttr(Audio::WwiseXmlTags::WwiseValueAttribute, pStateConnection->fValue);
                    }
                    return pConnectionNode;
                }

                case AudioControls::eWCT_WWISE_EVENT:
                {
                    XmlNodeRef pConnectionNode;
                    pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()).data());
                    pConnectionNode->setAttr(Audio::WwiseXmlTags::WwiseNameAttribute, pControl->GetName().c_str());
                    return pConnectionNode;
                }

                case AudioControls::eWCT_WWISE_AUX_BUS:
                {
                    XmlNodeRef pConnectionNode;
                    pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()).data());
                    pConnectionNode->setAttr(Audio::WwiseXmlTags::WwiseNameAttribute, pControl->GetName().c_str());
                    return pConnectionNode;
                }

                case AudioControls::eWCT_WWISE_SOUND_BANK:
                {
                    XmlNodeRef pConnectionNode = GetISystem()->CreateXmlNode(TypeToTag(pControl->GetType()).data());
                    pConnectionNode->setAttr(Audio::WwiseXmlTags::WwiseNameAttribute, pControl->GetName().c_str());
                    if (pControl->IsLocalised())
                    {
                        pConnectionNode->setAttr(Audio::WwiseXmlTags::WwiseLocalizedAttribute, "true");
                    }
                    return pConnectionNode;
                }
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    const AZStd::string_view CAudioSystemEditor_wwise::GetTypeIcon(TImplControlType type) const
    {
        switch (type)
        {
        case eWCT_WWISE_EVENT:
            return ":/Editor/WwiseIcons/event_nor.png";
        case eWCT_WWISE_RTPC:
            return ":/Editor/WwiseIcons/gameparameter_nor.png";
        case eWCT_WWISE_SWITCH:
            return ":/Editor/WwiseIcons/switch_nor.png";
        case eWCT_WWISE_AUX_BUS:
            return ":/Editor/WwiseIcons/auxbus_nor.png";
        case eWCT_WWISE_SOUND_BANK:
            return ":/Editor/WwiseIcons/soundbank_nor.png";
        case eWCT_WWISE_GAME_STATE:
            return ":/Editor/WwiseIcons/state_nor.png";
        case eWCT_WWISE_SWITCH_GROUP:
            return ":/Editor/WwiseIcons/switchgroup_nor.png";
        case eWCT_WWISE_GAME_STATE_GROUP:
            return ":/Editor/WwiseIcons/stategroup_nor.png";
        default:
            // should make a "default"/empty icon...
            return ":/Editor/WwiseIcons/switchgroup_nor.png";
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
    TImplControlTypeMask CAudioSystemEditor_wwise::GetCompatibleTypes(EACEControlType eATLControlType) const
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
    CID CAudioSystemEditor_wwise::GetID(const AZStd::string_view sName) const
    {
        return static_cast<CID>(AZ::Crc32(sName.data()));
    }

    //-------------------------------------------------------------------------------------------//
    AZStd::string CAudioSystemEditor_wwise::GetName() const
    {
        return "Wwise";
    }

    //-------------------------------------------------------------------------------------------//
    void CAudioSystemEditor_wwise::UpdateConnectedStatus()
    {
        for (const auto& idCountPair : m_connectionsByID)
        {
            if (idCountPair.second > 0)
            {
                IAudioSystemControl* pControl = GetControl(idCountPair.first);
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
    AZStd::string CAudioSystemEditor_wwise::GetDataPath() const
    {
        AZStd::string path(Path::GetEditingGameDataFolder());
        AzFramework::StringFunc::Path::Join(path.c_str(), "/sounds/wwise_project/", path);
        return path;
    }
} // namespace AudioControls
