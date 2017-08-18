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
#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////
//! CanvasSizeToolbar provides controls to configure the canvas size
class CanvasSizeToolbarSection
{
public: // methods

    explicit CanvasSizeToolbarSection(QToolBar* parent);
    virtual ~CanvasSizeToolbarSection();

    void InitWidgets(QToolBar* parent, bool addSeparator);

    //! Given a canvas set, select a preset or automatically populate the custom canvas size text boxes
    //!
    //! This method is called when loading a canvas
    void SetInitialResolution(const AZ::Vector2& canvasSize);

    void SetIndex(int index);

    const QString& IndexToString(int index);

protected: // types

    //! Simple encapsulation of canvas size width and height presets, along with descriptions.
    struct CanvasSizePresets
    {
        CanvasSizePresets(const QString& desc, int width, int height)
            : description(desc)
            , width(width)
            , height(height) { }

        QString description;
        int width;
        int height;
    };

protected: // methods

    //! Updates the state of the custom canvas size fields based on the currently selected index.
    //
    //! When "other..." is selected, the fields become visible and take focus. Otherwise, this
    //! method hides the fields (as they are not needed when using preset canvas sizes).
    void ToggleLineEditBoxes(int currentIndex);

    //! Sets the canvas size based on the current canvas ComboBox selection
    virtual void SetCanvasSizeByComboBoxIndex(int comboIndex) = 0;

    virtual void OnComboBoxIndexChanged(EditorWindow* editorWindow, int index) = 0;

    virtual void AddSpecialPresets() {}

    //! Attempts to parse the canvas size presets JSON
    //
    //! \return True if successful, false otherwise
    bool ParseCanvasSizePresetsJson(const QJsonDocument& jsonDoc);

    //! Initializes canvas size presets options via JSON file
    //
    //! Falls back on hard-coded defaults if JSON file parsing fails.
    void InitCanvasSizePresets();

    //! Called when the user is finished entering text for custom canvas width size.
    void LineEditWidthEditingFinished();

    //! Called when the user is finished entering text for custom canvas height size.
    void LineEditHeightEditingFinished();

protected: // members

    int m_comboIndex;

    // Canvas size presets
    typedef AZStd::vector<CanvasSizePresets> ComboBoxOptions;
    ComboBoxOptions m_canvasSizePresets;
    QComboBox* m_combobox;

    // Custom canvas size
    QLineEdit* m_lineEditCanvasWidth;
    QLabel* m_labelCustomSizeDelimiter;
    QLineEdit* m_lineEditCanvasHeight;

    QMetaObject::Connection m_lineEditCanvasWidthConnection;
    QMetaObject::Connection m_lineEditCanvasHeightConnection;

    // The toolbar actions for custom canvas size are exclusively used for controlling
    // the visibility of the widgets within the toolbar.
    QAction* m_canvasWidthAction;
    QAction* m_canvasDelimiterAction;
    QAction* m_canvasHeightAction;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! ReferenceCanvasSizeToolbarSection provides controls to configure the reference canvas size
//! (a.k.a. authored canvas size) or the canvas being edited
class ReferenceCanvasSizeToolbarSection
    : public CanvasSizeToolbarSection
{
public: // methods

    explicit ReferenceCanvasSizeToolbarSection(QToolBar* parent, bool addSeparator = false);

private: // methods

    //! Sets the canvas size based on the current canvas ComboBox selection
    void SetCanvasSizeByComboBoxIndex(int comboIndex) override;

    void OnComboBoxIndexChanged(EditorWindow* editorWindow, int index) override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
//! PreviewCanvasSizeToolbarSection provides controls to configure the preview canvas size
class PreviewCanvasSizeToolbarSection
    : public CanvasSizeToolbarSection
{
public: // methods

    explicit PreviewCanvasSizeToolbarSection(QToolBar* parent, bool addSeparator = false);

private: // methods

    //! Sets the canvas size based on the current canvas ComboBox selection
    void SetCanvasSizeByComboBoxIndex(int comboIndex) override;

    void OnComboBoxIndexChanged(EditorWindow* editorWindow, int index) override;

    void AddSpecialPresets() override;
};
