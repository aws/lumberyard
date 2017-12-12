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

#include <AzCore/Component/ComponentBus.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiCanvasManagerInterface
    : public AZ::EBusTraits
{
public: // types

    typedef std::vector<AZ::EntityId> CanvasEntityList;

public:
    //! Create a canvas
    virtual AZ::EntityId CreateCanvas() = 0;

    //! Load a canvas
    virtual AZ::EntityId LoadCanvas(const AZStd::string& canvasPathname) = 0;

    //! Unload a canvas
    virtual void UnloadCanvas(AZ::EntityId canvasEntityId) = 0;

    //! Find a loaded canvas by pathname
    virtual AZ::EntityId FindLoadedCanvasByPathName(const AZStd::string& canvasPathname) = 0;

    //! Get a list of canvases that are loaded in game, this is sorted by draw order
    virtual CanvasEntityList GetLoadedCanvases() = 0;
};
typedef AZ::EBus<UiCanvasManagerInterface> UiCanvasManagerBus;
