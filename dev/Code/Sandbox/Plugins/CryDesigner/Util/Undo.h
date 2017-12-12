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
////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012
// -------------------------------------------------------------------------
//  File name:   Undo.h
//  Created:     Mar/25/2012 by Jaesik
//  Description:
//
////////////////////////////////////////////////////////////////////////////
#include "Core/Model.h"
#include "Tools/Select/SelectTool.h"

namespace CD
{
    class Model;
    class ModelCompiler;

    class DesignerUndo
        : public IUndoObject
    {
    public:
        DesignerUndo()
            :  m_Tool(CD::eDesigner_Object) {}
        DesignerUndo(CBaseObject* pObj, const CD::Model* pModel, const char* undoDescription = NULL);

        static bool IsAKindOfDesignerTool(CEditTool* pEditTool);
        static CD::Model* GetModel(const GUID& objGUID);
        static ModelCompiler* GetCompiler(const GUID& objGUID);
        static CBaseObject* GetBaseObject(const GUID& objGUID);
        static CD::SMainContext MainContext(const GUID& objGUID);
        static void RestoreEditTool(CD::Model* pModel, REFGUID objGUID, CD::EDesignerTool tool);

    protected:
        int GetSize()
        {
            return sizeof(*this);
        }

        QString GetDescription()
        {
            return m_undoDescription;
        };

        void SetDescription(const QString& description)
        {
            if (!description.isEmpty())
            {
                m_undoDescription = description;
            }
        }

        CD::SMainContext MainContext() const
        {
            return MainContext(m_ObjGUID);
        }

        void RestoreEditTool(CD::Model* pModel)
        {
            RestoreEditTool(pModel, m_ObjGUID, m_Tool);
        }

        virtual void Undo(bool bUndo);
        virtual void Redo();

        void StoreEditorTool();
        void SwitchTool(CD::EDesignerTool tool) { m_Tool = tool; }

        void SetObjGUID(REFGUID guid)
        {
            m_ObjGUID = guid;
        }

    private:
        QString m_undoDescription;
        GUID m_ObjGUID;
        CD::EDesignerTool m_Tool;
        _smart_ptr<CD::Model> m_undo;
        Matrix34 m_UndoWorldTM;
        _smart_ptr<CD::Model> m_redo;
        Matrix34 m_RedoWorldTM;
    };

    class UndoSelection
        : public DesignerUndo
    {
    public:

        UndoSelection(ElementManager& selectionContext, CBaseObject* pObj, const char* undoDescription = NULL);

        int GetSize(){return sizeof(*this); }

        SelectTool* GetSelectTool();
        void Undo(bool bUndo);
        void Redo();

    private:

        void CopyElements(ElementManager& sourceElements, ElementManager& destElements);
        void ReplacePolygonsWithExistingPolygonsInModel(ElementManager& elements);

        ElementManager m_SelectionContextForUndo;
        ElementManager m_SelectionContextForRedo;
    };

    class UndoTextureMapping
        : public IUndoObject
    {
    public:
        UndoTextureMapping(){}
        UndoTextureMapping(CBaseObject* pObj, const char* undoDescription = NULL)
            : m_ObjGUID(pObj->GetId())
        {
            SetDescription(undoDescription);
            SaveDesignerTexInfoContext(m_UndoContext);
        }

    protected:
        int GetSize()
        {
            return sizeof(*this);
        }

        QString GetDescription()
        {
            return m_undoDescription;
        };

        void SetDescription(const char* description)
        {
            if (description)
            {
                m_undoDescription = description;
            }
        }

        void Undo(bool bUndo);
        void Redo();

        void SetObjGUID(REFGUID guid){ m_ObjGUID = guid; }

    private:

        struct SContextInfo
        {
            GUID m_PolygonGUID;
            CD::STexInfo m_TexInfo;
            int m_MatID;
        };

        void RestoreTexInfo(const std::vector<SContextInfo>& contextList);
        void SaveDesignerTexInfoContext(std::vector<SContextInfo>& contextList);

        QString m_undoDescription;
        GUID m_ObjGUID;

        std::vector<SContextInfo> m_UndoContext;
        std::vector<SContextInfo> m_RedoContext;
    };

    class UndoSubdivision
        : public IUndoObject
    {
    public:
        UndoSubdivision(){}
        UndoSubdivision(const CD::SMainContext& mc);

        void Undo(bool bUndo) override;
        void Redo() override;
        int GetSize() override { return sizeof(*this); }
        QString GetDescription() override { return "Designer Subdivision"; }

    private:

        void UpdatePanel();

        GUID m_ObjGUID;
        int m_SubdivisionLevelForUndo;
        int m_SubdivisionLevelForRedo;
    };
};