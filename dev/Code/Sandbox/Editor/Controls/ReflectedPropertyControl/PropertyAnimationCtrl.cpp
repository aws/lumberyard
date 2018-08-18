#include "StdAfx.h"

#include "PropertyAnimationCtrl.h"
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QToolButton>
#include <AzQtComponents/Utilities/QtWindowUtilities.h>
#include <LmbrCentral/Animation/CharacterAnimationBus.h>

#include "CharacterEditor/AnimationBrowser.h"
#include "Controls/MultiMonHelper.h"
#include "Util/UIEnumerations.h"
#include "IResourceSelectorHost.h"
#include "MathConversion.h"


AnimationPropertyCtrl::AnimationPropertyCtrl(QWidget *pParent)
    : QWidget(pParent),
    m_pAnimBrowser(nullptr)
{
    m_animationLabel = new QLabel;

    m_pBrowseButton = new QToolButton;
    m_pBrowseButton->setIcon(QIcon(":/reflectedPropertyCtrl/img/file_browse.png"));
    m_pApplyButton = new QToolButton;
    m_pApplyButton->setIcon(QIcon(":/reflectedPropertyCtrl/img/apply.png"));

    m_pApplyButton->setFocusPolicy(Qt::StrongFocus);
    m_pBrowseButton->setFocusPolicy(Qt::StrongFocus);

    QHBoxLayout *pLayout = new QHBoxLayout(this);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->addWidget(m_animationLabel, 1);
    pLayout->addWidget(m_pBrowseButton);
    pLayout->addWidget(m_pApplyButton);

    connect(m_pBrowseButton, &QAbstractButton::clicked, this, &AnimationPropertyCtrl::OnBrowseClicked);
    connect(m_pApplyButton, &QAbstractButton::clicked, this, &AnimationPropertyCtrl::OnApplyClicked);
};

AnimationPropertyCtrl::~AnimationPropertyCtrl()
{
}


void AnimationPropertyCtrl::SetValue(const CReflectedVarAnimation &animation)
{
    m_animation = animation;
    m_animationLabel->setText(animation.m_animation.c_str());
}

CReflectedVarAnimation AnimationPropertyCtrl::value() const
{
    return m_animation;
}

void AnimationPropertyCtrl::OnBrowseClicked()
{
    ICharacterInstance *pCharacterInstance = NULL;

    if (IsLegacyEntityId(m_animation.m_entityID))
    {
        EntityId entityId = GetLegacyEntityId(m_animation.m_entityID);
        IEntity *pEntity = gEnv->pEntitySystem ? gEnv->pEntitySystem->GetEntity(entityId) : nullptr;
        if (pEntity)
            pCharacterInstance = pEntity->GetCharacter(0);
    }
    else
    {
        LmbrCentral::CharacterAnimationRequestBus::EventResult(pCharacterInstance, m_animation.m_entityID, &LmbrCentral::CharacterAnimationRequestBus::Events::GetCharacterInstance);
    }
    
    // If a proper character instance exists in the entity specified in the variable,
    // load the animation list in the animation browser.
    if (pCharacterInstance)
    {
        if (m_pAnimBrowser == nullptr)
        {
            // want this thing to be floating, but want it attached to the parent, so that it always displays on top
            m_pAnimBrowser = new CAnimationBrowser(this, Qt::Window);
            m_pAnimBrowser->setAttribute(Qt::WA_DeleteOnClose);
            m_pAnimBrowser->AddOnDblClickCallback(functor(*this, &AnimationPropertyCtrl::OnApplyClicked));

            m_pAnimBrowser->show();
        }
        else
        {
            m_pAnimBrowser->raise();
        }

        m_pAnimBrowser->SetModel_SKEL(pCharacterInstance);

        // Position the animation browser right below the browse button.
        m_pAnimBrowser->window()->move(m_pBrowseButton->mapToGlobal(QPoint(0, m_pBrowseButton->height() + 1)));

        // Make sure it auto-resizes, which will also ensure it's on screen after moving it
        m_pAnimBrowser->window()->adjustSize();

        AzQtComponents::EnsureWindowWithinScreenGeometry(m_pAnimBrowser);
    }
    // If not, just open the whole character editor so that the user can select a character.
    else
    {
        GetIEditor()->ExecuteCommand("general.open_pane 'Character Editor'");
    }
}

void AnimationPropertyCtrl::OnApplyClicked()
{
    CUIEnumerations &roGeneralProxy = CUIEnumerations::GetUIEnumerationsInstance();
    QStringList cSelectedAnimations;
    size_t nTotalAnimations(0);
    size_t nCurrentAnimation(0);

    QString combinedString = GetIEditor()->GetResourceSelectorHost()->GetGlobalSelection("animation");
    SplitString(combinedString, cSelectedAnimations, ',');

    nTotalAnimations = cSelectedAnimations.size();
    for (nCurrentAnimation = 0; nCurrentAnimation < nTotalAnimations; ++nCurrentAnimation)
    {
        QString& rstrCurrentAnimAction = cSelectedAnimations[nCurrentAnimation];
        if (!rstrCurrentAnimAction.isEmpty())
        {
            m_animation.m_animation = rstrCurrentAnimAction.toUtf8().data();
            m_animationLabel->setText(m_animation.m_animation.c_str());
            emit ValueChanged(m_animation);
        }
    }
}

QWidget* AnimationPropertyCtrl::GetFirstInTabOrder()
{
    return m_pBrowseButton;
}
QWidget* AnimationPropertyCtrl::GetLastInTabOrder()
{
    return m_pApplyButton;
}

void AnimationPropertyCtrl::UpdateTabOrder()
{
    setTabOrder(m_pBrowseButton, m_pApplyButton);
}


QWidget* AnimationPropertyWidgetHandler::CreateGUI(QWidget *pParent)
{
    AnimationPropertyCtrl* newCtrl = aznew AnimationPropertyCtrl(pParent);
    connect(newCtrl, &AnimationPropertyCtrl::ValueChanged, newCtrl, [newCtrl]()
    {
        EBUS_EVENT(AzToolsFramework::PropertyEditorGUIMessages::Bus, RequestWrite, newCtrl);
    });
    return newCtrl;
}


void AnimationPropertyWidgetHandler::ConsumeAttribute(AnimationPropertyCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
{
    Q_UNUSED(GUI);
    Q_UNUSED(attrib);
    Q_UNUSED(attrValue);
    Q_UNUSED(debugName);
}

void AnimationPropertyWidgetHandler::WriteGUIValuesIntoProperty(size_t index, AnimationPropertyCtrl* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    Q_UNUSED(index);
    Q_UNUSED(node);
    CReflectedVarAnimation val = GUI->value();
    instance = static_cast<property_t>(val);
}

bool AnimationPropertyWidgetHandler::ReadValuesIntoGUI(size_t index, AnimationPropertyCtrl* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node)
{
    Q_UNUSED(index);
    Q_UNUSED(node);
    CReflectedVarAnimation val = instance;
    GUI->SetValue(val);
    return false;
}


#include <Controls/ReflectedPropertyControl/PropertyAnimationCtrl.moc>

