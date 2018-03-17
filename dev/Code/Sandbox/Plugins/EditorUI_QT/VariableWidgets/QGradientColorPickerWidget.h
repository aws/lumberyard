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
#ifndef QGradientColorPickerWidget_h__
#define QGradientColorPickerWidget_h__
#include <QWidget>
#include <QGridLayout>
#include <QPropertyAnimation>
#include "CurveEditorContent.h"
#include <QPushButton>
#include <QLineEdit>
#include <Controls/QToolTipWidget.h>
#include <QWidgetAction>
#include <QGraphicsEffect>

class CCurveEditor;
struct ISplineInterpolator;
class QCustomGradientWidget;
struct SCurveEditorKey;
struct SCurveEditorCurve;
class CCurveEditorControl;
struct SCurveEditorContent;
class QAmazonLineEdit;

class QGradientColorPickerWidget
    : public QWidget
{
    Q_OBJECT
public:
    QGradientColorPickerWidget(SCurveEditorContent content, QGradientStops gradientHueInfo, QWidget* parent);
    QGradientColorPickerWidget(XmlNodeRef spline, SCurveEditorContent content, QGradientStops gradientHueInfo, QWidget* parent);
    ~QGradientColorPickerWidget();

    unsigned int AddGradientStop(float stop, QColor color);

    void RemoveGradientStop(const QGradientStop& stop);

    SCurveEditorKey& addKey(const float& time, const float& value);
    SCurveEditorKey& addKey(SCurveEditorKey const& addKey);

    void syncToInterpolator();

    void SetCurve(SCurveEditorCurve curve);

    SCurveEditorCurve GetCurve();

    SCurveEditorContent GetContent();

    void SetContent(SCurveEditorContent content);

    void SetGradientStops(QGradientStops stops);

    QCustomGradientWidget& GetGradient();

    QGradientStops GetStops();

    QColor GetHueAt(float x);

    float GetAlphaAt(float y);

    void UpdateAlpha();

    float GetMinAlpha();
    float GetMaxAlpha();

    virtual void paintEvent(QPaintEvent* event);
    virtual bool eventFilter(QObject* obj, QEvent* event);

    QColor GetKeyColor(unsigned int id);


    void SetCallbackAddCurveToLibary(std::function<void()> callback);
    void SetCallbackAddGradientToLibary(std::function<void(QString)> callback);
    void SetCallbackCurveChanged(std::function<void(struct SCurveEditorCurve)> callback);
    void SetCallbackGradientChanged(std::function<void(QGradientStops)> callback);
    void SetCallbackCurrentColorRequired(std::function<QColor()> callback);
    void SetCallbackColorChanged(std::function<void(QColor)> callback);
    void SetCallbackLocationChanged(std::function<void(short)> callback);
    void SetSelectedGradientPosition(int location);
    void SetSelectedCurveKeyPosition(float u, float v);

    void UpdateColors(const QMap<QString, QColor>& colorMap);

signals:
    void SignalSelectCurveKey(float u, float v);

protected:
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void resizeEvent(QResizeEvent* event);
    virtual void leaveEvent(QEvent* event);

    void PassThroughtSignalSelectCurveKey(CCurveEditorControl* key = nullptr);

    std::function<void()> callback_add_curve_to_library;
    std::function<void(QString)> callback_add_gradient_to_library;
    std::function<void(struct SCurveEditorCurve)> callback_curve_changed;
    std::function<void(QGradientStops)> callback_gradient_changed;
    std::function<QColor()> callback_color_required;
    std::function<void(QColor)> callback_color_changed;
    std::function<void(short)> callback_location_changed;

    void BuildCurveMenu(bool curveKeyMenu);
    void UpdateIcons();
    void SyncEditor();
    enum class Gradients
    {
        ALPHA_RANGE = 0,
        HUE_RANGE,
        NUM_GRADIENTS
    };

    //////////////////////////////////////////////////////////////////////////
    //Gradient Key
    struct GradientKey
    {
        GradientKey();
        GradientKey(const GradientKey& other);
        GradientKey(QGradientStop stop, QRect rect, QSize size = QSize(12.0f, 12.0f));
        bool ContainsPoint(const QPoint& p);
        void Paint(QPainter& painter, const QColor& inactive, const QColor& hovered, const QColor& selected);
        void SetUsableRegion(QRect rect);

        QGradientStop GetStop();
        void SetStop(QGradientStop val);
        bool IsSelected() const;
        void SetSelected(bool val);
        bool IsHovered() const;
        void SetHovered(bool val);
        QPoint LocalPos();
        void SetTime(qreal val);
    protected:
        QPolygonF createControlPolygon();
        QRect m_usableRegion;
        QSize m_size;
        QGradientStop m_stop;
        bool m_isSelected;
        bool m_isHovered;
    };
    //////////////////////////////////////////////////////////////
    GradientKey* GetHoveredKey();
    GradientKey* GetSelectedKey();
    void OnGradientChanged();
    //////////////////////////////////////////////////////////////////////////

    void onPickColor(GradientKey* key);
    QRect GetUsableRegion();


    CCurveEditor* m_curveEditor;
    SCurveEditorContent m_content;
    SCurveEditorCurve m_defaultCurve;
    ISplineInterpolator* m_spline;
    QCustomGradientWidget* m_gradient;
    QVector<GradientKey> m_hueStops;
    QGridLayout layout;
    QMenu* m_curveMenu;

    QMenu* m_gradientMenu;
    QPushButton* m_gradientMenuBtn;
    QAmazonLineEdit*  m_gradientMenuEdit;
    QWidgetAction*  m_gradientMenuBtnAction;
    QWidgetAction* m_gradientMenuEditAction;
    QGraphicsDropShadowEffect* m_dropShadow;
    QColor m_gradientKeyInactive;
    QColor m_gradientKeyHovered;
    QColor m_gradientKeySelected;
    QToolTipWidget* m_tooltip;

    float alphaMin, alphaMax;
    bool m_leftDown;
    bool m_rightDown;
    bool m_needsRepainting;
    const QSize m_gradientSize;
};
#endif // QGradientColorPickerWidget_h__
