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

#include <ATLControlsModel.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/conversions.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AudioControlsEditorUndo.h>
#include <IEditor.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    CID CATLControlsModel::m_nextId = ACE_INVALID_CID;

    //-------------------------------------------------------------------------------------------//
    CATLControlsModel::CATLControlsModel()
        : m_bSuppressMessages(false)
    {
        ClearDirtyFlags();
    }

    //-------------------------------------------------------------------------------------------//
    CATLControlsModel::~CATLControlsModel()
    {
        Clear();
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* CATLControlsModel::CreateControl(const AZStd::string& sControlName, EACEControlType eType, CATLControl* pParent)
    {
        AZStd::shared_ptr<CATLControl> pControl = AZStd::make_shared<CATLControl>(sControlName, GenerateUniqueId(), eType, this);
        if (pControl)
        {
            if (pParent)
            {
                pControl->SetParent(pParent);
            }

            InsertControl(pControl);

            if (!CUndo::IsSuspended())
            {
                CUndo undo("Audio Control Created");
                CUndo::Record(new CUndoControlAdd(pControl->GetId()));
            }
        }
        return pControl.get();
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::RemoveControl(CID id)
    {
        if (id != ACE_INVALID_CID)
        {
            ControlList children;
            size_t size = m_controls.size();
            for (auto it = m_controls.begin(); it != m_controls.end(); ++it)
            {
                AZStd::shared_ptr<CATLControl>& pControl = *it;
                if (pControl && pControl->GetId() == id)
                {
                    pControl->ClearConnections();
                    OnControlRemoved(pControl.get());

                    // Remove control from parent
                    CATLControl* pParent = pControl->GetParent();
                    if (pParent)
                    {
                        pParent->RemoveChild(pControl.get());
                    }

                    if (!CUndo::IsSuspended())
                    {
                        CUndo::Record(new CUndoControlRemove(pControl));
                    }
                    m_controls.erase(it, it + 1);
                    break;
                }
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* CATLControlsModel::GetControlByID(CID id) const
    {
        if (id != ACE_INVALID_CID)
        {
            size_t size = m_controls.size();
            for (size_t i = 0; i < size; ++i)
            {
                if (m_controls[i]->GetId() == id)
                {
                    return m_controls[i].get();
                }
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    bool CATLControlsModel::IsNameValid(const AZStd::string_view name, EACEControlType type, const AZStd::string_view scope, const CATLControl* const pParent) const
    {
        const size_t size = m_controls.size();
        for (size_t i = 0; i < size; ++i)
        {
            if (m_controls[i]
                && m_controls[i]->GetType() == type
                && (AzFramework::StringFunc::Equal(m_controls[i]->GetName().c_str(), name.data()))
                && (m_controls[i]->GetScope().empty() || m_controls[i]->GetScope() == scope)
                && (m_controls[i]->GetParent() == pParent))
            {
                return false;
            }
        }
        return true;
    }

    //-------------------------------------------------------------------------------------------//
    AZStd::string CATLControlsModel::GenerateUniqueName(const AZStd::string_view sRootName, EACEControlType eType, const AZStd::string_view sScope, const CATLControl* const pParent) const
    {
        AZStd::string uniqueName = sRootName;
        AZ::u32 number = 1;
        while (!IsNameValid(uniqueName, eType, sScope, pParent))
        {
            uniqueName = AZStd::string::format("%s_%u", sRootName.data(), number++);
        }

        return uniqueName;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::AddScope(AZStd::string scopeName, bool bLocalOnly)
    {
        AZStd::to_lower(scopeName.begin(), scopeName.end());
        const size_t size = m_scopes.size();
        for (int i = 0; i < size; ++i)
        {
            if (m_scopes[i].name == scopeName)
            {
                return;
            }
        }
        m_scopes.push_back(SControlScope(scopeName, bLocalOnly));
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::ClearScopes()
    {
        m_scopes.clear();
    }

    //-------------------------------------------------------------------------------------------//
    bool CATLControlsModel::ScopeExists(AZStd::string scopeName) const
    {
        AZStd::to_lower(scopeName.begin(), scopeName.end());
        const size_t size = m_scopes.size();
        for (int i = 0; i < size; ++i)
        {
            if (m_scopes[i].name == scopeName)
            {
                return true;
            }
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------//
    int CATLControlsModel::GetScopeCount() const
    {
        return m_scopes.size();
    }

    //-------------------------------------------------------------------------------------------//
    SControlScope CATLControlsModel::GetScopeAt(int index) const
    {
        if (index < m_scopes.size())
        {
            return m_scopes[index];
        }
        return SControlScope();
    }

    //-------------------------------------------------------------------------------------------//
    AZStd::string CATLControlsModel::GetPlatformAt(AZ::u32 index)
    {
        if (index < m_platforms.size())
        {
            return m_platforms[index];
        }
        return "";
    }

    //-------------------------------------------------------------------------------------------//
    AZ::u32 CATLControlsModel::GetPlatformCount()
    {
        return m_platforms.size();
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::AddPlatform(AZStd::string platformName)
    {
        AZStd::to_lower(platformName.begin(), platformName.end());
        if (AZStd::find(m_platforms.begin(), m_platforms.end(), platformName) == m_platforms.end())
        {
            m_platforms.push_back(platformName);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::Clear()
    {
        m_controls.clear();
        m_scopes.clear();
        m_platforms.clear();
        m_connectionGroups.clear();
        ClearDirtyFlags();
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::AddConnectionGroup(const AZStd::string_view name)
    {
        if (AZStd::find(m_connectionGroups.begin(), m_connectionGroups.end(), name) == m_connectionGroups.end())
        {
            m_connectionGroups.push_back(name);
        }
    }

    //-------------------------------------------------------------------------------------------//
    int CATLControlsModel::GetConnectionGroupId(const AZStd::string_view name)
    {
        size_t size = m_connectionGroups.size();
        for (int i = 0; i < size; ++i)
        {
            if (m_connectionGroups[i].compare(name) == 0)
            {
                return i;
            }
        }
        return -1;
    }

    //-------------------------------------------------------------------------------------------//
    int CATLControlsModel::GetConnectionGroupCount() const
    {
        return m_connectionGroups.size();
    }

    //-------------------------------------------------------------------------------------------//
    AZStd::string CATLControlsModel::GetConnectionGroupAt(int index) const
    {
        if (index < m_connectionGroups.size())
        {
            return m_connectionGroups[index];
        }
        return "";
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::AddListener(IATLControlModelListener* pListener)
    {
        if (AZStd::find(m_listeners.begin(), m_listeners.end(), pListener) == m_listeners.end())
        {
            m_listeners.push_back(pListener);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::RemoveListener(IATLControlModelListener* pListener)
    {
        auto it = AZStd::find(m_listeners.begin(), m_listeners.end(), pListener);
        if (it != m_listeners.end())
        {
            m_listeners.erase(it);
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::OnControlAdded(CATLControl* pControl)
    {
        if (!m_bSuppressMessages)
        {
            for (auto listener : m_listeners)
            {
                listener->OnControlAdded(pControl);
            }
            m_bControlTypeModified[pControl->GetType()] = true;
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::OnControlRemoved(CATLControl* pControl)
    {
        if (!m_bSuppressMessages)
        {
            for (auto listener : m_listeners)
            {
                listener->OnControlRemoved(pControl);
            }
            m_bControlTypeModified[pControl->GetType()] = true;
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::OnConnectionAdded(CATLControl* pControl, IAudioSystemControl* pMiddlewareControl)
    {
        if (!m_bSuppressMessages)
        {
            for (auto listener : m_listeners)
            {
                listener->OnConnectionAdded(pControl, pMiddlewareControl);
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::OnConnectionRemoved(CATLControl* pControl, IAudioSystemControl* pMiddlewareControl)
    {
        if (!m_bSuppressMessages)
        {
            for (auto listener : m_listeners)
            {
                listener->OnConnectionRemoved(pControl, pMiddlewareControl);
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::OnControlModified(CATLControl* pControl)
    {
        if (!m_bSuppressMessages)
        {
            for (auto listener : m_listeners)
            {
                listener->OnControlModified(pControl);
            }
            m_bControlTypeModified[pControl->GetType()] = true;
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::SetSuppressMessages(bool bSuppressMessages)
    {
        m_bSuppressMessages = bSuppressMessages;
    }

    //-------------------------------------------------------------------------------------------//
    bool CATLControlsModel::IsDirty()
    {
        for (int i = 0; i < eACET_NUM_TYPES; ++i)
        {
            if (m_bControlTypeModified[i])
            {
                return true;
            }
        }
        return false;
    }

    //-------------------------------------------------------------------------------------------//
    bool CATLControlsModel::IsTypeDirty(EACEControlType eType)
    {
        if (eType != eACET_NUM_TYPES)
        {
            return m_bControlTypeModified[eType];
        }
        return true;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::ClearDirtyFlags()
    {
        for (int i = 0; i < eACET_NUM_TYPES; ++i)
        {
            m_bControlTypeModified[i] = false;
        }
    }

    //-------------------------------------------------------------------------------------------//
    CATLControl* CATLControlsModel::FindControl(const AZStd::string_view sControlName, EACEControlType eType, const AZStd::string_view sScope, CATLControl* pParent) const
    {
        if (pParent)
        {
            const size_t size = pParent->ChildCount();
            for (size_t i = 0; i < size; ++i)
            {
                CATLControl* pControl = pParent->GetChild(i);
                if (pControl
                    && pControl->GetName() == sControlName
                    && pControl->GetType() == eType
                    && pControl->GetScope() == sScope)
                {
                    return pControl;
                }
            }
        }
        else
        {
            const size_t size = m_controls.size();
            for (size_t i = 0; i < size; ++i)
            {
                CATLControl* pControl = m_controls[i].get();
                if (pControl
                    && pControl->GetName() == sControlName
                    && pControl->GetType() == eType
                    && pControl->GetScope() == sScope)
                {
                    return pControl;
                }
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    AZStd::shared_ptr<CATLControl> CATLControlsModel::TakeControl(CID nID)
    {
        const size_t size = m_controls.size();
        for (size_t i = 0; i < size; ++i)
        {
            if (m_controls[i]->GetId() == nID)
            {
                AZStd::shared_ptr<CATLControl> pControl = m_controls[i];
                RemoveControl(nID);
                return pControl;
            }
        }
        return nullptr;
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::InsertControl(AZStd::shared_ptr<CATLControl> pControl)
    {
        if (pControl)
        {
            m_controls.push_back(pControl);

            CATLControl* pParent = pControl->GetParent();
            if (pParent)
            {
                pParent->AddChild(pControl.get());
            }

            OnControlAdded(pControl.get());
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::ClearAllConnections()
    {
        const size_t size = m_controls.size();
        for (size_t i = 0; i < size; ++i)
        {
            CATLControl* pControl = m_controls[i].get();
            if (pControl)
            {
                pControl->ClearConnections();
            }
        }
    }

    //-------------------------------------------------------------------------------------------//
    void CATLControlsModel::ReloadAllConnections()
    {
        const size_t size = m_controls.size();
        for (size_t i = 0; i < size; ++i)
        {
            CATLControl* pControl = m_controls[i].get();
            if (pControl)
            {
                pControl->ReloadConnections();
            }
        }
    }

} // namespace AudioControls
