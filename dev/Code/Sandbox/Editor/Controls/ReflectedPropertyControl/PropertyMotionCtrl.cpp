#include "StdAfx.h"

#include "PropertyMotionCtrl.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>
#include <LmbrCentral/Animation/CharacterAnimationBus.h>
//#include <AzToolsFramework/UI/PropertyEditor/PropertyAssetCtrl.hxx>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include "CharacterEditor/AnimationBrowser.h"
#include "Controls/MultiMonHelper.h"
#include "Util/UIEnumerations.h"
#include "IResourceSelectorHost.h"
#include "MathConversion.h"


MotionPropertyCtrl::MotionPropertyCtrl(QWidget *pParent)
    : QWidget(pParent)
{
    m_motionLabel = new QLabel;

    m_pBrowseButton = new QToolButton;
    m_pBrowseButton->setIcon(QIcon(":/reflectedPropertyCtrl/img/file_browse.png"));
    m_pApplyButton = new QToolButton;
    m_pApplyButton->setIcon(QIcon(":/reflectedPropertyCtrl/img/apply.png"));

    m_pApplyButton->setFocusPolicy(Qt::StrongFocus);
    m_pBrowseButton->setFocusPolicy(Qt::StrongFocus);

    QHBoxLayout *pLayout = new QHBoxLayout(this);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->addWidget(m_motionLabel, 1);
    pLayout->addWidget(m_pBrowseButton);
    pLayout->addWidget(m_pApplyButton);

    connect(m_pBrowseButton, &QAbstractButton::clicked, this, &MotionPropertyCtrl::OnBrowseClicked);
    connect(m_pApplyButton, &QAbstractButton::clicked, this, &MotionPropertyCtrl::OnApplyClicked);
};

MotionPropertyCtrl::~MotionPropertyCtrl()
{
}


void MotionPropertyCtrl::SetValue(const CReflectedVarMotion &motion)
{
    m_motion = motion;
    m_motionLabel->setText(motion.m_motion.c_str());
}

CReflectedVarMotion MotionPropertyCtrl::value() const
{
    return m_motion;
}

void MotionPropertyCtrl::OnBrowseClicked()
{

    static AZ::Data::AssetType emotionFXMotionAssetType("{00494B8E-7578-4BA2-8B28-272E90680787}"); // from MotionAsset.h in EMotionFX Gem

    // Request the AssetBrowser Dialog and set a type filter
    AssetSelectionModel selection = AssetSelectionModel::AssetTypeSelection(emotionFXMotionAssetType);
    selection.SetSelectedAssetId(m_motion.m_assetId);
    AzToolsFramework::EditorRequests::Bus::Broadcast(&AzToolsFramework::EditorRequests::BrowseForAssets, selection);
    if (selection.IsValid())
    {
        auto product = azrtti_cast<const ProductAssetBrowserEntry*>(selection.GetResult());
        if (product != nullptr)
        {
            m_motion.m_motion = product->GetRelativePath();
            m_motion.m_assetId = product->GetAssetId();
            m_motionLabel->setText(m_motion.m_motion.c_str());
            emit ValueChanged(m_motion);
        }
    }
}

// TODO: Might be able to delete this function
void MotionPropertyCtrl::OnApplyClicked()
{
#if 0
    CUIEnumerations &roGeneralProxy = CUIEnumerations::GetUIEnumerationsInstance();
    QStringList cSelectedMotions;
    size_t nTotalMotions(0);
    size_t nCurrentMotion(0);

    QString combinedString = GetIEditor()->GetResourceSelectorHost()->GetGlobalSelection("motion");
    SplitString(combinedString, cSelectedMotions, ',');

    nTotalMotions = cSelectedMotions.size();
    for (nCurrentMotion = 0; nCurrentMotion < nTotalMotions; ++nCurrentMotion)
    {
        QString& rstrCurrentAnimAction = cSelectedMotions[nCurrentMotion];
        if (!rstrCurrentAnimAction.isEmpty())
        {
            m_motion.m_motion = rstrCurrentAnimAction.toLatin1().data();
            m_motionLabel->setText(m_motion.m_motion.c_str());
            emit ValueChanged(m_motion);
        }
    }
#endif
}

QWidget* MotionPropertyCtrl::GetFirstInTabOrder()
{
    return m_pBrowseButton;
}
QWidget* MotionPropertyCtrl::GetLastInTabOrder()
{
    return m_pApplyButton;
}

void MotionPropertyCtrl::UpdateTabOrder()
{
    setTabOrder(m_pBrowseButton, m_pApplyButton);
}


QWidget* MotionPropertyWidgetHandler::CreateGUI(QWidget *pParent)
{
    MotionPropertyCtrl* newCtrl = aznew MotionPropertyCtrl(pParent);
    connect(newCtrl, &MotionPropertyCtrl::ValueChanged, newCtrl, [newCtrl]()
    {
        EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
    });
    return newCtrl;
}


void MotionPropertyWidgetHandler::ConsumeAttribute(MotionPropertyCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    Q_UNUSED(GUI);
    Q_UNUSED(attrib);
    Q_UNUSED(attrValue);
    Q_UNUSED(debugName);
}

void MotionPropertyWidgetHandler::WriteGUIValuesIntoProperty(size_t index, MotionPropertyCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    Q_UNUSED(index);
    Q_UNUSED(node);
    CReflectedVarMotion val = GUI->value();
    instance = static_cast<property_t>(val);
}

bool MotionPropertyWidgetHandler::ReadValuesIntoGUI(size_t index, MotionPropertyCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    Q_UNUSED(index);
    Q_UNUSED(node);
    CReflectedVarMotion val = instance;
    GUI->SetValue(val);
    return false;
}


#include <Controls/ReflectedPropertyControl/PropertyMotionCtrl.moc>

