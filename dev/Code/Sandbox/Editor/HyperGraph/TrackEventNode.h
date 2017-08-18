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

#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_TRACKEVENTNODE_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_TRACKEVENTNODE_H
#pragma once


#include <IMovieSystem.h>
#include "FlowGraphNode.h"

#define TRACKEVENT_CLASS ("TrackEvent")
#define TRACKEVENTNODE_TITLE    QStringLiteral("TrackEvent")
#define TRACKEVENTNODE_CLASS    QStringLiteral("TrackEvent")
#define TRACKEVENTNODE_DESC     QStringLiteral("Outputs for Trackview Events")

class CTrackEventNode
    : public CFlowNode
    , public ITrackEventListener
{
public:
    static const char* GetClassType()
    {
        return TRACKEVENT_CLASS;
    }
    CTrackEventNode();
    virtual ~CTrackEventNode();

    // CHyperNode overwrites
    virtual void Init();
    virtual void Done();
    virtual CHyperNode* Clone();
    virtual void Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar);

    virtual QString GetTitle() const;
    virtual QString GetClassName() const;
    virtual QString GetDescription();
    virtual QColor GetCategoryColor() override { return QColor(220, 40, 40); }
    virtual void OnInputsChanged();

    // Description:
    //      Re-populates output ports based on input sequence
    // Arguments:
    //      bLoading - TRUE if called from serialization on loading
    virtual void PopulateOutput(bool bLoading);

    // ~ITrackEventListener
    virtual void OnTrackEvent(IAnimSequence* pSequence, int reason, const char* event, void* pUserData);

protected:
    //! Add an output port for an event
    virtual void AddOutputEvent(const char* event);

    //! Remove an output port for an event (including any edges)
    virtual void RemoveOutputEvent(const char* event);

    //! Rename an output port for an event
    virtual void RenameOutputEvent(const char* event, const char* newName);

    //! Move up a specified output port once in the list of output ports
    virtual void MoveUpOutputEvent(const char* event);

    //! Move down a specified output port once in the list of output ports
    virtual void MoveDownOutputEvent(const char* event);

private:
    IAnimSequence* m_pSequence; // Current sequence
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_TRACKEVENTNODE_H
