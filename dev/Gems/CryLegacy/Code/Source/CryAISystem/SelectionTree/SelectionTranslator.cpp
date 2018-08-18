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
#include "SelectionTranslator.h"


bool SelectionTranslator::LoadFromXML(const BlockyXmlBlocks::Ptr& blocks, const SelectionTree& selectionTree,
    const XmlNodeRef& rootNode, const char* scopeName, const char* fileName)
{
    typedef std::map<string, string> TempDirectTranslations;
    TempDirectTranslations directTranslations;

    typedef std::multimap<string, QualifiedTranslation> TempQualifiedTranslations;
    TempQualifiedTranslations qualifiedTranslations;

    BlockyXmlNodeRef blockyNode(blocks, scopeName, rootNode, fileName);
    while (XmlNodeRef childNode = blockyNode.next())
    {
        if (!_stricmp(childNode->getTag(), "Map"))
        {
            const char* nodeName = 0;
            if (childNode->haveAttr("node"))
            {
                childNode->getAttr("node", &nodeName);
            }
            else
            {
                AIWarning("Missing 'node' attribute for tag '%s' in file '%s' at line %d.",
                    childNode->getTag(), fileName, childNode->getLine());

                return false;
            }

            const char* targetName = 0;
            if (childNode->haveAttr("target"))
            {
                childNode->getAttr("target", &targetName);
            }
            else
            {
                AIWarning("Missing 'target' attribute for tag '%s' in file '%s' at line %d.",
                    childNode->getTag(), fileName, childNode->getLine());

                return false;
            }

            // add even qualified node names, just for duplicate checking
            std::pair<TempDirectTranslations::iterator, bool> iresult = directTranslations.insert(
                    TempDirectTranslations::value_type(nodeName, targetName));

            if (!iresult.second)
            {
                AIWarning("Duplicate translation definition '%s' in file '%s' at line %d.",
                    nodeName, fileName, childNode->getLine());

                return false;
            }

            if (strchr(nodeName, ':'))
            {
                stack_string name(nodeName);
                size_t colon = name.rfind(':');

                string leafName(name.Right(name.length() - colon - 1));
                name = name.Left(name.length() - leafName.length() - 1);

                TempQualifiedTranslations::iterator it = qualifiedTranslations.insert(
                        TempQualifiedTranslations::value_type(leafName, QualifiedTranslation()));

                QualifiedTranslation& qualified = it->second;
                qualified.target = targetName;

                int start = 0;
                stack_string parentName = name.Tokenize(":", start);

                while (!parentName.empty())
                {
                    qualified.ancestors.push_back(parentName);
                    parentName = name.Tokenize(":", start);
                }
            }
        }
        else
        {
            AIWarning("Unexpected tag '%s' in file '%s' at line %d. 'Map' expected.",
                childNode->getTag(), fileName, childNode->getLine());

            return false;
        }
    }

    uint32 nodeCount = selectionTree.GetNodeCount();
    for (uint32 i = 0; i < nodeCount; ++i)
    {
        const SelectionTreeNode& node = selectionTree.GetNodeAt(i);
        if (node.GetType() != SelectionTreeNode::Leaf)
        {
            continue;
        }

        TempDirectTranslations::iterator dirTranslationsIt = directTranslations.find(CONST_TEMP_STRING(node.GetName()));
        if (dirTranslationsIt != directTranslations.end())
        {
            m_translations.insert(Translations::value_type(node.GetNodeID(), dirTranslationsIt->second));
        }
        else
        {
            TempQualifiedTranslations::iterator it = qualifiedTranslations.find(CONST_TEMP_STRING(node.GetName()));

            while ((it != qualifiedTranslations.end()) && (it->first == node.GetName()))
            {
                QualifiedTranslation& qualified = it->second;
                uint32 ancestorCount = qualified.ancestors.size();

                SelectionNodeID parentID = node.GetParentID();
                while (ancestorCount && parentID)
                {
                    const SelectionTreeNode& parentNode = selectionTree.GetNode(parentID);
                    if (qualified.ancestors[ancestorCount - 1] != parentNode.GetName())
                    {
                        break;
                    }

                    --ancestorCount;
                    parentID = parentNode.GetParentID();
                }

                if (!ancestorCount)
                {
                    m_translations.insert(Translations::value_type(node.GetNodeID(), qualified.target));
                    break;
                }

                ++it;
            }
        }
    }

    return true;
}

const char* SelectionTranslator::GetTranslation(const SelectionNodeID& nodeID) const
{
    Translations::const_iterator it = m_translations.find(nodeID);
    if (it != m_translations.end())
    {
        return it->second.c_str();
    }

    return 0;
}