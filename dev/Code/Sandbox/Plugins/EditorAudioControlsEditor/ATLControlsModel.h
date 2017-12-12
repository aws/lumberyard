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

#include "CryString.h"
#include "AudioControl.h"
#include "common/IAudioConnection.h"

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    // available levels where the controls can be stored
    struct SControlScope
    {
        SControlScope() {}
        SControlScope(const string& _name, bool _bOnlyLocal)
            : name(_name)
            , bOnlyLocal(_bOnlyLocal)
        {}

        string name;

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
        CATLControl* CreateControl(const string& sControlName, EACEControlType type, CATLControl* pParent = nullptr);
        void RemoveControl(CID id);

        CATLControl* GetControlByID(CID id) const;
        CATLControl* FindControl(const string& sControlName, EACEControlType eType, const string& sScope, CATLControl* pParent = nullptr) const;

        // Platforms
        string GetPlatformAt(uint index);
        void AddPlatform(const string& name);
        uint GetPlatformCount();

        // Connection Groups
        void AddConnectionGroup(const string& name);
        int GetConnectionGroupId(const string& name);
        int GetConnectionGroupCount() const;
        string GetConnectionGroupAt(int index) const;

        // Scope
        void AddScope(const string& name, bool bLocalOnly = false);
        void ClearScopes();
        int GetScopeCount() const;
        SControlScope GetScopeAt(int index) const;
        bool ScopeExists(const string& name) const;

        // Helper functions
        bool IsNameValid(const string& name, EACEControlType type, const string& scope, const CATLControl* const pParent = nullptr) const;
        string GenerateUniqueName(const string& sRootName, EACEControlType eType, const string& sScope, const CATLControl* const pParent = nullptr) const;
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

        std::shared_ptr<CATLControl> TakeControl(CID nID);
        void InsertControl(std::shared_ptr<CATLControl> pControl);

        static CID m_nextId;
        std::vector<std::shared_ptr<CATLControl> > m_controls;
        std::vector<string> m_platforms;
        std::vector<SControlScope> m_scopes;
        std::vector<string> m_connectionGroups;

        std::vector<IATLControlModelListener*> m_listeners;
        bool m_bSuppressMessages;
        bool m_bControlTypeModified[eACET_NUM_TYPES];
    };
} // namespace AudioControls
