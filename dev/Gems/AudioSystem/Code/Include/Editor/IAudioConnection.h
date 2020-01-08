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

#include <ACETypes.h>
#include <Serialization/IArchive.h>
#include <Serialization/STL.h>

namespace AudioControls
{
    class IAudioSystemControl;

    // lumberyard-refactor:
    // since this class is in a file named IAudioConnection.h and this is a concrete class,
    // it is named badly.  The 'I' should denote an interface.  Two ways to go here:
    // 1) Make a real Interface class, keep it named IAudioConnection and derive a concrete
    //    'shared' class that has all this implementation.
    // 2) Rename the file and class to AudioConnection, then it is no longer confusing.

    //-------------------------------------------------------------------------------------------//
    class IAudioConnection
    {
    public:
        IAudioConnection(CID nID)
            : m_nID(nID)
        {
        }

        virtual ~IAudioConnection() {}

        CID GetID() const
        {
            return m_nID;
        }

        const AZStd::string& GetGroup() const
        {
            return m_sGroup;
        }
        void SetGroup(const AZStd::string& group)
        {
            m_sGroup = group;
        }

        virtual bool HasProperties()
        {
            return false;
        }

        virtual void Serialize(Serialization::IArchive& ar)
        {
        }

    private:
        CID m_nID;
        AZStd::string m_sGroup;
    };
} // namespace AudioControls
