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

#include "pch.h"
#include "SourceAssetContent.h"
#include <IBackgroundTaskManager.h>
#include <IEditor.h>
#include "CharacterToolSystem.h"
#include "ExplorerFileList.h"
#include <Windows.h> // Sleep

namespace CharacterTool
{
    using std::vector;

    struct SourceNodeView
    {
        int nodeIndex;

        SourceNodeView()
            : nodeIndex(-1)
        {
        }

        SourceNodeView(int nodeIndex)
            : nodeIndex(nodeIndex)
        {
        }

        void Serialize(IArchive& ar)
        {
            SourceAssetContent* content = ar.FindContext<SourceAssetContent>();
            if (!content)
            {
                return;
            }

            const SourceAsset::Scene& scene = content->scene;
            SourceAsset::Settings& settings = content->settings;

            if (size_t(nodeIndex) >= scene.nodes.size())
            {
                return;
            }

            const SourceAsset::Node& node = scene.nodes[nodeIndex];
            ar(node.name, "name", "!^");
            vector<SourceNodeView> children;
            children.resize(node.children.size());
            for (size_t i = 0; i < children.size(); ++i)
            {
                children[i].nodeIndex = node.children[i];
            }
            ar(children, "children", "^=");

            if (node.mesh >= 0)
            {
                bool usedAsMesh = settings.UsedAsMesh(node.name.c_str());
                bool wasUsed = usedAsMesh;
                ar(usedAsMesh, "usedAsMesh", "^^");
                if (usedAsMesh && !wasUsed)
                {
                    content->changingView = true;
                    SourceAsset::MeshImport mesh;
                    mesh.nodeName = node.name;
                    settings.meshes.push_back(mesh);
                }
                else if (!usedAsMesh && wasUsed)
                {
                    content->changingView = true;
                    for (size_t i = 0; i < settings.meshes.size(); ++i)
                    {
                        if (settings.meshes[i].nodeName == node.name)
                        {
                            settings.meshes.erase(settings.meshes.begin() + i);
                            break;
                        }
                    }
                }
            }
            else
            {
                bool usedAsSkeleton = settings.UsedAsSkeleton(node.name.c_str());
                bool wasUsed = usedAsSkeleton;
                ar(usedAsSkeleton, "usedAsSkeleton", "^^");
                if (usedAsSkeleton && !wasUsed)
                {
                    content->changingView = true;
                    SourceAsset::SkeletonImport skel;
                    skel.nodeName = node.name;
                    settings.skeletons.push_back(skel);
                }
                else if (!usedAsSkeleton && wasUsed)
                {
                    content->changingView = true;
                    for (size_t i = 0; i < settings.meshes.size(); ++i)
                    {
                        if (settings.skeletons[i].nodeName == node.name)
                        {
                            settings.skeletons.erase(settings.skeletons.begin() + i);
                            break;
                        }
                    }
                }
            }
        }
    };

    void SourceAssetContent::Serialize(IArchive& ar)
    {
        if (ar.IsEdit())
        {
            if (state == STATE_LOADING)
            {
                string loading = "Loading Asset...";
                ar(loading, "loading", "!<");
            }
            else
            {
                Serialization::SContext<SourceAssetContent> x(ar, this);

                changingView = false;
                if (ar.OpenBlock("scene", "Scene"))
                {
                    SourceNodeView root(0);
                    ar(root, "root", "<+");
                    ar.CloseBlock();
                }

                if (!changingView)
                {
                    ar(settings, "settings", "Import Settings");
                }
            }
        }
        else
        {
            settings.Serialize(ar);
        }
    }

    // ---------------------------------------------------------------------------

    static void LoadTestScene(SourceAsset::Scene* scene)
    {
        SourceAsset::Mesh mesh;
        mesh.name = "Mesh 1";
        scene->meshes.push_back(mesh);
        mesh.name = "Mesh 2";
        scene->meshes.push_back(mesh);
        mesh.name = "Mesh 3";
        scene->meshes.push_back(mesh);

        SourceAsset::Node node;
        node.name = "Root";
        node.children.push_back(1);
        node.children.push_back(2);
        scene->nodes.push_back(node);
        node.children.clear();
        node.name = "Node 1";
        node.mesh = 0;
        scene->nodes.push_back(node);
        node.name = "Node 2";
        node.mesh = 1;
        scene->nodes.push_back(node);
    }

    struct CreateAssetManifestTask
        : IBackgroundTask
    {
        System* m_system;
        string m_assetFilename;
        string m_description;
        unsigned int m_entryId;

        CreateAssetManifestTask(System* system, const char* assetFilename, unsigned int entryId)
            : m_system(system)
            , m_assetFilename(assetFilename)
            , m_entryId(entryId)
        {
            m_description = "Preview ";
            m_description += assetFilename;

            SEntry<SourceAssetContent>* entry = m_system->sourceAssetList->GetEntryById<SourceAssetContent>(m_entryId);

            entry->content.state = SourceAssetContent::STATE_LOADING;
            m_system->sourceAssetList->CheckIfModified(m_entryId, 0, true);
        }

        void Delete() override { delete this; }
        const char* Description() const override { return m_description.c_str(); }
        const char* ErrorMessage() const override { return ""; }

        ETaskResult Work() override
        {
            Sleep(1000);
            return eTaskResult_Completed;
        }

        void Finalize() override
        {
            SEntry<SourceAssetContent>* entry = m_system->sourceAssetList->GetEntryById<SourceAssetContent>(m_entryId);
            if (!entry)
            {
                return;
            }

            if (entry->content.loadingTask != this)
            {
                return;
            }

            entry->content.loadingTask = 0;

            LoadTestScene(&entry->content.scene);
            entry->content.state = SourceAssetContent::STATE_LOADED;
            m_system->sourceAssetList->CheckIfModified(m_entryId, 0, true);
        }
    };

    // ---------------------------------------------------------------------------

    bool RCAssetLoader::Load(EntryBase* entryBase, const char* filename, LoaderContext* context)
    {
        SEntry<SourceAssetContent>* entry = static_cast<SEntry<SourceAssetContent>*>(entryBase);
        if (entry->content.loadingTask)
        {
            entry->content.loadingTask->Cancel();
        }
        CreateAssetManifestTask* task = new CreateAssetManifestTask(context->system, filename, entryBase->id);
        GetIEditor()->GetBackgroundTaskManager()->AddTask(task, eTaskPriority_RealtimePreview, eTaskThreadMask_IO);
        entry->content.loadingTask = task;
        return true;
    }
}
