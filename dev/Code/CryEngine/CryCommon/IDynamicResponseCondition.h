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

// Description : An Interface for Conditions that can be added to Response Segments

#ifndef _DYNAMICRESPONSECONDITION_H_
#define _DYNAMICRESPONSECONDITION_H_

#include <IXml.h>

namespace DRS
{
    struct IResponseActor;
    struct IVariableCollection;


    struct IResponseCondition
    {
        virtual ~IResponseCondition() {}

        typedef unsigned int ConditionIdentifierHash;
        // <title IsMet>
        // Syntax: its called by the DRS system, to evaluate if this condition is fulfilled
        // Description:
        //      the condition is required to check all needed data as fast as possible and return if the response can therefore be executed
        // Arguments:
        //      pObject - a pointer to the current active Actor for this response, can be used to get local variable collections from or use as execution object
        //      pContextVariableCollection - a pointer to the context variable collection for the current event.
        // Returns:
        //      needs to return, if the response is allowed to execute
        virtual bool IsMet(const IResponseActor* pObject, const IVariableCollection* pContextVariableCollection) = 0;

        // <title Release>
        // Syntax: its called by the DRS system, when this condition instance is no longer needed by the DRS
        // Description:
        //      Informs the Condition that the DRS will not use this instance any more. Its up to the implementation to decide what needs to be done.
        //      The Condition could be deleted, or still remains in some cache so that is can be re-used. straight forward implementation could be a "delete this;"
        // Arguments:
        // Returns:
        virtual void Release() = 0;
    };

    struct IResponseConditionCreator
    {
        // <title CreateOrGetConditionFromXML>
        // Syntax: its called by the DRS system, when a condition of the type is requested by a response definition
        // Description:
        //      Will Create a new Condition with the parameters from the xml-node, or may return a existing one, if a action with these parameters already exist
        //      Remark: The DRS will call ReleaseCondition for every condition requested through this
        // Arguments:
        //      pConditionNode - an pointer to an cry-xml-node from the response definition which contains the data for the condition.
        // Returns:
        //      Will return the generated or re-used condition if successful. 0 otherwise
        virtual IResponseCondition* CreateOrGetConditionFromXML(XmlNodeRef pConditionNode) = 0;

        // <title Release>
        // Syntax: its called by the DRS system, on shutdown
        // Description:
        //      Will Release the ActionCreator. Depending on the caching strategy of the creator, it might destroy all created actions. So dont use any Actions of the type after calling release.
        // Arguments:
        // Returns:
        // See Also:
        virtual void Release() = 0;
    protected:
        virtual ~IResponseConditionCreator() {}
    };
}  //namespace DRS

#endif  //_DYNAMICRESPONSECONDITION_H_