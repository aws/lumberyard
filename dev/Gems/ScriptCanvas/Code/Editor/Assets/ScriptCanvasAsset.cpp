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

#include "precompiled.h"

#include <AzCore/Asset/AssetManagerBus.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>

#include <ScriptCanvas/Bus/ScriptCanvasBus.h>
#include <Core/ScriptCanvasBus.h>
#include <ScriptCanvas/Components/EditorGraph.h>

namespace ScriptCanvasEditor
{
    ScriptCanvasData::ScriptCanvasData()
    {

    }

    ScriptCanvasData::~ScriptCanvasData()
    {

    }

    ScriptCanvasData::ScriptCanvasData(ScriptCanvasData&& other)
        : m_scriptCanvasEntity(AZStd::move(other.m_scriptCanvasEntity))
    {
    }

    ScriptCanvasData& ScriptCanvasData::operator=(ScriptCanvasData&& other)
    {
        m_scriptCanvasEntity = AZStd::move(other.m_scriptCanvasEntity);
        return *this;
    }

    void ScriptCanvasData::Reflect(AZ::ReflectContext* reflectContext)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(reflectContext))
        {
            serializeContext->Class<ScriptCanvasData>()
                ->Field("m_scriptCanvas", &ScriptCanvasData::m_scriptCanvasEntity)
                ;
        }
    }

    AZ::Entity* ScriptCanvasAsset::GetScriptCanvasEntity() const
    {
        return m_data.m_scriptCanvasEntity.get();
    }

    void ScriptCanvasAsset::SetScriptCanvasEntity(AZ::Entity* scriptCanvasEntity)
    {
        if (m_data.m_scriptCanvasEntity.get() != scriptCanvasEntity)
        {
            m_data.m_scriptCanvasEntity.reset(scriptCanvasEntity);
        }
    }

    ScriptCanvasData& ScriptCanvasAsset::GetScriptCanvasData()
    {
        return m_data;
    }

    const ScriptCanvasData& ScriptCanvasAsset::GetScriptCanvasData() const
    {
        return m_data;
    }
} // namespace ScriptCanvasEditor
