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

// Description : Implements IHyperNode interface.


#ifndef CRYINCLUDE_EDITOR_HYPERGRAPH_HYPERGRAPHNODE_H
#define CRYINCLUDE_EDITOR_HYPERGRAPH_HYPERGRAPHNODE_H
#pragma once

#include "IHyperGraph.h"
#include "FlowGraphVariables.h"
#include <IFlowSystem.h>
#include <stack>
#include <vector>
#include <tuple>


// forward declarations
class CHyperGraph;
class CHyperNode;
class QHyperGraphWidget;
struct IHyperNodePainter;
struct IQHyperNodePainter;
class QRectF;
class CBlackBoxNode;
struct IHyperNodePainter;


#include "QDisplayList.h"

#undef GetClassName
// Typedefs for code readability
typedef _smart_ptr<CHyperNode> THyperNodePtr;

enum EHyperNodeSubObjectId
{
    eSOID_InputTransparent = -1,

    eSOID_InputGripper = 1,
    eSOID_OutputGripper,
    eSOID_AttachedEntity,
    eSOID_Title,
    eSOID_Background,
    eSOID_Minimize,
    eSOID_Border_UpRight,
    eSOID_Border_Right,
    eSOID_Border_DownRight,
    eSOID_Border_Down,
    eSOID_Border_DownLeft,
    eSOID_Border_Left,
    eSOID_Border_UpLeft,
    eSOID_Border_Up,

    eSOID_FirstInputPort = 1000,
    eSOID_LastInputPort = 1999,
    eSOID_FirstOutputPort = 2000,
    eSOID_LastOutputPort = 2999,
    eSOID_ShiftFirstOutputPort = 3000,
    eSOID_ShiftLastOutputPort = 3999,
};


#define GRID_SIZE 10

//////////////////////////////////////////////////////////////////////////
// Description of hyper node port.
// Used for both input and output ports.
//////////////////////////////////////////////////////////////////////////
class CHyperNodePort
{
public:
    _smart_ptr<IVariable> pVar;

    // True if input/false if output.
    unsigned int bInput : 1;
    unsigned int bSelected : 1;
    unsigned int bVisible : 1;
    unsigned int bAllowMulti : 1;
    unsigned int nConnected;
    int nPortIndex;
    // Local rectangle in node space, where this port was drawn.
    QRectF rect;

    CHyperNodePort()
    {
        nPortIndex = 0;
        bSelected = false;
        bInput = false;
        bVisible = true;
        bAllowMulti = false;
        nConnected = 0;
    }
    CHyperNodePort(IVariable* _pVar, bool _bInput, bool _bAllowMulti = false)
    {
        nPortIndex = 0;
        bInput = _bInput;
        pVar = _pVar;
        bSelected = false;
        bVisible = true;
        bAllowMulti = _bAllowMulti;
        nConnected = 0;
    }
    CHyperNodePort(const CHyperNodePort& port)
    {
        bInput = port.bInput;
        bSelected = port.bSelected;
        bVisible = port.bVisible;
        nPortIndex = port.nPortIndex;
        nConnected = port.nConnected;
        bAllowMulti = port.bAllowMulti;
        rect = port.rect;
        pVar = port.pVar->Clone(true);
    }
    QString GetName() const { return pVar->GetName(); }
    QString GetHumanName() const { return pVar->GetHumanName(); }
};

enum EHyperNodeFlags
{
    EHYPER_NODE_ENTITY         = 0x0001,
    EHYPER_NODE_ENTITY_VALID   = 0x0002,
    EHYPER_NODE_GRAPH_ENTITY   = 0x0004,   // This node targets graph default entity.
    EHYPER_NODE_GRAPH_ENTITY2  = 0x0008,   // Second graph default entity.
    EHYPER_NODE_INSPECTED      = 0x0010,   // Node is being inspected [dynamic, visual-only hint]
    EHYPER_NODE_HIDE_UI        = 0x0100,   // Not visible in components tree.
    EHYPER_NODE_CUSTOM_COLOR1  = 0x0200,   // Custom Color1
    EHYPER_NODE_UNREMOVEABLE     = 0x0400,   // Node cannot be deleted by user
};

//////////////////////////////////////////////////////////////////////////
// IHyperNode is an interface to the single hyper graph node.
//////////////////////////////////////////////////////////////////////////
class SANDBOX_API CHyperNode
    : public IHyperNode
{
public:
    typedef std::vector<CHyperNodePort> Ports;

    //////////////////////////////////////////////////////////////////////////
    // Node clone context.
    struct CloneContext
    {
    };
    //////////////////////////////////////////////////////////////////////////

    CHyperNode();
    virtual ~CHyperNode();

    //////////////////////////////////////////////////////////////////////////
    // IHyperNode
    //////////////////////////////////////////////////////////////////////////
    virtual IHyperGraph* GetGraph() const;
    virtual HyperNodeID GetId() const { return m_id; }
    virtual QString GetName() const { return m_name; };
    virtual void SetName(const QString& sName);
    //////////////////////////////////////////////////////////////////////////

    virtual QString GetClassName() const { return m_classname; };
    virtual QString GetDescription() const { return ""; }
    virtual void Serialize(XmlNodeRef& node, bool bLoading, CObjectArchive* ar = 0);

    virtual IHyperNode* GetParent() { return 0; }
    virtual IHyperGraphEnumerator* GetChildrenEnumerator() { return 0; }

    virtual QColor GetCategoryColor() { return QColor(160, 160, 165); }
    virtual void DebugPortActivation(TFlowPortId port, const char* value) {}
    virtual bool IsPortActivationModified(const CHyperNodePort* port = NULL) { return false; }
    virtual void ClearDebugPortActivation() {}
    virtual QString GetDebugPortValue(const CHyperNodePort& pp) { return GetPortName(pp); }
    virtual void ResetDebugPortActivation(CHyperNodePort* port) {}
    virtual bool IsDebugPortActivated(CHyperNodePort* port) { return false; }
    virtual bool IsObsolete() const { return false; }

    /**
    * Entity ID resolution for compatibility with Component Entities. If a CryEntity flowgraph
    * references a component entity, we need to resolve the flowgraph node's ID after the component
    * entities are loaded.
    */
    virtual bool NeedsEntityIdResolve() const { return false; }
    virtual void ResolveEntityId() {}

    /**
    * Adds a configured Input port to the given HyperGraphNode
    * @param portName The Name of the port to be added
    * @param defaultValue Default value that the port is set to
    * @param portDescription Tooltip description of this port
    * @param enumeratedValues A vector or pairs indicating enumerated values to be listed in a dropdown, given that this is an enumerated port
    *                         this is nullable and if left blank will allow for a non enumerated port to be created
    * @param isInputPort Indicates if the port is an Input port (false sets the port to be output instead)
    * @param portDataType Indicates the type of the port being added
    */
    template <class PortType>
    void AddPortEx(const QString& portName, PortType defaultValue,
        const QString& portDescription, std::vector<std::pair< QString, PortType> >* enumeratedValues  = nullptr,
        const bool isInputPort = true, const int portDataType = IVariable::DT_SIMPLE)
    {
        // The port currently being created for addition
        CHyperNodePort currentPort;

        // If this port does not have enumerated values
        if (enumeratedValues == nullptr)
        {
            // Then add a regular port
            currentPort.pVar = new CVariableFlowNode < PortType >;
        }
        // Otherwise read enumerated values and add an enumerated port
        else
        {
            CVariableFlowNodeEnum<PortType>* pEnumVar = new CVariableFlowNodeEnum < PortType >;
            for (auto& enumerationValue : * enumeratedValues)
            {
                pEnumVar->AddEnumItem(enumerationValue.first, enumerationValue.second);
            }
            currentPort.pVar = pEnumVar;
        }

        currentPort.bInput = isInputPort;
        currentPort.pVar->Set(defaultValue);
        currentPort.pVar->SetName(portName);
        currentPort.pVar->SetDescription(portDescription.toUtf8().data());
        currentPort.pVar->SetDataType(portDataType);

        AddPort(currentPort);
    }

    /**
    * Adds a configured Input port to the given HyperGraphNode
    * @param portName The Name of the port to be added
    * @param defaultValue Default value that the port is set to
    * @param portDescription Tooltip description of this port
    * @param isInputPort Indicates if the port is an Input port (false sets the port to be output instead)
    * @param portDataType Indicates the type of the port being added
    */
    template <class PortType>
    void AddPortEx(const QString& portName, PortType defaultValue,
        const QString& portDescription,
        const bool isInputPort = true, const int portDatatype = IVariable::DT_SIMPLE)
    {
        AddPortEx<PortType>(portName, defaultValue, portDescription, nullptr, isInputPort, portDatatype);
    }

    void SetFlag(uint32 nFlag, bool bSet)
    {
        if (bSet)
        {
            m_nFlags |= nFlag;
        }
        else
        {
            m_nFlags &= ~nFlag;
        }
    }
    bool CheckFlag(uint32 nFlag) const { return ((m_nFlags & nFlag) == nFlag); }
    uint32 GetFlags() const { return m_nFlags; }

    // Changes node class.
    void SetGraph(CHyperGraph* pGraph) { m_pGraph = pGraph; }
    void SetClass(const QString& classname)
    {
        m_classname = classname;
    }

    QPointF GetPos() const { return m_rect.topLeft(); }

    virtual void SetPos(const QPointF& pos);

    // Initializes HyperGraph node for specific graph.
    virtual void Init() = 0;
    // Deinitializes HyperGraph node.
    virtual void Done();
    // Clone this hyper graph node.
    virtual CHyperNode* Clone() = 0;
    virtual void CopyFrom(const CHyperNode& src);

    virtual void PostClone(CBaseObject* pFromObject, CObjectCloneContext& ctx) {};

    //////////////////////////////////////////////////////////////////////////
    Ports* GetInputs() { return &m_inputs; };
    Ports* GetOutputs() { return &m_outputs; };
    //////////////////////////////////////////////////////////////////////////

    // Finds a port by name.
    virtual CHyperNodePort* FindPort(const QString& portname, bool bInput);
    // Finds a port by index
    virtual CHyperNodePort* FindPort(int idx, bool bInput);

    CHyperNodePort* CreateMissingPort(const char* portname, bool bInput, bool bAttribute = false);
    int GetMissingPortCount() const { return m_iMissingPort; }

    // Adds a new port to the hyper graph node.
    // Only used by hypergraph manager.
    void AddPort(const CHyperNodePort& port);
    void ClearPorts();

    // Invalidate is called to notify UI that some parameters of graph node have been changed.
    void Invalidate(bool redraw, bool isModified = true);

    //////////////////////////////////////////////////////////////////////////
    // Drawing interface.
    //////////////////////////////////////////////////////////////////////////
    void Draw(QHyperGraphWidget* pView, QPainter* gr, bool render);                 // Qt version

    const QRectF& GetRect() const
    {
        return m_rect;
    }
    void SetRect(const QRectF& rc);

    // Calculates node's screen size.
    QSizeF CalculateSize();

    enum
    {
        MAX_DRAW_PRIORITY = 255
    };
    virtual int GetDrawPriority() { return m_drawPriority; }
    virtual const QRectF* GetResizeBorderRect() const { return NULL; }
    virtual void SetResizeBorderRect(const QRectF& newBordersRect) {}
    virtual void OnZoomChange(float zoom) {}
    virtual bool IsGridBound() { return true; }
    virtual bool IsEditorSpecialNode() { return false; }
    virtual bool IsFlowNode() { return true; }
    virtual bool IsTooltipShowable() { return true; }

    virtual TFlowNodeId GetTypeId() { return InvalidFlowNodeTypeId; }

    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    virtual void SetSelected(bool bSelected);
    bool IsSelected() const { return m_bSelected; };

    void SelectInputPort(int nPort, bool bSelected);
    void SelectOutputPort(int nPort, bool bSelected);

    void SetModified(bool bModified = true);

    // Called when any input ports values changed.
    virtual void OnInputsChanged() {};

    // called when we are about to enter game mode
    virtual void OnEnteringGameMode() {}

    // get FlowNodeId if it is s FlowGraph node
    virtual TFlowNodeId GetFlowNodeId() {return InvalidFlowNodeId; }

    // Called when edge is unlinked from this node.
    virtual void Unlinked(bool bInput) {};

    virtual int GetCustomSelectionMode() { return -1; }

    //////////////////////////////////////////////////////////////////////////
    // Context Menu processing.
    //////////////////////////////////////////////////////////////////////////
    virtual void PopulateContextMenu(QMenu* menu, int baseCommandId);
    virtual void OnContextMenuCommand(int nCmd);

    //////////////////////////////////////////////////////////////////////////
    virtual CVarBlock* GetInputsVarBlock();

    virtual QString GetTitle() const;
    virtual bool IsEntityValid () const { return false; }
    virtual QString GetEntityTitle() const { return ""; };
    virtual QString GetPortName(const CHyperNodePort& port);
    virtual QString GetUIClassName() const { return m_classname; }

    virtual QString VarToValue(IVariable* var);


    bool GetAttachQRect(int i, QRectF* pRect)
    {
        return m_qdispList.GetAttachRect(i, pRect);
    }

    virtual int GetObjectAt(const QPointF& point)
    {
        return m_qdispList.GetObjectAt(point - GetPos());
    }

    virtual CHyperNodePort* GetPortAtPoint(const QPointF& point)
    {
        int obj = GetObjectAt(point);
        if (obj >= eSOID_FirstInputPort && obj <= eSOID_LastInputPort)
        {
            if (!m_outputs.empty())
            {
                return &m_inputs.at(obj - eSOID_FirstInputPort);
            }
        }
        else if (obj >= eSOID_FirstOutputPort && obj <= eSOID_LastOutputPort)
        {
            if (!m_outputs.empty())
            {
                return &m_outputs.at(obj - eSOID_FirstOutputPort);
            }
        }
        return 0;
    }

    void UnselectAllPorts();

    void RecordUndo();
    virtual IUndoObject* CreateUndo();

    IHyperGraphEnumerator* GetRelatedNodesEnumerator();

    //////////////////////////////////////////////////////////////////////////
    //Black Box code
    void SetBlackBox(CHyperNode* pNode){ m_pBlackBox = pNode; Invalidate(true); }

    //! Returns a valid pointer to the Group only when it is Minimized
    //! Otherwise returns null
    CHyperNode* GetBlackBoxWhenMinimized() const;

    //! Returns a valid pointer to the Group node to which this node is attached
    //! null if this node is not part of a Group
    CBlackBoxNode* GetBlackBox() const;

    int GetDebugCount() const { return m_debugCount; }
    void IncrementDebugCount() { m_debugCount++; }
    void ResetDebugCount() { m_debugCount = 0; }

protected:
    void Redraw();      // Qt version

protected:
    friend class CHyperGraph;
    friend class CHyperGraphManager;

    CHyperGraph* m_pGraph;
    HyperNodeID m_id;

    QString m_name;
    QString m_classname;

    QRectF m_rect;

    uint32 m_nFlags;
    int m_drawPriority;

    unsigned int m_bSizeValid : 1;
    unsigned int m_bSelected : 1;

    // Input/Output ports.
    Ports m_inputs;
    Ports m_outputs;

    IQHyperNodePainter* m_pqPainter;
    QDisplayList m_qdispList;

    CHyperNode* m_pBlackBox;

    int m_iMissingPort;

private:
    int m_debugCount;
};

template <typename It>
class CHyperGraphIteratorEnumerator
    : public IHyperGraphEnumerator
{
public:
    typedef It Iterator;

    CHyperGraphIteratorEnumerator(Iterator begin, Iterator end)
        : m_begin(begin)
        , m_end(end)
    {
    }
    virtual void Release() { delete this; };
    virtual IHyperNode* GetFirst()
    {
        if (m_begin != m_end)
        {
            return *m_begin++;
        }
        return 0;
    }
    virtual IHyperNode* GetNext()
    {
        if (m_begin != m_end)
        {
            return *m_begin++;
        }
        return 0;
    }

private:
    Iterator m_begin;
    Iterator m_end;
};


//////////////////////////////////////////////////////////////////////////
class CHyperNodeDescendentEnumerator
    : public IHyperGraphEnumerator
{
    CHyperNode* m_pNextNode;

    std::stack<IHyperGraphEnumerator*> m_stack;

public:
    CHyperNodeDescendentEnumerator(CHyperNode* pParent)
        : m_pNextNode(pParent)
    {
    }
    virtual ~CHyperNodeDescendentEnumerator(){}
    virtual void Release() { delete this; }
    virtual IHyperNode* GetFirst()
    {
        return Iterate();
    }
    virtual IHyperNode* GetNext()
    {
        return Iterate();
    }

private:
    IHyperNode* Iterate()
    {
        CHyperNode* pNode = m_pNextNode;
        if (!pNode)
        {
            return 0;
        }

        // Check whether the current node has any children.
        IHyperGraphEnumerator* children = m_pNextNode->GetChildrenEnumerator();
        CHyperNode* pChild = 0;
        if (children)
        {
            pChild = static_cast<CHyperNode*>(children->GetFirst());
        }
        if (!pChild && children)
        {
            children->Release();
        }

        // The current node has children.
        if (pChild)
        {
            m_pNextNode = pChild;
            m_stack.push(children);
        }
        else
        {
            // The current node has no children - go to the next node.
            CHyperNode* pNextNode = 0;
            while (!pNextNode && !m_stack.empty())
            {
                pNextNode = static_cast<CHyperNode*>(m_stack.top()->GetNext());
                if (!pNextNode)
                {
                    m_stack.top()->Release();
                    m_stack.pop();
                }
            }
            m_pNextNode = pNextNode;
        }

        return pNode;
    }
};

#endif // CRYINCLUDE_EDITOR_HYPERGRAPH_HYPERGRAPHNODE_H
