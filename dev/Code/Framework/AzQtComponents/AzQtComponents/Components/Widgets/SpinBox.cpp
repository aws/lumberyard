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
#include <cmath>

#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzQtComponents/Components/Style.h>

#include <QApplication>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QObject>
#include <QPainter>
#include <QSettings>
#include <QStyle>
#include <QStyleOptionSpinBox>
#include <QMenu>

namespace AzQtComponents
{

static const char* g_hoverControlPropertyName = "HoverControl";
static const char* g_hoveredPropertyName = "hovered";
static const char* g_spinBoxUpPressedPropertyName = "SpinBoxUpButtonPressed";
static const char* g_spinBoxDownPressedPropertyName = "SpinBoxDownButtonPressed";

class SpinBoxWatcher : public QObject
{
public:
    explicit SpinBoxWatcher(QObject* parent = nullptr);

    SpinBox::Config m_config;

    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    enum State
    {
        Inactive,
        Dragging,
        ProcessingArrowButtons
    };
    int m_xPos = 0;
    bool m_emittedChangeBegun = false;
    State m_state = Inactive;

    bool filterSpinBoxEvents(QAbstractSpinBox* spinBox, QEvent* event);
    bool filterLineEditEvents(QLineEdit* lineEdit, QEvent* event);
    bool handleMouseDragStepping(QAbstractSpinBox* spinBox, QEvent* event);

    void initStyleOption(QAbstractSpinBox* spinBox, QStyleOptionSpinBox* styleOption);
    QAbstractSpinBox::StepEnabled stepEnabled(QAbstractSpinBox* spinBox);

    void emitValueChangeBegan(QAbstractSpinBox* spinBox);
    void emitValueChangeEnded(QAbstractSpinBox* spinBox);

    void resetCursor(QAbstractSpinBox* spinBox);
};

SpinBoxWatcher::SpinBoxWatcher(QObject* parent)
    : QObject(parent)
    , m_config(SpinBox::defaultConfig())
{

}

bool SpinBoxWatcher::eventFilter(QObject* watched, QEvent* event)
{
    bool filterEvent = false;
    if (auto spinBox = qobject_cast<QAbstractSpinBox*>(watched))
    {
        filterEvent = filterSpinBoxEvents(spinBox, event);
    }
    else if (auto lineEdit = qobject_cast<QLineEdit*>(watched))
    {
        filterEvent = filterLineEditEvents(lineEdit, event);
    }

    return filterEvent ? filterEvent : QObject::eventFilter(watched, event);
}

bool SpinBoxWatcher::filterSpinBoxEvents(QAbstractSpinBox* spinBox, QEvent* event)
{
    if (!spinBox || !spinBox->isEnabled())
    {
        return false;
    }

    bool filterEvent = false;
    switch (event->type())
    {
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::HoverMove:
        {
            resetCursor(spinBox);
            break;
        }

        case QEvent::Enter:
        case QEvent::Leave:
        {
            if (auto lineEdit = spinBox->findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly))
            {
                // Recalculate the line edit geometry so that the value runs off the right edge
                // if the SpinBox buttons are not visible.
                QStyleOptionSpinBox styleOption;
                initStyleOption(spinBox, &styleOption);
                const auto newGeometry = spinBox->style()->subControlRect(QStyle::CC_SpinBox,
                                                                          &styleOption,
                                                                          QStyle::SC_SpinBoxEditField,
                                                                          spinBox);
                if (lineEdit->geometry() != newGeometry)
                {
                    lineEdit->setGeometry(newGeometry);
                }
            }

            // Have to account for Qt not setting this properly for spinboxes
            // We manually set a hovered property that can be matched against in the stylesheet
            spinBox->setProperty(g_hoveredPropertyName, bool(event->type() == QEvent::Enter));

            // Qt, for performance reasons, doesn't re-evaluate css rules when a dynamic property changes
            // so we have to force it to.
            spinBox->style()->unpolish(spinBox);
            spinBox->style()->polish(spinBox);

            break;
        }

        case QEvent::Paint:
        {
            // Update the mouse cursor. QPaintEvent is used because it occurs when the mouse
            // moves between sub-controls, which means we don't have to enable mouseTracking. We
            // can't do this in SpinBox::drawSpinBox because there we only have a const QWidget
            // pointer.

            if (m_state == Inactive)
            {
                resetCursor(spinBox);
            }
            break;
        }

        case QEvent::FocusOut:
        {
            // Checks whether valueChangeBegun has been emitted and emits valueChangeEnded if
            // required. This handles the case where a new value has been typed without pressing
            // return or enter, or after a wheel event.
            emitValueChangeEnded(spinBox);

            break;
        }

        case QEvent::KeyPress:
        {
            auto keyEvent = static_cast<QKeyEvent*>(event);

            // Handle up/down arrows
            bool up = false;
            switch (keyEvent->key())
            {
                case Qt::Key_Up:
                    up = true;
                    // Fall through

                case Qt::Key_Down:
                    if (!keyEvent->isAutoRepeat())
                    {
                        emitValueChangeBegan(spinBox);
                    }
                    break;
            }

            // Handle digits
            const bool isDigit = (keyEvent->key() >= Qt::Key_0) && (keyEvent->key() <= Qt::Key_9);
            const bool isForFloatingPointDecimal = (qobject_cast<QDoubleSpinBox*>(spinBox) && (keyEvent->key() == Qt::Key_Period));
            if (isDigit ||
                keyEvent->key() == Qt::Key_Backspace ||
                isForFloatingPointDecimal
               )
            {
                emitValueChangeBegan(spinBox);
                // emitValueChangeEnded is called when Qt::Key_Return or Qt::Key_Enter is released,
                // or in QEvent::FocusOut.
            }

            break;
        }

        case QEvent::KeyRelease:
        {
            auto keyEvent = static_cast<QKeyEvent*>(event);
            if (m_emittedChangeBegun && !keyEvent->isAutoRepeat())
            {
                switch (keyEvent->key())
                {
                    case Qt::Key_Up:
                    case Qt::Key_Down:
                    case Qt::Key_Return:
                    case Qt::Key_Enter:
                        emitValueChangeEnded(spinBox);
                        break;
                }
            }
            break;
        }

        case QEvent::MouseButtonDblClick:
        case QEvent::MouseButtonPress:
        {
            auto mouseEvent = static_cast<QMouseEvent*>(event);

            if (mouseEvent->button() == Qt::LeftButton)
            {
                QStyleOptionSpinBox styleOption;
                initStyleOption(spinBox, &styleOption);
                const auto control = spinBox->style()->hitTestComplexControl(QStyle::CC_SpinBox,
                                                                             &styleOption,
                                                                             mouseEvent->pos(),
                                                                             spinBox);
                const auto enabledSteps = stepEnabled(spinBox);
                if (((control == QStyle::SC_SpinBoxUp) && (enabledSteps & QSpinBox::StepUpEnabled)) ||
                    ((control == QStyle::SC_SpinBoxDown) && (enabledSteps & QSpinBox::StepDownEnabled)))
                {
                    emitValueChangeBegan(spinBox);
                    // emitValueChangeEnded is called in SpinBoxWatcher::handleMouseDragStepping

                    m_state = ProcessingArrowButtons;

                    if (control == QStyle::SC_SpinBoxUp)
                    {
                        spinBox->setProperty(g_spinBoxUpPressedPropertyName, true);
                    }
                    else if (control == QStyle::SC_SpinBoxDown)
                    {
                        spinBox->setProperty(g_spinBoxDownPressedPropertyName, true);
                }
            }
            }
            break;
        }

        case QEvent::MouseButtonRelease:
        {
            spinBox->setProperty(g_spinBoxUpPressedPropertyName, false);
            spinBox->setProperty(g_spinBoxDownPressedPropertyName, false);
            break;
        }

        case QEvent::Wheel:
        {
            auto wheelEvent = static_cast<QWheelEvent*>(event);

            // Qt::ScrollEnd is only supported on macOS and only applies to trackpads, not scroll
            // wheels.
            if (wheelEvent->phase() == Qt::ScrollEnd)
            {
                emitValueChangeEnded(spinBox);
                break;
            }

            const auto angleDelta = wheelEvent->angleDelta();
            if (angleDelta.isNull() || m_emittedChangeBegun)
            {
                break;
            }
            else if (angleDelta.y() != 0)
            {
                const auto enabledSteps = stepEnabled(spinBox);
                if (((angleDelta.y() < 0) && (enabledSteps & QSpinBox::StepDownEnabled)) ||
                    ((angleDelta.y() > 0) && (enabledSteps & QSpinBox::StepUpEnabled)))
                {
                    emitValueChangeBegan(spinBox);
                    // emitValueChangeEnded is called in QEvent::FocusOut
                }
            }
            break;
        }
    }

    return filterEvent ? filterEvent : handleMouseDragStepping(spinBox, event);
}

bool SpinBoxWatcher::filterLineEditEvents(QLineEdit* lineEdit, QEvent* event)
{
    if (!lineEdit || !lineEdit->isEnabled())
    {
        return false;
    }

    auto spinBox = qobject_cast<QAbstractSpinBox*>(lineEdit->parent());
    if (!spinBox || !spinBox->isEnabled())
    {
        return false;
    }

    bool filterEvent = false;
    switch (event->type())
    {
        case QEvent::MouseButtonDblClick:
        {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            // Select whole number only on double click
            if ((mouseEvent->button() & Qt::LeftButton) && qobject_cast<QDoubleSpinBox*>(spinBox))
            {
                filterEvent = true;
                mouseEvent->accept();
                lineEdit->selectAll();
            }
            break;
        }
    }

    return filterEvent;
}

bool SpinBoxWatcher::handleMouseDragStepping(QAbstractSpinBox* spinBox, QEvent* event)
{
    if (!spinBox || !event)
    {
        return false;
    }

    switch (event->type())
    {
        case QEvent::MouseButtonPress:
        {
            auto mouseEvent = static_cast<QMouseEvent*>(event);

            if ((mouseEvent->button() & Qt::LeftButton) && (m_state == Inactive))
            {
                m_xPos = mouseEvent->x();
                emitValueChangeBegan(spinBox);
                m_state = Dragging;
            }
            break;
        }

        case QEvent::MouseButtonDblClick:
        {
            emitValueChangeBegan(spinBox);
            break;
        }

        case QEvent::MouseMove:
        {
            auto mouseEvent = static_cast<QMouseEvent*>(event);
            if ((m_state == Dragging) && (mouseEvent->buttons() & Qt::LeftButton))
            {
                const int delta = mouseEvent->x() - m_xPos;
                if (qAbs(delta) <= qAbs(m_config.pixelsPerStep))
                {
                    break;
                }

                m_xPos = mouseEvent->x();
                int step = delta > 0 ? 1 : -1;
                if (mouseEvent->modifiers() & Qt::ShiftModifier)
                {
                    step *= 10;
                }

                spinBox->stepBy(step);
            }
            break;
        }

        case QEvent::MouseButtonRelease:
        {
            emitValueChangeEnded(spinBox);
            m_state = Inactive;

            resetCursor(spinBox);
            break;
        }

        default:
            break;
    }

    return false;
}

void SpinBoxWatcher::initStyleOption(QAbstractSpinBox* spinBox, QStyleOptionSpinBox* option)
{
    Q_ASSERT(spinBox);
    Q_ASSERT(option);

    if (qobject_cast<QSpinBox*>(spinBox) || qobject_cast<QDoubleSpinBox*>(spinBox))
    {
        // QAbstractSpinBox::initStyleOption is protected so we have to replicate that logic here instead of using it directly.
        enum QSpinBoxPrivateButton {
            None = 0x000,
            Keyboard = 0x001,
            Mouse = 0x002,
            Wheel = 0x004,
            ButtonMask = 0x008,
            Up = 0x010,
            Down = 0x020,
            DirectionMask = 0x040
        };

        option->initFrom(spinBox);
        option->activeSubControls = QStyle::SC_None;
        option->buttonSymbols = spinBox->buttonSymbols();
        option->subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField;

        bool buttonUpPressed = spinBox->property(g_spinBoxUpPressedPropertyName).toBool();
        bool buttonDownPressed = spinBox->property(g_spinBoxDownPressedPropertyName).toBool();

        if (option->buttonSymbols != QAbstractSpinBox::NoButtons)
        {
            option->subControls |= QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;

            if (buttonUpPressed)
            {
                option->activeSubControls = QStyle::SC_SpinBoxUp;
            }
            else if (buttonDownPressed)
            {
                option->activeSubControls = QStyle::SC_SpinBoxDown;
            }
        }

        if (buttonUpPressed || buttonDownPressed)
    {
            option->state |= QStyle::State_Sunken;
    }
        else
    {
            option->activeSubControls = spinBox->property(g_hoverControlPropertyName).value<QStyle::SubControl>();
        }

        option->stepEnabled = spinBox->style()->styleHint(QStyle::SH_SpinControls_DisableOnBounds)
            ? stepEnabled(spinBox)
            : (QAbstractSpinBox::StepDownEnabled | QAbstractSpinBox::StepUpEnabled);

        option->frame = spinBox->hasFrame();
    }
    else
    {
        // Initialize it as best we can
        option->initFrom(spinBox);
        option->frame = spinBox->hasFrame();
        option->subControls = QStyle::SC_SpinBoxFrame | QStyle::SC_SpinBoxEditField;
        if (!(spinBox->buttonSymbols() & QAbstractSpinBox::NoButtons))
        {
            option->subControls |= QStyle::SC_SpinBoxUp | QStyle::SC_SpinBoxDown;
        }
    }
}

QAbstractSpinBox::StepEnabled SpinBoxWatcher::stepEnabled(QAbstractSpinBox* spinBox)
{
    Q_ASSERT(spinBox);

    // QAbstractSpinBox::stepEnabled is protected; we can access it in the DoubleSpinBox and SpinBox objects, but not for anything else.

    QAbstractSpinBox::StepEnabled stepEnabled = QAbstractSpinBox::StepNone;
    if (auto azSpinBox = qobject_cast<SpinBox*>(spinBox))
    {
        stepEnabled = azSpinBox->stepEnabled();
    }
    else if (auto azDoubleSpinBox = qobject_cast<DoubleSpinBox*>(spinBox))
    {
        stepEnabled = azDoubleSpinBox->stepEnabled();
    }
    else
    {
        if (spinBox->isReadOnly())
        {
            return QAbstractSpinBox::StepNone;
        }

        // Assume we can step
        stepEnabled |= QAbstractSpinBox::StepUpEnabled;
        stepEnabled |= QAbstractSpinBox::StepDownEnabled;
    }

    return stepEnabled;
}

void SpinBoxWatcher::emitValueChangeBegan(QAbstractSpinBox* spinBox)
{
    if (m_emittedChangeBegun)
    {
        return;
    }

    if (auto azSpinBox = qobject_cast<SpinBox*>(spinBox))
    {
        m_emittedChangeBegun = true;
        emit azSpinBox->valueChangeBegan();
    }
    else if (auto azDoubleSpinBox = qobject_cast<DoubleSpinBox*>(spinBox))
    {
        m_emittedChangeBegun = true;
        emit azDoubleSpinBox->valueChangeBegan();
    }
}

void SpinBoxWatcher::emitValueChangeEnded(QAbstractSpinBox* spinBox)
{
    if (!m_emittedChangeBegun)
    {
        return;
    }

    if (auto azSpinBox = qobject_cast<SpinBox*>(spinBox))
    {
        emit azSpinBox->valueChangeEnded();
        m_emittedChangeBegun = false;
    }
    else if (auto azDoubleSpinBox = qobject_cast<DoubleSpinBox*>(spinBox))
    {
        emit azDoubleSpinBox->valueChangeEnded();
        m_emittedChangeBegun = false;
    }
}

void SpinBoxWatcher::resetCursor(QAbstractSpinBox* spinBox)
{
    QStyleOptionSpinBox styleOption;
    initStyleOption(spinBox, &styleOption);
    const auto pos = spinBox->mapFromGlobal(QCursor::pos());
    const QStyle::SubControl control = spinBox->style()->hitTestComplexControl(QStyle::CC_SpinBox,
        &styleOption,
        pos,
        spinBox);

    if ((control == QStyle::SC_SpinBoxUp) || (control == QStyle::SC_SpinBoxDown))
    {
        spinBox->setProperty(g_hoverControlPropertyName, control);
        spinBox->setCursor(Qt::ArrowCursor);
    }
    else
    {
        spinBox->setCursor(m_config.scrollCursor);
        spinBox->setProperty(g_hoverControlPropertyName, QStyle::SC_None);
    }
}

SpinBox::Config SpinBox::loadConfig(QSettings& settings)
{
    Config config = defaultConfig();
    config.pixelsPerStep = settings.value(QStringLiteral("PixelsPerStep"), config.pixelsPerStep).toInt();

    const auto cursorPath = settings.value(QStringLiteral("ScrollCursor")).toString();
    const QPixmap cursorIcon(cursorPath);
    if (!cursorIcon.isNull())
    {
        config.scrollCursor = QCursor(cursorIcon);
    }

    return config;
}

SpinBox::Config SpinBox::defaultConfig()
{
    Config config;
    config.pixelsPerStep = 10;
    config.scrollCursor = QCursor(QPixmap(":/stylesheet/img/UI20/spinbox-cursor-scroll.png"));
    return config;
}

static int s_watcherReferenceCount = 0;
QPointer<SpinBoxWatcher> SpinBox::m_spinBoxWatcher = nullptr;

void SpinBox::initializeWatcher()
{
    if (!m_spinBoxWatcher)
    {
        Q_ASSERT(s_watcherReferenceCount == 0);
    m_spinBoxWatcher = new SpinBoxWatcher;
}

    s_watcherReferenceCount++;
}

void SpinBox::uninitializeWatcher()
{
    Q_ASSERT(!m_spinBoxWatcher.isNull());
    Q_ASSERT(s_watcherReferenceCount > 0);

    s_watcherReferenceCount--;

    if (s_watcherReferenceCount == 0)
    {
    delete m_spinBoxWatcher;
        m_spinBoxWatcher = nullptr;
}
}

bool SpinBox::drawSpinBox(const QProxyStyle* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const SpinBox::Config& config)
{
    Q_UNUSED(config);

    if (const auto styleOption = qstyleoption_cast<const QStyleOptionSpinBox*>(option))
    {
        // Only draw the up and down controls if the spinbox is enabled and has the mouse over it
        if (!(styleOption->state & QStyle::State_Enabled) || !(styleOption->state & QStyle::State_MouseOver))
        {
            QStyleOptionSpinBox copy = *styleOption;
            copy.subControls &= ~QStyle::SC_SpinBoxUp;
            copy.subControls &= ~QStyle::SC_SpinBoxDown;
            style->baseStyle()->drawComplexControl(QStyle::CC_SpinBox, &copy, painter, widget);
            return true;
        }
    }
    return false;
}

QRect SpinBox::editFieldRect(const QProxyStyle* style, const QStyleOptionComplex* option, const QWidget* widget, const Config& config)
{
    Q_UNUSED(config);

    if (auto spinBoxOption = qstyleoption_cast<const QStyleOptionSpinBox *>(option))
    {
        // Long values should run off the right side of the spinbox, so extend the SC_SpinBoxEditField
        // to the edge of the spinbox contents but don't overlap with the buttons
        const auto baseRect = style->baseStyle()->subControlRect(QStyle::CC_SpinBox,
                                                                 option,
                                                                 QStyle::SC_SpinBoxEditField,
                                                                 widget);
        const auto buttonRect = style->baseStyle()->subControlRect(QStyle::CC_SpinBox,
                                                                   option,
                                                                   QStyle::SC_SpinBoxUp,
                                                                   widget);
        QRect rect(baseRect);
        rect.setRight(buttonRect.x());
        rect.adjust(0, 0, -1, 0); // Don't overlap buttonRect or the spinbox border

        if (!(spinBoxOption->state & QStyle::State_Enabled) ||
            !(spinBoxOption->state & QStyle::State_MouseOver))
        {
            // The buttons aren't visible, so extend the QLineEdit to the inside of the spinbox border
            rect.adjust(0, 0, buttonRect.width(), 0);
        }
        return rect;
    }
    return QRect();
}

bool SpinBox::polish(QProxyStyle* style, QWidget* widget, const SpinBox::Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(config);
    Q_ASSERT(!m_spinBoxWatcher.isNull());

    if (!qobject_cast<QSpinBox*>(widget) && !qobject_cast<QDoubleSpinBox*>(widget))
    {
        // QDateTime inherits from QAbstractSpinBox and shouldn't be included here.
        return false;
    }

    if (auto spinBox = qobject_cast<QAbstractSpinBox*>(widget))
    {
        Style* newStyle = qobject_cast<Style*>(style);
        if (newStyle)
        {
            newStyle->repolishOnSettingsChange(spinBox);
        }

        m_spinBoxWatcher->m_config = config;
        spinBox->installEventFilter(m_spinBoxWatcher);

        if (auto lineEdit = spinBox->findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly))
        {
            lineEdit->installEventFilter(m_spinBoxWatcher);
        }
        return true;
    }
    return false;
}

bool SpinBox::unpolish(QProxyStyle* style, QWidget* widget, const SpinBox::Config& config)
{
    Q_UNUSED(style);
    Q_UNUSED(config);

    Q_ASSERT(!m_spinBoxWatcher.isNull());

    if (auto spinBox = qobject_cast<QAbstractSpinBox*>(widget))
    {
        spinBox->removeEventFilter(m_spinBoxWatcher);

        if (auto lineEdit = spinBox->findChild<QLineEdit*>(QString(), Qt::FindDirectChildrenOnly))
        {
            lineEdit->removeEventFilter(m_spinBoxWatcher);
        }
        return true;
    }
    return false;
}

SpinBox::SpinBox(QWidget* parent)
    : QSpinBox(parent)
{
    setShiftIncreasesStepRate(true);
    m_lineEdit = new internal::SpinBoxLineEdit(this);
    connect(m_lineEdit, &internal::SpinBoxLineEdit::globalUndoTriggered, this, &SpinBox::globalUndoTriggered);
    connect(m_lineEdit, &internal::SpinBoxLineEdit::globalRedoTriggered, this, &SpinBox::globalRedoTriggered);
    setLineEdit(m_lineEdit);

    setRange(0, 100);
}

bool SpinBox::isUndoAvailable() const
{
    return lineEdit()->isUndoAvailable();
}

bool SpinBox::isRedoAvailable() const
{
    return lineEdit()->isRedoAvailable();
}

void SpinBox::focusInEvent(QFocusEvent* event)
{
    QSpinBox::focusInEvent(event);
    lineEdit()->deselect();
}

static QAction* findAction(QList<QAction*>& actions, const QString& actionText)
{
    // would be much smarter to use QKeySequences to identify the actions we're
    // looking for, but QLineEdit doesn't actually use them in the same way
    // anything else does, so we can't
    for (QAction* action : actions)
    {
        if (action->text().contains(actionText))
        {
            return action;
        }
    }

    return nullptr;
}

template <typename SpinBoxType>
void spinBoxContextMenuEvent(QContextMenuEvent* ev, SpinBoxType* spinBox, QLineEdit* lineEdit, uint stepEnabled)
{
    // We want to override the context menu's undo/redo in the case that the lineEdit control does not currently have any undo/redo available
    // but the QAbstractSpinBox doesn't really expose any way to do that nicely.
    // As a result, we have to replicate the context menu code from QAbstractSpinBox instead,
    // and also pull out the undo and redo actions so that we can override them.

    if (QMenu* menu = lineEdit->createStandardContextMenu())
    {
        auto menuActions = menu->actions();

        QAction* undoAction = findAction(menuActions, QObject::tr("&Undo"));
        if (undoAction)
        {
            QObject::disconnect(undoAction, &QAction::triggered, nullptr, nullptr);
            QObject::connect(undoAction, &QAction::triggered, spinBox, &SpinBoxType::globalUndoTriggered);

            // need a way for this to be overridden globally to know if there's any undo/redo available globally
        }

        QAction* redoAction = findAction(menuActions, QObject::tr("&Redo"));
        if (redoAction)
        {
            QObject::disconnect(redoAction, &QAction::triggered, nullptr, nullptr);
            QObject::connect(redoAction, &QAction::triggered, spinBox, &SpinBoxType::globalRedoTriggered);

            // need a way for this to be overridden globally to know if there's any undo/redo available globally

        }

        Q_EMIT spinBox->contextMenuAboutToShow(undoAction, redoAction);

        QAction* selectAllAction = findAction(menuActions, QObject::tr("Select All"));
        if (selectAllAction)
        {
            QObject::disconnect(selectAllAction, &QAction::triggered, nullptr, nullptr);
            QObject::connect(selectAllAction, &QAction::triggered, spinBox, &QAbstractSpinBox::selectAll);
        }

        menu->addSeparator();

        QAction* upAction = menu->addAction(QObject::tr("&Step up"));
        upAction->setEnabled(stepEnabled & QAbstractSpinBox::StepUpEnabled);
        QObject::connect(upAction, &QAction::triggered, spinBox, [spinBox] {
            spinBox->stepBy(1);
        });

        QAction* downAction = menu->addAction(QObject::tr("Step &down"));
        downAction->setEnabled(stepEnabled & QAbstractSpinBox::StepDownEnabled);
        QObject::connect(downAction, &QAction::triggered, spinBox, [spinBox] {
            spinBox->stepBy(-1);
        });

        menu->addSeparator();

        const QPoint pos = (ev->reason() == QContextMenuEvent::Mouse)
            ? ev->globalPos() : spinBox->mapToGlobal(QPoint(ev->pos().x(), 0)) + QPoint(spinBox->width() / 2, spinBox->height() / 2);

        menu->setAttribute(Qt::WA_DeleteOnClose);
        menu->popup(pos);

        ev->accept();
    }
}

void SpinBox::contextMenuEvent(QContextMenuEvent* ev)
{
    if (m_lineEdit->overrideUndoRedo())
    {
        spinBoxContextMenuEvent(ev, this, lineEdit(), stepEnabled());
    }
    else
    {
        return QSpinBox::contextMenuEvent(ev);
    }
}

DoubleSpinBox::DoubleSpinBox(QWidget* parent)
    : QDoubleSpinBox(parent)
{
    setShiftIncreasesStepRate(true);
    m_lineEdit = new internal::SpinBoxLineEdit(this);
    connect(m_lineEdit, &internal::SpinBoxLineEdit::globalUndoTriggered, this, &DoubleSpinBox::globalUndoTriggered);
    connect(m_lineEdit, &internal::SpinBoxLineEdit::globalRedoTriggered, this, &DoubleSpinBox::globalRedoTriggered);
    setLineEdit(m_lineEdit);

    setRange(0, 100);
}

bool DoubleSpinBox::isUndoAvailable() const
{
    return lineEdit()->isUndoAvailable();
}

bool DoubleSpinBox::isRedoAvailable() const
{
    return lineEdit()->isRedoAvailable();
}

void DoubleSpinBox::contextMenuEvent(QContextMenuEvent* ev)
{
    if (m_lineEdit->overrideUndoRedo())
    {
        spinBoxContextMenuEvent(ev, this, lineEdit(), stepEnabled());
    }
    else
    {
        return QDoubleSpinBox::contextMenuEvent(ev);
    }
}

QString DoubleSpinBox::textFromValue(double value) const
{
    // If user only enters a whole number, don't show empty decimal places.
    int precision = decimals();
    if (value == std::floor(value))
    {
        precision = 0;
    }

    QString str = locale().toString(value, 'f', precision);
    if (!isGroupSeparatorShown() && (qAbs(value) >= 1000.0))
    {
        str.remove(locale().groupSeparator());
    }

    return str;
}

void DoubleSpinBox::focusInEvent(QFocusEvent* event)
{
    QDoubleSpinBox::focusInEvent(event);
    lineEdit()->deselect();
}


namespace internal
{

    bool SpinBoxLineEdit::overrideUndoRedo() const
    {
        return !isUndoAvailable() && !isRedoAvailable() && !isReadOnly();
    }

    bool SpinBoxLineEdit::event(QEvent* ev)
    {
        switch (ev->type())
        {
        case QEvent::FocusOut:
        {
            // Explicitly set the text again on focusOut, so that the undo/redo queue for the line edit clears and the global undo/redo can kick in
            setText(text());
        }
        break;
        }

        return QLineEdit::event(ev);
    }

    void SpinBoxLineEdit::keyPressEvent(QKeyEvent* ev)
    {
        if (overrideUndoRedo())
        {
            // QLineEdit overrides the key press event handler
            // so we have to trap that too, directly, without assuming
            // that QAction's with shortcuts will do it.
            if (ev->matches(QKeySequence::Undo))
            {
                Q_EMIT globalUndoTriggered();
                ev->accept();
                return;
            }
            else if (ev->matches(QKeySequence::Redo))
            {
                Q_EMIT globalRedoTriggered();
                ev->accept();
                return;
            }
            else if (ev->matches(QKeySequence::SelectAll))
            {
                Q_EMIT selectAllTriggered();
                ev->accept();
                return;
            }
        }

        // fall through to the default
        QLineEdit::keyPressEvent(ev);
    }

} // namespace internal

} // namespace AzQtComponents
#include <Components/Widgets/SpinBox.moc>

