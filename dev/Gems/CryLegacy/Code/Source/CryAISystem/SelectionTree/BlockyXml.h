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

#ifndef CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_BLOCKYXML_H
#define CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_BLOCKYXML_H
#pragma once

/*
    This file implements a simple "lazy" XML reference resolver.

    It works by collecting all the available reference-able XML nodes
    and as a first step. Then it will resolve references while iterating through nodes.

    It supports 1 level of scoping, meaning it will look for references first in the specified scope
    and then on the global scope shall that fail.
*/

class BlockyXmlBlocks
    : public _reference_target_t
{
public:
    typedef _smart_ptr<BlockyXmlBlocks> Ptr;

    struct Block
    {
        Block()
        {
        }

        Block(const char* _fileName, const XmlNodeRef& _blockNode)
            : fileName(_fileName)
            , blockNode(_blockNode)
        {
        }

        string fileName;
        XmlNodeRef blockNode;
    };

    bool AddBlock(const char* scopeName, const char* name, const XmlNodeRef& node, const char* fileName = "<unknownFile>");
    Block GetBlock(const char* scopeName, const char* blockName);

private:
    typedef std::map<string, Block> Blocks;
    typedef std::map<string, Blocks> BlockScopes;
    BlockScopes m_scopes;
};


class BlockyXmlNodeRef
{
public:
    BlockyXmlNodeRef();
    BlockyXmlNodeRef(const BlockyXmlBlocks::Ptr& blocks, const char* scopeName, const XmlNodeRef& rootNode, const char* fileName = "<unknownFile>");
    BlockyXmlNodeRef(const BlockyXmlBlocks::Ptr& blocks, const char* scopeName, const BlockyXmlBlocks::Block& block);
    BlockyXmlNodeRef(const BlockyXmlNodeRef& other);

    void first();
    XmlNodeRef next();

private:
    XmlNodeRef m_rootNode;
    string m_scopeName;
    string m_fileName;
    BlockyXmlBlocks::Ptr m_blocks;
    std::unique_ptr<BlockyXmlNodeRef> m_currRef;
    uint16 m_currIdx;
};

#endif // CRYINCLUDE_CRYAISYSTEM_SELECTIONTREE_BLOCKYXML_H
