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

#include "CryLegacy_precompiled.h"
#include "XmlLoadGame.h"
#include "XmlSerializeHelper.h"

#include <IPlatformOS.h>

#if defined(AZ_RESTRICTED_PLATFORM)
    #if defined(AZ_PLATFORM_XENIA)
        #include "Xenia/XmlLoadGame_cpp_xenia.inl"
    #elif defined(AZ_PLATFORM_PROVO)
        #include "Provo/XmlLoadGame_cpp_provo.inl"
    #endif
#endif


struct CXmlLoadGame::Impl
{
    XmlNodeRef root;
    XmlNodeRef metadata;
    string       inputFile;

    std::vector<_smart_ptr<CXmlSerializeHelper> > sections;
};

CXmlLoadGame::CXmlLoadGame()
    : m_pImpl(new Impl)
{
}

CXmlLoadGame::~CXmlLoadGame()
{
}

bool CXmlLoadGame::Init(const char* name)
{
    if (GetISystem()->GetPlatformOS()->UsePlatformSavingAPI())
    {
        IPlatformOS::ISaveReaderPtr pSaveReader = GetISystem()->GetPlatformOS()->SaveGetReader(name);
        if (!pSaveReader)
        {
            return false;
        }

        size_t nFileSize;

        if ((pSaveReader->GetNumBytes(nFileSize) == IPlatformOS::eFOC_Failure) || (nFileSize <= 0))
        {
            return false;
        }

        std::vector<char> xmlData;
        xmlData.resize(nFileSize);

        if (pSaveReader->ReadBytes(&xmlData[0], nFileSize) == IPlatformOS::eFOC_Failure)
        {
            return false;
        }

        m_pImpl->root = GetISystem()->LoadXmlFromBuffer(&xmlData[0], nFileSize);
    }
    else
    {
        m_pImpl->root = GetISystem()->LoadXmlFromFile(name);
    }

    if (!m_pImpl->root)
    {
        return false;
    }

    m_pImpl->inputFile = name;

    m_pImpl->metadata = m_pImpl->root->findChild("Metadata");
    if (!m_pImpl->metadata)
    {
        return false;
    }

    return true;
}

bool CXmlLoadGame::Init(const XmlNodeRef& root, const char* fileName)
{
    m_pImpl->root = root;
    if (!m_pImpl->root)
    {
        return false;
    }

    m_pImpl->inputFile = fileName;

    m_pImpl->metadata = m_pImpl->root->findChild("Metadata");
    if (!m_pImpl->metadata)
    {
        return false;
    }

    return true;
}

const char* CXmlLoadGame::GetMetadata(const char* tag)
{
    return m_pImpl->metadata->getAttr(tag);
}

bool CXmlLoadGame::GetMetadata(const char* tag, int& value)
{
    return m_pImpl->metadata->getAttr(tag, value);
}

bool CXmlLoadGame::HaveMetadata(const char* tag)
{
    return m_pImpl->metadata->haveAttr(tag);
}

AZStd::unique_ptr<TSerialize> CXmlLoadGame::GetSection(const char* section)
{
    XmlNodeRef node = m_pImpl->root->findChild(section);
    if (!node)
    {
        return std::unique_ptr<TSerialize>();
    }

    _smart_ptr<CXmlSerializeHelper> pSerializer = new CXmlSerializeHelper;
    m_pImpl->sections.push_back(pSerializer);
    return AZStd::make_unique<TSerialize>(pSerializer->GetReader(node));
}

bool CXmlLoadGame::HaveSection(const char* section)
{
    return m_pImpl->root->findChild(section) != 0;
}

void CXmlLoadGame::Complete()
{
    delete this;
}

const char* CXmlLoadGame::GetFileName() const
{
    if (m_pImpl.get())
    {
        return m_pImpl.get()->inputFile;
    }
    return NULL;
}
