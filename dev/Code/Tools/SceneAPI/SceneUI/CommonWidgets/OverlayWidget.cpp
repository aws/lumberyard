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

#include <QEvent.h>
#include <QLayout.h>
#include <QMessageBox.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidgetLayer.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            const int OverlayWidget::s_invalidOverlayIndex = -1;

            OverlayWidget::LayerInfo::LayerInfo(OverlayWidgetLayer* layer, int id, int index)
                : m_layer(layer)
                , m_layerId(id)
                , m_layerIndex(index)
            {
            }

            OverlayWidget::OverlayWidget(QWidget* parent)
                : QWidget(parent)
                , m_layerIdCounter(0)
                , m_rootIndex(s_invalidOverlayIndex)
            {
                m_stack = new QStackedLayout();
                m_stack->setStackingMode(QStackedLayout::StackAll);
                setLayout(m_stack);

                // Guarantees there's always a root element.
                m_rootIndex = m_stack->addWidget(new QWidget());

                connect(m_stack, &QStackedLayout::widgetRemoved, this, &OverlayWidget::OnStackEntryRemoved);
            }

            void OverlayWidget::SetRoot(QWidget* root)
            {
                m_stack->removeItem(m_stack->itemAt(m_rootIndex));
                m_rootIndex = m_stack->insertWidget(m_rootIndex, root);
            }

            int OverlayWidget::PushLayer(QWidget* centerWidget, QWidget* breakoutWidget, const char* title, const OverlayWidgetButtonList& buttons)
            {
                return PushLayer(aznew OverlayWidgetLayer(this, centerWidget, breakoutWidget, title, buttons));
            }

            bool OverlayWidget::PopLayer()
            {
                if (!IsAtRoot())
                {
                    OverlayWidgetLayer* layer = qobject_cast<OverlayWidgetLayer*>(m_stack->currentWidget());
                    AZ_Assert(layer, "Expected a layer widget at the top of the overlay stack.");
                    return PopLayer(layer);
                }
                else
                {
                    return false;
                }
            }

            bool OverlayWidget::PopLayer(int layerId)
            {
                auto it = AZStd::find_if(m_layers.begin(), m_layers.end(), [layerId](const LayerInfo& layer) -> bool { return layer.m_layerId == layerId; });
                AZ_Assert(it != m_layers.end(), "Layer id (%i) is not a valid id for an overlay layer.", layerId);
                return PopLayer(it->m_layer);
            }

            bool OverlayWidget::PopAllLayers()
            {
                if (!CanClose())
                {
                    return false;
                }

                while (!IsAtRoot())
                {
                    if (!PopLayer())
                    {
                        return false;
                    }
                }
                return true;
            }

            void OverlayWidget::RefreshAll()
            {
                for (auto& it : m_layers)
                {
                    it.m_layer->Refresh();
                }
            }

            void OverlayWidget::RefreshLayer(int layerId)
            {
                auto it = AZStd::find_if(m_layers.begin(), m_layers.end(), [layerId](const LayerInfo& layer) -> bool { return layer.m_layerId == layerId; });
                AZ_Assert(it != m_layers.end(), "Layer id (%i) is not a valid id for an overlay layer.", layerId);
                it->m_layer->Refresh();
            }

            bool OverlayWidget::IsAtRoot() const
            {
                return m_stack->currentIndex() == m_rootIndex;
            }

            bool OverlayWidget::CanClose() const
            {
                return AZStd::all_of(m_layers.begin(), m_layers.end(), [](const LayerInfo& entry) -> bool { return entry.m_layer->CanClose(); });
            }

            int OverlayWidget::PushLayer(OverlayWidgetLayer* layer)
            {
                int id = m_layerIdCounter;
                m_layerIdCounter++;

                int index = m_stack->addWidget(layer);
                m_stack->setCurrentIndex(index);

                m_layers.emplace_back(layer, id, index);
                
                emit LayerAdded(id);
                return id;
            }

            bool OverlayWidget::PopLayer(OverlayWidgetLayer* layer)
            {
                auto it = AZStd::find_if(m_layers.begin(), m_layers.end(), [layer](const LayerInfo& entry) -> bool { return entry.m_layer == layer; });
                if (it == m_layers.end())
                {
                    // Layer has already been removed. This can happen as part of chain of close events, as this would also remove
                    //      the layer from the stack.
                    return true;
                }

                if (!layer->CanClose())
                {
                    return false;
                }

                // Remove it from the recorded layer stack as removing the layer from the widget stack can cause re-entry due to
                //      reacting to the close event in the layer.
                int id = it->m_layerId;
                int index = it->m_layerIndex;
                m_layers.erase(it);

                // Explicitly pop so that any open breakout windows get closed.
                layer->PopLayer();
                m_stack->removeWidget(layer);

                for (auto& it : m_layers)
                {
                    AZ_Assert(it.m_layerIndex != index, "Multiple overlay entries at stack index %i found.", index);
                    if (it.m_layerIndex > index)
                    {
                        it.m_layerIndex--;
                    }
                }

                // In case the top layer was removed, reset the widget at the top.
                if (m_stack->count() == index)
                {
                    m_stack->setCurrentIndex(m_stack->count() - 1);
                }
                AZ_Assert(m_stack->currentWidget(), "Too many layers popped.");
                
                emit LayerRemoved(id);
                return true;
            }

            void OverlayWidget::OnStackEntryRemoved(int index)
            {
                auto it = AZStd::find_if(m_layers.begin(), m_layers.end(), [index](const LayerInfo& entry) -> bool { return entry.m_layerIndex == index; });
                if (it == m_layers.end())
                {
                    // Layer has already been removed. This can happen as part of chain of close events, as this would also remove
                    //      the layer from the stack.
                    return;
                }
                PopLayer(it->m_layer);
            }

            int OverlayWidget::PushLayerToOverlay(OverlayWidget* overlay, QWidget* centerWidget, QWidget* breakoutWidget, 
                const char* title, const OverlayWidgetButtonList& buttons)
            {
                return PushLayer(overlay, aznew OverlayWidgetLayer(overlay, centerWidget, breakoutWidget, title, buttons), breakoutWidget != nullptr);
            }

            int OverlayWidget::PushLayerToContainingOverlay(QWidget* overlayChild, QWidget* centerWidget, QWidget* breakoutWidget, 
                const char* title, const OverlayWidgetButtonList& buttons)
            {
                OverlayWidget* overlay = GetContainingOverlay(overlayChild);
                return PushLayer(overlay, aznew OverlayWidgetLayer(overlay, centerWidget, breakoutWidget, title, buttons), breakoutWidget != nullptr);
            }

            OverlayWidget* OverlayWidget::GetContainingOverlay(QWidget* overlayChild)
            {
                while (overlayChild != nullptr)
                {
                    OverlayWidget* overlayWidget = qobject_cast<OverlayWidget*>(overlayChild);
                    if (overlayWidget)
                    {
                        return overlayWidget;
                    }
                    else
                    {
                        overlayChild = overlayChild->parentWidget();
                    }
                }
                return nullptr;
            }

            int OverlayWidget::PushLayer(OverlayWidget* targetOverlay, OverlayWidgetLayer* layer, bool hasBreakoutWindow)
            {
                int result;
                if (targetOverlay)
                {
                    result = targetOverlay->PushLayer(layer);
                }
                else
                {
                    result = s_invalidOverlayIndex;
                }
                return result;
            }
        } // namespace UI
    } // namespace SceneAPI
} // namespace AZ

#include <CommonWidgets/OverlayWidget.moc>