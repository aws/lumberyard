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

#include "platform.h"
#include "CryString.h"
#include "IAudioConnection.h"
#include "ACETypes.h"

namespace AudioControls
{
    // lumberyard-refactor:
    // since this class is in a file named IAudioSystemControl.h and this is a concrete class,
    // it is named badly.  The 'I' should denote an interface.  Two ways to go here:
    // 1) Make a real Interface class, keep it named IAudioSystemControl and derive a concrete
    //    'shared' class that has all this implementation.
    // 2) Rename the file and class to AudioSystemControl, then it is no longer confusing.

    //-------------------------------------------------------------------------------------------//
    class IAudioSystemControl
    {
    public:
        IAudioSystemControl::IAudioSystemControl()
            : m_name()
            , m_id(ACE_INVALID_CID)
            , m_type(AUDIO_IMPL_INVALID_TYPE)
            , m_bPlaceholder(false)
            , m_bLocalised(false)
            , m_parent(nullptr)
            , m_isConnected(false)
        {
        }

        IAudioSystemControl::IAudioSystemControl(const string& name, CID id, TImplControlType type)
            : m_name(name)
            , m_id(id)
            , m_type(type)
            , m_bPlaceholder(false)
            , m_bLocalised(false)
            , m_parent(nullptr)
            , m_isConnected(false)
        {
        }

        virtual ~IAudioSystemControl() {}

        // unique id for this control
        CID GetId() const { return m_id; }
        void SetId(CID id) { m_id = id; }

        TImplControlType GetType() const { return m_type; }
        void SetType(TImplControlType type) { m_type = type; }

        string GetName() const { return m_name; }
        void SetName(const string& name)
        {
            if (name != m_name)
            {
                m_name = name;
            }
        }

        bool IsPlaceholder() const { return m_bPlaceholder; }
        void SetPlaceholder(bool bIsPlaceholder) { m_bPlaceholder = bIsPlaceholder; }

        bool IsLocalised() const { return m_bLocalised; }
        void SetLocalised(bool bIsLocalised) { m_bLocalised = bIsLocalised; }

        bool IsConnected() const { return m_isConnected; }
        void SetConnected(bool isConnected) { m_isConnected = isConnected; }

        size_t ChildCount() const { return m_children.size(); }
        void AddChild(IAudioSystemControl* pChild) { m_children.push_back(pChild); pChild->SetParent(this); }
        IAudioSystemControl* GetChildAt(uint index) const { return m_children[index]; }
        void SetParent(IAudioSystemControl* pParent) { m_parent = pParent; }
        IAudioSystemControl* GetParent() const { return m_parent; }

    private:
        CID m_id;
        TImplControlType m_type;
        string m_name;
        bool m_bPlaceholder;
        bool m_bLocalised;
        bool m_isConnected;
        std::vector<IAudioSystemControl*> m_children;
        IAudioSystemControl* m_parent;
    };
} // namespace AudioControls
