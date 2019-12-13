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

#include <AudioControl.h>

#include <ACETypes.h>
#include <ATLControlsModel.h>
#include <AudioControlsEditorPlugin.h>
#include <AudioControlsEditorUndo.h>
#include <IAudioSystemControl.h>
#include <IEditor.h>
#include <ImplementationManager.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    const char* g_sDefaultGroup = "";


    //-------------------------------------------------------------------------------------------//
    CATLControl::CATLControl()
        : m_sName("")
        , m_nID(ACE_INVALID_CID)
        , m_eType(eACET_RTPC)
        , m_sScope("")
        , m_bAutoLoad(true)
        , m_pParent(nullptr)
        , m_pModel(nullptr)
    {
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl::CATLControl(const AZStd::string& sControlName, CID nID, EACEControlType eType, CATLControlsModel* pModel)
        : m_sName(sControlName)
        , m_nID(nID)
        , m_eType(eType)
        , m_sScope("")
        , m_bAutoLoad(true)
        , m_pParent(nullptr)
        , m_pModel(pModel)
    {
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl::~CATLControl()
    {
        // Same as ClearConnections but without signalling that 'this' control is being modified...
        if (CAudioControlsEditorPlugin::GetImplementationManager())
        {
            if (IAudioSystemEditor* audioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManager()->GetImplementation())
            {
                for (auto& connectionPtr : m_connectedControls)
                {
                    if (IAudioSystemControl* audioSystemControl = audioSystemImpl->GetControl(connectionPtr->GetID()))
                    {
                        audioSystemImpl->ConnectionRemoved(audioSystemControl);
                        SignalConnectionRemoved(audioSystemControl);
                    }
                }
            }
        }

        m_connectedControls.clear();
    }

    //-------------------------------------------------------------------------------------------//
    CID CATLControl::GetId() const
    {
        return m_nID;
    }

    //-------------------------------------------------------------------------------------------//
    EACEControlType CATLControl::GetType() const
    {
        return m_eType;
    }

    //-------------------------------------------------------------------------------------------//
    AZStd::string CATLControl::GetName() const
    {
        return m_sName;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SetId(CID id)
    {
        if (id != m_nID)
        {
            SignalControlAboutToBeModified();
            m_nID = id;
            SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SetType(EACEControlType type)
    {
        if (type != m_eType)
        {
            SignalControlAboutToBeModified();
            m_eType = type;
            SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SetName(const AZStd::string_view name)
    {
        if (name != m_sName)
        {
            SignalControlAboutToBeModified();
            m_sName = name;
            SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    AZStd::string CATLControl::GetScope() const
    {
        return m_sScope;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SetScope(const AZStd::string_view sScope)
    {
        if (m_sScope != sScope)
        {
            m_sScope = sScope;
            SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    bool CATLControl::HasScope() const
    {
        return !m_sScope.empty();
    }

    //-------------------------------------------------------------------------------------------//
    bool CATLControl::IsAutoLoad() const
    {
        return m_bAutoLoad;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SetAutoLoad(bool bAutoLoad)
    {
        if (bAutoLoad != m_bAutoLoad)
        {
            SignalControlAboutToBeModified();
            m_bAutoLoad = bAutoLoad;
            SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    int CATLControl::GetGroupForPlatform(const AZStd::string_view platform) const
    {
        auto it = m_groupPerPlatform.find(platform);
        if (it == m_groupPerPlatform.end())
        {
            return 0;
        }
        return it->second;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SetGroupForPlatform(const AZStd::string_view platform, int connectionGroupId)
    {
        if (m_groupPerPlatform[platform] != connectionGroupId)
        {
            SignalControlAboutToBeModified();
            m_groupPerPlatform[platform] = connectionGroupId;
            SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    size_t CATLControl::ConnectionCount() const
    {
        return m_connectedControls.size();
    }

    //-------------------------------------------------------------------------------------------//
    TConnectionPtr CATLControl::GetConnectionAt(int index)
    {
        if (index < m_connectedControls.size())
        {
            return m_connectedControls[index];
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    TConnectionPtr CATLControl::GetConnection(CID id, const AZStd::string_view group)
    {
        if (id != ACE_INVALID_CID)
        {
            const size_t size = m_connectedControls.size();
            for (size_t i = 0; i < size; ++i)
            {
                TConnectionPtr pConnection = m_connectedControls[i];
                if (pConnection && pConnection->GetID() == id && pConnection->GetGroup() == group)
                {
                    return pConnection;
                }
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    TConnectionPtr CATLControl::GetConnection(IAudioSystemControl* m_pAudioSystemControl, const AZStd::string_view group)
    {
        return GetConnection(m_pAudioSystemControl->GetId(), group);
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::AddConnection(TConnectionPtr pConnection)
    {
        if (pConnection)
        {
            SignalControlAboutToBeModified();
            m_connectedControls.push_back(pConnection);

            if (IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManager()->GetImplementation())
            {
                if (IAudioSystemControl* pAudioSystemControl = pAudioSystemImpl->GetControl(pConnection->GetID()))
                {
                    SignalConnectionAdded(pAudioSystemControl);
                }
            }
            SignalControlModified();
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::RemoveConnection(TConnectionPtr pConnection)
    {
        if (pConnection)
        {
            auto it = AZStd::find(m_connectedControls.begin(), m_connectedControls.end(), pConnection);
            if (it != m_connectedControls.end())
            {
                SignalControlAboutToBeModified();
                m_connectedControls.erase(it);

                if (IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManager()->GetImplementation())
                {
                    if (IAudioSystemControl* pAudioSystemControl = pAudioSystemImpl->GetControl(pConnection->GetID()))
                    {
                        SignalConnectionRemoved(pAudioSystemControl);
                    }
                }
                SignalControlModified();
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::ClearConnections()
    {
        SignalControlAboutToBeModified();
        if (IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManager()->GetImplementation())
        {
            for (auto& connectionPtr : m_connectedControls)
            {
                if (IAudioSystemControl* pAudioSystemControl = pAudioSystemImpl->GetControl(connectionPtr->GetID()))
                {
                    pAudioSystemImpl->ConnectionRemoved(pAudioSystemControl);
                    SignalConnectionRemoved(pAudioSystemControl);
                }
            }
        }
        m_connectedControls.clear();
        SignalControlModified();
    }

    //-------------------------------------------------------------------------------------------//
    bool CATLControl::IsFullyConnected() const
    {
        bool isConnected = ConnectionCount() > 0;
        //  Switches have no connections. Their child Switch_States do.
        if (m_eType == eACET_SWITCH)
        {
            isConnected = true;
            for (auto& childPtr : m_children)
            {
                if (!childPtr->IsFullyConnected())
                {
                    isConnected = false;
                    break;
                }
            }
        }
        else
        {
            if (IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManager()->GetImplementation())
            {
                for (auto& connectionPtr : m_connectedControls)
                {
                    if (IAudioSystemControl* pAudioSystemControl = pAudioSystemImpl->GetControl(connectionPtr->GetID()))
                    {
                        if (!pAudioSystemControl->IsConnected() || pAudioSystemControl->IsPlaceholder())
                        {
                            isConnected = false;
                            break;
                        }
                    }
                }
            }
        }

        return isConnected;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::RemoveConnection(IAudioSystemControl* pAudioSystemControl)
    {
        if (pAudioSystemControl)
        {
            const CID nID = pAudioSystemControl->GetId();
            auto it = m_connectedControls.begin();
            auto end = m_connectedControls.end();
            for (; it != end; ++it)
            {
                if ((*it)->GetID() == nID)
                {
                    SignalControlAboutToBeModified();
                    m_connectedControls.erase(it);
                    SignalConnectionRemoved(pAudioSystemControl);
                    SignalControlModified();
                    return;
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SignalControlModified()
    {
        if (m_pModel)
        {
            m_pModel->OnControlModified(this);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SignalControlAboutToBeModified()
    {
        if (!CUndo::IsSuspended())
        {
            CUndo undo("ATL Control Modified");
            CUndo::Record(new CUndoControlModified(GetId()));
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SignalConnectionAdded(IAudioSystemControl* pMiddlewareControl)
    {
        if (m_pModel)
        {
            m_pModel->OnConnectionAdded(this, pMiddlewareControl);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SignalConnectionRemoved(IAudioSystemControl* pMiddlewareControl)
    {
        if (m_pModel)
        {
            m_pModel->OnConnectionRemoved(this, pMiddlewareControl);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::ReloadConnections()
    {
        if (IAudioSystemEditor* pAudioSystemImpl = CAudioControlsEditorPlugin::GetImplementationManager()->GetImplementation())
        {
            TConnectionPerGroup::iterator it = m_connectionNodes.begin();
            TConnectionPerGroup::iterator end = m_connectionNodes.end();
            for (; it != end; ++it)
            {
                TXmlNodeList& nodeList = it->second;
                const size_t size = nodeList.size();
                for (size_t i = 0; i < size; ++i)
                {
                    if (TConnectionPtr pConnection = pAudioSystemImpl->CreateConnectionFromXMLNode(nodeList[i].m_xmlNode, m_eType))
                    {
                        AddConnection(pConnection);
                        nodeList[i].m_isValid = true;
                    }
                    else
                    {
                        nodeList[i].m_isValid = false;
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControl::SetParent(CATLControl* pParent)
    {
        m_pParent = pParent;
        if (m_pParent)
        {
            SetScope(m_pParent->GetScope());
        }
    }

} // namespace AudioControls
