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

#pragma once

#include "IAudioSystemEditor.h"
#include "IAudioConnection.h"

namespace AudioControls
{
    //-------------------------------------------------------------------------------------------//
    class CAudioSystemEditor_nosound
        : public IAudioSystemEditor
    {
    public:
        CAudioSystemEditor_nosound()
        {
        }

        ~CAudioSystemEditor_nosound() override
        {
        }

        //////////////////////////////////////////////////////////
        // IAudioSystemEditor implementation
        /////////////////////////////////////////////////////////
        void Reload() override
        {
        }

        IAudioSystemControl* CreateControl(const SControlDef& controlDefinition) override
        {
            return nullptr;
        }

        IAudioSystemControl* GetRoot() override
        {
            return nullptr;
        }

        IAudioSystemControl* GetControl(CID id) const override
        {
            return nullptr;
        }

        EACEControlType ImplTypeToATLType(TImplControlType type) const override
        {
            return eACET_NUM_TYPES;
        }

        TImplControlTypeMask GetCompatibleTypes(EACEControlType eATLControlType) const override
        {
            return static_cast<TImplControlTypeMask>(0);
        }

        TConnectionPtr CreateConnectionToControl(EACEControlType eATLControlType, IAudioSystemControl* pMiddlewareControl) override
        {
            return nullptr;
        }

        TConnectionPtr CreateConnectionFromXMLNode(XmlNodeRef pNode, EACEControlType eATLControlType) override
        {
            return nullptr;
        }

        XmlNodeRef CreateXMLNodeFromConnection(const TConnectionPtr pConnection, const EACEControlType eATLControlType) override
        {
            return static_cast<XmlNodeRef>(0);
        }

        void ConnectionRemoved(IAudioSystemControl* pControl) override
        {
        }

        string GetTypeIcon(TImplControlType type) const override
        {
            return string();
        }

        string GetName() const override
        {
            return "NoSound";
        }

        string GetDataPath() const override
        {
            return string();
        }

        void DataSaved() override
        {
        }
        /////////////////////////////////////////////////////////
    };
} // namespace AudioControls
