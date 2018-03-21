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
#include "AIDialog.h"

#include "AI/AIManager.h"
#include "AI/AiGoalLibrary.h"
#include "AI/AiBehaviorLibrary.h"

#include <QStringListModel>

#include <ui_AIDialog.h>

class AIBehaviorsModel
    : public QAbstractListModel
{
public:
    AIBehaviorsModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    virtual ~AIBehaviorsModel() { }

    int rowCount(const QModelIndex& parent = {}) const override
    {
        return parent.isValid() ? 0 : m_behaviors.size();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() >= m_behaviors.size())
        {
            return {};
        }

        auto behavior = m_behaviors[index.row()];

        switch (role)
        {
        case Qt::DisplayRole:
            return behavior->GetName();

        case Qt::UserRole:
            return qVariantFromValue<CAIBehavior*>(behavior);
        }

        return {};
    }

    void Reload()
    {
        CAIBehaviorLibrary* lib = GetIEditor()->GetAI()->GetBehaviorLibrary();

        beginResetModel();
        lib->GetBehaviors(m_behaviors);
        endResetModel();
    }

private:
    std::vector<CAIBehaviorPtr> m_behaviors;
};

Q_DECLARE_METATYPE(CAIBehavior*);

// CAIDialog dialog

CAIDialog::CAIDialog(QWidget* pParent /*=NULL*/)
    : QDialog(pParent)
    , m_behaviorsModel(new AIBehaviorsModel(this))
    , ui(new Ui::CAIDialog)
{
    //m_propWnd = 0;

    ui->setupUi(this);
    ui->m_behaviors->setModel(m_behaviorsModel);

    OnInitDialog();

    connect(ui->m_editButton, &QPushButton::clicked, this, &CAIDialog::OnBnClickedEdit);
    connect(ui->m_refreshButton, &QPushButton::clicked, this, &CAIDialog::OnBnClickedReload);
    connect(ui->m_behaviors, &QListView::doubleClicked, this, &CAIDialog::OnLbnDblClk);
    connect(ui->m_behaviors->selectionModel(), &QItemSelectionModel::selectionChanged, this, &CAIDialog::OnLbnSelchange);
    connect(ui->m_buttonBox, &QDialogButtonBox::accepted, this, &CAIDialog::accept);
    connect(ui->m_buttonBox, &QDialogButtonBox::rejected, this, &CAIDialog::reject);
}

CAIDialog::~CAIDialog()
{
}
// CAIDialog message handlers

void CAIDialog::OnLbnDblClk()
{
    if (ui->m_behaviors->selectionModel()->hasSelection())
    {
        accept();
    }
}

void CAIDialog::OnInitDialog()
{
    ui->m_newButton->setEnabled(false);
    ui->m_deleteButton->setEnabled(false);
    ui->m_listCaption->setText(tr("Choose AI behavior:"));
    ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    ReloadBehaviors();
    /*
    XmlNodeRef root("Root");
    CAIGoalLibrary *lib = GetIEditor()->GetAI()->GetGoalLibrary();
    std::vector<CAIGoalPtr> goals;
    lib->GetGoals( goals );
    for (int i = 0; i < goals.size(); i++)
    {
        CAIGoal *goal = goals[i];
        XmlNodeRef goalNode = root->newChild(goal->GetName());
        goalNode->setAttr("type","List");

        // Copy Goal params.
        if (goal->GetParamsTemplate())
        {
            XmlNodeRef goalParams = goal->GetParamsTemplate()->clone();
            for (int k = 0; k < goalParams->getChildCount(); k++)
            {
                goalNode->addChild(goalParams->getChild(k));
            }
        }
        for (int j = 0; j < goal->GetStageCount(); j++)
        {
            CAIGoalStage &stage = goal->GetStage(j);
            CAIGoal *stageGoal = lib->FindGoal(stage.name);
            if (stageGoal)
            {
                XmlNodeRef stageNode = goalNode->newChild(stage.name);
                stageNode->setAttr("type","List");
                //if (stage.params)
                {
                    XmlNodeRef templ = stageGoal->GetParamsTemplate()->clone();
                    //CXmlTemplate::GetValues( templ,stage.params );
                    for (int k = 0; k < templ->getChildCount(); k++)
                    {
                        stageNode->addChild(templ->getChild(k));
                    }
                }
            }
        }
    }

    root = XmlHelpers::LoadXmlFromFile( "C:\\MasterCD\\ai.xml" );

    CRect rc( 10,10,300,500 );
    m_propWnd = new CPropertiesWindow( root,"Props","Props" );
    m_propWnd->Resizable = false;
    m_propWnd->OpenChildWindow( GetSafeHwnd(),rc,true );
    m_propWnd->ForceRefresh();
    m_propWnd->Show(1);
	*/

	// EXCEPTION: OCX Property Pages should return FALSE
}

//////////////////////////////////////////////////////////////////////////
void CAIDialog::ReloadBehaviors()
{
    m_behaviorsModel->Reload();
	}

void CAIDialog::SetAIBehavior(const QString& behavior)
	{
    m_aiBehavior = behavior;

    auto index = m_behaviorsModel->match(m_behaviorsModel->index(0, 0), Qt::DisplayRole, m_aiBehavior, 1, Qt::MatchFixedString);
    if (!index.isEmpty())
    {
        ui->m_behaviors->setCurrentIndex(index.first());
    }
}

void CAIDialog::OnBnClickedEdit()
{
    CAIBehavior* behavior = GetIEditor()->GetAI()->GetBehaviorLibrary()->FindBehavior(m_aiBehavior);
    if (!behavior)
    {
        return;
    }

    behavior->Edit();
}

void CAIDialog::OnBnClickedReload()
{
    CAIBehavior* behavior = GetIEditor()->GetAI()->GetBehaviorLibrary()->FindBehavior(m_aiBehavior);
    if (!behavior)
    {
        return;
    }
    behavior->ReloadScript();
}

void CAIDialog::OnLbnSelchange()
{
    auto index = ui->m_behaviors->currentIndex();
    if (!index.isValid())
    {
        return;
    }

    ui->m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);

    m_aiBehavior = index.data().toString();

    CAIBehavior* behavior = index.data(Qt::UserRole).value<CAIBehavior*>();
    if (behavior)
    {
        ui->m_description->setText(behavior->GetDescription());
    }
    else
    {
        ui->m_description->setText(QString());
    }
}

#include <AIDialog.moc>
