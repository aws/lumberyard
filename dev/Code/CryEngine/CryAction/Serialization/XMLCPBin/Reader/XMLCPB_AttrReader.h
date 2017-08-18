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

#ifndef CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_ATTRREADER_H
#define CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_ATTRREADER_H
#pragma once

#include "../XMLCPB_Common.h"

namespace XMLCPB {
    class CReader;

    class CAttrReader
    {
        friend class CReader;
    public:

        CAttrReader(CReader& Reader);
        CAttrReader(const CAttrReader& other)
            : m_Reader(other.m_Reader) { *this = other; }
        ~CAttrReader() {}

        CAttrReader& operator =(const CAttrReader& other)
        {
            if (this != &other)
            {
                m_type = other.m_type;
                m_nameId = other.m_nameId;
                m_addr = other.m_addr;
            }
            return *this;
        }

        void InitFromCompact(FlatAddr offset, uint16 header);

        FlatAddr GetAddrNextAttr() const;
        const char* GetName() const;
        eAttrDataType GetBasicDataType() const { return XMLCPB::GetBasicDataType(m_type); }

        void Get(int32& val) const;
        void Get(int64& val) const;
        void Get(uint64& val) const;
        void Get(int16& val) const;
        void Get(int8& val) const;
        void Get(uint8& val) const;
        void Get(uint16& val) const;
        void Get(uint& val) const;
        void Get(bool& val) const;
        void Get(float& val) const;
        void Get(double& val) const;
        void Get(Vec2& val) const;
        void Get(Ang3& val) const;
        void Get(Vec3& val) const;
        void Get(Quat& val) const;
        void Get(const char*& pStr) const;
        void Get(uint8*& rdata, uint32& outSize) const;


        void GetValueAsString(string& str) const;


        #ifndef _RELEASE
        StringID GetNameId() const { return m_nameId; }  // debug help
        #endif

    private:

        FlatAddr GetDataAddr() const { return m_addr;  }
        float UnpackFloatInSemiConstType(uint8 mask, uint32 ind, FlatAddr& addr) const;


        template<class T>
        void ValueToString(string& str, const char* formatString) const;

        eAttrDataType       m_type;
        StringID                m_nameId;
        FlatAddr                m_addr;
        CReader&                m_Reader;
    };

    template<class T>
    void CAttrReader::ValueToString(string& str, const char* formatString) const
    {
        T val;
        Get(val);
        str.Format(formatString, val);
    }
} // end namespace
#endif // CRYINCLUDE_CRYACTION_SERIALIZATION_XMLCPBIN_READER_XMLCPB_ATTRREADER_H
