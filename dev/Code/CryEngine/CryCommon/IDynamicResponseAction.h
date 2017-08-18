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

// Description : An Interface for actions that can be executed by Response Segments

#ifndef _DYNAMICRESPONSEACTION_H_
#define _DYNAMICRESPONSEACTION_H_

#include <IXml.h>

namespace DRS
{
    struct IResponseAction;

    struct IResponseActionCreator
    {
        // <title CreateOrGetActionFromXML>
        // Syntax: its called by the DRS system, when a action of the type is requested by a response definition
        // Description:
        //      Will Create a new Action with the parameters from the xml-node, or may return a existing one, if a action with these parameters already exist
        // Arguments:
        //      pResponseSegmentNode - an pointer to an cry-xml-node from the response definition which contains the data for action.
        // Returns:
        //      Will return the generated or re-used action if successful. 0 otherwise
        virtual IResponseAction* CreateOrGetActionFromXML(XmlNodeRef pResponseSegmentNode) = 0;

        // <title Release>
        // Syntax: its called by the DRS system, on shutdown
        // Description:
        //      Will Release the ActionCreator. Depending on the caching strategy of the creator, it might destroy all created actions. So dont use any Actions of the type after calling release.
        // Arguments:
        // Returns:
        // See Also:
        virtual void Release() = 0;

    protected:
        virtual ~IResponseActionCreator() {} //we want to be destroyed only via out release function
    };

    struct IResponseInstance;
    struct IResponseActionInstance;

    struct IResponseAction
    {
        // <title Execute>
        // Syntax: its called by the DRS system, when a response segment is executed which contains a action of this type
        // Description:
        //      Will execute the action. If the action is not an instantaneous action, it can return an ActionInstance, which is then updated as long as needed by the DRS to finish the execution of the action
        // Arguments:
        //      pResponseInstance - the ResponseInstance that requested the execution of the action. From the ResponseInstance the active actor and the context variables of signal can be obtained, if needed for the execution
        // Returns:
        //      if needed an actionInstance is returned. If the action was already completed during the function 0 will be returned. (For instant-effect-actions like setVariable)
        // See Also:
        //      IResponseInstance
        virtual IResponseActionInstance* Execute(IResponseInstance* pResponseInstance) = 0;

        // <title Release>
        // Syntax: its called by the DRS, when not needed anymore. Normally, when a ResponseSegment is unloaded/destroyed.
        // Description:
        //      Will Release the Action. What is actually happening depends on the implementation, it might decide not to actually destroy it, if the action-creator is caching actions for example.
        //      Must match the allocation strategy used in the corresponding IResponseActionCreator::CreateOrGetActionFromXML)
        // Arguments:
        // Returns:
        // See Also:
        virtual void Release() = 0;

        // <title GetVerboseInfo>
        // Syntax: Will be called from the DRS for debug output
        // Description:
        //      Just some debug informations, that describe the action and its parameter. So that a history-log with executed actions can be generated.
        // Arguments:
        // Returns:
        //      a descriptive string containing the action name and its current parameter
        // See Also:
        virtual const string GetVerboseInfo() = 0;
    };

    //one concrete running instance of the actions. (There might be actions that dont need instances, because their action is instantaneously.
    struct IResponseActionInstance
    {
        enum eCurrentState
        {
            CS_RUNNING,
            CS_FINISHED,
            CS_CANCELED,
            CS_ERROR
        };

        // <title Update>
        // Syntax: its called by the DRS system, as long as the action instance is running
        // Description:
        //      This method continues the execution of an started action that takes some time to finish. Its called from the DRS System until it stops returning CS_RUNNING.
        // Arguments:
        // Returns:
        //      returns the current state of the execution of the action. CS_RUNNING means still needs time to finish, CS_FINISHED means finished successful,
        //      CS_CANCELED means the action was canceled from the outside before it was finished, CS_ERROR means an error happened during execution
        // See Also:
        virtual eCurrentState Update() = 0;

        // <title Cancel>
        // Syntax: its called by the DRS system, when someone requested a cancellation of a running response (segment)
        // Description:
        //      Will be called when someone requested a cancellation of a running response (segment). Its up to the action instance how to handle this request.
        //      There might be cases for example, where the action decides to first finish the current sentence before actually stop. So the DRS will continue updating the ActionInstance,
        //      until the actionInstance decides to stop (by returning CS_CANCELED or CS_FINISHED in the Update method).
        // Arguments:
        // Returns:
        // See Also:
        //      Update()
        virtual void Cancel() = 0;

        // <title Release>
        // Syntax: its called by the DRS system, when the ActionInstance is no longer needed. Normally when the ActionInstance is finished, or when a running ResponseInstance is destroyed.
        // Description:
        //      Will Release the action instance.  (could be a { delete this; } in most cases. Must match the allocation used in IDynamicResponseAction::Execute)
        // Arguments:
        // Returns:
        // See Also:
        virtual void Release() = 0;

        // <title GetVerboseInfo>
        // Syntax: For debug only
        // Description:
        //      Just some debug informations, that describe the actionInstance and its parameter and its current process. So that a list of running action instances can be generated
        // Arguments:
        // Returns:
        //      a descriptive string containing informations about the current state of the actionInstance
        // See Also:
        virtual const char* GetVerboseInfo() = 0;  //For debug purposes only

    protected:
        virtual ~IResponseActionInstance() {}  //we want to be destroyed only via out release function
    };
}  //namespace

#endif  //_DYNAMICRESPONSEACTION_H_