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

// include the required headers
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include "NavigationLinkWidget.h"
#include "NavigateWidget.h"
#include <QLabel>
#include <QPushButton>
#include <QFocusEvent>
#include <QLineEdit>
#include <QMenu>


namespace EMStudio
{
    // the constructor
    NavigationLinkWidget::NavigationLinkWidget(AnimGraphPlugin* plugin, QWidget* parent)
        : QWidget(parent)
    {
        mPlugin         = plugin;
        mNode           = nullptr;
        mAnimGraph     = nullptr;
        mNavigationLink = nullptr;
        mInnerWidget    = nullptr;
        mLinksLayout    = nullptr;

        setObjectName("TransparentWidget");

        QHBoxLayout* mainLayout = new QHBoxLayout();
        mainLayout->setMargin(0);
        mainLayout->setSpacing(0);
        mainLayout->setSizeConstraint(QLayout::SetNoConstraint);
        setLayout(mainLayout);

        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
        //setMaximumWidth(300);
        setMaximumHeight(28);
        setFocusPolicy(Qt::ClickFocus);

        // needed in order to trigger the basic update at the end of the constructor
        mLastSearchMode = true;
        Update(nullptr, nullptr);
    }


    // destructor
    NavigationLinkWidget::~NavigationLinkWidget()
    {
    }


    QWidget* NavigationLinkWidget::AddNodeToHierarchyNavigationLink(EMotionFX::AnimGraphNode* node, QHBoxLayout* hLayout)
    {
        QWidget* link = nullptr;

        // get the parent node
        EMotionFX::AnimGraphNode* parentNode = nullptr;
        if (node)
        {
            parentNode = node->GetParentNode();
        }

        if (parentNode)
        {
            AddNodeToHierarchyNavigationLink(parentNode, hLayout);
        }

        if (node)
        {
            // add the node itself to the navigation link
            QString str;
            link = new QLabel(str.sprintf("<a href='#'>%s</a>", node->GetName()), this);
            link->setProperty("node", node->GetName());
            link->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);
            connect(link, SIGNAL(linkActivated(const QString&)), this, SLOT(OnHierarchyNavigationLinkClicked(const QString&)));
            hLayout->addWidget(link);

            // add the button with which we can access children
            QSize imgSize(16, 16);
            QSize spacerSize(24, 24);
            QLabel* spacer = new QLabel("", this);
            spacer->setMaximumSize(spacerSize);
            spacer->setMinimumSize(spacerSize);
            spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
            QPixmap img = MysticQt::GetMysticQt()->FindIcon("Images/AnimGraphPlugin/NavPath.png").pixmap(imgSize);
            spacer->setPixmap(img);
            hLayout->addWidget(spacer);
        }

        return link;
    }


    void NavigationLinkWidget::OnHierarchyNavigationLinkClicked(const QString& text)
    {
        QLabel* link = qobject_cast<QLabel*>(sender());
        QVariant linkVariant = link->property("node");
        if (linkVariant.isNull())
        {
            return;
        }

        QString linkNodeText = linkVariant.toString();
        mPlugin->GetNavigateWidget()->ShowGraphByNodeName(linkNodeText.toUtf8().data(), true);
    }


    void NavigationLinkWidget::OnShowNode()
    {
        QAction* action = qobject_cast<QAction*>(sender());
        mPlugin->GetNavigateWidget()->ShowGraphByNodeName(action->whatsThis().toUtf8().data(), true);
    }


    void NavigationLinkWidget::Update(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node)
    {
        // change mode before updating
        if (mLastSearchMode)
        {
            OnModeChanged(false);
        }

        // we update to the exact same, skip that!
        if (mNode == node && mAnimGraph == animGraph)
        {
            return;
        }

        // update our current node
        mNode           = node;
        mAnimGraph     = animGraph;

        if (mNavigationLink)
        {
            mNavigationLink->hide();
            mNavigationLink->deleteLater();
            mNavigationLink = nullptr;
        }

        if (mAnimGraph)
        {
            mNavigationLink = new QWidget();
            mNavigationLink->setObjectName("AnimGraphNavigationLinkInnerWidget");
            mNavigationLink->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

            QHBoxLayout* layout = new QHBoxLayout();
            layout->setMargin(0);
            layout->setSpacing(2);
            layout->setDirection(QBoxLayout::LeftToRight);
            mNavigationLink->setLayout(layout);

            QWidget* lastLink = AddNodeToHierarchyNavigationLink(node, layout);
            if (lastLink)
            {
                lastLink->setEnabled(false);
            }

            mNavigationLink->setLayout(layout);
            mLinksLayout->addWidget(mNavigationLink);
        }
    }


    void NavigationLinkWidget::OnModeChanged(bool searchMode)
    {
        // we update to the exact same, skip that!
        if (mLastSearchMode == searchMode)
        {
            return;
        }

        if (mInnerWidget)
        {
            mInnerWidget->hide();
            mInnerWidget->deleteLater();
        }

        mInnerWidget = new QWidget();
        mInnerWidget->setObjectName("AnimGraphNavigationLinkWidget");
        layout()->addWidget(mInnerWidget);

        QHBoxLayout* hLayout = new QHBoxLayout();
        hLayout->setMargin(0);
        hLayout->setSpacing(2);
        hLayout->setDirection(QBoxLayout::LeftToRight);

        if (searchMode == false)
        {
            // add a dummy spacer widget
            QWidget* dummyWidget = new QWidget();
            dummyWidget->setObjectName("TransparentWidget");
            dummyWidget->setMinimumWidth(3);
            dummyWidget->setMaximumWidth(3);
            hLayout->addWidget(dummyWidget);

            // the navigation link widgets
            QWidget* navigationHelperWidget = new QWidget();
            navigationHelperWidget->setObjectName("AnimGraphNavigationLinkWidget");
            navigationHelperWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

            mLinksLayout = new QHBoxLayout();
            mLinksLayout->setMargin(0);
            mLinksLayout->setSpacing(2);
            mLinksLayout->setDirection(QBoxLayout::LeftToRight);
            navigationHelperWidget->setLayout(mLinksLayout);

            hLayout->addWidget(navigationHelperWidget);

            dummyWidget = new QWidget();
            dummyWidget->setObjectName("TransparentWidget");
            dummyWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
            //connect( dummyWidget, SIGNAL(clicked()), this, SLOT(StartSearchMode()) );
            hLayout->addWidget(dummyWidget);

        }
        else
        {
            m_searchWidget = new AzQtComponents::FilteredSearchWidget(mInnerWidget);
            m_searchWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            hLayout->addWidget(m_searchWidget);
        }

        mInnerWidget->setLayout(hLayout);
        mLastSearchMode = searchMode;
    }


    void NavigationLinkWidget::focusOutEvent(QFocusEvent* event)
    {
        MCORE_UNUSED(event);

        //if (event->reason() != Qt::ActiveWindowFocusReason)
        //  return;

        Update(mAnimGraph, mNode);
    }


    void NavigationLinkWidget::DropDownChildren()
    {
        if (mAnimGraph == nullptr)
        {
            return;
        }

        QPushButton* button = qobject_cast<QPushButton*>(sender());
        QPoint globalBottomLeft = button->mapToGlobal(QPoint(0, button->geometry().bottom()));

        EMotionFX::AnimGraphNode* node = mAnimGraph->RecursiveFindNodeByName(FromQtString(button->whatsThis()).c_str());

        QMenu menu(button);

        if (node)
        {
            // get the number of child nodes, iterate through them and add them to the pop up menu
            const uint32 numChildNodes = node->GetNumChildNodes();
            for (uint32 i = 0; i < numChildNodes; ++i)
            {
                EMotionFX::AnimGraphNode* childNode = node->GetChildNode(i);

                // only add nodes that have children, as we can't show a graph of a node which hasn't any child nodes
                if (childNode->GetNumChildNodes() == 0)
                {
                    continue;
                }

                QAction* action = menu.addAction(childNode->GetName());
                action->setWhatsThis(childNode->GetName());
                connect(action, SIGNAL(triggered()), this, SLOT(OnShowNode()));
            }
        }
        else // we have selected the root state machine
        {
            MCORE_ASSERT(false);

            // add them to the pop up menu
            EMotionFX::AnimGraphNode* stateMachineNode = mAnimGraph->GetRootStateMachine();

            // only add nodes that have children, as we can't show a graph of a node which hasn't any child nodes
            if (stateMachineNode->GetNumChildNodes() != 0)
            {
                QAction* action = menu.addAction(stateMachineNode->GetName());
                action->setWhatsThis(stateMachineNode->GetName());
                connect(action, SIGNAL(triggered()), this, SLOT(OnShowNode()));
            }
        }

        if (menu.isEmpty() == false)
        {
            menu.exec(globalBottomLeft);
        }
    }


    void NavigationLinkWidget::DropDownHistory()
    {
        if (mAnimGraph == nullptr)
        {
            return;
        }

        QPushButton* button = qobject_cast<QPushButton*>(sender());

        QPoint globalBottomLeft = mapToGlobal(QPoint(0, button->geometry().bottom()));
        //QPoint globalBottomRight = button->mapToGlobal( QPoint(button->geometry().right(), 0) );

        NavigationLinkDropdownHistory* dialog = new NavigationLinkDropdownHistory(this, mPlugin);

        dialog->move(globalBottomLeft);
        dialog->resize(width(), 150);

        dialog->show();
    }


    // constructor
    NavigationLinkDropdownHistory::NavigationLinkDropdownHistory(NavigationLinkWidget* parentWidget, AnimGraphPlugin* plugin)
        : QDialog(parentWidget, Qt::Popup)
    {
        mPlugin = plugin;

        // create our list widget
        QListWidget* historyWidget = new QListWidget();
        historyWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
        historyWidget->setAlternatingRowColors(true);

        // put the widget in a layout
        QHBoxLayout* layout = new QHBoxLayout();
        layout->setMargin(0);
        layout->addWidget(historyWidget);
        setLayout(layout);

        // get the total number of history items and set the max number shown in the history
        const int32     numHistoryItems     = plugin->GetNumHistoryItems();
        const uint32    maxNumHistoryItems  = 100;

        // create the array where we store the names of the last visited nodes and reserve space for the strings
        MCore::Array<AZStd::string> historyNodeNames;
        historyNodeNames.Reserve(maxNumHistoryItems);

        // fill the node names array
        AZStd::string nodeName;
        for (int32 i = numHistoryItems - 1; i > 0; i--)
        {
            // break the loop if we have a given amount of items already
            if (historyNodeNames.GetLength() >= maxNumHistoryItems)
            {
                break;
            }

            EMotionFX::AnimGraphNode* animGraphNode = plugin->GetHistoryItem(i).FindNode();
            if (animGraphNode == nullptr)
            {
                // deal with the root node
                if (historyNodeNames.Find("") == MCORE_INVALIDINDEX32)
                {
                    historyNodeNames.Add("");
                }
            }
            else
            {
                // only add nodes that have children, as we can't show a graph of a node which hasn't any child nodes
                if (animGraphNode->GetNumChildNodes() == 0)
                {
                    continue;
                }

                // add the node name to the array
                if (historyNodeNames.Find(animGraphNode->GetName()) == MCORE_INVALIDINDEX32)
                {
                    historyNodeNames.Add(animGraphNode->GetName());
                }
            }
        }

        // fill the menu
        const uint32 numHistoryNodeNames = historyNodeNames.GetLength();
        for (uint32 i = 0; i < numHistoryNodeNames; ++i)
        {
            QListWidgetItem* item = new QListWidgetItem(historyWidget);
            if (historyNodeNames[i] == "")
            {
                item->setText("Root"); // root node handling
            }
            else
            {
                item->setText(historyNodeNames[i].c_str()); // normal node handling
            }
            item->setWhatsThis(historyNodeNames[i].c_str());

            // add the item to the list widget and connect it
            historyWidget->addItem(item);
            connect(historyWidget, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(OnShowNode(QListWidgetItem*)));
        }
    }


    // destructor
    NavigationLinkDropdownHistory::~NavigationLinkDropdownHistory()
    {
    }


    // when pressing a history item, show the node
    void NavigationLinkDropdownHistory::OnShowNode(QListWidgetItem* item)
    {
        mPlugin->GetNavigateWidget()->ShowGraphByNodeName(item->whatsThis().toUtf8().data(), true);
        accept();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigationLinkWidget.moc>
