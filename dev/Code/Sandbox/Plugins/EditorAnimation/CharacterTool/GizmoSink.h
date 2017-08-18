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

#pragma once
#include <vector>
#include <Serialization/Decorators/IGizmoSink.h>
#include <Serialization/Decorators/LocalFrame.h>
#include "../EditorCommon/ManipScene.h"
#include "Explorer.h"

namespace Manip {
    class CScene;
}

namespace CharacterTool
{
    using std::vector;

    class CharacterDocument;
    class CharacterSpaceProvider
        : public Manip::ISpaceProvider
    {
    public:
        CharacterSpaceProvider(CharacterDocument* document)
            : m_document(document) {}
        Manip::SSpaceAndIndex FindSpaceIndexByName(int spaceType, const char* name, int parentsUp) const override;
        QuatT GetTransform(const Manip::SSpaceAndIndex& si) const override;
    private:
        CharacterDocument* m_document;
    };

    struct ExplorerEntry;

    enum GizmoLayer
    {
        GIZMO_LAYER_NONE = -1,
        GIZMO_LAYER_SCENE,
        GIZMO_LAYER_CHARACTER,
        GIZMO_LAYER_ANIMATION,
        GIZMO_LAYER_COUNT
    };

    class GizmoSink
        : public Serialization::IGizmoSink
    {
    public:
        GizmoSink()
            : m_lastIndex(0)
            , m_scene(0) {}
        void SetScene(Manip::CScene* scene) { m_scene = scene; }
        ExplorerEntry* ActiveEntry() { return m_activeEntry.get(); }

        void Clear(GizmoLayer layer);
        void BeginWrite(ExplorerEntry* entry, GizmoLayer layer);
        void BeginRead(GizmoLayer layer);
        void EndRead();

        int CurrentGizmoIndex() const override;
        int Write(const Serialization::LocalPosition& position, const Serialization::GizmoFlags& gizmoFlags, const void* handle) override;
        int Write(const Serialization::LocalOrientation& position, const Serialization::GizmoFlags& gizmoFlags, const void* handle) override;
        int Write(const Serialization::LocalFrame& position, const Serialization::GizmoFlags& gizmoFlags, const void* handle) override;
        bool Read(Serialization::LocalPosition* position, Serialization::GizmoFlags* gizmoFlags, const void* handle) override;
        bool Read(Serialization::LocalOrientation* position, Serialization::GizmoFlags* gizmoFlags, const void* handle) override;
        bool Read(Serialization::LocalFrame* position, Serialization::GizmoFlags* gizmoFlags, const void* handle) override;
        void SkipRead() override;
        void Reset(const void* handle);
    private:
        int m_lastIndex;
        int m_currentLayer;
        Manip::CScene* m_scene;
        _smart_ptr<ExplorerEntry> m_activeEntry;
    };
}
