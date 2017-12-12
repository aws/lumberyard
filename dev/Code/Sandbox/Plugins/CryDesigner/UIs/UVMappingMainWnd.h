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

#include <QMainWindow>
#include "Core/PolygonMesh.h"
#include "Objects/DesignerObject.h"

class QViewport;
class QToolbar;
struct SRenderContext;

namespace CD
{
    class UVMappingMainWnd
        : public QMainWindow
        , IEditorNotifyListener
    {
        Q_OBJECT

    public:

        UVMappingMainWnd();
        ~UVMappingMainWnd();

        void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

    public slots:

        void OnRender(const SRenderContext& rc);
        void OnViewportUpdate();

    private:

        void RenderUnwrappedMesh(const SRenderContext& rc);
        void RenderUnwrappedPolygon(const SRenderContext& rc, const PolygonPtr pPolygon);
        void UpdateObject();

        QViewport* m_pViewport;
        QToolBar* m_pToolBar;

        _smart_ptr<PolygonMesh> m_pPolygonMesh;
        _smart_ptr<DesignerObject> m_pObject;
    };
}