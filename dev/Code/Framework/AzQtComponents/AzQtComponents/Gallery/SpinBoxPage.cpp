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

#include "SpinBoxPage.h"
#include <Gallery/ui_SpinBoxPage.h>

#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <QUndoCommand>
#include <QPointer>
#include <QEvent>
#include <QAction>
#include <QMenu>

template<typename Spinbox, typename ValueType>
class SpinBoxChangedCommand : public QUndoCommand
{
public:
    SpinBoxChangedCommand(Spinbox* objectChanging, ValueType oldValue, ValueType newValue)
        : QUndoCommand(QStringLiteral("Change %1 to %2").arg(oldValue).arg(newValue))
        , m_spinBox(objectChanging)
        , m_oldValue(oldValue)
        , m_newValue(newValue)
    {
    }

    void undo() override
    {
        m_spinBox->setValue(m_oldValue);

        if (m_spinBox->hasFocus())
        {
            m_spinBox->selectAll();
        }
    }

    void redo() override
    {
        if (!m_firstTime)
        {
            m_spinBox->setValue(m_newValue);

            if (m_spinBox->hasFocus())
            {
                m_spinBox->selectAll();
            }
        }

        m_firstTime = false;
    }

private:
    QPointer<Spinbox> m_spinBox;

    bool m_firstTime = true;
    ValueType m_oldValue;
    ValueType m_newValue;
};

SpinBoxPage::SpinBoxPage(QWidget* parent)
: QWidget(parent)
, ui(new Ui::SpinBoxPage)
{
    ui->setupUi(this);
    ui->focusSpinBox->setFocus();
    ui->filledInDoubleSpinBox->setDecimals(40);
    ui->filledInDoubleSpinBox->setValue(3.1415926535897932384626433832795028841971);
    setFocusProxy(ui->focusSpinBox);

    ui->focusDoubleSpinBox->setRange(0.0, 1.0);
    ui->focusDoubleSpinBox->setSingleStep(0.01);
    ui->focusDoubleSpinBox->setValue(0.07);

    QString exampleText = R"(

QAbstractSpinBox docs: <a href="https://doc.qt.io/qt-5/qabstractspinbox.html">https://doc.qt.io/qt-5/qabstractspinbox.html</a><br/>
QSpinBox docs: <a href="https://doc.qt.io/qt-5/qspinbox">https://doc.qt.io/qt-5/qspinbox</a><br/>
QDoubleSpinBox docs: <a href="https://doc.qt.io/qt-5/qdoublespinbox">https://doc.qt.io/qt-5/qdoublespinbox</a><br/>

<pre>
#include &lt;AzQtComponents/Components/Widgets/SpinBox.h&gt;

AzQtComponents::DoubleSpinBox* doubleSpinBox;

// Assuming you've created a DoubleSpinBox already (either in code or via .ui file):

// To set the range to 0 - 1 and step by 0.01:
doubleSpinBox->setRange(0.0, 1.0);
doubleSpinBox->setSingleStep(0.01);
)";

    ui->exampleText->setHtml(exampleText);

    track<AzQtComponents::SpinBox, int>(ui->defaultSpinBox);
    track<AzQtComponents::DoubleSpinBox, double>(ui->defaultDoubleSpinBox);
    track<AzQtComponents::SpinBox, int>(ui->errorSpinBox);
    track<AzQtComponents::DoubleSpinBox, double>(ui->errorDoubleSpinBox);
    track<AzQtComponents::SpinBox, int>(ui->filledInSpinBox);
    track<AzQtComponents::DoubleSpinBox, double>(ui->filledInDoubleSpinBox);
    track<AzQtComponents::SpinBox, int>(ui->focusSpinBox);
    track<AzQtComponents::DoubleSpinBox, double>(ui->focusDoubleSpinBox);
}

SpinBoxPage::~SpinBoxPage()
{
}

template <typename SpinBoxType, typename ValueType>
void SpinBoxPage::track(SpinBoxType* spinBox)
{
    // connect to changes in the spinboxes and listen for the undo state
    QObject::connect(spinBox, &SpinBoxType::valueChangeBegan, this, [this, spinBox]() {
        ValueType oldValue = spinBox->value();
        spinBox->setProperty("OldValue", oldValue);
    });

    QObject::connect(spinBox, &SpinBoxType::valueChangeEnded, this, [this, spinBox]() {
        ValueType oldValue = spinBox->property("OldValue").template value<ValueType>();
        ValueType newValue = spinBox->value();

        if (oldValue != newValue)
        {
            m_undoStack.push(new SpinBoxChangedCommand<SpinBoxType, ValueType>(spinBox, oldValue, newValue));

            spinBox->setProperty("OldValue", newValue);
        }
    });

    QObject::connect(spinBox, &SpinBoxType::globalUndoTriggered, &m_undoStack, &QUndoStack::undo);
    QObject::connect(spinBox, &SpinBoxType::globalRedoTriggered, &m_undoStack, &QUndoStack::redo);

    QObject::connect(spinBox, &SpinBoxType::contextMenuAboutToShow, this, [this](QMenu* menu, QAction* undo, QAction* redo) {
        Q_UNUSED(menu);

        undo->setEnabled(m_undoStack.canUndo());
        redo->setEnabled(m_undoStack.canRedo());
    });
}

#include <Gallery/SpinBoxPage.moc>

