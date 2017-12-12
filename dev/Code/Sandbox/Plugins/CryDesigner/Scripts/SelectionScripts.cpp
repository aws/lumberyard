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

#include "StdAfx.h"
#include "Util/BoostPythonHelpers.h"
#include "Objects/DesignerObject.h"
#include "Tools/DesignerTool.h"
#include "Tools/Select/SelectTool.h"
#include "Tools/Select/SelectGrowTool.h"
#include "Tools/Select/InvertSelectionTool.h"
#include "Tools/Select/SelectAllNoneTool.h"
#include "Tools/Select/SelectConnectedTool.h"
#include "Tools/Select/LoopSelectionTool.h"
#include "Tools/Select/RingSelectionTool.h"
#include "Tools/Select/SelectAllNoneTool.h"
#include "Util/ElementManager.h"
#include "BasicScripts.h"

namespace BPython
{
    DesignerTool* GetDesignerEditTool()
    {
        return qobject_cast<DesignerTool*>(GetIEditor()->GetEditTool());
    }

    void PyDesignerSelect(ElementID nElementKeyID)
    {
        DesignerElementsPtr pElements = s_bdpc.FindElements(nElementKeyID);
        if (pElements == NULL)
        {
            throw std::logic_error("The element doesn't exist.");
        }
        CD::SMainContext mc(GetContext());
        mc.pSelected->Add(*pElements);
        UpdateSelection(mc);
    }

    void PyDesignerDeselect(ElementID nElementKeyID)
    {
        DesignerElementsPtr pElements = s_bdpc.FindElements(nElementKeyID);
        if (pElements == NULL)
        {
            throw std::logic_error("The element doesn't exist.");
        }
        CD::SMainContext mc(GetContext());
        mc.pSelected->Erase(*pElements);
        UpdateSelection(mc);
    }

    void PyDesignerSetSelection(ElementID nElementKeyID)
    {
        DesignerElementsPtr pElements = s_bdpc.FindElements(nElementKeyID);
        if (pElements == NULL)
        {
            throw std::logic_error("The element doesn't exist.");
        }
        CD::SMainContext mc(GetContext());
        mc.pSelected->Clear();
        mc.pSelected->Set(*pElements);
        UpdateSelection(mc);
    }

    void PyDesignerSelectAllVertices()
    {
        CD::SMainContext mc(GetContext());
        SelectAllNoneTool::SelectAllVertices(mc.pObject, mc.pModel);
        UpdateSelection(mc);
        if (GetDesignerEditTool())
        {
            GetDesignerEditTool()->SwitchTool(CD::eDesigner_Vertex);
        }
    }

    void PyDesignerSelectAllEdges()
    {
        CD::SMainContext mc(GetContext());
        SelectAllNoneTool::SelectAllEdges(mc.pObject, mc.pModel);
        UpdateSelection(mc);
        if (GetDesignerEditTool())
        {
            GetDesignerEditTool()->SwitchTool(CD::eDesigner_Edge);
        }
    }

    void PyDesignerSelectAllFaces()
    {
        CD::SMainContext mc(GetContext());
        SelectAllNoneTool::SelectAllFaces(mc.pObject, mc.pModel);
        UpdateSelection(mc);
        if (GetDesignerEditTool())
        {
            GetDesignerEditTool()->SwitchTool(CD::eDesigner_Face);
        }
    }

    void PyDesignerDeselectAllVertices()
    {
        CD::SMainContext mc(GetContext());
        SelectAllNoneTool::DeselectAllVertices();
        UpdateSelection(mc);
    }

    void PyDesignerDeselectAllEdges()
    {
        CD::SMainContext mc(GetContext());
        SelectAllNoneTool::DeselectAllEdges();
        UpdateSelection(mc);
    }

    void PyDesignerDeselectAllFaces()
    {
        CD::SMainContext mc(GetContext());
        SelectAllNoneTool::DeselectAllFaces();
        UpdateSelection(mc);
    }

    void PyDesignerDeselectAll()
    {
        CD::SMainContext mc(GetContext());
        mc.pSelected->Clear();
        UpdateSelection(mc);
    }

    void PyDesignerGrowSelection()
    {
        CD::SMainContext mc(GetContext());
        SelectGrowTool::GrowSelection(mc);
        UpdateSelection(mc);
    }

    void PyDesignerSelectConnectedFaces()
    {
        CD::SMainContext mc(GetContext());
        SelectConnectedTool::SelectConnectedPolygons(mc);
        UpdateSelection(mc);
    }

    void PyDesignerLoopSelection()
    {
        CD::SMainContext mc(GetContext());
        LoopSelectionTool::LoopSelection(mc);
        UpdateSelection(mc);
    }

    void PyDesignerRingSelection()
    {
        CD::SMainContext mc(GetContext());
        RingSelectionTool::RingSelection(mc);
        UpdateSelection(mc);
    }

    void PyDesignerInvertSelection()
    {
        CD::SMainContext mc(GetContext());
        InvertSelectionTool::InvertSelection(mc);
        UpdateSelection(mc);
    }

    void PyDesignerSelectVertexMode()
    {
        if (GetDesignerEditTool())
        {
            GetDesignerEditTool()->SwitchTool(CD::eDesigner_Vertex);
        }
    }

    void PyDesignerSelectEdgeMode()
    {
        if (GetDesignerEditTool())
        {
            GetDesignerEditTool()->SwitchTool(CD::eDesigner_Edge);
        }
    }

    void PyDesignerSelectFaceMode()
    {
        if (GetDesignerEditTool())
        {
            GetDesignerEditTool()->SwitchTool(CD::eDesigner_Face);
        }
    }

    void PyDesignerSelectPivotMode()
    {
        if (GetDesignerEditTool())
        {
            GetDesignerEditTool()->SwitchTool(CD::eDesigner_Pivot);
        }
    }
}

#ifndef AZ_TESTS_ENABLED
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerSelect,
    designer,
    select,
    "Selects elements with nID.",
    "designer.select( int nID )");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerDeselect,
    designer,
    deselect,
    "Deselects elements with nID.",
    "designer.deselect( int nID )");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerSetSelection,
    designer,
    set_selection,
    "Deselect the existing selections and sets the new selection.",
    "designer.set_selection( int nID )");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerSelectAllVertices,
    designer,
    select_all_vertices,
    "Selects all vertices.",
    "designer.select_all_vertices()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerSelectAllEdges,
    designer,
    select_all_edges,
    "Selects all edges.",
    "designer.select_all_edges()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerSelectAllFaces,
    designer,
    select_all_faces,
    "Selects all faces.",
    "designer.select_all_faces()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerDeselectAllVertices,
    designer,
    deselect_all_vertices,
    "Deselects all vertices.",
    "designer.deselect_all_vertices()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerDeselectAllEdges,
    designer,
    deselect_all_edges,
    "Deselects all edges.",
    "designer.deselect_all_edges()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerDeselectAllFaces,
    designer,
    deselect_all_faces,
    "Deselects all faces.",
    "designer.deselect_all_faces()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerDeselectAll,
    designer,
    deselect_all,
    "Deselects all elements.",
    "designer.deselect_all()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerSelectConnectedFaces,
    designer,
    select_connected_faces,
    "Selects all connected faces with the selected elements.",
    "designer.select_connected_faces()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerGrowSelection,
    designer,
    grow_selection,
    "Grows selection",
    "designer.grow_selection()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerLoopSelection,
    designer,
    loop_selection,
    "Selects a loop of edges that are connected in a end to end.",
    "designer.loop_selection()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerRingSelection,
    designer,
    ring_selection,
    "Selects a sequence of edges that are not connected, but on opposite sides to each other continuing along a face loop.",
    "designer.ring_selection()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerInvertSelection,
    designer,
    invert_selection,
    "Selects all components that are not selected, and deselect currently selected components.",
    "designer.invert_selection()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerSelectVertexMode,
    designer,
    select_vertexmode,
    "Select Vertex Mode of CryDesigner",
    "designer.select_vertexmode()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerSelectEdgeMode,
    designer,
    select_edgemode,
    "Select Edge Mode of CryDesigner",
    "designer.select_edgemode()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerSelectFaceMode,
    designer,
    select_facemode,
    "Select Face Mode of CryDesigner",
    "designer.select_facemode()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerSelectPivotMode,
    designer,
    select_pivotmode,
    "Select Pivot Mode of CryDesigner",
    "designer.select_pivotmode()");
#endif // AZ_TESTS_ENABLED