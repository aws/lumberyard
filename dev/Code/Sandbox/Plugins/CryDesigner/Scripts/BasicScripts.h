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

#pragma once

#include "Util/BoostPythonHelpers.h"
#include "Util/ElementManager.h"

class DesignerObject;

namespace CD
{
    class Model;
    class ModelCompiler;
    class Polygon;
};

namespace BPython
{
    CD::SMainContext GetContext();
    void CompileModel(CD::SMainContext& mc, bool bForce = false);
    BrushVec3 FromSVecToBrushVec3(const SPyWrappedProperty::SVec& sVec);
    void UpdateSelection(CD::SMainContext& mc);

    typedef qint64 ElementID;
    class CBrushDesignerPythonContext
    {
    public:
        CBrushDesignerPythonContext()
        {
            Init();
        }

        ~CBrushDesignerPythonContext()
        {
            ClearElementVariables();
        }

        CBrushDesignerPythonContext(const CBrushDesignerPythonContext& context)
        {
            operator = (context);
        }

        CBrushDesignerPythonContext& operator = (const CBrushDesignerPythonContext& context)
        {
            bMoveTogether = context.bMoveTogether;
            bAutomaticUpdateMesh = context.bAutomaticUpdateMesh;

            ClearElementVariables();

            std::set<ElementManager*>::iterator ii = context.elementVariables.begin();
            for (; ii != context.elementVariables.end(); ++ii)
            {
                (*ii)->AddRef();
                elementVariables.insert(*ii);
            }

            return *this;
        }

        void Init()
        {
            elementVariables.clear();
            bMoveTogether = true;
            bAutomaticUpdateMesh = true;
        }

        bool bMoveTogether;
        bool bAutomaticUpdateMesh;

        ElementID RegisterElements(DesignerElementsPtr pElements);
        DesignerElementsPtr FindElements(ElementID id);
        void ClearElementVariables();

    private:
        std::set<ElementManager*> elementVariables;
    };

    extern CBrushDesignerPythonContext s_bdpc;
    extern CBrushDesignerPythonContext s_bdpc_before_init;

    void OutputPolygonPythonCreationCode(CD::Polygon* pPolygon);
};