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

#include "IDataBaseManager.h"
#include "Serialization/Enum.h"

class BaseTool;
class DesignerTool;

namespace CD
{
    enum EToolGroup
    {
        eToolGroup_BasicSelection,
        eToolGroup_BasicSelectionCombination,
        eToolGroup_Selection,
        eToolGroup_Shape,
        eToolGroup_Edit,
        eToolGroup_Modify,
        eToolGroup_Surface,
        eToolGroup_Misc
    };

    enum EDesignerTool
    {
        // BasicSelection Tools
        eDesigner_Vertex = 0x0001,
        eDesigner_Edge = 0x0002,
        eDesigner_Face = 0x0004,

        // BasicSelectionCombination Tools
        eDesigner_VertexEdge = eDesigner_Vertex | eDesigner_Edge,
        eDesigner_VertexFace = eDesigner_Vertex | eDesigner_Face,
        eDesigner_EdgeFace = eDesigner_Edge | eDesigner_Face,
        eDesigner_VertexEdgeFace = eDesigner_Vertex | eDesigner_Edge | eDesigner_Face,

        // Rest BasicSelection Tools
        eDesigner_Pivot,
        eDesigner_Object,

        // Selection Tools
        eDesigner_AllNone,
        eDesigner_Connected,
        eDesigner_Grow,
        eDesigner_Loop,
        eDesigner_Ring,
        eDesigner_Invert,

        // Shape Tools
        eDesigner_Box,
        eDesigner_Sphere,
        eDesigner_Cylinder,
        eDesigner_Cone,
        eDesigner_Rectangle,
        eDesigner_Disc,
        eDesigner_Line,
        eDesigner_Curve,
        eDesigner_Stair,
        eDesigner_StairProfile,
        eDesigner_CubeEditor,

        // Edit Tools
        eDesigner_Extrude,
        eDesigner_Offset,
        eDesigner_Weld,
        eDesigner_Collapse,
        eDesigner_Fill,
        eDesigner_Flip,
        eDesigner_Merge,
        eDesigner_Separate,
        eDesigner_Remove,
        eDesigner_Copy,
        eDesigner_RemoveDoubles,

        // Modify Tools
        eDesigner_LoopCut,
        eDesigner_Bevel,
        eDesigner_Subdivision,
        eDesigner_Boolean,
        eDesigner_Slice,
        eDesigner_Mirror,
        eDesigner_Magnet,
        eDesigner_Lathe,
        eDesigner_CircleClone,
        eDesigner_ArrayClone,

        // Surface Tools
        eDesigner_UVMapping,
        eDesigner_SmoothingGroup,

        // Misc Tools
        eDesigner_Pivot2Bottom,
        eDesigner_ResetXForm,
        eDesigner_Export,
        eDesigner_SnapToGrid,
        eDesigner_HideFace,
        eDesigner_ShortcutManager,

        // Invalid
        eDesigner_Invalid = 0x7FFFFFFF
    };

    class Polygon;
}

class IBasePanel
{
public:
    virtual void Done()   {}
    virtual void Update() {}
};

class IDesignerPanel
    : public IBasePanel
{
public:
    virtual void SetDesignerTool(DesignerTool* pTool) = 0;
    virtual void DisableButton(CD::EDesignerTool tool) = 0;
    virtual void SetButtonCheck(CD::EDesignerTool tool, int nCheckID) = 0;
    virtual void OnDataBaseItemEvent(IDataBaseItem* pItem, EDataBaseItemEvent event) = 0;
    virtual void MaterialChanged() = 0;
    virtual int  GetSubMatID() const = 0;
    virtual void SetSubMatID(int nID) = 0;
    virtual void UpdateCloneArrayButtons() = 0;
};

class IDesignerSubPanel
    : public IBasePanel
{
public:
    virtual void SetDesignerTool(DesignerTool* pTool) = 0;
    virtual void OnEditorNotifyEvent(EEditorNotifyEvent event) = 0;
    virtual void UpdateBackFaceCheckBox(CD::Model* pModel) = 0;
};

class ISmoothingGroupPanel
    : public IBasePanel
{
public:
    virtual void ShowAllNumbers() = 0;
    virtual void HideNumber(int nNumber) = 0;
    virtual void ClearAllSelectionsOfNumbers(int nExcludedID = -1) = 0;
};

class ICubeEditorPanel
    : public IBasePanel
{
public:
    virtual BrushFloat GetCubeSize() const = 0;
    virtual bool IsSidesMerged() const = 0;
    virtual bool IsAddButtonChecked() const = 0;
    virtual bool IsRemoveButtonChecked() const = 0;
    virtual bool IsPaintButtonChecked() const = 0;
    virtual int  GetSubMatID() const = 0;
    virtual void SetSubMatID(int nID) const = 0;
    virtual bool SetMaterial(CMaterial* pMaterial) = 0;
    virtual void SelectPrevBrush() = 0;
    virtual void SelectNextBrush() = 0;
    virtual void UpdateSubMaterialComboBox() = 0;
};

namespace CD
{
    template<class _Panel>
    void DestroyPanel(_Panel** pPanel)
    {
        if (*pPanel)
        {
            (*pPanel)->Done();
            *pPanel = NULL;
        }
    }

    template<class _Key, class _Base, class _Arg0, class _KeyPred = std::less<_Key> >
    class DesignerFactory
    {
    public:

        struct ICreator
        {
            virtual ~ICreator() {}
            virtual _Base* Create(_Key key, _Arg0 tool) = 0;
        };

        template<class _DerivedPanel, class _Constructor>
        struct GeneralPanelCreator
            : public ICreator
        {
            GeneralPanelCreator(_Key key)
            {
                DesignerFactory::the().Add(key, this);
            }
            _Base* Create(_Key key, _Arg0 arg0) override
            {
                return _Constructor::Create();
            }
        };

        template<class _DerivedTool, class _DerivedPanel>
        struct AttributePanelCreator
            : public ICreator
        {
            AttributePanelCreator(_Key key)
            {
                DesignerFactory::the().Add(key, this);
            }

            _Base* Create(_Key key, _Arg0 tool) override
            {
                return new _DerivedPanel((_DerivedTool*)tool);
            }
        };

        template<class _DerivedTool>
        struct ToolCreator
            : public ICreator
        {
            ToolCreator(_Key key)
            {
                DesignerFactory::the().Add(key, this);
            }

            _Base* Create(_Key key, _Arg0 arg0) override
            {
                return new _DerivedTool(key);
            }
        };

        typedef std::map<_Key, ICreator*, _KeyPred> Creators;

        void Add(_Key& key, ICreator* const creator)
        {
            creators[key] = creator;
        }

        _Base* Create(_Key key, _Arg0 arg0 = _Arg0())
        {
            typename Creators::iterator ii = creators.find(key);
            if (ii != creators.end())
            {
                return ii->second->Create(ii->first, arg0);
            }
            return NULL;
        }

        static DesignerFactory& the()
        {
            static DesignerFactory factory;
            return factory;
        }

    private:
        DesignerFactory() {}
        Creators creators;
    };

    typedef std::map<CD::EDesignerTool, _smart_ptr<BaseTool> > TOOLDESIGNER_MAP;
    typedef std::map<CD::EToolGroup, std::set<CD::EDesignerTool> > TOOLGROUP_MAP;

    class ToolGroupRegister
    {
    public:
        ToolGroupRegister(EToolGroup group, EDesignerTool tool, const char* name, const char* classname);

        void Register();

        ToolGroupRegister* m_next;
        EToolGroup m_group;
        EDesignerTool m_tool;
        const char* m_name;
        const char* m_className;
    };

    class ToolGroupMapper
    {
    public:
        static ToolGroupMapper& the()
        {
            static ToolGroupMapper toolGroupMapper;
            return toolGroupMapper;
        }

        void AddTool(class ToolGroupRegister* tool)
        {
            tool->m_next = m_head;
            m_head = tool;
        }

        void AddTool(EToolGroup group, EDesignerTool tool)
        {
            m_ToolGroupMap[group].insert(tool);
        }

        std::vector<CD::EDesignerTool> GetToolList(EToolGroup group)
        {
            lazyRegister();
            auto iter = m_ToolGroupMap.find(group);
            std::vector<CD::EDesignerTool> toolList;
            toolList.insert(toolList.begin(), iter->second.begin(), iter->second.end());
            return toolList;
        }

    private:
        void lazyRegister()
        {
            if (m_head)
            {
                ToolGroupRegister* tool = m_head;
                while (tool)
                {
                    tool->Register();
                    tool = tool->m_next;
                }
                m_head = nullptr;
            }
        }
        ToolGroupMapper(){}
        TOOLGROUP_MAP m_ToolGroupMap;
        ToolGroupRegister* m_head;
    };

    inline ToolGroupRegister::ToolGroupRegister(EToolGroup group, EDesignerTool tool, const char* name, const char* classname)
        : m_next(nullptr)
        , m_group(group)
        , m_tool(tool)
        , m_name(name)
        , m_className(classname)
    {
        ToolGroupMapper::the().AddTool(this);
    }

    inline void ToolGroupRegister::Register()
    {
        ToolGroupMapper::the().AddTool(m_group, m_tool);
        Serialization::getEnumDescription<CD::EDesignerTool>().add(m_tool, m_name, m_className);
    }
}

typedef CD::DesignerFactory<const char*, IBasePanel, BaseTool*> uiFactory;
typedef CD::DesignerFactory<CD::EDesignerTool, IBasePanel, BaseTool*> uiPanelFactory;
typedef CD::DesignerFactory<CD::EDesignerTool, BaseTool, int> toolFactory;

#define REGISTER_GENERALPANEL(key, derived_panel, constructor) \
    static uiFactory::GeneralPanelCreator<derived_panel, constructor> generalFactory##derived_panel##key(#key);

#define REGISTER_DESIGNER_TOOL(_key, _group, _name, _class)            \
    static toolFactory::ToolCreator<_class> toolFactory##_class(_key); \
    static CD::ToolGroupRegister toolGroupRegister##_class(_group, _key, _name,#_class);

#define REGISTER_DESIGNER_TOOL_WITH_PANEL(_key, _group, _name, _class, _panel) \
    REGISTER_DESIGNER_TOOL(_key, _group, _name, _class)                        \
    static uiPanelFactory::AttributePanelCreator<_class, _panel> attributeUIFactory##_class(_key);