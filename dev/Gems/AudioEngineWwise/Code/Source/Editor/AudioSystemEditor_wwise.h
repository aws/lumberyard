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

#include <IAudioSystemEditor.h>
#include <IAudioConnection.h>
#include <IAudioSystemControl.h>
#include <AudioWwiseLoader.h>

namespace AudioControls
{
    class IAudioConnectionInspectorPanel;

    //-------------------------------------------------------------------------------------------//
    class CRtpcConnection
        : public IAudioConnection
    {
    public:
        explicit CRtpcConnection(CID nID)
            : IAudioConnection(nID)
            , fMult(1.0f)
            , fShift(0.0f)
        {}

        virtual ~CRtpcConnection() {}

        virtual bool HasProperties() override { return true; }

        virtual void Serialize(Serialization::IArchive& ar) override
        {
            ar(fMult, "mult", "Multiply");
            ar(fShift, "shift", "Shift");
        }

        float fMult;
        float fShift;
    };

    using TRtpcConnectionPtr = AZStd::shared_ptr<CRtpcConnection>;

    //-------------------------------------------------------------------------------------------//
    class CStateToRtpcConnection
        : public IAudioConnection
    {
    public:
        explicit CStateToRtpcConnection(CID nID)
            : IAudioConnection(nID)
            , fValue(0.0f)
        {}

        virtual ~CStateToRtpcConnection() {}

        virtual bool HasProperties() override { return true; }

        virtual void Serialize(Serialization::IArchive& ar) override
        {
            ar(fValue, "value", "Value");
        }

        float fValue;
    };

    using TStateConnectionPtr = AZStd::shared_ptr<CStateToRtpcConnection>;


    //-------------------------------------------------------------------------------------------//
    class CAudioSystemEditor_wwise
        : public IAudioSystemEditor
    {
        friend class CAudioWwiseLoader;

    public:
        CAudioSystemEditor_wwise();
        ~CAudioSystemEditor_wwise() override;

        //////////////////////////////////////////////////////////
        // IAudioSystemEditor implementation
        /////////////////////////////////////////////////////////
        void Reload() override;
        IAudioSystemControl* CreateControl(const SControlDef& controlDefinition) override;
        IAudioSystemControl* GetRoot() override { return &m_rootControl; }
        IAudioSystemControl* GetControl(CID id) const override;
        EACEControlType ImplTypeToATLType(TImplControlType type) const override;
        TImplControlTypeMask GetCompatibleTypes(EACEControlType eATLControlType) const override;
        TConnectionPtr CreateConnectionToControl(EACEControlType eATLControlType, IAudioSystemControl* pMiddlewareControl) override;
        TConnectionPtr CreateConnectionFromXMLNode(XmlNodeRef pNode, EACEControlType eATLControlType) override;
        XmlNodeRef CreateXMLNodeFromConnection(const TConnectionPtr pConnection, const EACEControlType eATLControlType) override;
        const AZStd::string_view GetTypeIcon(TImplControlType type) const override;
        AZStd::string GetName() const override;
        AZStd::string GetDataPath() const;
        void DataSaved() override {}
        void ConnectionRemoved(IAudioSystemControl* pControl) override;
        //////////////////////////////////////////////////////////

    private:
        IAudioSystemControl* GetControlByName(AZStd::string sName, bool bIsLocalised = false, IAudioSystemControl* pParent = nullptr) const;

        // Gets the ID of the control given its name. As controls can have the same name
        // if they're under different parents, the name of the parent is also needed (if there is one)
        CID GetID(const AZStd::string_view sName) const;

        void UpdateConnectedStatus();

        IAudioSystemControl m_rootControl;

        using TControlPtr = AZStd::shared_ptr<IAudioSystemControl>;
        using TControlMap = AZStd::unordered_map<CID, TControlPtr>;
        TControlMap m_controls;

        using TConnectionsMap = AZStd::unordered_map<CID, int>;
        TConnectionsMap m_connectionsByID;
        AudioControls::CAudioWwiseLoader m_loader;
    };

} // namespace AudioControls
