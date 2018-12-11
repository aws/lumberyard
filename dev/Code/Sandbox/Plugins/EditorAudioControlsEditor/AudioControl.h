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
#include "common/IAudioConnection.h"
#include "common/ACETypes.h"
#include <IXml.h>

namespace AudioControls
{
    extern const char* g_sDefaultGroup;

    class CATLControlsModel;

    //-------------------------------------------------------------------------------------------//
    struct SRawConnectionData
    {
        SRawConnectionData(XmlNodeRef node, bool bIsValid)
            : xmlNode(node)
            , bValid(bIsValid) {}
        XmlNodeRef xmlNode;

        // indicates if the connection is valid for the currently loaded middle-ware
        bool bValid;
    };

    typedef std::vector<SRawConnectionData> TXMLNodeList;
    typedef std::map<string, TXMLNodeList> TConnectionPerGroup;

    //-------------------------------------------------------------------------------------------//
    class CATLControl
    {
        friend class CAudioControlsLoader;
        friend class CAudioControlsWriter;
        friend class CUndoControlModified;

    public:
        CATLControl();
        CATLControl(const string& sControlName, CID nID, EACEControlType eType, CATLControlsModel* pModel);
        ~CATLControl();

        CID GetId() const;

        EACEControlType GetType() const;

        string GetName() const;
        void SetName(const string& name);

        bool HasScope() const;
        string GetScope() const;
        void SetScope(const string& sScope);

        bool IsAutoLoad() const;
        void SetAutoLoad(bool bAutoLoad);

        CATLControl* GetParent() const
        {
            return m_pParent;
        }
        void SetParent(CATLControl* pParent);

        size_t ChildCount() const
        {
            return m_children.size();
        }

        CATLControl* GetChild(uint index) const
        {
            return m_children[index];
        }

        void AddChild(CATLControl* pChildControl)
        {
            m_children.push_back(pChildControl);
        }

        void RemoveChild(CATLControl* pChildControl)
        {
            m_children.erase(std::remove(m_children.begin(), m_children.end(), pChildControl), m_children.end());
        }

        // Controls can group connection according to a platform
        // This is used primarily for the Preload Requests
        int GetGroupForPlatform(const string& platform) const;
        void SetGroupForPlatform(const string& platform, int connectionGroupId);

        size_t ConnectionCount() const;
        void AddConnection(TConnectionPtr pConnection);
        void RemoveConnection(TConnectionPtr pConnection);
        void RemoveConnection(IAudioSystemControl* pAudioSystemControl);
        void ClearConnections();
        TConnectionPtr GetConnectionAt(int index);
        TConnectionPtr GetConnection(CID id, const string& group = g_sDefaultGroup);
        TConnectionPtr GetConnection(IAudioSystemControl* m_pAudioSystemControl, const string& group = g_sDefaultGroup);
        void ReloadConnections();
        bool IsFullyConnected() const;

        void SignalControlAboutToBeModified();
        void SignalControlModified();
        void SignalConnectionAdded(IAudioSystemControl* pMiddlewareControl);
        void SignalConnectionRemoved(IAudioSystemControl* pMiddlewareControl);

    private:
        void SetId(CID id);
        void SetType(EACEControlType type);
        CID m_nID;
        EACEControlType m_eType;
        string m_sName;
        string m_sScope;
        std::map<string, int> m_groupPerPlatform;
        std::vector<TConnectionPtr> m_connectedControls;
        std::vector<CATLControl*> m_children;
        CATLControl* m_pParent;
        bool m_bAutoLoad;

        // All the raw connection nodes. Used for reloading the data when switching middleware.
        TConnectionPerGroup m_connectionNodes;
        CATLControlsModel* m_pModel;
    };
} // namespace AudioControls
