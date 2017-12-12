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
#pragma once

#include <LyShine/IUiRenderer.h>
#include <IRenderer.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Implementation of IUiRenderInterface
//
class UiRenderer
    : public IUiRenderer
{
public: // member functions

    //! Constructor, constructed by the LyShine class
    UiRenderer();

    // IUiRenderer

    ~UiRenderer() override;
    void BeginCanvasRender(AZ::Vector2 viewportSize) override;
    void EndCanvasRender() override;
    int GetBaseState() override;
    void SetBaseState(int state) override;
    uint32 GetStencilRef() override;
    void SetStencilRef(uint32) override;
    void IncrementStencilRef() override;
    void DecrementStencilRef() override;

    // ~IUiRenderer

private:

    AZ_DISABLE_COPY_MOVE(UiRenderer);

public: // static member functions

protected: // attributes

    int     m_baseState;
    uint32  m_stencilRef;
};
