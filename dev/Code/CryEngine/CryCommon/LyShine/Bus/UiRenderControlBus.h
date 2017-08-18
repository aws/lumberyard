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
//! The UiRenderControlBus is used for controling render state changes.
//! An example use is a mask component that needs to setup stencil write before rendering its
//! components to increment the stencil buffer, switch to stencil test before rendering the child
//! elements and then do a second pass to decrement the stencil buffer.
//! The interface is designed to be flexible and could also be used for setting up scissoring or
//! rendering to a texture.
class UiRenderControlInterface
    : public AZ::ComponentBus
{
public: // types

    enum class Pass
    {
        First,
        Second
    };

public: // member functions

    virtual ~UiRenderControlInterface() {}

    //! This should setup the state required for rendering the components on this element
    //! This will also ways get called once and can optionally get called a second time
    //! after the child elements are rendered.
    //! \param pass, will be set to First for the first pass, Second for second pass
    virtual void SetupBeforeRenderingComponents(Pass pass) = 0;

    //! This should make state changes required after the components on this element
    //! have been rendered and (if pass == First) before the child elements are rendered
    //! \param pass, will be set to First for the first pass, Second for second pass
    virtual void SetupAfterRenderingComponents(Pass pass) = 0;

    //! This should make state changes needed after the child elements have been rendered.
    //! This method can also request a second pass render of the components on this element
    //! in which case SetupBeforeRenderingComponents and SetupAfterRenderingComponents will
    //! be called again but with pass set to Second.
    //! NOTE: This method is conly called once, it is not called again after the second pass
    //! since the child components are only ever rendered once.
    //! \param isSecondComponentsPassRequired, out parameter used to request second pass
    virtual void SetupAfterRenderingChildren(bool& isSecondComponentsPassRequired) = 0;

public: // static member data

    //! Only one component on a entity can implement the events
    static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
};

typedef AZ::EBus<UiRenderControlInterface> UiRenderControlBus;
