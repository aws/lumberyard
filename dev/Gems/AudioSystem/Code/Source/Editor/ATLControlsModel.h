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

#include <AudioControl.h>
#include <IAudioConnection.h>
#include <AzCore/std/string/string_view.h>

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    // available levels where the controls can be stored
    struct SControlScope
    {
        SControlScope() {}
        SControlScope(const AZStd::string& _name, bool _bOnlyLocal)
            : name(_name)
            , bOnlyLocal(_bOnlyLocal)
        {}

        AZStd::string name;

        // if true, there is a level in the game audio
        // data that doesn't exist in the global list
        // of levels for your project
        bool bOnlyLocal;
    };

    //-------------------------------------------------------------------------------------------//
    struct IATLControlModelListener
    {
        virtual ~IATLControlModelListener() = default;
        virtual void OnControlAdded(CATLControl* pControl) {}
        virtual void OnControlModified(CATLControl* pControl) {}
        virtual void OnControlRemoved(CATLControl* pControl) {}
        virtual void OnConnectionAdded(CATLControl* pControl, IAudioSystemControl* pMiddlewareControl) {}
        virtual void OnConnectionRemoved(CATLControl* pControl, IAudioSystemControl* pMiddlewareControl) {}
    };

    //-------------------------------------------------------------------------------------------//
    class CATLControlsModel
    {
        friend class IUndoControlOperation;
        friend class CUndoControlModified;
        friend class CATLControl;

    public:
        CATLControlsModel();
        ~CATLControlsModel();

        void Clear();
        CATLControl* CreateControl(const AZStd::string& sControlName, EACEControlType type, CATLControl* pParent = nullptr);
        void RemoveControl(CID id);

        CATLControl* GetControlByID(CID id) const;
        CATLControl* FindControl(const AZStd::string_view sControlName, EACEControlType eType, const AZStd::string_view sScope, CATLControl* pParent = nullptr) const;

        // Platforms
        AZStd::string GetPlatformAt(AZ::u32 index);
        void AddPlatform(AZStd::string platformName);
        AZ::u32 GetPlatformCount();

        // Connection Groups
        void AddConnectionGroup(const AZStd::string_view name);
        int GetConnectionGroupId(const AZStd::string_view name);
        int GetConnectionGroupCount() const;
        AZStd::string GetConnectionGroupAt(int index) const;

        // Scope
        void AddScope(AZStd::string scopeName, bool bLocalOnly = false);
        void ClearScopes();
        int GetScopeCount() const;
        SControlScope GetScopeAt(int index) const;
        bool ScopeExists(AZStd::string scopeName) const;

        // Helper functions
        bool IsNameValid(const AZStd::string_view name, EACEControlType type, const AZStd::string_view scope, const CATLControl* const pParent = nullptr) const;
        AZStd::string GenerateUniqueName(const AZStd::string_view sRootName, EACEControlType eType, const AZStd::string_view sScope, const CATLControl* const pParent = nullptr) const;
        void ClearAllConnections();
        void ReloadAllConnections();

        void AddListener(IATLControlModelListener* pListener);
        void RemoveListener(IATLControlModelListener* pListener);
        void SetSuppressMessages(bool bSuppressMessages);
        bool IsTypeDirty(EACEControlType eType);
        bool IsDirty();
        void ClearDirtyFlags();

    private:
        void OnControlAdded(CATLControl* pControl);
        void OnControlModified(CATLControl* pControl);
        void OnConnectionAdded(CATLControl* pControl, IAudioSystemControl* pMiddlewareControl);
        void OnConnectionRemoved(CATLControl* pControl, IAudioSystemControl* pMiddlewareControl);
        void OnControlRemoved(CATLControl* pControl);

        CID GenerateUniqueId() { return ++m_nextId; }

        AZStd::shared_ptr<CATLControl> TakeControl(CID nID);
        void InsertControl(AZStd::shared_ptr<CATLControl> pControl);

        static CID m_nextId;
        AZStd::vector<AZStd::shared_ptr<CATLControl> > m_controls;
        AZStd::vector<AZStd::string> m_platforms;
        AZStd::vector<SControlScope> m_scopes;
        AZStd::vector<AZStd::string> m_connectionGroups;

        AZStd::vector<IATLControlModelListener*> m_listeners;
        bool m_bSuppressMessages;
        bool m_bControlTypeModified[eACET_NUM_TYPES];
    };
} // namespace AudioControls
