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

#include "NodePaletteWidget.h"
#include <MCore/Source/LogManager.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include "AnimGraphPlugin.h"
#include <QVBoxLayout>
#include <QListWidget>
#include <QTabBar>
#include <QIcon>
#include <QAction>
#include <QMimeData>
#include <QLabel>
#include <QTextEdit>


namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(NodePaletteWidget::EventHandler, EMotionFX::EventHandlerAllocator, 0)

    // return the mime data
    QMimeData* NodePaletteList::mimeData(const QList<QListWidgetItem*> items) const
    {
        if (items.count() != 1)
        {
            return nullptr;
        }

        const QListWidgetItem* item = items.at(0);

        // create the data and set the text
        QMimeData* mimeData = new QMimeData();
        QString textData = "EMotionFX::AnimGraphNode;";
        textData += item->data(Qt::UserRole).toString().toUtf8().data();
        textData += ";" + item->text(); // add the palette name as generated name prefix (spaces will be removed from it
        mimeData->setText(textData);

        return mimeData;
    }


    // return the supported mime types
    QStringList NodePaletteList::mimeTypes() const
    {
        QStringList result;
        result.append("text/plain");
        return result;
    }


    // get the allowed drop actions
    Qt::DropActions NodePaletteList::supportedDropActions() const
    {
        return Qt::CopyAction;
    }


    NodePaletteWidget::EventHandler::EventHandler(NodePaletteWidget* widget)
    {
        mWidget = widget;
    }


    NodePaletteWidget::EventHandler::~EventHandler()
    {
    }


    NodePaletteWidget::EventHandler* NodePaletteWidget::EventHandler::Create(NodePaletteWidget* widget)
    {
        return aznew NodePaletteWidget::EventHandler(widget);
    }


    void NodePaletteWidget::EventHandler::OnCreatedNode(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node)
    {
        if (mWidget->mNode && node->GetParentNode() == mWidget->mNode)
        {
            mWidget->Init(animGraph, mWidget->mNode);
        }
    }


    void NodePaletteWidget::EventHandler::OnRemovedChildNode(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* parentNode)
    {
        if (mWidget->mNode && parentNode && parentNode == mWidget->mNode)
        {
            mWidget->Init(animGraph, mWidget->mNode);
        }
    }


    // constructor
    NodePaletteWidget::NodePaletteWidget(AnimGraphPlugin* plugin)
        : QWidget()
        , mPlugin(plugin)
    {
        mNode   = nullptr;
        mTabBar = nullptr;
        mList   = nullptr;

        // create the default layout
        mLayout = new QVBoxLayout();
        mLayout->setMargin(0);
        mLayout->setSpacing(0);

        // create the initial text
        mInitialText = new QLabel("<c>Create and activate a <b>Anim Graph</b> first.<br>Then <b>drag and drop</b> items from the<br>palette into the <b>Anim Graph window</b>.</c>");
        mInitialText->setAlignment(Qt::AlignCenter);
        mInitialText->setTextFormat(Qt::RichText);
        mInitialText->setMaximumSize(10000, 10000);
        mInitialText->setMargin(0);
        mInitialText->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        // add the initial text in the layout
        mLayout->addWidget(mInitialText);

        // create the tabbar
        mTabBar = new QTabBar();
        mTabBar->addTab("Sources");
        mTabBar->addTab("Blending");
        mTabBar->addTab("Controllers");
        mTabBar->addTab("Logic");
        mTabBar->addTab("Math");
        mTabBar->addTab("Misc");
        mTabBar->setVisible(false);
        connect(mTabBar, SIGNAL(currentChanged(int)), this, SLOT(OnChangeCategoryTab(int)));

        // add the tabbar in the layout
        mLayout->addWidget(mTabBar);

        // create the list
        mList = new NodePaletteList();
        mList->setViewMode(QListView::IconMode);
        mList->setUniformItemSizes(false);
        mList->setSelectionMode(QAbstractItemView::SingleSelection);
        mList->setMovement(QListView::Static);
        mList->setWrapping(true);
        mList->setTextElideMode(Qt::ElideRight);
        mList->setDragEnabled(true);
        mList->setWordWrap(true);
        mList->setFlow(QListView::LeftToRight);
        mList->setResizeMode(QListView::Adjust);
        mList->setDragDropMode(QAbstractItemView::DragOnly);
        mList->setIconSize(QSize(48, 48));
        mList->setVisible(false);

        // add the list in the layout
        mLayout->addWidget(mList);

        // set the default layout
        setLayout(mLayout);

        // register the event handler
        mEventHandler = EventHandler::Create(this);
        EMotionFX::GetEventManager().AddEventHandler(mEventHandler);
    }


    // destructor
    NodePaletteWidget::~NodePaletteWidget()
    {
        EMotionFX::GetEventManager().RemoveEventHandler(mEventHandler, true);
    }


    // init everything for editing a blend tree
    void NodePaletteWidget::Init(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node)
    {
        // set the node
        mNode = node;

        // check if the anim graph is not valid
        // on this case we show a message to say no one anim graph is activated
        if (animGraph == nullptr)
        {
            // set the layout params
            mLayout->setMargin(0);
            mLayout->setSpacing(0);

            // set the widget visible or not
            mInitialText->setVisible(true);
            mTabBar->setVisible(false);
            mList->setVisible(false);
        }
        else
        {
            // set the layout params
            mLayout->setMargin(2);
            mLayout->setSpacing(2);

            // set the widget visible or not
            mInitialText->setVisible(false);
            mTabBar->setVisible(true);
            mList->setVisible(true);

            // update the current tab
            OnChangeCategoryTab(mTabBar->currentIndex());
        }
    }


    AZStd::string NodePaletteWidget::GetNodeIconFileName(EMotionFX::AnimGraphNode* node)
    {
        AZStd::string filename      = AZStd::string::format("/Images/AnimGraphPlugin/%s.png", node->RTTI_GetTypeName());
        AZStd::string fullFilename  = AZStd::string::format("%s/Images/AnimGraphPlugin/%s.png", MysticQt::GetDataDir().c_str(), node->RTTI_GetTypeName());

        if (QFile::exists(fullFilename.c_str()) == false)
        {
            return "/Images/AnimGraphPlugin/UnknownNode.png";
        }

        return filename;
    }


    // register list widget icons
    void NodePaletteWidget::RegisterItems(EMotionFX::AnimGraphObject* object, EMotionFX::AnimGraphObject::ECategory category)
    {
        mList->clear();

        const AZStd::unordered_set<AZ::TypeId>& nodeObjectTypes = EMotionFX::AnimGraphObjectFactory::GetUITypes();
        for (const AZ::TypeId& nodeObjectType : nodeObjectTypes)
        {
            EMotionFX::AnimGraphObject* curObject = EMotionFX::AnimGraphObjectFactory::Create(nodeObjectType);

            if (mPlugin->CheckIfCanCreateObject(object, curObject, category))           
            {
                EMotionFX::AnimGraphNode* curNode = static_cast<EMotionFX::AnimGraphNode*>(curObject);
                QListWidgetItem* item = new QListWidgetItem(MysticQt::GetMysticQt()->FindIcon(GetNodeIconFileName(curNode).c_str()), curNode->GetPaletteName(), mList, NodePaletteList::NODETYPE_BLENDNODE);
                item->setToolTip(curNode->RTTI_GetTypeName());
                item->setData(Qt::UserRole, azrtti_typeid(curNode).ToString<AZStd::string>().c_str());
            }

            if (curObject)
            {
                delete curObject;
            }
        }
    }


    // a tab changed
    void NodePaletteWidget::OnChangeCategoryTab(int index)
    {
        switch (index)
        {
            case 0:
                RegisterItems(mNode, EMotionFX::AnimGraphNode::CATEGORY_SOURCES);
                return;
            case 1:
                RegisterItems(mNode, EMotionFX::AnimGraphNode::CATEGORY_BLENDING);
                return;
            case 2:
                RegisterItems(mNode, EMotionFX::AnimGraphNode::CATEGORY_CONTROLLERS);
                return;
            case 3:
                RegisterItems(mNode, EMotionFX::AnimGraphNode::CATEGORY_LOGIC);
                return;
            case 4:
                RegisterItems(mNode, EMotionFX::AnimGraphNode::CATEGORY_MATH);
                return;
            case 5:
                RegisterItems(mNode, EMotionFX::AnimGraphNode::CATEGORY_MISC);
                return;
            default:
                AZ_Assert(false, "Unsupported category tab.")
        };
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodePaletteWidget.moc>
