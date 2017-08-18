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
#include "QHyperNodePainter_BlackBox.h"
#include "BlackBoxNode.h"
#include "HyperGraph.h"

#include <QtUtil.h>

// Minimize box
static const float g_minimizeBoxPositionX = 5.0f;
static const float g_minimizeBoxPositionY = 8.0f;
static const float g_minimizeBoxDimension = 20.0f;

static const float g_nodeHighlightFactor = 1.2f;
static const float g_transparentAlpha = 50.0f;
static const int g_colorValueMax = 255;
static const int g_colorValueMin = 0;

static const float g_defaultFontSize = 10.0f;
static const float g_fontSizeSmall = 15.0f;
static const float g_fontSizeMedium = 20.0f;
static const float g_fontSizeLarge = 40.0f;

// UI element compensation
static const float g_portsOuterMargin = 6.0f;
static const float g_borderThickness = 3.0f;
static const float g_groupTitleOffset = 20.0f;

namespace
{
    struct SGroupAssets
    {
        SGroupAssets()
            : portTextFont("Tahoma", g_defaultFontSize)
            , groupHeader(QColor(g_colorValueMax, g_colorValueMax, g_colorValueMax))
            //brushBackground( QColor(255,214,0) ),
            //brushBackgroundSelected( QColor(255,247,204) ),
        {
            stringFormatCentered = Qt::AlignHCenter;
            stringFormatOutputText = Qt::AlignRight;
            stringFormatInputText = Qt::AlignLeft;
        }
        ~SGroupAssets()
        {
        }

        /************************************************************************/
        /* Settings for various elements of the group                           */
        /************************************************************************/

        // Brush used for group header
        QBrush groupHeader;
        // Font for Other (ports) group text
        QFont portTextFont;

        // String formats (Used for alignment)
        int stringFormatCentered;
        int stringFormatOutputText;
        int stringFormatInputText;
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

QHyperNodePainter_BlackBox::QHyperNodePainter_BlackBox()
    : m_groupFillBrush(QBrush(QColor(g_colorValueMax)))
    , m_penBorder(QPen(QColor(g_colorValueMax)))
{
}

void QHyperNodePainter_BlackBox::Paint(CHyperNode* pNode, QDisplayList* pList)
{
    static SGroupAssets* pAssets = 0;
    if (!pAssets)
    {
        pAssets = new SGroupAssets();
    }

    CBlackBoxNode* pBB = (CBlackBoxNode*)pNode;
    bool collapsed = pBB->IsMinimized();
    QRectF bbRect = pNode->GetRect();
    float minX, minY;
    minX = bbRect.x();
    minY = bbRect.y();
    float maxX, maxY;
    maxX = minX;
    maxY = minY;

    // Getting Node inputs and applying them

    // Vector that holds the raw color values as obtained from the UI
    Vec3 groupColorRaw(g_colorValueMax, g_colorValueMax, g_colorValueMax);

    // Boolean that indicates if the Background rect is drawn filled or empty
    bool drawBackgroundRectFilled = true;

    // Size of the title font
    int fontSize = 0;
    const CHyperNode::Ports& inputs = *pNode->GetInputs();

    // Fetch inputs data from ports
    for (int i = 0; i < inputs.size(); ++i)
    {
        const CHyperNodePort& port = inputs[i];
        if (QString::compare(port.pVar->GetName(), "Color") == 0)
        {
            port.pVar->Get(groupColorRaw);
            // Scale color value
            groupColorRaw *= g_colorValueMax;
        }
        else if (QString::compare(port.pVar->GetName(), "DisplayFilled") == 0)
        {
            port.pVar->Get(drawBackgroundRectFilled);
        }
        else if (QString::compare(port.pVar->GetName(), "TextSize") == 0)
        {
            port.pVar->Get(fontSize);
        }
    }
    // Highlight the node if it is selected then accent the color so as to highlight it
    if (pNode->IsSelected())
    {
        groupColorRaw *= g_nodeHighlightFactor;
    }

    // Clamp color value
    groupColorRaw.x = CLAMP(groupColorRaw.x, g_colorValueMin, g_colorValueMax);
    groupColorRaw.y = CLAMP(groupColorRaw.y, g_colorValueMin, g_colorValueMax);
    groupColorRaw.z = CLAMP(groupColorRaw.z, g_colorValueMin, g_colorValueMax);

    // Create a color depending on the input
    // Color for the rectangles
    QColor groupRectColor(groupColorRaw.x, groupColorRaw.y, groupColorRaw.z, g_transparentAlpha);

    // Updating color for the background color of the rectangle
    m_groupFillBrush.setColor(groupRectColor);

    // Indicates the color for the border of the group
    QColor groupBorderColor(groupColorRaw.x, groupColorRaw.y, groupColorRaw.z);

    // Updating color of the borders for the rectangle
    m_penBorder.setColor(groupBorderColor);

    // Setting the width of the border on the Group Rectangle
    m_penBorder.setWidth(g_borderThickness);

    QDisplayRectangle* groupBackgroundRect = nullptr;

    // If the Group is not in the collapsed state
    if (!collapsed)
    {
        // Then we should add the background rect to the display list
        groupBackgroundRect = pList->Add<QDisplayRectangle>()
                ->SetHitEvent(eSOID_ShiftFirstOutputPort)
                ->SetStroked(m_penBorder);

        if (drawBackgroundRectFilled)
        {
            // Fill it with the transparent background color
            groupBackgroundRect->SetFilled(m_groupFillBrush);
        }
    }

    QString text = pNode->GetName();
    text.replace("\\n", "\r\n");

    // choose the font size depending on the size specified for this commentBox, and the current zoom
    int normalSize;
    switch (fontSize)
    {
    case CBlackBoxNode::FontSize_Small:
    {
        normalSize = g_fontSizeSmall;
        break;
    }
    case CBlackBoxNode::FontSize_Medium:
    {
        normalSize = g_fontSizeMedium;
        break;
    }
    case CBlackBoxNode::FontSize_Large:
    {
        normalSize = g_fontSizeLarge;
        break;
    }
    default:
    {
        normalSize = g_fontSizeMedium;
        break;
    }
    }

    // Adding the display string
    QDisplayString* pString = pList->Add<QDisplayString>()
            ->SetHitEvent(eSOID_ShiftFirstOutputPort)
            ->SetText(text)
            ->SetBrush(pAssets->groupHeader)
            ->SetFont(GetFont(normalSize))
            ->SetStringFormat(pAssets->stringFormatCentered);

    //******************************************************

    std::vector<SDefaultRenderPort> inputPorts;
    std::vector<SDefaultRenderPort> outputPorts;
    std::vector<CHyperNodePort*> inputPortsPtr;
    std::vector<CHyperNodePort*> outputPortsPtr;

    inputPorts.reserve(10);
    outputPorts.reserve(10);

    CHyperGraph* pGraph = (CHyperGraph*)pNode->GetGraph();

    QRectF rect = pList->GetBounds();

    float width = rect.width() + 32;
    float curY = 30;
    float height = curY;
    float outputsWidth = 0.0f;
    float inputsWidth = 0.0f;

    std::vector<CHyperNode*>* pNodes = pBB->GetNodesSafe();

    for (int i = 0; i < pNodes->size(); ++i)
    {
        CHyperNode* pNextNode = (*pNodes)[i];

        if (!collapsed)
        {
            QRectF nodeRect = pNextNode->GetRect();
            if (nodeRect.x() < minX)
            {
                minX = nodeRect.x();
            }
            if (nodeRect.y() < minY)
            {
                minY = nodeRect.y();
            }
            if (nodeRect.x() + nodeRect.width() > maxX)
            {
                maxX = nodeRect.x() + nodeRect.width();
            }
            if (nodeRect.y() + nodeRect.height() > maxY)
            {
                maxY = nodeRect.y() + nodeRect.height();
            }
        }

        // Only show output ports when the group is collapsed
        if (collapsed)
        {
            const CHyperNode::Ports& outputs = *pNextNode->GetOutputs();
            // output ports
            for (size_t i = 0; i < outputs.size(); i++)
            {
                const CHyperNodePort& pp = outputs[i];
                if (pp.nConnected != 0)
                {
                    std::vector<CHyperEdge*> edges;
                    if (pp.nConnected > 1)
                    {
                        pGraph->FindEdges(pNextNode, edges);
                    }
                    else
                    {
                        edges.push_back(pGraph->FindEdge(pNextNode, (CHyperNodePort*)(&pp)));
                    }

                    for (int i = 0; i < edges.size(); ++i)
                    {
                        const CHyperEdge* pEdge = edges[i];
                        if ((pEdge && !pBB->IncludesNode(pEdge->nodeIn)) || !pEdge)
                        {
                            QString fullyQualifiedPortName = pNextNode->GetTitle() + QStringLiteral(":") + pNextNode->GetPortName(pp);

                            bool doesPortAlreadyExist = false;
                            for (auto currentPort : outputPorts)
                            {
                                if (fullyQualifiedPortName.compare(currentPort.pText->getText()) == 0)
                                {
                                    doesPortAlreadyExist = true;
                                }
                            }

                            if (!doesPortAlreadyExist)
                            {
                                SDefaultRenderPort pr;

                                QBrush pBrush = pAssets->groupHeader;
                                pr.pPortArrow = pList->Add<QDisplayPortArrow>()
                                        ->SetFilled(pBrush)
                                        ->SetHitEvent(eSOID_FirstOutputPort + i);
                                //->SetStroked( &pAssets->penPort );

                                pr.pText = pList->Add<QDisplayString>()
                                        ->SetFont(pAssets->portTextFont)
                                        ->SetStringFormat(pAssets->stringFormatOutputText)
                                        ->SetBrush(pAssets->groupHeader)
                                        ->SetHitEvent(eSOID_FirstOutputPort + i)
                                        ->SetText(fullyQualifiedPortName);
                                pr.id = 1000 + i;
                                outputPorts.push_back(pr);
                                outputPortsPtr.push_back((CHyperNodePort*)(&pp));
                            }
                        }
                    }
                }
            }
        }

        // Only show input ports when the group is collapsed
        if (collapsed)
        {
            const CHyperNode::Ports& inputs = *pNextNode->GetInputs();
            // input ports
            for (size_t i = 0; i < inputs.size(); i++)
            {
                const CHyperNodePort& pp = inputs[i];
                if (pp.nConnected != 0)
                {
                    std::vector<CHyperEdge*> edges;
                    if (pp.nConnected > 1)
                    {
                        pGraph->FindEdges(pNextNode, edges);
                    }
                    else
                    {
                        edges.push_back(pGraph->FindEdge(pNextNode, (CHyperNodePort*)(&pp)));
                    }

                    for (int i = 0; i < edges.size(); ++i)
                    {
                        const CHyperEdge* pEdge = edges[i];
                        if ((pEdge && !pBB->IncludesNode(pEdge->nodeOut)) || !pEdge)
                        {
                            SDefaultRenderPort pr;

                            QBrush pBrush = pAssets->groupHeader;
                            pr.pPortArrow = pList->Add<QDisplayPortArrow>()
                                    ->SetFilled(pBrush)
                                    ->SetHitEvent(eSOID_FirstInputPort + i);

                            QString fullyQualifiedPortName = pNextNode->GetTitle() + QStringLiteral(":") + pNextNode->GetPortName(pp);

                            pr.pText = pList->Add<QDisplayString>()
                                    ->SetFont(pAssets->portTextFont)
                                    ->SetStringFormat(pAssets->stringFormatInputText)
                                    ->SetBrush(pAssets->groupHeader)
                                    ->SetHitEvent(eSOID_FirstInputPort + i)
                                    ->SetText(fullyQualifiedPortName);
                            pr.id = 1000 + i;

                            inputPorts.push_back(pr);
                            inputPortsPtr.push_back((CHyperNodePort*)(&pp));
                        }
                    }
                }
            }
        }

        //**********************************************************

        for (size_t i = 0; i < outputPorts.size(); i++)
        {
            const SDefaultRenderPort& p = outputPorts[i];
            outputsWidth = std::max<float>(outputsWidth, p.pText->GetBounds().width());
        }
        width = max(width, outputsWidth + 3 * g_portsOuterMargin);
        for (size_t i = 0; i < inputPorts.size(); i++)
        {
            const SDefaultRenderPort& p = inputPorts[i];
            inputsWidth = std::max<float>(inputsWidth, p.pText->GetBounds().width());
        }
        width = max(width, inputsWidth + 3 * g_portsOuterMargin);

        // Only draw ports if the group is collapsed
        if (collapsed)
        {
            //drawing ports
            for (size_t i = 0; i < inputPorts.size(); i++)
            {
                const SDefaultRenderPort& p = inputPorts[i];
                QPointF textLoc(g_portsOuterMargin + 6, curY);
                p.pText->SetLocation(textLoc);
                rect = p.pText->GetBounds();
                if (p.pBackground)
                {
                    p.pBackground->SetRect(rect.width() + 3, curY, g_portsOuterMargin + rect.width() + 3, rect.height());
                }
                curY += rect.height();
                rect.setHeight(rect.height() - 4.0f);
                rect.setWidth(rect.height());
                rect.moveTop(rect.top() + 2.0f);
                rect.moveLeft(0);

                pBB->SetPort(inputPortsPtr[i], QPointF(rect.x(), rect.y() + rect.height() * 0.5f));

                if (p.pPortArrow)
                {
                    p.pPortArrow->SetRect(rect);
                }
                if (p.pRectangle)
                {
                    p.pRectangle->SetRect(rect);
                }
                pList->SetAttachRect(p.id, QRectF(width, rect.y() + rect.height() * 0.5f, 0.0f, 0.0f));
            }
            height = max(height, curY);

            curY = 30;

            for (size_t i = 0; i < outputPorts.size(); i++)
            {
                const SDefaultRenderPort& p = outputPorts[i];
                QPointF textLoc(width * 2.0f - g_portsOuterMargin, curY);
                p.pText->SetLocation(textLoc);
                rect = p.pText->GetBounds();
                if (p.pBackground)
                {
                    p.pBackground->SetRect(width - g_portsOuterMargin - rect.width() - 3, curY, g_portsOuterMargin + rect.width() + 3, rect.height());
                }
                curY += rect.height();
                rect.setHeight(rect.height() - 4.0f);
                rect.setWidth(rect.height());
                rect.moveTop(rect.top() + 2.0f);
                rect.moveLeft(width * 2.0f - g_portsOuterMargin);

                pBB->SetPort(outputPortsPtr[i], QPointF(rect.x(), rect.y() + rect.height() * 0.5f));

                if (p.pPortArrow)
                {
                    p.pPortArrow->SetRect(rect);
                }
                if (p.pRectangle)
                {
                    p.pRectangle->SetRect(rect);
                }
                pList->SetAttachRect(p.id, QRectF(width, rect.y() + rect.height() * 0.5f, 0.0f, 0.0f));
            }
            height = max(height, curY);
        }
    }

    //****************************************************************

    rect.setWidth(width * 2.0f + 5);
    rect.setHeight(height + 12.0f);
    rect.moveTo(0.f, 0.f);

    // Small Rect that is used for the collapsed view
    QDisplayRectangle* groupTitleRect = pList->Add<QDisplayRectangle>()
            ->SetHitEvent(eSOID_Title)
            ->SetStroked(m_penBorder)
            ->SetRect(rect);

    // set the visibility of the small rect
    groupTitleRect->SetVisible(collapsed);

    // fill it with the transparent background color
    groupTitleRect->SetFilled(m_groupFillBrush);


    // Add Triangle used as minimize arrow to display list
    QDisplayMinimizeArrow* pMinimizeArrow = pList->Add<QDisplayMinimizeArrow>()
            ->SetStroked(m_penBorder)
            ->SetFilled(pAssets->groupHeader)
            ->SetHitEvent(eSOID_Minimize);

    // Set the rotation of the Minimize arrow
    pMinimizeArrow->SetOrientation(!collapsed);

    pMinimizeArrow->SetRect(g_minimizeBoxPositionX, g_minimizeBoxPositionY,
        g_minimizeBoxDimension, g_minimizeBoxDimension);
    //output text
    pString->SetLocation(QPointF(rect.width() * 0.5f, 0.0f));

    // If the Black Box rect is not collapsed, then the rect needs to be rendered
    // this piece of code figures out the size of it
    if (!collapsed)
    {
        rect.moveLeft(minX - bbRect.x());
        rect.moveTop(minY - bbRect.y());
        if (maxX < pNode->GetPos().x() + rect.width())
        {
            maxX = pNode->GetPos().x() + rect.width();
        }
        if (maxY < pNode->GetPos().y() + rect.height())
        {
            maxY = pNode->GetPos().y() + rect.height();
        }
        rect.setWidth(fabsf(minX - maxX));
        rect.setHeight(fabsf(minY - maxY));
        rect.translate(-10.0f, -10.0f);
        rect.setWidth(rect.width() + 20.0f);
        rect.setHeight(rect.height() + 20.0f);
        groupBackgroundRect->SetRect(rect);
    }
}
