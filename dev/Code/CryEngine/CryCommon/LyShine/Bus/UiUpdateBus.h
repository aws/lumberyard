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
class UiUpdateInterface
    : public AZ::ComponentBus
{
public: // member functions

    virtual ~UiUpdateInterface() {}

    //! Update the component. This is called when the game is running.
    //! It is different from the TickBus in that the update order follows the element hierarchy.
    virtual void Update() = 0;

    //! Update the component while in the editor.
    //! This is called every frame when in the editor and the game is NOT running.
    //! This may be eliminated when we have a preview mode in the editor which calls the
    //! regular update.
    virtual void UpdateInEditor() {}

public: // static member data

    //! Multiple components on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
};

typedef AZ::EBus<UiUpdateInterface> UiUpdateBus;
