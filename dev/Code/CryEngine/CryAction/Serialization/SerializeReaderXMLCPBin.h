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

#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_SERIALIZEREADERXMLCPBIN_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_SERIALIZEREADERXMLCPBIN_H
#pragma once

#include "SimpleSerialize.h"
#include <stack>
#include <ITimer.h>
#include <IValidator.h>
#include <ISystem.h>
#include "XMLCPBin/Reader/XMLCPB_ReaderInterface.h"

class CSerializeReaderXMLCPBin
    : public CSimpleSerializeImpl<true, eST_SaveGame>
{
public:
    CSerializeReaderXMLCPBin(XMLCPB::CNodeLiveReaderRef nodeRef, XMLCPB::CReaderInterface& binReader);

    template <class T_Value>
    bool Value(const char* name, T_Value& value)
    {
        DefaultValue(value); // Set input value to default.
        if (m_nErrors)
        {
            return false;
        }

        if (!GetAttr(CurNode(), name, value))
        {
            // the attrs are not saved if they already had the default value
            return false;
        }
        return true;
    }

    bool Value(const char* name, ScriptAnyValue& value);
    bool Value(const char* name, int8& value);
    bool Value(const char* name, string& value);
    bool Value(const char* name, CTimeValue& value);
    bool Value(const char* name, XmlNodeRef& value);
    bool ValueByteArray(const char* name, uint8*& rdata, uint32& outSize);

    template <class T_Value, class T_Policy>
    bool Value(const char* name, T_Value& value, const T_Policy& policy)
    {
        return Value(name, value);
    }

    void BeginGroup(const char* szName);
    bool BeginOptionalGroup(const char* szName, bool condition);
    void EndGroup();
    const char* GetStackInfo() const;

private:
    template <class T_Value>
    ILINE bool GetAttr(XMLCPB::CNodeLiveReaderRef& node, const char* name, T_Value& value)
    {
        bool found = node->ReadAttr(name, value);
        return found;
    }
    ILINE bool GetAttr(XMLCPB::CNodeLiveReaderRef& node, const char* name, SSerializeString& value)
    {
        const char* pVal = NULL;
        bool found = node->ReadAttr(name, pVal);
        if (found)
        {
            value = pVal;
        }
        return found;
    }
    ILINE bool GetAttr(XMLCPB::CNodeLiveReaderRef& node, const char* name, const string& value)
    {
        assert(false);
        return false;
    }
    ILINE bool GetAttr(XMLCPB::CNodeLiveReaderRef& node, const char* name, SNetObjectID& value)
    {
        assert(false);
        return false;
    }

    void RecursiveReadIntoXmlNodeRef(XMLCPB::CNodeLiveReaderRef BNode, XmlNodeRef& xmlNode);

private:
    //CTimeValue m_curTime;
    XMLCPB::CNodeLiveReaderRef& CurNode() { assert(!m_nodeStack.empty()); return m_nodeStack.back(); }

    int m_nErrors;
    std::vector<XMLCPB::CNodeLiveReaderRef> m_nodeStack;
    XMLCPB::CReaderInterface& m_binReader;

    bool ReadScript(XMLCPB::CNodeLiveReaderRef node, ScriptAnyValue& value);

    //////////////////////////////////////////////////////////////////////////
    // Set Defaults.
    //////////////////////////////////////////////////////////////////////////
    void DefaultValue(bool& v) const { v = false; };
    void DefaultValue(float& v) const { v = 0; };
    void DefaultValue(double& v) const { v = 0; };
    void DefaultValue(int8& v) const { v = 0; };
    void DefaultValue(uint8& v) const { v = 0; };
    void DefaultValue(int16& v) const { v = 0; };
    void DefaultValue(uint16& v) const { v = 0; };
    void DefaultValue(int32& v) const { v = 0; };
    void DefaultValue(uint32& v) const { v = 0; };
    void DefaultValue(int64& v) const { v = 0; };
    void DefaultValue(uint64& v) const { v = 0; };
    void DefaultValue(Vec2& v) const { v.x = 0; v.y = 0; };
    void DefaultValue(Vec3& v) const { v.x = 0; v.y = 0; v.z = 0; };
    void DefaultValue(Ang3& v) const { v.x = 0; v.y = 0; v.z = 0; };
    void DefaultValue(Quat& v) const { v.w = 1.0f; v.v.x = 0; v.v.y = 0; v.v.z = 0; };
    void DefaultValue(ScriptAnyValue& v) const {  };
    void DefaultValue(CTimeValue& v) const { v.SetValue(0); };
    //void DefaultValue( char *str ) const { if (str) str[0] = 0; };
    void DefaultValue(string& str) const { str = ""; };
    void DefaultValue(const string& str) const {};
    void DefaultValue(SNetObjectID& id) const {};
    void DefaultValue(SSerializeString& str) const {};
    void DefaultValue(uint8*& rdata, uint32& outSize) const;
    //////////////////////////////////////////////////////////////////////////
};

#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_SERIALIZEREADERXMLCPBIN_H
