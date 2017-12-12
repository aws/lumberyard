#pragma once

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

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/JSON/document.h>
#include <SceneAPI/SceneCore/Events/GraphMetaInfoBus.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/Utilities/PatternMatcher.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneCore
        {
            class PatternMatcher;
        }
        namespace SceneData
        {
            class SoftNameTypes
                : public Events::GraphMetaInfoBus::Handler
            {
            public:
                AZ_CLASS_ALLOCATOR_DECL

                SoftNameTypes();
                ~SoftNameTypes() override;

                bool InitializeFromConfigFile(const char* filePath);
                bool InitializeFromConfigFile(const AZStd::string& filePath);
                void InitializeWithDefaults();

                // Uses a pattern matcher to compare the name of the node to the pattern and accepts the node if it matches.
                //      If "includeChildren" is true, the children of an accepted node are also included.
                void AddNodeNameBasedVirtualType(const AZStd::string& name, const SceneCore::PatternMatcher& matcher, bool includeChildren);
                void AddNodeNameBasedVirtualType(AZStd::string&& name, const SceneCore::PatternMatcher& matcher, bool includeChildren);
                void AddNodeNameBasedVirtualType(const AZStd::string& name, SceneCore::PatternMatcher&& matcher, bool includeChildren);
                void AddNodeNameBasedVirtualType(AZStd::string&& name, SceneCore::PatternMatcher&& matcher, bool includeChildren);

                // Uses a pattern matcher to compare the file to the pattern. If it matches nodes are accepted if they derive
                //      or are of a type in the "types" list. If "inclusiveMatch" is false, nodes that match the "types" are
                //      rejected and all other nodes are accepted.
                void AddFileNameBasedVirtualType(const AZStd::string& name, const SceneCore::PatternMatcher& matcher,
                    const AZStd::vector<AZ::Uuid>& types, bool inclusiveMatch);
                void AddFileNameBasedVirtualType(AZStd::string&& name, const SceneCore::PatternMatcher& matcher,
                    const AZStd::vector<AZ::Uuid>& types, bool inclusiveMatch);
                void AddFileNameBasedVirtualType(const AZStd::string& name, SceneCore::PatternMatcher&& matcher,
                    const AZStd::vector<AZ::Uuid>& types, bool inclusiveMatch);
                void AddFileNameBasedVirtualType(AZStd::string&& name, SceneCore::PatternMatcher&& matcher,
                    const AZStd::vector<AZ::Uuid>& types, bool inclusiveMatch);
                void AddFileNameBasedVirtualType(const AZStd::string& name, const SceneCore::PatternMatcher& matcher,
                    AZStd::vector<AZ::Uuid>&& types, bool inclusiveMatch);
                void AddFileNameBasedVirtualType(AZStd::string&& name, const SceneCore::PatternMatcher& matcher,
                    AZStd::vector<AZ::Uuid>&& types, bool inclusiveMatch);
                void AddFileNameBasedVirtualType(const AZStd::string& name, SceneCore::PatternMatcher&& matcher,
                    AZStd::vector<AZ::Uuid>&& types, bool inclusiveMatch);
                void AddFileNameBasedVirtualType(AZStd::string&& name, SceneCore::PatternMatcher&& matcher,
                    AZStd::vector<AZ::Uuid>&& types, bool inclusiveMatch);
                
                bool IsVirtualType(const Containers::Scene& scene, Containers::SceneGraph::NodeIndex node);

                void GetVirtualTypes(AZStd::set<Crc32>& types, const Containers::Scene& scene,
                    Containers::SceneGraph::NodeIndex node) override;
                void GetVirtualTypeName(AZStd::string& name, Crc32 type) override;

                void Clear();

            private:
                class VirtualType
                {
                public:
                    explicit VirtualType(AZStd::string&& name);

                    virtual bool IsVirtualType(const Containers::Scene& scene, Containers::SceneGraph::NodeIndex node) const = 0;

                    virtual const AZStd::string& GetName() const;
                    virtual Crc32 GetNameId() const;

                protected:
                    VirtualType() = default;

                    void SetName(AZStd::string&& name);

                    AZStd::string m_name;
                    Crc32 m_nameId;
                };

                class NodeNameBasedVirtualType : public VirtualType
                {
                public:
                    NodeNameBasedVirtualType(AZStd::string&& name, SceneCore::PatternMatcher&& matcher, bool includeChildren);
                    
                    static AZStd::unique_ptr<NodeNameBasedVirtualType> LoadFromJson(AZStd::string&& name, 
                        rapidjson::Document::ConstMemberIterator member);

                    bool IsVirtualType(const Containers::Scene& scene, Containers::SceneGraph::NodeIndex node) const override;

                protected:
                    NodeNameBasedVirtualType();

                    SceneCore::PatternMatcher m_matcher;
                    bool m_includeChildren;
                };

                class FileNameBasedVirtualType : public VirtualType
                {
                public:
                    FileNameBasedVirtualType(AZStd::string&& name, SceneCore::PatternMatcher&& matcher, AZStd::vector<AZ::Uuid>&& types, bool inclusiveMatch);

                    static AZStd::unique_ptr<FileNameBasedVirtualType> LoadFromJson(AZStd::string&& name,
                        rapidjson::Document::ConstMemberIterator member);

                    bool IsVirtualType(const Containers::Scene& scene, Containers::SceneGraph::NodeIndex node) const override;

                protected:
                    FileNameBasedVirtualType();

                    AZStd::vector<AZ::Uuid> m_nodeTypes;
                    SceneCore::PatternMatcher m_matcher;
                    
                    mutable const Containers::Scene* m_cachedScene;
                    mutable bool m_cachedNameMatch;
                    
                    // If true a node matches if it's type is in m_nodeTypes, if false when it's not.
                    bool m_matchInclusive;
                };

                AZStd::string LoadTextFile(const char* filePath) const;
                bool ParseJson(const AZStd::string& json);

                AZStd::vector<AZStd::unique_ptr<VirtualType>> m_types;
                bool m_connected;
            };
        } // SceneData
    } // SceneAPI
} // AZ
