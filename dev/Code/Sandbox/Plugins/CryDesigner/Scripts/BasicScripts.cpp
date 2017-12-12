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
#include "BasicScripts.h"
#include "Tools/DesignerTool.h"
#include "Objects/DesignerObject.h"
#include "Tools/Select/SelectTool.h"
#include "ToolFactory.h"

#include <QDebug>

namespace BPython
{
    CBrushDesignerPythonContext s_bdpc;
    CBrushDesignerPythonContext s_bdpc_before_init;

    CD::SMainContext GetContext()
    {
        CSelectionGroup* pGroup = GetIEditor()->GetObjectManager()->GetSelection();

        if ((!pGroup) || (pGroup->IsEmpty()))
        {
            throw std::logic_error("Please select an object.");
        }

        CBaseObject* pObject = pGroup->GetObject(0);
        if (!qobject_cast<DesignerObject*>(pObject))
        {
            throw std::logic_error("The selected object isn't a designer object type");
        }

        CEditTool* pEditTool = GetIEditor()->GetEditTool();
        if (pEditTool == NULL || !qobject_cast<DesignerTool*>(pEditTool))
        {
            throw std::logic_error("The selected object isn't a designer object type");
        }

        DesignerObject* pDesignerObject = (DesignerObject*)pObject;
        DesignerTool* pDesignerTool = (DesignerTool*)pEditTool;

        CD::SMainContext mc;
        mc.pObject = pObject;
        mc.pModel = pDesignerObject->GetModel();
        mc.pCompiler = pDesignerObject->GetCompiler();
        mc.pSelected = pDesignerTool->GetSelectedElements();

        return mc;
    }

    void CompileModel(CD::SMainContext& mc, bool bForce)
    {
        if (bForce || s_bdpc.bAutomaticUpdateMesh)
        {
            CD::UpdateAll(mc, CD::eUT_DataBase | CD::eUT_Mesh);
        }
    }

    void UpdateSelection(CD::SMainContext& mc)
    {
        CD::UpdateDrawnEdges(mc);
        if (CD::GetDesigner())
        {
            CD::GetDesigner()->UpdateSelectionMeshFromSelectedElements(mc);
        }
    }

    ElementID CBrushDesignerPythonContext::RegisterElements(DesignerElementsPtr pElements)
    {
        ElementManager* pPureElements = pElements.get();
        pPureElements->AddRef();
        elementVariables.insert(pPureElements);
        return (ElementID)pPureElements;
    }

    DesignerElementsPtr CBrushDesignerPythonContext::FindElements(ElementID id)
    {
        std::set<ElementManager*>::iterator ii = elementVariables.find((ElementManager*)id);
        if (ii == elementVariables.end())
        {
            return NULL;
        }
        return *ii;
    }

    void CBrushDesignerPythonContext::ClearElementVariables()
    {
        std::set<ElementManager*>::iterator ii = elementVariables.begin();
        for (; ii != elementVariables.end(); ++ii)
        {
            (*ii)->Release();
        }
        elementVariables.clear();
    }

    BrushVec3 FromSVecToBrushVec3(const SPyWrappedProperty::SVec& sVec)
    {
        return CD::ToBrushVec3(Vec3(sVec.x, sVec.y, sVec.z));
    }

    void OutputPolygonPythonCreationCode(CD::Polygon* pPolygon)
    {
        std::vector<CD::PolygonPtr> outerPolygons;
        pPolygon->GetSeparatedPolygons(outerPolygons, CD::eSP_OuterHull);

        std::vector<CD::PolygonPtr> innerPolygons;
        pPolygon->GetSeparatedPolygons(innerPolygons, CD::eSP_InnerHull);

        qDebug() << "designer.start_polygon_addition()";

        std::vector<CD::SVertex> vList;
        for (int i = 0, iPolygonCount(outerPolygons.size()); i < iPolygonCount; ++i)
        {
            outerPolygons[i]->GetLinkedVertices(vList);
            for (int k = 0, iVListCount(vList.size()); k < iVListCount; ++k)
            {
                qDebug() << QString("designer.add_vertex_to_polygon((%1,%2,%3))").arg(vList[k].pos.x, vList[k].pos.y, vList[k].pos.z);
            }
        }

        if (!innerPolygons.empty())
        {
            for (int i = 0, iPolygonCount(innerPolygons.size()); i < iPolygonCount; ++i)
            {
                innerPolygons[i]->GetLinkedVertices(vList);
                qDebug() << "\ndesigner.start_to_add_another_hole()";
                for (int k = 0, iVListCount(vList.size()); k < iVListCount; ++k)
                {
                    qDebug() << QString("designer.add_vertex_to_polygon((%1,%2,%3))").arg(vList[k].pos.x, vList[k].pos.y, vList[k].pos.z);
                }
            }
        }

        qDebug() << "designer.finish_polygon_addition()";
    }

    void PyDesignerStart()
    {
        s_bdpc_before_init = s_bdpc;
        s_bdpc.Init();
    }

    void PyDesignerEnd()
    {
        s_bdpc.ClearElementVariables();
        s_bdpc = s_bdpc_before_init;
    }

    void PyDesignerSetEnv(pSPyWrappedProperty name, pSPyWrappedProperty value)
    {
        if (name->type != SPyWrappedProperty::eType_String)
        {
            throw std::logic_error("Invalid name type.");
        }

        if (name->stringValue == "move_together" || name->stringValue == "automatic_update_mesh")
        {
            if (value->type != SPyWrappedProperty::eType_Bool)
            {
                throw std::logic_error("the type of the value should be Bool.");
            }
        }

        if (name->stringValue == "move_together")
        {
            s_bdpc.bMoveTogether = value->property.boolValue;
        }
        else if (name->stringValue == "automatic_update_mesh")
        {
            s_bdpc.bAutomaticUpdateMesh = value->property.boolValue;
        }
        else
        {
            throw std::logic_error("The name doesn't exist.");
        }
    }

    void PyDesignerRecordUndo()
    {
        CUndo undo("Calls python scripts in a designer object");
        CD::SMainContext mc(GetContext());
        mc.pModel->RecordUndo("Calls python scripts in a designer object", mc.pObject);
    }

    void PyDesignerUpdateMesh()
    {
        CompileModel(GetContext(), true);
    }
}

#ifndef AZ_TESTS_ENABLED
REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerStart,
    designer,
    start,
    "Initializes all external variables used in the designer scripts.",
    "designer.start()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerEnd,
    designer,
    end,
    "Restores the designer context to the context used before initializing.",
    "designer.end()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerSetEnv,
    designer,
    set_env,
    "Sets environment variables.",
    "designer.set_env( str name, [bool || int || float || str || (float,float,float)] paramValue )");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerRecordUndo,
    designer,
    record_undo,
    "Records the current designer states to be undone.",
    "designer.record_undo()");

REGISTER_ONLY_PYTHON_COMMAND_WITH_EXAMPLE(BPython::PyDesignerUpdateMesh,
    designer,
    update_mesh,
    "Updates mesh.",
    "designer.update_mesh()");
#endif //AZ_TESTS_ENABLED