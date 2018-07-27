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

// Description : Manages one-off reader/writer usages for Xml serialization


#include "CryLegacy_precompiled.h"
#include "XmlSerializeHelper.h"

//////////////////////////////////////////////////////////////////////////
CXmlSerializedObject::CXmlSerializedObject(const char* szSection)
    : m_nRefCount(0)
    , m_sTag(szSection)
{
    assert(szSection && szSection[0]);
}

//////////////////////////////////////////////////////////////////////////
void CXmlSerializedObject::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->Add(*this);
    if (m_XmlNode)
    {
        m_XmlNode.GetMemoryUsage(pSizer);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CXmlSerializedObject::IsEmpty() const
{
    return (!m_XmlNode || (m_XmlNode->getChildCount() <= 0 && m_XmlNode->getNumAttributes() <= 0));
}

//////////////////////////////////////////////////////////////////////////
void CXmlSerializedObject::Reset()
{
    m_XmlNode = XmlNodeRef();
}

//////////////////////////////////////////////////////////////////////////
void CXmlSerializedObject::Serialize(TSerialize& serialize)
{
    serialize.Value(m_sTag.c_str(), m_XmlNode);
}

//////////////////////////////////////////////////////////////////////////
void CXmlSerializedObject::CreateRootNode()
{
    m_XmlNode = gEnv->pSystem->CreateXmlNode(m_sTag.c_str());
    assert(m_XmlNode);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CXmlSerializeHelper::CXmlSerializeHelper()
    : m_nRefCount(1)
    , m_pSerializer()
{
    m_pSerializer = GetISystem()->GetXmlUtils()->CreateXmlSerializer();
}

//////////////////////////////////////////////////////////////////////////
CXmlSerializeHelper::~CXmlSerializeHelper()
{
}

//////////////////////////////////////////////////////////////////////////
void CXmlSerializeHelper::GetMemoryUsage(ICrySizer* pSizer) const
{
    pSizer->Add(*this);
    pSizer->AddObject(m_pSerializer);
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<ISerializedObject> CXmlSerializeHelper::CreateSerializedObject(const char* szSection)
{
    assert(szSection && szSection[0]);
    return _smart_ptr<ISerializedObject>(new CXmlSerializedObject(szSection));
}

//////////////////////////////////////////////////////////////////////////
CXmlSerializedObject* CXmlSerializeHelper::GetXmlSerializedObject(ISerializedObject* pObject)
{
    assert(pObject);

    CXmlSerializedObject* pXmlObject = NULL;
    if (pObject && CXmlSerializedObject::GUID == pObject->GetGUID())
    {
        pXmlObject = static_cast<CXmlSerializedObject*>(pObject);
        assert(pXmlObject);
    }

    return pXmlObject;
}

//////////////////////////////////////////////////////////////////////////
bool CXmlSerializeHelper::Write(ISerializedObject* pObject, TSerializeFunc serializeFunc, void* pArgument /*= NULL*/)
{
    assert(pObject);

    bool bResult = false;

    if (CXmlSerializedObject* pXmlObject = GetXmlSerializedObject(pObject))
    {
        pXmlObject->CreateRootNode();
        ISerialize* pWriter = GetWriter(pXmlObject->GetXmlNode());

        TSerialize stateWriter(pWriter);
        bResult = serializeFunc(stateWriter, pArgument);
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
bool CXmlSerializeHelper::Read(ISerializedObject* pObject, TSerializeFunc serializeFunc, void* pArgument /*= NULL*/)
{
    assert(pObject);

    bool bResult = false;

    if (CXmlSerializedObject* pXmlObject = GetXmlSerializedObject(pObject))
    {
        ISerialize* pReader = GetReader(pXmlObject->GetXmlNode());

        TSerialize stateReader(pReader);
        bResult = serializeFunc(stateReader, pArgument);
    }

    return bResult;
}

//////////////////////////////////////////////////////////////////////////
ISerialize* CXmlSerializeHelper::GetWriter(XmlNodeRef& node)
{
    return m_pSerializer->GetWriter(node);
}

//////////////////////////////////////////////////////////////////////////
ISerialize* CXmlSerializeHelper::GetReader(XmlNodeRef& node)
{
    return m_pSerializer->GetReader(node);
}
