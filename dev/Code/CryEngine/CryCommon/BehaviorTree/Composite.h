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

#ifndef CRYINCLUDE_CRYCOMMON_BEHAVIORTREE_COMPOSITE_H
#define CRYINCLUDE_CRYCOMMON_BEHAVIORTREE_COMPOSITE_H
#pragma once

#include "Node.h"

#ifdef USING_BEHAVIOR_TREE_SERIALIZATION
#include <Serialization/BoostSharedPtr.h>
#endif

namespace BehaviorTree
{
    template <typename ChildType = INodePtr>
    class Composite
        : public Node
    {
        typedef Node BaseClass;

    public:
        void AddChild(const ChildType& child)
        {
            m_children.push_back(child);
        }

        virtual LoadResult LoadFromXml(const XmlNodeRef& xml, const LoadContext& context) override
        {
            return BaseClass::LoadFromXml(xml, context);
        }

#ifdef USING_BEHAVIOR_TREE_SERIALIZATION
        virtual void Serialize(Serialization::IArchive& archive) override
        {
            archive(m_children, "children", "^[+<>]");

            if (archive.IsEdit())
            {
                for (int i = 0; i < m_children.size(); ++i)
                {
                    if (!m_children[i])
                    {
                        archive.Error(m_children, "Node must be specified.");
                    }
                }
            }

            BaseClass::Serialize(archive);
        }
#endif

#ifdef USING_BEHAVIOR_TREE_XML_DESCRIPTION_CREATION
        virtual XmlNodeRef CreateXmlDescription() override
        {
            XmlNodeRef xml = GetISystem()->CreateXmlNode("Composite");

            for (int i = 0; i < m_children.size(); ++i)
            {
                if (m_children[i])
                {
                    xml->addChild(m_children[i]->CreateXmlDescription());
                }
            }

            return xml;
        }
#endif

    protected:
        typedef std::vector<ChildType> Children;
        Children m_children;
    };

	// Workaround for VS2013 - ADL lookup issue with std::vector iterator, disabling ADL for iter_swap on this compiler
	#if defined(AZ_COMPILER_MSVC) && AZ_COMPILER_MSVC <= 1800
		AZSTD_ADL_FIX_FUNCTION_SPEC_1_2(iter_swap, AZStd::shared_ptr<INode>*);
	#endif

	class CompositeWithChildLoader
        : public Composite<INodePtr>
    {
        typedef Composite<INodePtr> BaseClass;

    public:
        virtual LoadResult LoadFromXml(const XmlNodeRef& xml, const LoadContext& context) override
        {
            IF_UNLIKELY (BaseClass::LoadFromXml(xml, context) == LoadFailure)
            {
                return LoadFailure;
            }

            return ConstructChildNodesFromXml(xml, context);
        }

    protected:
        LoadResult ConstructChildNodesFromXml(const XmlNodeRef& xml, const LoadContext& context)
        {
            for (int i = 0; i < xml->getChildCount(); ++i)
            {
                INodePtr child = context.nodeFactory.CreateNodeFromXml(xml->getChild(i), context);

                if (child)
                {
                    AddChild(child);
                }
                else
                {
                    return LoadFailure;
                }
            }

            return !m_children.empty() ? LoadSuccess : LoadFailure;
        }
    };
}

#endif // CRYINCLUDE_CRYCOMMON_BEHAVIORTREE_COMPOSITE_H
