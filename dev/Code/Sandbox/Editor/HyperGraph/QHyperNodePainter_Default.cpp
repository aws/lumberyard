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
#include "StdAfx.h"
#include "QHyperNodePainter_Default.h"
#include "HyperGraph/FlowGraphDebuggerEditor.h"
#include "FlowGraphNode.h"
#include <IFlowGraphDebugger.h>
#include "HyperGraph.h"

#include <QtUtil.h>
#include <QtUtilWin.h>

static QColor TransparentColor(const QColor& c)
{
    QColor r = c;
    r.setAlpha(GET_TRANSPARENCY);
    return r;
}

#define NODE_BACKGROUND_COLOR  TransparentColor(gSettings.hyperGraphColors.colorNodeBkg)
#define NODE_BACKGROUND_SELECTED_COLOR  TransparentColor(gSettings.hyperGraphColors.colorNodeSelected)
#define NODE_BACKGROUND_CUSTOM_COLOR1  TransparentColor(gSettings.hyperGraphColors.colorCustomNodeBkg)
#define NODE_BACKGROUND_CUSTOM_SELECTED_COLOR1  TransparentColor(gSettings.hyperGraphColors.colorCustomSelectedNodeBkg)
#define NODE_OUTLINE_COLOR  TransparentColor(gSettings.hyperGraphColors.colorNodeOutline)
#define NODE_OUTLINE_COLOR_SELECTED TransparentColor(gSettings.hyperGraphColors.colorNodeOutlineSelected)
#define NODE_DEBUG_TITLE_COLOR  TransparentColor(gSettings.hyperGraphColors.colorDebugNodeTitle)
#define NODE_DEBUG_COLOR  TransparentColor(gSettings.hyperGraphColors.colorDebugNode)

#define TEXT_COLOR  TransparentColor(gSettings.hyperGraphColors.colorText)

#define TITLE_TEXT_COLOR    TransparentColor(gSettings.hyperGraphColors.colorTitleText)
#define TITLE_ENTITY_TEXT_INAVLID_COLOR TransparentColor(gSettings.hyperGraphColors.colorEntityTextInvalid)
#define TITLE_TEXT_SELECTED_COLOR    TransparentColor(gSettings.hyperGraphColors.colorTitleTextSelected)

#define PORT_BACKGROUND_COLOR  TransparentColor(gSettings.hyperGraphColors.colorPort)
#define PORT_BACKGROUND_SELECTED_COLOR  TransparentColor(gSettings.hyperGraphColors.colorPortSelected)
#define PORT_TEXT_COLOR TransparentColor(gSettings.hyperGraphColors.colorText)
#define PORT_DEBUGGING_COLOR TransparentColor(gSettings.hyperGraphColors.colorPortDebugging)
#define PORT_DEBUGGING_TEXT_COLOR TransparentColor(gSettings.hyperGraphColors.colorPortDebuggingText)

#define DOWN_ARROW_COLOR    TransparentColor(gSettings.hyperGraphColors.colorDownArrow)

#define BREAKPOINT_COLOR    TransparentColor(gSettings.hyperGraphColors.colorBreakPoint)
#define BREAKPOINT_DISABLED_COLOR   TransparentColor(gSettings.hyperGraphColors.colorBreakPointDisabled)
#define BREAKPOINT_ARROW_COLOR  TransparentColor(gSettings.hyperGraphColors.colorBreakPointArrow)

#define ENTITY_PORT_NOT_CONNECTED_COLOR TransparentColor(gSettings.hyperGraphColors.colorEntityPortNotConnected)


// Sizing.
#define TITLE_X_OFFSET 4
#define TITLE_Y_OFFSET 0
#define TITLE_HEIGHT 16
#define MINIMIZE_BOX_WIDTH 16

static const float BREAKPOINT_X_OFFSET = 10.0f;
static const float MINIMIZE_BOX_MAX_HEIGHT = 12.0f;

#define PORT_DOWN_ARROW_WIDTH 20
#define PORT_DOWN_ARROW_HEIGHT 8
#define PORTS_OUTER_MARGIN 12
#define PORTS_INNER_MARGIN 12
#define PORTS_Y_SPACING 14

static QColor PortTypeToColor[] =
{
    QColor(0x00, 0xFF, 0x00),
    QColor(0xFF, 0x00, 0x00),
    QColor(0x00, 0x50, 0xFF),
    QColor(0xFF, 0xFF, 0xFF),
    QColor(0xFF, 0x00, 0xFF),
    QColor(0xFF, 0xFF, 0x00),
    QColor(0x00, 0xFF, 0xFF),
    QColor(0x7F, 0x00, 0xFF),
    QColor(0x00, 0xFF, 0xFF),
    QColor(0xFF, 0x7F, 0x00),
    QColor(0x00, 0xFF, 0x7F),
    QColor(0x7F, 0x7F, 0x7F),
    QColor(0x00, 0x00, 0x00)
};

static const int NumPortColors = sizeof(PortTypeToColor) / sizeof(*PortTypeToColor);

namespace
{
    struct SAssets
    {
        SAssets()
            : font("Tahoma", 10.0f)
            , brushBackgroundSelected(NODE_BACKGROUND_SELECTED_COLOR)
            , brushBackgroundUnselected(NODE_BACKGROUND_COLOR)
            , brushBackgroundCustomSelected1(NODE_BACKGROUND_CUSTOM_SELECTED_COLOR1)
            , brushBackgroundCustomUnselected1(NODE_BACKGROUND_CUSTOM_COLOR1)
            , penOutline(NODE_OUTLINE_COLOR)
            , penOutlineSelected(NODE_OUTLINE_COLOR_SELECTED)
            , brushTitleTextSelected(TITLE_TEXT_SELECTED_COLOR)
            , brushTitleTextUnselected(TITLE_TEXT_COLOR)
            , brushEntityTextValid(TEXT_COLOR)
            , brushEntityTextInvalid(TITLE_ENTITY_TEXT_INAVLID_COLOR)
            , penPort(PORT_BACKGROUND_COLOR)
            , sfInputPort()
            , sfOutputPort()
            , brushPortText(TEXT_COLOR)
            , brushPortSelected(PORT_BACKGROUND_SELECTED_COLOR)
            , breakPoint(BREAKPOINT_COLOR)
            , breakPointArrow(BREAKPOINT_ARROW_COLOR)
            , penBreakpointDisabled(BREAKPOINT_DISABLED_COLOR)
            , brushPortDebugActivated(PORT_DEBUGGING_COLOR)
            , brushPortDebugText(PORT_DEBUGGING_TEXT_COLOR)
            , brushMinimizeArrow(NODE_OUTLINE_COLOR)
            , penDownArrow(DOWN_ARROW_COLOR)
            , brushEntityPortNotConnected(ENTITY_PORT_NOT_CONNECTED_COLOR)
            , brushDebugNodeTitle(NODE_DEBUG_TITLE_COLOR)
            , brushDebugNode(NODE_DEBUG_COLOR)
        {
            sfInputPort = Qt::AlignLeft | Qt::AlignVCenter;
            sfOutputPort = Qt::AlignRight | Qt::AlignVCenter;

            for (int i = 0; i < NumPortColors; i++)
            {
                QColor c = PortTypeToColor[i];
                c.setAlpha(GET_TRANSPARENCY);
                brushPortFill[i] = QBrush(c);
            }
        }
        ~SAssets()
        {
        }

        void Update()
        {
            brushBackgroundSelected.setColor(NODE_BACKGROUND_SELECTED_COLOR);
            brushBackgroundUnselected.setColor(NODE_BACKGROUND_COLOR);
            brushBackgroundCustomSelected1.setColor(NODE_BACKGROUND_CUSTOM_SELECTED_COLOR1);
            brushBackgroundCustomUnselected1.setColor(NODE_BACKGROUND_CUSTOM_COLOR1);
            penOutline.setColor(NODE_OUTLINE_COLOR);
            penOutlineSelected.setColor(NODE_OUTLINE_COLOR_SELECTED);
            brushTitleTextSelected.setColor(TITLE_TEXT_SELECTED_COLOR);
            brushTitleTextUnselected.setColor(TITLE_TEXT_COLOR);
            brushEntityTextValid.setColor(TEXT_COLOR);
            brushEntityTextInvalid.setColor(TITLE_ENTITY_TEXT_INAVLID_COLOR);
            penPort.setColor(PORT_BACKGROUND_COLOR);
            brushPortText.setColor(TEXT_COLOR);
            brushPortSelected.setColor(PORT_BACKGROUND_SELECTED_COLOR);
            breakPoint.setColor(BREAKPOINT_COLOR);
            breakPointArrow.setColor(BREAKPOINT_ARROW_COLOR);
            penBreakpointDisabled.setColor(BREAKPOINT_DISABLED_COLOR);
            brushPortDebugActivated.setColor(PORT_DEBUGGING_COLOR);
            brushPortDebugText.setColor(PORT_DEBUGGING_TEXT_COLOR);
            brushMinimizeArrow.setColor(NODE_OUTLINE_COLOR);
            penDownArrow.setColor(DOWN_ARROW_COLOR);
            brushEntityPortNotConnected.setColor(ENTITY_PORT_NOT_CONNECTED_COLOR);
            brushDebugNodeTitle.setColor(NODE_DEBUG_TITLE_COLOR);
            brushDebugNode.setColor(NODE_DEBUG_COLOR);

            for (int i = 0; i < NumPortColors; i++)
            {
                QColor c = PortTypeToColor[i];
                c.setAlpha(GET_TRANSPARENCY);
                brushPortFill[i].setColor(c);
            }
        }

        QFont font;
        QBrush brushBackgroundSelected;
        QBrush brushBackgroundUnselected;
        QBrush brushBackgroundCustomSelected1;
        QBrush brushBackgroundCustomUnselected1;
        QBrush brushDebugNodeTitle;
        QBrush brushDebugNode;
        QPen penOutline;
        QPen penOutlineSelected;
        QBrush brushTitleTextSelected;
        QBrush brushTitleTextUnselected;
        QBrush brushEntityTextValid;
        QBrush brushEntityTextInvalid;
        QPen penPort;
        QPen penDownArrow;
        int sfInputPort;
        int sfOutputPort;
        QBrush brushPortText;
        QBrush brushPortSelected;
        QBrush breakPoint;
        QBrush breakPointArrow;
        QPen penBreakpointDisabled;
        QBrush brushPortDebugActivated;
        QBrush brushPortDebugText;
        QBrush brushMinimizeArrow;
        QBrush brushPortFill[NumPortColors];
        QBrush brushEntityPortNotConnected;
    };
}

struct SDefaultRenderPort
{
    SDefaultRenderPort()
        : pPortArrow(0)
        , pRectangle(0)
        , pText(0)
        , pBackground(0) {}
    QDisplayPortArrow* pPortArrow;
    QDisplayRectangle* pRectangle;
    QDisplayString* pText;
    QDisplayRectangle* pBackground;
    int id;
};

void QHyperNodePainter_Default::Paint(CHyperNode* pNode, QDisplayList* pList)
{
    static SAssets* pAssets = 0;

    if (pNode->GetBlackBoxWhenMinimized()) //hide node
    {
        return;
    }

    if (!pAssets)
    {
        pAssets = new SAssets();
    }
    pAssets->Update();

    const bool bIsFlowgraph = reinterpret_cast<CHyperGraph*>(pNode->GetGraph())->IsFlowGraph();
    CFlowNode* pFlowNode = bIsFlowgraph ? reinterpret_cast<CFlowNode*>(pNode) : NULL;

    // background
    QDisplayRectangle* pBackgroundRect = pList->Add<QDisplayRectangle>();

    if (pNode->IsSelected())
    {
        pBackgroundRect->SetStroked(pAssets->penOutlineSelected);
    }
    else
    {
        pBackgroundRect->SetStroked(pAssets->penOutline);
    }

    pBackgroundRect->SetHitEvent(eSOID_Background);

    // node title
    QDisplayRectangle* pTitleRect = pList->Add<QDisplayRectangle>();

    if (pNode->IsSelected())
    {
        pTitleRect->SetStroked(pAssets->penOutlineSelected);
    }
    else
    {
        pTitleRect->SetStroked(pAssets->penOutline);
    }

    pTitleRect->SetHitEvent(eSOID_Title);

    QDisplayMinimizeArrow* pMinimizeArrow = pList->Add<QDisplayMinimizeArrow>()
            ->SetStroked(pAssets->penOutline)
            ->SetFilled(pAssets->brushMinimizeArrow)
            ->SetHitEvent(eSOID_Minimize);
    QString title = pNode->GetTitle();
    if (gSettings.bFlowGraphShowNodeIDs)
    {
        title += QString("\nID=%1").arg(pNode->GetId());
    }

    QDisplayString* pTitleString = pList->Add<QDisplayString>()
            ->SetFont(pAssets->font)
            ->SetText(title)
            ->SetHitEvent(eSOID_Title);

    // selected/unselected fill
    const bool bDebugNode = pFlowNode ? (pFlowNode->GetCategory() == EFLN_DEBUG) : false;
    pTitleString->SetBrush(pNode->IsSelected() ? pAssets->brushTitleTextSelected : pAssets->brushTitleTextUnselected);
    pTitleRect->SetFilled(bDebugNode ? pAssets->brushDebugNodeTitle : pAssets->brushBackgroundSelected);

    pBackgroundRect->SetFilled(bDebugNode ? pAssets->brushDebugNode : pAssets->brushBackgroundUnselected);

    bool bAnyHiddenInput = false;
    bool bAnyHiddenOutput = false;

    bool bEntityInputPort = false;
    bool bEntityPortConnected = false;

    // attached entity?
    QDisplayRectangle* pEntityRect = 0;
    QDisplayString* pEntityString = 0;
    QDisplayPortArrow* pEntityEllipse = 0;
    if (pNode->CheckFlag(EHYPER_NODE_ENTITY))
    {
        bEntityInputPort = true;
        pEntityRect = pList->Add<QDisplayRectangle>()
                ->SetStroked(pNode->IsSelected() ? pAssets->penOutlineSelected : pAssets->penOutline)
                ->SetHitEvent(eSOID_FirstInputPort);

        pEntityEllipse = pList->Add<QDisplayPortArrow>()
                ->SetFilled(pAssets->brushPortFill[IVariable::UNKNOWN])
                ->SetHitEvent(eSOID_FirstInputPort);

        QString str(pNode->GetEntityTitle());
        pEntityString = pList->Add<QDisplayString>()
                ->SetHitEvent(eSOID_FirstInputPort)
                ->SetFont(pAssets->font)
                ->SetText(str);

        if (pNode->GetInputs() && (*pNode->GetInputs())[0].nConnected != 0)
        {
            bEntityPortConnected = true;
        }

        if (pNode->IsEntityValid() ||
            pNode->CheckFlag(EHYPER_NODE_GRAPH_ENTITY) ||
            pNode->CheckFlag(EHYPER_NODE_GRAPH_ENTITY2) ||
            bEntityPortConnected)
        {
            pEntityRect->SetFilled(pAssets->brushBackgroundSelected);
            pEntityString->SetBrush(pAssets->brushEntityTextValid);
        }
        else
        {
            pEntityRect->SetFilled(pAssets->brushEntityPortNotConnected);
            pEntityString->SetBrush(pAssets->brushEntityTextInvalid);
        }
    }

    std::vector<SDefaultRenderPort> inputPorts;
    std::vector<SDefaultRenderPort> outputPorts;
    const CHyperNode::Ports& inputs = *pNode->GetInputs();
    const CHyperNode::Ports& outputs = *pNode->GetOutputs();

    inputPorts.reserve(inputs.size());
    outputPorts.reserve(outputs.size());

    // input ports
    for (size_t i = 0; i < inputs.size(); i++)
    {
        if (bEntityInputPort && i == 0)
        {
            continue;
        }
        const CHyperNodePort& pp = inputs[i];
        if (pp.bVisible || pp.nConnected != 0)
        {
            SDefaultRenderPort pr;

            const QBrush& pBrush = pAssets->brushPortFill[pp.pVar->GetType()];

            pr.pBackground = pList->Add<QDisplayRectangle>()
                    ->SetHitEvent(eSOID_FirstInputPort + i);

            if (pp.bSelected)
            {
                pr.pBackground->SetFilled(pAssets->brushPortSelected);
            }

            else if (pNode->IsPortActivationModified(&pp))
            {
                pr.pBackground = pList->Add<QDisplayRectangle>()
                        ->SetFilled(pAssets->brushPortDebugActivated)
                        ->SetHitEvent(eSOID_FirstInputPort + i);
            }

            pr.pPortArrow = pList->Add<QDisplayPortArrow>()
                    ->SetFilled(pBrush)
                    ->SetHitEvent(eSOID_FirstInputPort + i);

            QString str(pNode->IsPortActivationModified() ? pNode->GetDebugPortValue(pp) : pNode->GetPortName(pp));
            pr.pText = pList->Add<QDisplayString>()
                    ->SetFont(pAssets->font)
                    ->SetStringFormat(pAssets->sfInputPort)
                    ->SetBrush(pNode->IsPortActivationModified(&pp) ?  pAssets->brushPortDebugText : pAssets->brushPortText)
                    ->SetHitEvent(eSOID_FirstInputPort + i)
                    ->SetText(str);
            pr.id = i;
            inputPorts.push_back(pr);
        }
        else
        {
            bAnyHiddenInput = true;
        }
    }
    // output ports
    for (size_t i = 0; i < outputs.size(); i++)
    {
        const CHyperNodePort& pp = outputs[i];

        const bool hasBreakPoint = bIsFlowgraph ? GetIEditor()->GetFlowGraphDebuggerEditor()->HasBreakpoint(pFlowNode, &outputs[i]) : false;
        if (pp.bVisible || pp.nConnected != 0 || hasBreakPoint)
        {
            SDefaultRenderPort pr;

            const QBrush& pBrush = pAssets->brushPortFill[pp.pVar->GetType()];

            pr.pBackground = pList->Add<QDisplayRectangle>()
                    ->SetHitEvent(eSOID_FirstOutputPort + i);

            if (pp.bSelected)
            {
                pr.pBackground->SetFilled(pAssets->brushPortSelected);
            }

            pr.pPortArrow = pList->Add<QDisplayPortArrow>()
                    ->SetFilled(pBrush)
                    ->SetHitEvent(eSOID_FirstOutputPort + i);

            QString str(pNode->GetPortName(pp));
            pr.pText = pList->Add<QDisplayString>()
                    ->SetFont(pAssets->font)
                    ->SetStringFormat(pAssets->sfOutputPort)
                    ->SetBrush(pAssets->brushPortText)
                    ->SetHitEvent(eSOID_FirstOutputPort + i)
                    ->SetText(str);
            pr.id = 1000 + i;
            outputPorts.push_back(pr);
        }
        else
        {
            bAnyHiddenOutput = true;
        }
    }

    // calculate size now
    QSizeF size(0, 0);

    QRectF rect = pTitleString->GetTextBounds();
    float titleHeight = rect.height();
    float curY = rect.height();
    float width = rect.width() + MINIMIZE_BOX_WIDTH; // Add minimize box size.
    float entityHeight = 0;
    QRectF entityTextRect = rect;

    IFlowGraphDebuggerConstPtr pFlowGraphDebugger = GetIFlowGraphDebuggerPtr();
    const SBreakPoint breakPoint = pFlowGraphDebugger ? pFlowGraphDebugger->GetBreakpoint() : SBreakPoint();

    const bool breakPointHit = (breakPoint.flowGraph != NULL);
    const bool isModule = breakPointHit && (breakPoint.flowGraph->GetType() == IFlowGraph::eFGT_Module);
    const bool isModuleDummy = pFlowNode && isModule && (pFlowNode->GetIFlowGraph()->GetType() == IFlowGraph::eFGT_Module);

    if (pEntityString)
    {
        entityTextRect = pEntityString->GetTextBounds();
        entityTextRect.translate(0, curY);
        float entityWidth = entityTextRect.width() + PORTS_OUTER_MARGIN;
        width = std::max<float>(width, entityWidth);
        curY += entityTextRect.height();
        entityHeight = entityTextRect.height();

        if (pFlowNode)
        {
            const bool hasBreakPoint = GetIEditor()->GetFlowGraphDebuggerEditor()->HasBreakpoint(pFlowNode, &inputs[0]);

            QRectF rectBreak = entityTextRect;
            rectBreak.translate(-BREAKPOINT_X_OFFSET, (entityHeight - BREAKPOINT_X_OFFSET) * 0.5f);
            rectBreak.setWidth(BREAKPOINT_X_OFFSET);
            rectBreak.setHeight(BREAKPOINT_X_OFFSET);

            const bool bTriggered = (breakPointHit || isModuleDummy) && !breakPoint.addr.isOutput && (pFlowNode->GetFlowNodeId() == breakPoint.addr.node && breakPoint.addr.port == 0);
            if (hasBreakPoint)
            {
                SFlowAddress addr;
                addr.isOutput = false;
                addr.node = pNode->GetFlowNodeId();
                addr.port = 0;

                bool bIsEnabled = false;
                bool bIsTracepoint = false;
                CheckBreakpoint(pFlowNode->GetIFlowGraph(), addr, bIsEnabled, bIsTracepoint);

                if (bIsTracepoint)
                {
                    QDisplayTracepoint* pTracePoint = pList->Add<QDisplayTracepoint>();

                    if (bIsEnabled)
                    {
                        pTracePoint->SetFilled(pAssets->breakPoint);
                    }
                    else
                    {
                        pTracePoint->SetStroked(pAssets->penBreakpointDisabled);
                    }

                    pTracePoint->SetRect(rectBreak);
                }
                else
                {
                    QDisplayBreakpoint* pBreakPoint = pList->Add<QDisplayBreakpoint>();

                    if (bIsEnabled)
                    {
                        pBreakPoint->SetFilled(pAssets->breakPoint);
                    }
                    else
                    {
                        pBreakPoint->SetStroked(pAssets->penBreakpointDisabled);
                    }

                    if (bTriggered)
                    {
                        pBreakPoint->SetFilledArrow(pAssets->breakPointArrow);
                    }

                    pBreakPoint->SetTriggered(bTriggered);
                    pBreakPoint->SetRect(rectBreak);
                }
            }
            else if (bTriggered)
            {
                QDisplayPortArrow* pBreakArrow = pList->Add<QDisplayPortArrow>();
                pBreakArrow->SetSize(6);
                pBreakArrow->SetFilled(pAssets->breakPointArrow);
                pBreakArrow->SetRect(rectBreak);
            }
        }
    }

    float portStartY = curY;
    float inputsWidth = 0.0f;
    for (size_t i = 0; i < inputPorts.size(); i++)
    {
        const SDefaultRenderPort& p = inputPorts[i];
        QPointF textLoc(PORTS_OUTER_MARGIN, curY);
        p.pText->SetLocation(textLoc);
        rect = p.pText->GetBounds();
        curY += rect.height();
        inputsWidth = std::max<float>(inputsWidth, rect.width());
        rect.setHeight(rect.height() - 4.0f);
        rect.moveTop(rect.top() + 2.0f);
        rect.setWidth(rect.height());
        rect.moveLeft(2.0f);
        if (p.pPortArrow)
        {
            p.pPortArrow->SetRect(rect);
        }
        if (p.pRectangle)
        {
            p.pRectangle->SetRect(rect);
        }
        pList->SetAttachRect(p.id, QRectF(0, rect.y() + rect.height() * 0.5f, 0.0f, 0.0f));

        if (pFlowNode)
        {
            const int portIndex = p.id;
            const bool hasBreakPoint = GetIEditor()->GetFlowGraphDebuggerEditor()->HasBreakpoint(pFlowNode, &inputs[portIndex]);

            QRectF rectBreak = rect;
            rectBreak.translate(-(BREAKPOINT_X_OFFSET + 2.0f), (rect.height() - BREAKPOINT_X_OFFSET) * 0.5f);
            rectBreak.setWidth(BREAKPOINT_X_OFFSET);
            rectBreak.setHeight(BREAKPOINT_X_OFFSET);

            const bool bTriggered = !breakPoint.addr.isOutput && (breakPointHit || isModuleDummy) && portIndex == breakPoint.addr.port && breakPoint.addr.node == pFlowNode->GetFlowNodeId();
            if (hasBreakPoint)
            {
                SFlowAddress addr;
                addr.isOutput = false;
                addr.node = pNode->GetFlowNodeId();
                addr.port = portIndex;

                bool bIsEnabled = false;
                bool bIsTracepoint = false;
                CheckBreakpoint(pFlowNode->GetIFlowGraph(), addr, bIsEnabled, bIsTracepoint);

                if (bIsTracepoint)
                {
                    QDisplayTracepoint* pTracePoint = pList->Add<QDisplayTracepoint>();

                    if (bIsEnabled)
                    {
                        pTracePoint->SetFilled(pAssets->breakPoint);
                    }
                    else
                    {
                        pTracePoint->SetStroked(pAssets->penBreakpointDisabled);
                    }

                    pTracePoint->SetRect(rectBreak);
                }
                else
                {
                    QDisplayBreakpoint* pBreakPoint = pList->Add<QDisplayBreakpoint>();

                    if (bIsEnabled)
                    {
                        pBreakPoint->SetFilled(pAssets->breakPoint);
                    }
                    else
                    {
                        pBreakPoint->SetStroked(pAssets->penBreakpointDisabled);
                    }

                    if (bTriggered)
                    {
                        pBreakPoint->SetFilledArrow(pAssets->breakPointArrow);
                    }

                    pBreakPoint->SetTriggered(bTriggered);
                    pBreakPoint->SetRect(rectBreak);
                }
            }
            else if (bTriggered)
            {
                QDisplayPortArrow* pBreakArrow = pList->Add<QDisplayPortArrow>();
                pBreakArrow->SetSize(6);
                pBreakArrow->SetFilled(pAssets->breakPointArrow);
                pBreakArrow->SetRect(rectBreak);
            }
        }
    }
    curY = portStartY;
    for (size_t i = 0; i < inputPorts.size(); i++)
    {
        const SDefaultRenderPort& p = inputPorts[i];
        rect = p.pText->GetBounds();
        if (p.pBackground)
        {
            p.pBackground->SetRect(1, curY, PORTS_OUTER_MARGIN + rect.width(), rect.height());
        }
        curY += rect.height();
    }
    float height = curY;
    float outputsWidth = 0.0f;
    for (size_t i = 0; i < outputPorts.size(); i++)
    {
        const SDefaultRenderPort& p = outputPorts[i];
        outputsWidth = std::max<float>(outputsWidth, p.pText->GetBounds().width());
    }
    width = max(width, inputsWidth + outputsWidth + 3 * PORTS_OUTER_MARGIN);
    curY = portStartY;
    for (size_t i = 0; i < outputPorts.size(); i++)
    {
        const SDefaultRenderPort& p = outputPorts[i];
        QPointF textLoc(width - PORTS_OUTER_MARGIN, curY);
        p.pText->SetLocation(textLoc);
        rect = p.pText->GetBounds();
        if (p.pBackground)
        {
            p.pBackground->SetRect(width - PORTS_OUTER_MARGIN - rect.width() - 3, curY, PORTS_OUTER_MARGIN + rect.width() + 3, rect.height());
        }
        curY += rect.height();
        rect.setHeight(rect.height() - 4.0f);
        rect.setWidth(rect.height());
        rect.moveTop(rect.top() + 2.0f);
        rect.moveLeft(width - PORTS_OUTER_MARGIN * 0.5f - 3);
        if (p.pPortArrow)
        {
            p.pPortArrow->SetRect(rect);
        }
        if (p.pRectangle)
        {
            p.pRectangle->SetRect(rect);
        }
        pList->SetAttachRect(p.id, QRectF(width, rect.y() + rect.height() * 0.5f, 0.0f, 0.0f));

        if (pFlowNode)
        {
            const int portIndex = p.id - 1000;
            bool hasBreakPoint = GetIEditor()->GetFlowGraphDebuggerEditor()->HasBreakpoint(pFlowNode, &outputs[portIndex]);

            QRectF rectBreak = rect;
            rectBreak.translate(BREAKPOINT_X_OFFSET, (rect.height() - BREAKPOINT_X_OFFSET) * 0.5f);
            rectBreak.setWidth(BREAKPOINT_X_OFFSET);
            rectBreak.setHeight(BREAKPOINT_X_OFFSET);

            const bool bTriggered = breakPoint.addr.isOutput && (breakPointHit || isModuleDummy) && portIndex == breakPoint.addr.port && breakPoint.addr.node == pFlowNode->GetFlowNodeId();
            if (hasBreakPoint)
            {
                SFlowAddress addr;
                addr.isOutput = true;
                addr.node = pNode->GetFlowNodeId();
                addr.port = portIndex;

                bool bIsEnabled = false;
                bool bIsTracepoint = false;
                CheckBreakpoint(pFlowNode->GetIFlowGraph(), addr, bIsEnabled, bIsTracepoint);

                if (bIsTracepoint)
                {
                    QDisplayTracepoint* pTracePoint = pList->Add<QDisplayTracepoint>();

                    if (bIsEnabled)
                    {
                        pTracePoint->SetFilled(pAssets->breakPoint);
                    }
                    else
                    {
                        pTracePoint->SetStroked(pAssets->penBreakpointDisabled);
                    }

                    pTracePoint->SetRect(rectBreak);
                }
                else
                {
                    QDisplayBreakpoint* pBreakPoint = pList->Add<QDisplayBreakpoint>();

                    if (bIsEnabled)
                    {
                        pBreakPoint->SetFilled(pAssets->breakPoint);
                    }
                    else
                    {
                        pBreakPoint->SetStroked(pAssets->penBreakpointDisabled);
                    }

                    if (bTriggered)
                    {
                        pBreakPoint->SetFilledArrow(pAssets->breakPointArrow);
                    }

                    pBreakPoint->SetTriggered(bTriggered);
                    pBreakPoint->SetRect(rectBreak);
                }
            }
            else if (bTriggered)
            {
                QDisplayPortArrow* pBreakArrow = pList->Add<QDisplayPortArrow>();
                pBreakArrow->SetSize(6);
                pBreakArrow->SetFilled(pAssets->breakPointArrow);
                pBreakArrow->SetRect(rectBreak);
            }
        }
    }
    height = max(height, curY);

    // down arrows if any ports are hidden.
    if (bAnyHiddenInput || bAnyHiddenOutput)
    {
        height += 1;
        // Draw hidden ports gripper.
        if (bAnyHiddenInput)
        {
            QDisplayRectangle* pGrip = pList->Add<QDisplayRectangle>()
                    ->SetHitEvent(eSOID_InputGripper);

            QRectF rect = QRectF(1, height, PORT_DOWN_ARROW_WIDTH, PORT_DOWN_ARROW_HEIGHT);
            //pGrip->SetColors( pAssets->colorTitleUnselectedA, pAssets->colorTitleUnselectedB );
            if (pNode->IsSelected())
            {
                if (pNode->CheckFlag(EHYPER_NODE_CUSTOM_COLOR1))
                {
                    pGrip->SetFilled(pAssets->brushBackgroundCustomSelected1);
                }
                else
                {
                    pGrip->SetFilled(pAssets->brushBackgroundSelected);
                }
            }
            else
            {
                if (pNode->CheckFlag(EHYPER_NODE_CUSTOM_COLOR1))
                {
                    pGrip->SetFilled(pAssets->brushBackgroundCustomUnselected1);
                }
                else
                {
                    pGrip->SetFilled(pAssets->brushBackgroundUnselected);
                }
            }
            pGrip->SetRect(rect);

            // Draw arrow.
            AddDownArrow(pList, QPointF(rect.x() + rect.width() / 2, (rect.y() + rect.bottom()) / 2 + 3), pAssets->penDownArrow);
        }
        if (bAnyHiddenOutput)
        {
            QDisplayRectangle* pGrip = pList->Add<QDisplayRectangle>()
                    ->SetHitEvent(eSOID_OutputGripper);

            QRectF rect = QRectF(width - PORT_DOWN_ARROW_WIDTH - 1, height, PORT_DOWN_ARROW_WIDTH, PORT_DOWN_ARROW_HEIGHT);
            //pGrip->SetColors( pAssets->colorTitleUnselectedA, pAssets->colorTitleUnselectedB );
            pGrip->SetRect(rect);
            if (pNode->IsSelected())
            {
                if (pNode->CheckFlag(EHYPER_NODE_CUSTOM_COLOR1))
                {
                    pGrip->SetFilled(pAssets->brushBackgroundCustomSelected1);
                }
                else
                {
                    pGrip->SetFilled(pAssets->brushBackgroundSelected);
                }
            }
            else
            {
                if (pNode->CheckFlag(EHYPER_NODE_CUSTOM_COLOR1))
                {
                    pGrip->SetFilled(pAssets->brushBackgroundCustomUnselected1);
                }
                else
                {
                    pGrip->SetFilled(pAssets->brushBackgroundUnselected);
                }
            }


            // Draw arrow.
            AddDownArrow(pList, QPointF(rect.x() + rect.width() / 2, (rect.y() + rect.bottom()) / 2 + 3), pAssets->penDownArrow);
        }

        height += PORT_DOWN_ARROW_HEIGHT + 1;
    }

    // resize backing boxes...
    pTitleString->SetRect(2, 0, width - MINIMIZE_BOX_WIDTH, titleHeight);
    pTitleRect->SetRect(0, 0, width, titleHeight);
    pMinimizeArrow->SetRect(width - (MINIMIZE_BOX_WIDTH - 2), 2, 12, min(MINIMIZE_BOX_MAX_HEIGHT, titleHeight - 4));
    curY = titleHeight;
    if (pEntityRect)
    {
        pEntityString->SetRect(PORTS_OUTER_MARGIN, curY, width - PORTS_OUTER_MARGIN, entityTextRect.height());
        pEntityRect->SetRect(0, curY, width, entityTextRect.height());
        pEntityEllipse->SetRect(2, curY + 2, entityTextRect.height() - 4, entityTextRect.height() - 4);
        pList->SetAttachRect(0, QRectF(0, curY + entityHeight * 0.5f, 0.0f, 0.0f));
        curY += entityHeight;
    }
    pBackgroundRect->SetRect(0, curY, width, height - curY);
}

//////////////////////////////////////////////////////////////////////////
void QHyperNodePainter_Default::AddDownArrow(QDisplayList* pList, const QPointF& pnt, const QPen& pPen)
{
    QPointF p = pnt;
    //  p.Y += 0;
    pList->AddLine(QPointF(p.x(), p.y()), QPointF(p.x(), p.y()), pPen);
    pList->AddLine(QPointF(p.x() - 1, p.y() - 1), QPointF(p.x() + 1, p.y() - 1), pPen);
    pList->AddLine(QPointF(p.x() - 2, p.y() - 2), QPointF(p.x() + 2, p.y() - 2), pPen);
    pList->AddLine(QPointF(p.x() - 3, p.y() - 3), QPointF(p.x() + 3, p.y() - 3), pPen);
    pList->AddLine(QPointF(p.x() - 4, p.y() - 4), QPointF(p.x() + 4, p.y() - 4), pPen);
}

void QHyperNodePainter_Default::CheckBreakpoint(IFlowGraphPtr pFlowGraph, const SFlowAddress& addr, bool& bIsBreakPoint, bool& bIsTracepoint)
{
    IFlowGraphDebuggerConstPtr pFlowGraphDebugger = GetIFlowGraphDebuggerPtr();

    if (pFlowGraphDebugger)
    {
        bIsBreakPoint = pFlowGraphDebugger->IsBreakpointEnabled(pFlowGraph, addr);
        bIsTracepoint = pFlowGraphDebugger->IsTracepoint(pFlowGraph, addr);
    }
}
