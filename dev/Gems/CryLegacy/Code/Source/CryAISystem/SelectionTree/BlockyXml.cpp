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
#include "BlockyXml.h"


bool BlockyXmlBlocks::AddBlock(const char* scopeName, const char* name, const XmlNodeRef& node, const char* fileName)
{
    assert(scopeName && name);

    std::pair<BlockScopes::iterator, bool> siresult = m_scopes.insert(
            BlockScopes::value_type(scopeName, Blocks()));
    Blocks& blocks = siresult.first->second;

    std::pair<Blocks::iterator, bool> iresult = blocks.insert(
            Blocks::value_type(name, Block(fileName, node)));

    if (iresult.second)
    {
        return true;
    }

    AIWarning("Duplicate block definition '%s' in file '%s' at line %d. First defined in '%s'.",
        name, fileName, node->getLine(), iresult.first->second.fileName.c_str());

    return false;
}

BlockyXmlBlocks::Block BlockyXmlBlocks::GetBlock(const char* scopeName, const char* blockName)
{
    assert(scopeName && blockName);

    BlockScopes::iterator scopeIt = m_scopes.find(CONST_TEMP_STRING(scopeName));

    bool globalScope = false;
    if ((scopeIt == m_scopes.end()) && (_stricmp(scopeName, "Global")))
    {
        globalScope = true;
        scopeIt = m_scopes.find(CONST_TEMP_STRING("Global"));
    }

    while (scopeIt != m_scopes.end())
    {
        Blocks& blocks = scopeIt->second;
        Blocks::iterator it = blocks.find(blockName);

        if (it != blocks.end())
        {
            return it->second;
        }
        else if (!globalScope)
        {
            globalScope = true;
            scopeIt = m_scopes.find(CONST_TEMP_STRING("Global"));
        }
        else
        {
            break;
        }
    }

    return Block();
}

BlockyXmlNodeRef::BlockyXmlNodeRef()
    : m_currIdx(0)
{
}

BlockyXmlNodeRef::BlockyXmlNodeRef(const BlockyXmlBlocks::Ptr& blocks, const char* scopeName, const XmlNodeRef& rootNode,
    const char* fileName)
    : m_rootNode(rootNode)
    , m_scopeName(scopeName)
    , m_fileName(fileName)
    , m_blocks(blocks)
    , m_currIdx(0)
{
}

BlockyXmlNodeRef::BlockyXmlNodeRef(const BlockyXmlBlocks::Ptr& blocks, const char* scopeName, const BlockyXmlBlocks::Block& block)
    : m_rootNode(block.blockNode)
    , m_scopeName(scopeName)
    , m_fileName(block.fileName)
    , m_blocks(blocks)
    , m_currIdx(0)
{
}

BlockyXmlNodeRef::BlockyXmlNodeRef(const BlockyXmlNodeRef& other)
    : m_rootNode(other.m_rootNode)
    , m_scopeName(other.m_scopeName)
    , m_fileName(other.m_fileName)
    , m_blocks(other.m_blocks)
    , m_currIdx(other.m_currIdx)
{
    if (other.m_currRef.get())
    {
        m_currRef.reset(new BlockyXmlNodeRef(*other.m_currRef.get()));
    }
}

void BlockyXmlNodeRef::first()
{
    m_currIdx = 0;
    m_currRef.reset();
}

XmlNodeRef BlockyXmlNodeRef::next()
{
    if (m_currIdx >= m_rootNode->getChildCount())
    {
        return XmlNodeRef();
    }

    XmlNodeRef currNode = m_rootNode->getChild(m_currIdx);
    if (_stricmp(currNode->getTag(), "Ref"))
    {
        ++m_currIdx;
        return currNode;
    }

    if (m_currRef.get())
    {
        if (currNode = m_currRef->next())
        {
            return currNode;
        }
        else
        {
            m_currRef.reset();
            ++m_currIdx;
            return next();
        }
    }

    const char* blockName = 0;
    if (currNode->haveAttr("name"))
    {
        currNode->getAttr("name", &blockName);
    }

    if (m_blocks != NULL)
    {
        const BlockyXmlBlocks::Block& block = m_blocks->GetBlock(m_scopeName.c_str(), blockName);
        if (block.blockNode)
        {
            m_currRef.reset(new BlockyXmlNodeRef(m_blocks, m_scopeName.c_str(), block));

            return next();
        }
    }

    AIWarning("Unresolved block reference '%s' in file '%s' at line %d.", blockName, m_fileName.c_str(), currNode->getLine());

    return XmlNodeRef();
}