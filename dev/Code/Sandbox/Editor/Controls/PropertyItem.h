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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_PROPERTYITEM_H
#define CRYINCLUDE_EDITOR_CONTROLS_PROPERTYITEM_H
#pragma once

// Editor Core
#include <Util/VariablePropertyType.h>

// forward declarations.
class CNumberCtrl;
class CPropertyCtrl;
class CInPlaceEdit;
class CInPlaceComboBox;
class CFillSliderCtrl;
class CSplineCtrl;
template<class T>
class CWidgetWrapper;
class CColorGradientCtrl;
class CSliderCtrlEx;
class CInPlaceColorButton;
struct IVariable;

class CInPlaceButton;
class CInPlaceCheckBox;

class CAnimationBrowser;
class CColorGradientCtrlWrapper;

/** Item of CPropertyCtrl.
        Every property item reflects value of single XmlNode.
*/
class SANDBOX_API CPropertyItem
    : public CRefCountBase
{
public:
    typedef std::vector<CString>                    TDValues;
    typedef std::map<CString, TDValues>      TDValuesContainer;

protected:
private:

public:
    // Variables.

    // Constructors.
    CPropertyItem(CPropertyCtrl* pCtrl);
    virtual ~CPropertyItem();

    //! Set xml node to this property item.
    virtual void SetXmlNode(XmlNodeRef& node);

    //! Set variable.
    virtual void SetVariable(IVariable* var);

    //! Get Variable.
    IVariable* GetVariable() const { return m_pVariable; }

    /** Get type of property item.
    */
    virtual int GetType() { return m_type; }

    /** Get name of property item.
    */
    virtual CString GetName() const { return m_name; };

    /** Set name of property item.
    */
    virtual void SetName(const char* sName) { m_name = sName; };

    /** Called when item becomes selected.
    */
    virtual void SetSelected(bool selected);

    /** Get if item is selected.
    */
    bool IsSelected() const { return m_bSelected; };

    /** Get if item is currently expanded.
    */
    bool IsExpanded() const { return m_bExpanded; };

    /** Get if item can be expanded (Have children).
    */
    bool IsExpandable() const { return m_bExpandable; };

    /** Check if item cannot be category.
    */
    bool IsNotCategory() const { return m_bNoCategory; };

    /** Check if item must be bold.
    */
    bool IsBold() const;

    /** Check if item must be disabled.
    */
    bool IsDisabled() const;

    /** Check if item must be drawn.
    */
    bool IsInvisible() const;

    /** Get height of this item.
    */
    virtual int GetHeight();

    /** Called by PropertyCtrl to draw value of this item.
    */
    virtual void DrawValue(CDC* dc, CRect rect);

    /** Called by PropertyCtrl when item selected to creare in place editing control.
    */
    virtual void CreateInPlaceControl(CWnd* pWndParent, CRect& rect);
    /** Called by PropertyCtrl when item deselected to destroy in place editing control.
    */
    virtual void DestroyInPlaceControl(bool bRecursive = false);

    virtual void CreateControls(CWnd* pWndParent, CRect& textRect, CRect& ctrlRect);

    /** Move in place control to new position.
    */
    virtual void MoveInPlaceControl(const CRect& rect);

    // Enable/Disables all controls
    virtual void EnableControls(bool bEnable);

    /** Set Focus to inplace control.
    */
    virtual void SetFocus();

    /** Set data from InPlace control to Item value.
    */
    virtual void SetData(CWnd* pWndInPlaceControl){};

    //////////////////////////////////////////////////////////////////////////
    // Mouse notifications.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnLButtonDown(UINT nFlags, CPoint point);
    virtual void OnRButtonDown(UINT nFlags, CPoint point) {};
    virtual void OnLButtonDblClk(UINT nFlags, CPoint point);
    virtual void OnMouseWheel(UINT nFlags, short zDelta, CPoint point);

    /** Changes value of item.
    */
    virtual void SetValue(const char* sValue, bool bRecordUndo = true, bool bForceModified = false);

    /** Returns current value of property item.
    */
    virtual const char* GetValue() const;

    /** Get Item's XML node.
    */
    XmlNodeRef& GetXmlNode() { return m_node; };

    //////////////////////////////////////////////////////////////////////////
    //! Get description of this item.
    CString GetTip() const;

    //! Return image index of this property.
    int GetImage() const { return m_image; };

    //! Return true if this property item is modified.
    bool IsModified() const { return m_modified; }

    bool HasDefaultValue(bool bChildren = false) const;

    /** Get script default value of property item.
    */
    virtual bool HasScriptDefault() const { return m_strScriptDefault != m_strNoScriptDefault; };

    /** Get script default value of property item.
    */
    virtual CString GetScriptDefault() const { return m_strScriptDefault; };

    /** Set script default value of property item.
    */
    virtual void SetScriptDefault(const char* sScriptDefault) { m_strScriptDefault = sScriptDefault; };

    /** Set script default value of property item.
    */
    virtual void ClearScriptDefault() { m_strScriptDefault = m_strNoScriptDefault; };

    //////////////////////////////////////////////////////////////////////////
    // Childs.
    //////////////////////////////////////////////////////////////////////////
    //! Expand child nodes.
    virtual void SetExpanded(bool expanded);

    //! Reload Value from Xml Node (hierarchicaly reload children also).
    virtual void ReloadValues();

    //! Get number of child nodes.
    int GetChildCount() const { return m_childs.size(); };
    //! Get Child by id.
    CPropertyItem* GetChild(int index) const { return m_childs[index]; };

    //! Parent of this item.
    CPropertyItem* GetParent() const { return m_parent; };

    //! Add Child item.
    void AddChild(CPropertyItem* item);

    //! Delete child item.
    void RemoveChild(CPropertyItem* item);

    //! Delete all child items
    void RemoveAllChildren();

    //! Find item that reference specified property.
    CPropertyItem* FindItemByVar(IVariable* pVar);
    //! Get full name, including names of all parents.
    virtual CString GetFullName() const;
    //! Find item by full specified item.
    CPropertyItem* FindItemByFullName(const CString& name);

    void ReceiveFromControl();

    CFillSliderCtrl* const GetFillSlider() const{return m_cFillSlider; }
    void EnableNotifyWithoutValueChange(bool bFlag);

    // default number of increments to cover the range of a property
    static const float s_DefaultNumStepIncrements;

    // for a consistent Feel, compute the step size for a numerical slider for the specified min/max, rounded to precision
    inline static float ComputeSliderStep(float sliderMin, float sliderMax, const float precision = .01f)
    {
        float step;
        step = int_round(((sliderMax - sliderMin) / CPropertyItem::s_DefaultNumStepIncrements) / precision) * precision;
        // prevent rounding down to zero
        return (step > precision) ? step : precision;
    }

    //! Trigger a variable changed call
    void Touch();

protected:
    //////////////////////////////////////////////////////////////////////////
    // Private methods.
    //////////////////////////////////////////////////////////////////////////
    void SendToControl();
    void CheckControlActiveColor();
    void RepositionWindow(CWnd* pWnd, CRect rc);

    void OnChildChanged(CPropertyItem* child);

    void OnEditChanged();
    void OnNumberCtrlUpdate(CNumberCtrl* ctrl);
    void OnFillSliderCtrlUpdate(CSliderCtrlEx* ctrl);
    void OnNumberCtrlBeginUpdate(CNumberCtrl* ctrl) {};
    void OnNumberCtrlEndUpdate(CNumberCtrl* ctrl) {};
    void OnSplineCtrlUpdate(CSplineCtrl* ctrl);

    void OnComboSelection();

    void OnCheckBoxButton();
    void OnColorBrowseButton();
    void OnColorChange(const QColor &col);
    void OnFileBrowseButton();
    void OnTextureBrowseButton();
    void OnTextureApplyButton();
    void OnTextureEditButton();
    void OnPsdEditButton();
    void OnAnimationBrowseButton();
    void OnAnimationApplyButton();
    void OnShaderBrowseButton();
    void OnParticleBrowseButton();
    void OnParticleApplyButton();
    void OnAIBehaviorBrowseButton();
    void OnAIAnchorBrowseButton();
#ifdef USE_DEPRECATED_AI_CHARACTER_SYSTEM
    void OnAICharacterBrowseButton();
#endif
    void OnAIPFPropertiesListBrowseButton();
    void OnAIEntityClassesBrowseButton();
    void OnEquipBrowseButton();
    void OnEAXPresetBrowseButton();
    void OnMaterialBrowseButton();
    void OnMaterialLookupButton();
    void OnMaterialPickSelectedButton();
    void OnGameTokenBrowseButton();
    void OnSequenceBrowseButton();
    void OnSequenceIdBrowseButton();
    void OnAudioMusicThemeAndMoodBrowseButton();
    void OnAudioMusicPatternsBrowseButton();
    void OnAudioMusicLogicPresetsBrowseButton();
    void OnAudioMusicLogicEventsBrowseButton();
    void OnMissionObjButton();
    void OnUserBrowseButton();
    void OnLocalStringBrowseButton();
    void OnExpandButton();
    void OnFlareDatabaseButton();
    void OnUiElementBrowseButton();
    void OnUiElementPickSelectedButton();

    void OnSOClassBrowseButton();
    void OnSOClassesBrowseButton();
    void OnSOStateBrowseButton();
    void OnSOStatesBrowseButton();
    void OnSOStatePatternBrowseButton();
    void OnSOActionBrowseButton();
    void OnSOHelperBrowseButton();
    void OnSONavHelperBrowseButton();
    void OnSOAnimHelperBrowseButton();
    void OnSOEventBrowseButton();
    void OnSOTemplateBrowseButton();

    void OnCustomActionBrowseButton();

    void OnLightAnimationBrowseButton();
    void OnResourceSelectorButton();

    void RegisterCtrl(CWnd* pCtrl);

    void ParseXmlNode(bool bRecursive = true);

    //! String to color.
    COLORREF StringToColor(const CString& value);
    //! String to boolean.
    bool GetBoolValue();

    //! Convert variable value to value string.
    void VarToValue();
    CString GetDrawValue();

    //! Convert from value to variable.
    void ValueToVar();

    //! Release used variable.
    void ReleaseVariable();
    //! Callback called when variable change.
    void OnVariableChange(IVariable* var);

    TDValues*   GetEnumValues(const CString& strPropertyName);

    void AddChildrenForPFProperties();
    void AddChildrenForAIEntityClasses();
    void PopulateAITerritoriesList();
    void PopulateAIWavesList();

    void CreateInPlaceComboBox(CWnd* pWndParent, DWORD style);
    void PopulateInPlaceComboBoxFromEnumList();
    void PopulateInPlaceComboBox(const std::vector<CString>& items);
    
private:
    CString m_name;
    PropertyType m_type;

    CString m_value;

    //////////////////////////////////////////////////////////////////////////
    // Flags for this property item.
    //////////////////////////////////////////////////////////////////////////
    //! True if item selected.
    unsigned int m_bSelected : 1;
    //! True if item currently expanded
    unsigned int m_bExpanded : 1;
    //! True if item can be expanded
    unsigned int m_bExpandable : 1;
    //! True if children can be edited in parent
    unsigned int m_bEditChildren : 1;
    //! True if children displayed in parent field.
    unsigned int m_bShowChildren : 1;
    //! True if item can not be category.
    unsigned int m_bNoCategory : 1;
    //! If tru ignore update that comes from childs.
    unsigned int m_bIgnoreChildsUpdate : 1;
    //! If can move in place controls.
    unsigned int m_bMoveControls : 1;
    //! True if item modified.
    unsigned int m_modified : 1;
    //! True if list-box is not to be sorted.
    unsigned int m_bUnsorted : 1;

    bool    m_bForceModified;
    bool    m_bDontSendToControl;

    // Used for number controls.
    float m_rangeMin;
    float m_rangeMax;
    float m_step;
    bool  m_bHardMin, m_bHardMax;     // Values really limited by this range.
    int   m_nHeight;

    // Xml node.
    XmlNodeRef m_node;

    //! Pointer to the variable for this item.
    TSmartPtr<IVariable> m_pVariable;

    //////////////////////////////////////////////////////////////////////////
    // InPlace controls.
    CColorCtrl<CStatic>* m_pStaticText;
    CNumberCtrl* m_cNumber;
    CNumberCtrl* m_cNumber1;
    CNumberCtrl* m_cNumber2;
    CNumberCtrl* m_cNumber3;
    CFillSliderCtrl* m_cFillSlider;
    CInPlaceEdit* m_cEdit;
    CWidgetWrapper<CSplineCtrl>* m_cSpline;
    CColorGradientCtrlWrapper* m_cColorSpline;
    CInPlaceComboBox* m_cCombo;
    CInPlaceButton* m_cButton;
    CInPlaceButton* m_cButton2;
    CInPlaceButton* m_cButton3;
    CInPlaceButton* m_cExpandButton;
    CInPlaceButton* m_cButton4;
    CInPlaceButton* m_cButton5;
    CInPlaceCheckBox* m_cCheckBox;
    CInPlaceColorButton* m_cColorButton;
    //////////////////////////////////////////////////////////////////////////
    std::vector<CWnd*> m_controls;

    //! Owner property control.
    CPropertyCtrl* m_propertyCtrl;

    //! Parent item.
    CPropertyItem* m_parent;

    // Enum.
    IVarEnumListPtr m_enumList;
    CUIEnumsDatabase_SEnum* m_pEnumDBItem;

    CString m_tip;
    int m_image;

    float m_valueMultiplier;

    // Childs.
    typedef std::vector<TSmartPtr<CPropertyItem> > Childs;
    Childs m_childs;

    //////////////////////////////////////////////////////////////////////////
    friend class CPropertyCtrlEx;
    int m_nCategoryPageId;

    CAnimationBrowser* m_pAnimBrowser;
    CWnd* m_pControlsHostWnd;
    CRect m_rcText;
    CRect m_rcControl;

    static HDWP s_HDWP;
    CString m_strNoScriptDefault;
    CString m_strScriptDefault;
};

typedef _smart_ptr<CPropertyItem> CPropertyItemPtr;

#endif // CRYINCLUDE_EDITOR_CONTROLS_PROPERTYITEM_H
