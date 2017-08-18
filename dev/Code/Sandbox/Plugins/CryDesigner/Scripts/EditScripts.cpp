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
#include "BasicScripts.h"
#include "Objects/DesignerObject.h"
#include "Tools/Edit/WeldTool.h"
#include "Tools/Edit/FillSpaceTool.h"
#include "Tools/Edit/FlipTool.h"
#include "Tools/Edit/MergeTool.h"
#include "Tools/Edit/SeparateTool.h"
#include "Tools/Edit/RemoveTool.h"
#include "Tools/Edit/CopyTool.h"
#include "Tools/Edit/RemoveDoublesTool.h"
#include "Tools/Select/SelectTool.h"
#include "Tools/DesignerTool.h"
#include "Core/Model.h"
#include "Core/ModelCompiler.h"

namespace BPython
{
    ElementID PyDesignerFlip()
    {
        CD::SMainContext mc = GetContext();
        if (mc.pSelected->IsEmpty())
        {
            throw std::logic_error("There are no selected elements.");
        }

        ElementManager flippedElements;
        FlipTool::FlipPolygons(mc, flippedElements);
        CompileModel(mc);
        return s_bdpc.RegisterElements(new ElementManager(flippedElements));
    }

    void PyDesignerMergeFaces()
    {
        CD::SMainContext mc = GetContext();
        if (mc.pSelected->IsEmpty())
        {
            throw std::logic_error("There are no selected elements.");
        }

        MergeTool::MergePolygons(mc);
        mc.pSelected->Clear();
        CompileModel(mc);
    }

    void PyDesignerSeparate()
    {
        CD::SMainContext mc = GetContext();
        if (mc.pSelected->IsEmpty())
        {
            throw std::logic_error("There are no selected elements.");
        }

        SeparateTool::Separate(mc);
        CompileModel(mc);
    }

    void PyDesignerRemove()
    {
        CD::SMainContext mc = GetContext();
        if (mc.pSelected->IsEmpty())
        {
            throw std::logic_error("There are no selected elements.");
        }

        RemoveTool::RemoveSelectedElements(mc, false);
        mc.pSelected->Clear();
        CompileModel(mc);
    }

    ElementID PyDesignerCopy()
    {
        CD::SMainContext mc = GetContext();
        if (mc.pSelected->IsEmpty())
        {
            throw std::logic_error("There are no selected elements.");
        }

        ElementManager copiedElements;
        CopyTool::Copy(mc, &copiedElements);
        CompileModel(mc);

        return s_bdpc.RegisterElements(new ElementManager(copiedElements));
    }

    void PyDesignerRemoveDoubles(float fDistance)
    {
        CD::SMainContext mc = GetContext();
        if (mc.pSelected->IsEmpty())
        {
            throw std::logic_error("There are no selected elements.");
        }

        RemoveDoublesTool::RemoveDoubles(mc, fDistance);
        mc.pSelected->Clear();
        CompileModel(mc);
    }

    void PyDesignerWeld()
    {
        CD::SMainContext mc = GetContext();
        if (mc.pSelected->IsEmpty())
        {
            throw std::logic_error("There are no selected elements.");
        }

        std::vector<BrushVec3> vertices;
        for (int i = 0, iSelectedElementCount(mc.pSelected->GetCount()); i < iSelectedElementCount; ++i)
        {
            if (mc.pSelected->Get(i).IsVertex())
            {
                vertices.push_back(mc.pSelected->Get(i).GetPos());
            }
        }

        if (vertices.size() != 2)
        {
            throw std::logic_error("Only two vertices need to be selected.");
        }

        WeldTool::Weld(mc, vertices[0], vertices[1]);
        mc.pSelected->Clear();
        CompileModel(mc);
    }
}

#ifndef AZ_TESTS_ENABLED
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerFlip,
    designer,
    flip,
    "Flips the selected faces. Returns nID which points out fliped polygons.",
    "designer.flip()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerCopy,
    designer,
    copy,
    "Copies the selected faces. Returns nID which points out copied polygons.",
    "designer.copy()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerMergeFaces,
    designer,
    merge_faces,
    "Merges the selected faces.",
    "designer.merge_faces()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerRemove,
    designer,
    remove,
    "Removes the selected faces or edges.",
    "designer.remove()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerSeparate,
    designer,
    separate,
    "Separates the selected faces to a new designer object.",
    "designer.separate()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerRemoveDoubles,
    designer,
    remove_doubles,
    "Removes doubles from the select elements within the specified distance.",
    "designer.remove_doubles( float distance )");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerWeld,
    designer,
    weld,
    "Puts the first selected vertex together into the second selected vertex.",
    "designer.weld()");
#endif // AZ_TESTS_ENABLED