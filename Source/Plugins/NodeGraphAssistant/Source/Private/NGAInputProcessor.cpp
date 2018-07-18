// Copyright 2018 yangxiangyun
// All Rights Reserved

#include "NGAInputProcessor.h"
#include "CoreMinimal.h"

#include "SlateApplication.h"

#ifdef NGA_With_DragConnection_API 
#include "DragConnection.cpp"
#else
#include "_ImportModulAPI/DragConnection.cpp"
#endif

#include "SGraphPanel.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "EdGraph.h"
#include "GraphEditorSettings.h"

#include "ScopedTransaction.h"
#include "Editor/TransBuffer.h"

#include "NodeGraphAssistantConfig.h"
#include "NodeHelper.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "AIGraph.h"
#include "AnimationStateMachineGraph.h"

#include "EdGraphUtilities.h"
#include "IPluginManager.h"

#include "MultiBox/MultiBoxBuilder.h"
#include "MaterialEditorModule.h"
#include "BlueprintEditorModule.h"


//#pragma optimize("", off)

NGAInputProcessor::NGAInputProcessor()
{
	FNodeGraphAssistantCommands::Register();
	UICommandList = MakeShared<FUICommandList>();
	const FNodeGraphAssistantCommands& Commands = FNodeGraphAssistantCommands::Get();

	UICommandList->MapAction(Commands.BypassNodes,
		FExecuteAction::CreateRaw(this, &NGAInputProcessor::TryProcessAsBypassNodesEvent));
	UICommandList->MapAction(Commands.SelectDownStreamNodes,
		FExecuteAction::CreateRaw(this, &NGAInputProcessor::OnSelectLinkedNodes, false, true));
	UICommandList->MapAction(Commands.SelectUpStreamNodes,
		FExecuteAction::CreateRaw(this, &NGAInputProcessor::OnSelectLinkedNodes, true, false));
	UICommandList->MapAction(Commands.RearrangeNode,
		FExecuteAction::CreateRaw(this, &NGAInputProcessor::TryProcessAsRearrangeNodesEvent));
	UICommandList->MapAction(Commands.CycleWireDrawStyle,
		FExecuteAction::CreateRaw(this, &NGAInputProcessor::CycleWireDrawStyle)
	, FCanExecuteAction());
	UICommandList->MapAction(Commands.ToggleMaterialGraphWireColor,
		FExecuteAction::CreateLambda([] {const_cast<UNodeGraphAssistantConfig*>(GetDefault<UNodeGraphAssistantConfig>())->OverrideMaterialGraphPinColor = !GetDefault<UNodeGraphAssistantConfig>()->OverrideMaterialGraphPinColor; }));


	if (GetDefault<UNodeGraphAssistantConfig>()->WireStyleToolBarButton)
	{
		struct Local
		{
			static void FillToolbarMat(FToolBarBuilder& ToolbarBuilder)
			{
				ToolbarBuilder.BeginSection("NGA");
				{
					ToolbarBuilder.AddToolBarButton(FNodeGraphAssistantCommands::Get().CycleWireDrawStyle);
					ToolbarBuilder.AddToolBarButton(FNodeGraphAssistantCommands::Get().ToggleMaterialGraphWireColor);
				}
				ToolbarBuilder.EndSection();
			}
			static void FillToolbar(FToolBarBuilder& ToolbarBuilder)
			{
				ToolbarBuilder.BeginSection("NGA");
				{
					ToolbarBuilder.AddToolBarButton(FNodeGraphAssistantCommands::Get().CycleWireDrawStyle);
				}
				ToolbarBuilder.EndSection();
			}
		};
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension(
			"Graph",
			EExtensionHook::After,
			UICommandList,
			FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar)
		);
		IMaterialEditorModule* MaterialEditorModule = &FModuleManager::LoadModuleChecked<IMaterialEditorModule>("MaterialEditor");
		MaterialEditorModule->GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
		ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension(
			"Debugging",
			EExtensionHook::After,
			UICommandList,
			FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar)
		);
		FBlueprintEditorModule* FBPModule = &FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
		FBPModule->GetMenuExtensibilityManager()->AddExtender(ToolbarExtender);
	}


	MyGraphPinConnectionFactory = MakeShareable(new FNGAGraphPinConnectionFactory());
	FEdGraphUtilities::RegisterVisualPinConnectionFactory(MyGraphPinConnectionFactory);
}

NGAInputProcessor::~NGAInputProcessor()
{
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().RearrangeNode);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().SelectUpStreamNodes);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().SelectDownStreamNodes);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().BypassNodes);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().CycleWireDrawStyle);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().ToggleMaterialGraphWireColor);
	UICommandList = nullptr;
	FNodeGraphAssistantCommands::Unregister(); 

	FEdGraphUtilities::UnregisterVisualPinConnectionFactory(MyGraphPinConnectionFactory);
}


bool NGAInputProcessor::IsCursorInsidePanel(const FPointerEvent& MouseEvent) const
{
	FSlateApplication& slateApp = FSlateApplication::Get();
	FWidgetPath widgetsUnderCursor = slateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), slateApp.GetInteractiveTopLevelWindows());
	for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
	{
		if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString() == "SGraphPanel")
		{
			return true;
		}
	}
	return false;
}


FArrangedWidget NGAInputProcessor::GetArrangedGraphPanel(const FPointerEvent& MouseEvent)
{
	FSlateApplication& slateApp = FSlateApplication::Get();
	FWidgetPath widgetsUnderCursor = slateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), slateApp.GetInteractiveTopLevelWindows());
	for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
	{
		if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString() == "SGraphPanel")
		{
			return widgetsUnderCursor.Widgets[i];
		}
	}
	return FArrangedWidget::NullWidget;
}


//not include comment node.
FArrangedWidget NGAInputProcessor::GetArrangedGraphNode(const FPointerEvent& MouseEvent)
{
	FSlateApplication& slateApp = FSlateApplication::Get();
	FWidgetPath widgetsUnderCursor = slateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), slateApp.GetInteractiveTopLevelWindows());

	for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
	{
		FString widgetName = widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString();
		if (widgetName.Contains("GraphNode") && widgetName != "SGraphNodeComment" && widgetName != "SGraphNodeMaterialComment")
		{
			//physics node
			if (!widgetName.Contains("OutputPin") && !widgetName.Contains("InputPin"))
			{
				return widgetsUnderCursor.Widgets[i];
			}
		}
	}
	return FArrangedWidget::NullWidget;
}


bool NGAInputProcessor::IsPinName(const FString& InName) const
{
	if (InName.Contains("GraphPin") || InName.Contains("BehaviorTreePin") || InName.Contains("OutputPin") || InName.Contains("SEnvironmentQueryPin"))
	{
		return true;
	}
	return false;
}


SGraphPanel* NGAInputProcessor::GetGraphPanel(const FPointerEvent& MouseEvent)
{
	FArrangedWidget arrangedGraphPanel = GetArrangedGraphPanel(MouseEvent);
	return static_cast<SGraphPanel*>(&arrangedGraphPanel.Widget.Get());
}


FSlateRect NGAInputProcessor::GetWidgetRect(FGeometry Geometry)
{
#if ENGINE_MINOR_VERSION > 16
	return Geometry.GetLayoutBoundingRect();
#else
	return Geometry.GetClippingRect();
#endif
}


SGraphPanel* NGAInputProcessor::GetCurrentGraphPanel()
{
	FSlateApplication& SlateApp = FSlateApplication::Get();
	FWidgetPath widgetsUnderCursor = SlateApp.LocateWindowUnderMouse(SlateApp.GetPlatformCursor()->GetPosition(), SlateApp.GetInteractiveTopLevelWindows());
	for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
	{
		if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString() == "SGraphPanel")
		{
			return static_cast<SGraphPanel*>(&widgetsUnderCursor.Widgets[i].Widget.Get());
		}
	}
	return nullptr;
}


FVector2D NGAInputProcessor::GraphPosToScreenPos(FArrangedWidget ArrangedPanel, FVector2D PanelPos)
{
	auto graphPanel = static_cast<SGraphPanel*>(&ArrangedPanel.Widget.Get());
	if (graphPanel)
	{
		PanelPos = (PanelPos - graphPanel->GetViewOffset()) * graphPanel->GetZoomAmount();
		return ArrangedPanel.Geometry.LocalToAbsolute(PanelPos);
	}
	return PanelPos;
}


FVector2D NGAInputProcessor::ScreenPosToGraphPos(FArrangedWidget ArrangedPanel, FVector2D ScreenPos)
{
	auto graphPanel = static_cast<SGraphPanel*>(&ArrangedPanel.Widget.Get());
	if (graphPanel)
	{
		auto ZoomStartOffset = ArrangedPanel.Geometry.AbsoluteToLocal(ScreenPos);
		return graphPanel->PanelCoordToGraphCoord(ZoomStartOffset);
	}
	return ScreenPos;
}


bool NGAInputProcessor::IsCursorOnEmptySpace(const FPointerEvent& MouseEvent) const
{
	FSlateApplication& slateApp = FSlateApplication::Get();
	FWidgetPath widgetsUnderCursor = slateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), slateApp.GetInteractiveTopLevelWindows());
	int32 widgetNum = widgetsUnderCursor.Widgets.Num();

	//panel is the last widget means click on empty space.
	if (widgetNum > 0 && widgetsUnderCursor.Widgets.Last().Widget->GetTypeAsString() == "SGraphPanel")
	{
		return true;
	}

	//if SComment is the second last widget,the cursor must be inside the blank area of the comment frame.
	if (widgetNum > 1)
	{
		FString widgetName = widgetsUnderCursor.Widgets[widgetNum - 2].Widget->GetTypeAsString();
		if (widgetName == "SGraphNodeComment" || widgetName == "SGraphNodeMaterialComment")
		{
			return true;
		}
	}

	return false;
}


bool NGAInputProcessor::IsDraggingConnection() const
{
	FSlateApplication& slateApp = FSlateApplication::Get();
	return slateApp.IsDragDropping() && slateApp.GetDragDroppingContent()->IsOfType<FDragConnection>();
}


//remember to call when block mouse up event.
void NGAInputProcessor::CancelDraggingReset(int32 UserIndex)
{
	FSlateApplication& SlateApp = FSlateApplication::Get();
	TSharedPtr< FDragDropOperation > LocalDragDropContent = SlateApp.GetDragDroppingContent();
	if (LocalDragDropContent.IsValid())
	{
		SlateApp.CancelDragDrop();
		LocalDragDropContent->OnDrop(true, FPointerEvent());
	}
	SlateApp.ReleaseMouseCaptureForUser(UserIndex);
	SlateApp.GetPlatformApplication()->SetCapture(nullptr);
	if (SlateApp.GetPlatformCursor().IsValid())
	{
		SlateApp.GetPlatformCursor()->Show(true);
	}
}


bool NGAInputProcessor::CanBeginOrEndPan(const FPointerEvent& MouseEvent) const
{
	if (IsDraggingConnection())
	{
//from 4.17.engine supports panning button configuration.
#if ENGINE_MINOR_VERSION > 16
		if (GetDefault<UGraphEditorSettings>()->PanningMouseButton == EGraphPanningMouseButton::Both)
		{
			if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
			{
				return true;
			}
			if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
			{
				return true;
			}
		}
		else if (GetDefault<UGraphEditorSettings>()->PanningMouseButton == EGraphPanningMouseButton::Right)
		{
			if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
			{
				return true;
			}
		}
		else if (GetDefault<UGraphEditorSettings>()->PanningMouseButton == EGraphPanningMouseButton::Middle)
		{
			if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
			{
				return true;
			}
		}
#else
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			return true;
		}
#endif

	}
	return false;
}


bool NGAInputProcessor::IsDragNPanning(const FPointerEvent & MouseEvent) const
{
	if (!MouseEvent.GetEffectingButton().IsValid())
	{
		if (IsDraggingConnection())
		{
			//from 4.17.engine supports panning button configuration.
#if ENGINE_MINOR_VERSION > 16
			if (GetDefault<UGraphEditorSettings>()->PanningMouseButton == EGraphPanningMouseButton::Both)
			{
				if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
				{
					return true;
				}
				if (MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
				{
					return true;
				}
			}
			else if (GetDefault<UGraphEditorSettings>()->PanningMouseButton == EGraphPanningMouseButton::Right)
			{
				if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
				{
					return true;
				}
			}
			else if (GetDefault<UGraphEditorSettings>()->PanningMouseButton == EGraphPanningMouseButton::Middle)
			{
				if (MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
				{
					return true;
				}
			}
#else
			if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
			{
				return true;
			}
#endif
		}
	}
	return false;
}


bool NGAInputProcessor::IsTryingToCutOffWire(const FPointerEvent& MouseEvent) const
{
	if (!IsDraggingConnection())
	{
		if (MouseEvent.IsAltDown() && !MouseEvent.IsShiftDown() && !MouseEvent.IsControlDown() && !MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
		{
			//alt+drag to orbit in viewport can get cursor to move inside panel;
			FSlateApplication& SlateApp = FSlateApplication::Get();
			if (SlateApp.GetPlatformCursor().IsValid() && SlateApp.GetPlatformCursor()->GetType() != EMouseCursor::None)
			{
				//check if inside panel first,otherwise user will not be able to alt+drag to duplicate actor or alt+drag to orbit around actor in level editor viewport,
				//because FEditorViewportClient::Tick gets skipped because entering responsive mode in below code.
				if (IsCursorInsidePanel(MouseEvent))
				{
					if (GetDefault<UNodeGraphAssistantConfig>()->DragCutOffWireMouseButton == ECutOffMouseButton::Middle)
					{
						if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton || MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
						{
							return true;
						}
					}
					if (GetDefault<UNodeGraphAssistantConfig>()->DragCutOffWireMouseButton == ECutOffMouseButton::Left)
					{
						if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton || MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}


//shift click to replicate connections.
bool NGAInputProcessor::TryProcessAsDupliWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.IsShiftDown() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (IsCursorInsidePanel(MouseEvent) && !SlateApp.IsDragDropping())
		{
			if (GetGraphPanel(MouseEvent)->GetGraphObj()->GetFName().ToString().Contains("ReferenceViewer"))
			{
				return false;
			}

			FWidgetPath widgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
			FScopedSwitchWorldHack SwitchWorld(widgetsUnderCursor);

			for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
			{
				if (IsPinName(widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString()))
				{
					TSharedRef<SGraphPin> curPin = StaticCastSharedRef<SGraphPin>(widgetsUnderCursor.Widgets[i].Widget);
					TSharedRef<SGraphPanel> curPanel = StaticCastSharedRef<SGraphPanel>(GetArrangedGraphPanel(MouseEvent).Widget);
					TArray<FGraphPinHandle> linkedPins;

					for (auto linkedPin : curPin->GetPinObj()->LinkedTo)
					{
						linkedPins.AddUnique(linkedPin);
					}
					if (linkedPins.Num() > 0)
					{
						TSharedPtr<FDragDropOperation> DragEvent = FDragConnection::New(curPanel, linkedPins);
						SlateApp.ProcessReply(widgetsUnderCursor, FReply::Handled().BeginDragDrop(DragEvent.ToSharedRef()), &widgetsUnderCursor, &MouseEvent, MouseEvent.GetUserIndex());
						return true;
					}
				}
			}
		}
	}
	return false;
}


//by default panning is bypassed when dragging connection(slate.cpp 5067 4.18), to be able to pan while dragging,need to manually route the pointer down event to graph panel.
//make sure no widget captured the mouse so we can pan now.
bool NGAInputProcessor::TryProcessAsBeginDragNPanEvent(const FPointerEvent& MouseEvent)
{
	FSlateApplication& SlateApp = FSlateApplication::Get();

	if (CanBeginOrEndPan(MouseEvent) && !SlateApp.HasUserMouseCapture(MouseEvent.GetUserIndex()))
	{
		FArrangedWidget arrangedGraphPanel = GetArrangedGraphPanel(MouseEvent);
		SGraphPanel* graphPanel = static_cast<SGraphPanel*>(&arrangedGraphPanel.Widget.Get());
		if (!graphPanel)
		{
			return false;
		}

		//this hand made event exclude left mouse button as pressed,so node panel wont display software cursor as "up-down" icon(node panel thinks we are going to zoom,but we are not).
		TSet<FKey> keys;
		keys.Add(MouseEvent.GetEffectingButton());
		FPointerEvent mouseEvent(
			MouseEvent.GetPointerIndex(),
			MouseEvent.GetScreenSpacePosition(),
			MouseEvent.GetLastScreenSpacePosition(),
			keys,
			MouseEvent.GetEffectingButton(),
			0,
			FModifierKeysState()
		);

		//enable panning.
		FWidgetPath widgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
		FReply reply = SlateApp.RoutePointerDownEvent(widgetsUnderCursor, mouseEvent);
		if (reply.IsEventHandled())
		{
			if (reply.ShouldThrottle() && !MouseButtonDownResponsivnessThrottle.IsValid())
			{
				MouseButtonDownResponsivnessThrottle = FSlateThrottleManager::Get().EnterResponsiveMode();
			}
			else if (!reply.ShouldThrottle() && MouseButtonDownResponsivnessThrottle.IsValid())
			{
				// Leave responsive mode if a widget choose not to throttle
				FSlateThrottleManager::Get().LeaveResponsiveMode(MouseButtonDownResponsivnessThrottle);
			}

			LastPanelScreenSpaceRect = GetWidgetRect(arrangedGraphPanel.Geometry);

			//[correct wire position]software cursor is in panel space,but preview connection wire is in screen space,when drag and panning,their positions will become mismatched.
			//to fix that we need to cache the software cursor panel position,so we can convert it to correct screen space position when drag and panning to keep connection wire aligned with it.
			LastGraphCursorGraphPos = ScreenPosToGraphPos(arrangedGraphPanel, MouseEvent.GetScreenSpacePosition());

			return true;
		}
	}
	return false;
}


//update visual elements(wire.cursor icon...) for drag and panning,or they will be at mismatched position.
void NGAInputProcessor::TryProcessAsBeingDragNPanEvent(const FPointerEvent& MouseEvent)
{
	FSlateApplication& SlateApp = FSlateApplication::Get();

	if (IsDragNPanning(MouseEvent))
	{
		FArrangedWidget arrangedGraphPanel = GetArrangedGraphPanel(MouseEvent);
		SGraphPanel* graphPanel = static_cast<SGraphPanel*>(&arrangedGraphPanel.Widget.Get());

		//when dragging and panning towards outside of the panel fast,the mouse move event will be picked up by another widget rather than graph panel.
		//check if still inside same panel.
		if (graphPanel && GetWidgetRect(arrangedGraphPanel.Geometry) == LastPanelScreenSpaceRect)
		{
			LastGraphCursorScreenPosClamped = GraphPosToScreenPos(arrangedGraphPanel, LastGraphCursorGraphPos);
		}
		else
		{
			LastGraphCursorScreenPosClamped = MouseEvent.GetScreenSpacePosition();
		}
		
		//when cursor is getting close to the edge of the panel,drag operation will send deferred pan request to accelerate pan speed, no need that.
		LastGraphCursorScreenPosClamped = FVector2D
		(
			FMath::Clamp(LastGraphCursorScreenPosClamped.X, LastPanelScreenSpaceRect.Left + 40.0f, LastPanelScreenSpaceRect.Right - 40.0f),
			FMath::Clamp(LastGraphCursorScreenPosClamped.Y, LastPanelScreenSpaceRect.Top + 40.0f, LastPanelScreenSpaceRect.Bottom - 40.0f)
		);

		TSet<FKey> keys;
		if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
		{
			keys.Add(EKeys::RightMouseButton);
		}
		if (MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
		{
			keys.Add(EKeys::MiddleMouseButton);
		}
		FPointerEvent mouseEvent(
			MouseEvent.GetPointerIndex(),
			LastGraphCursorScreenPosClamped, //[drag-pan]use clamped position,so deferred pan amount will be 0.
			LastGraphCursorScreenPosClamped,
			MouseEvent.GetCursorDelta(),//original delta is needed for panning.
			keys,
			FModifierKeysState()
		);

		//call OnDragOver with correct screen space software cursor position to update connection wire endpoint position.
		if (graphPanel)
		{
			graphPanel->OnDragOver(arrangedGraphPanel.Geometry, FDragDropEvent(mouseEvent, SlateApp.GetDragDroppingContent()));
		}

		//by default both mouse button down will zoom panel view,which we don't want when drag and panning,so override the event to trick graph panel to think we are panning.
		//one side effect is this will permanently remove left mouse from pressed state of slate until next time left mouse button is down.
		if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			const_cast<FPointerEvent&>(MouseEvent) = mouseEvent;
		}

		SlateApp.SetCursorPos(LastGraphCursorScreenPosClamped);
		if (SlateApp.GetPlatformCursor().IsValid())
		{
			SlateApp.GetPlatformCursor()->Show(false);
			DidIHideTheCursor = true;
		}

		SlateApp.GetDragDroppingContent()->SetDecoratorVisibility(false);
	}
	return;
}


//middle mouse double click select linked node.
bool NGAInputProcessor::TryProcessAsSelectStreamEvent(const FPointerEvent& MouseEvent)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EanbleSelectStream)
	{
		return false;
	}
	if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton && !IsCursorOnEmptySpace(MouseEvent))
	{
		FSlateApplication& SlateApp = FSlateApplication::Get();
		FWidgetPath widgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
		FScopedSwitchWorldHack SwitchWorld(widgetsUnderCursor);

		for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
		{
			if (IsPinName(widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString()))
			{
				return false;
			}

			if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString().Contains("GraphNode"))
			{
				SGraphPanel* graphPanel = GetGraphPanel(MouseEvent);
				if (graphPanel)
				{
					TSharedRef<SGraphNode> clickedSNode = StaticCastSharedRef<SGraphNode>(widgetsUnderCursor.Widgets[i].Widget);
					UEdGraphNode* clickedNode = clickedSNode->GetNodeObj();
					if (!clickedNode)
					{
						return false;
					}
					graphPanel->SelectionManager.SetNodeSelection(clickedNode, true);

#if ENGINE_MINOR_VERSION > 16
					FSlateRect nodeRect = widgetsUnderCursor.Widgets[i].Geometry.GetLayoutBoundingRect();
#else
					FSlateRect nodeRect = widgetsUnderCursor.Widgets[i].Geometry.GetClippingRect();
#endif

					//select up stream or down stream depending on which side of the node cursor is on.
					bool downStream = false;
					bool upstream = false;
					if (MouseEvent.GetScreenSpacePosition().X - nodeRect.Left > nodeRect.Right - MouseEvent.GetScreenSpacePosition().X)
					{
						downStream = true;
						upstream = false;
					}
					else
					{
						downStream = false;
						upstream = true;
					}

					TArray<UEdGraphNode*> nodesToCheck;
					nodesToCheck.Add(clickedNode);
					for (auto nodeToSelect : FNodeHelper::GetLinkedNodes(nodesToCheck, downStream, upstream))
					{
						graphPanel->SelectionManager.SetNodeSelection(nodeToSelect, true);
					}
					return true;
				}
				break;
			}
		}
	}
	return false;
}


//multi-connect,by routing drop event without destroying drag-drop operation
bool NGAInputProcessor::TryProcessAsMultiConnectEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (IsDraggingConnection() && !MouseEvent.IsAltDown())
	{
		SGraphPanel* graphPanel = GetCurrentGraphPanel();
		if (!graphPanel)
		{
			return false;
		}
		
		
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			HasMouseUpAfterDrag = true;
		}
		
		//drag and pan.
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && (MouseEvent.GetScreenSpacePosition() - LastMouseDownScreenPos).Size() > 10)
		{
			SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
			if (SlateApp.GetPlatformCursor().IsValid())
			{
				SlateApp.GetPlatformCursor()->Show(true);
			}

			return true;
		}

		FWidgetPath widgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
		FScopedSwitchWorldHack SwitchWorld(widgetsUnderCursor);

		//decide what to do depend on click on different widget.
		for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
		{
			FString widgetName = widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString();

			if (IsPinName(widgetName))
			{
				widgetsUnderCursor.Widgets[i].Widget->OnDrop(widgetsUnderCursor.Widgets[i].Geometry, FDragDropEvent(MouseEvent, SlateApp.GetDragDroppingContent()));
				if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
				{
					SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
					return true;
				}
				break;
			}
			if (widgetName.Contains("GraphNode") && widgetName != "SGraphNodeComment" && widgetName != "SGraphNodeMaterialComment")
			{
				if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					TryProcessAsLazyConnectEvent(SlateApp, MouseEvent);

					//lazy connect happens in draw event,so cancel after next draw
					int* timer = new int(1);
					NGADeferredEventDele deferredDele;
					deferredDele.BindLambda([this, MouseEvent, timer]()
					{
						if (*timer == 0)
						{
							delete timer;
							if (!MouseEvent.IsShiftDown() && (MouseEvent.GetScreenSpacePosition() - LastMouseDownScreenPos).Size() > 10)
							{
								CancelDraggingReset(MouseEvent.GetUserIndex());
							}
							return true;
						}
						else
						{
							(*timer)--;
							return false;
						}
					});
					TickEventListener.Add(deferredDele);
					return true;
				}
			}
			//decide what to do when drop on empty space,cancel or summon menu.
			if (widgetName == "SGraphPanel")
			{
				//if not dropping on any pin,right mouse button cancel dragging,left mouse button summon context menu.
				if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !MouseEvent.IsShiftDown())
				{
					return false;
				}
				break;
			}
		}

		if (MouseEvent.IsControlDown())
		{
			CancelDraggingReset(MouseEvent.GetUserIndex());
			return true;
		}
		//right click cancel.
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && (MouseEvent.GetScreenSpacePosition() - LastMouseDownScreenPos).Size() < 10 && !MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			CancelDraggingReset(MouseEvent.GetUserIndex());
			return true;
		}
		if (MouseEvent.IsShiftDown())
		{
			return true;
		}
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && (MouseEvent.GetScreenSpacePosition() - LastMouseDownScreenPos).Size() < 10)
		{
			return true;
		}
		
		CancelDraggingReset(MouseEvent.GetUserIndex());
		return true;
	}
	return false;
}


// double click on pin to highlight all wires that connect to this pin.
bool NGAInputProcessor::TryProcessAsClusterHighlightEvent(const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		FSlateApplication& SlateApp = FSlateApplication::Get();
		FWidgetPath widgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
		FScopedSwitchWorldHack SwitchWorld(widgetsUnderCursor);

		for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
		{
			//add the double clicked graph pin to our collection.
			if (IsPinName(widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString()))
			{
				SGraphPanel* graphPanel = GetGraphPanel(MouseEvent);
				if (graphPanel)
				{
					auto sourcePin = TWeakPtr<SGraphPin>(StaticCastSharedRef<SGraphPin>(widgetsUnderCursor.Widgets[i].Widget));

					HighLightedPins.Add(sourcePin);
					graphPanel->MarkedPin = sourcePin;

					return true;
				}
				break;
			}
		}
	}
	return false;
}


//todo:use connection factory to set pin highlight.
bool NGAInputProcessor::TryProcessAsSingleHighlightEvent(const FPointerEvent& MouseEvent)
{
	if (IsCursorOnEmptySpace(MouseEvent) && !IsDraggingConnection() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) && !MouseEvent.IsAltDown())
	{
		if ((MouseEvent.GetScreenSpacePosition() - LastMouseDownScreenPos).Size() < 10)
		{
			FArrangedWidget arrangedGraphPanel = GetArrangedGraphPanel(MouseEvent);
			SGraphPanel* graphPanel = static_cast<SGraphPanel*>(&arrangedGraphPanel.Widget.Get());
			if (!graphPanel)
			{
				return false;
			}

			graphPanel->MarkedPin.Reset();

			//convert regular click on empty space to shift click,so graph panel will set connection wire under cursor as marked and highlight it,so we don't need to press "shift".
			TSet<FKey> keys;
			keys.Add(EKeys::LeftMouseButton);
			FPointerEvent fakeMouseEvent(
				MouseEvent.GetPointerIndex(),
				MouseEvent.GetScreenSpacePosition(),
				MouseEvent.GetLastScreenSpacePosition(),
				keys,
				EKeys::LeftMouseButton,
				0,
				FModifierKeysState(true, false, false, false, false, false, false, false, false)
			);

			FSlateApplication& SlateApp = FSlateApplication::Get();
			FWidgetPath WidgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
			SlateApp.RoutePointerDownEvent(WidgetsUnderCursor, fakeMouseEvent);
			FReply reply = SlateApp.RoutePointerUpEvent(WidgetsUnderCursor, fakeMouseEvent);

			if (reply.IsEventHandled())
			{
				//graph panel can support constant highlight for one wire and tempt hover highlight for multiple wires,
				//in order to constantly highlight multiple wires,I have to keep a list of constant highlighted wires,and squeeze them into graph panel's hover set.
				if (graphPanel->MarkedPin.IsValid())// wire detected
				{
					if (MouseEvent.IsShiftDown())
					{
						if (HighLightedPins.Contains(graphPanel->MarkedPin))
						{
							HighLightedPins.Remove(graphPanel->MarkedPin);
							//keep marked pin reference alive,so we can have instant hover highlight.
							if (HighLightedPins.Num() > 0)
							{
								graphPanel->MarkedPin = HighLightedPins.Array().Last();
							}
							else
							{
								graphPanel->MarkedPin.Reset();
							}
						}
						else
						{
							HighLightedPins.Add(graphPanel->MarkedPin);
						}
						
					}
					else
					{
						for (auto pin : HighLightedPins)
						{
							if (pin.IsValid())
							{								
								graphPanel->RemovePinFromHoverSet(pin.Pin()->GetPinObj());
							}
						}
						HighLightedPins.Empty();
						HighLightedPins.Add(graphPanel->MarkedPin);
					}
				}
				else
				{
					//click on empty space remove all highlighted wires.
					TSet< TSharedRef<SWidget>> allPins;
					graphPanel->GetAllPins(allPins);
					for (auto pin: allPins)
					{
						graphPanel->RemovePinFromHoverSet(StaticCastSharedRef<SGraphPin>(pin)->GetPinObj());
					}
					//make sure clear pin from other graph from current graph
					//if unrecognized pin was send to graph,graph will dim other wires.
					for (auto pin : HighLightedPins)
					{
						if (pin.IsValid())
						{
							graphPanel->RemovePinFromHoverSet(pin.Pin()->GetPinObj());
						}
					}
					HighLightedPins.Empty();
				}

				//click on wire will create mouse capture,causing graph pin to be not clickable at first click,so release capture.
				SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
				SlateApp.GetPlatformApplication()->SetCapture(nullptr);
				return true;
			}
		}
	}
	return false;
}


bool NGAInputProcessor::TryProcessAsBeginCutOffWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EanbleCutoffWire)
	{
		return false;
	}

	if (!HasBegunCuttingWire && IsTryingToCutOffWire(MouseEvent))
	{
		HasBegunCuttingWire = true;

		//currently-hovered wire gets updated every slate tick,so no way to cut off multiple wires in one tick.
		//boost slate fps so we have less chance to miss a wire when cursor moves too fast.
		if (!MouseButtonDownResponsivnessThrottle.IsValid())
		{
			MouseButtonDownResponsivnessThrottle = FSlateThrottleManager::Get().EnterResponsiveMode();
		}

		if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
		{
			GEditor->BeginTransaction(TEXT(""), NSLOCTEXT("NodeGraphAssistant", "CutOffWire", "Cut off wire"), NULL);
		}

		if (!CutOffCusor.IsValid())
		{
			CutOffCusor = MakeShareable(new FHardwareCursor(IPluginManager::Get().FindPlugin(TEXT("NodeGraphAssistant"))->GetBaseDir() / TEXT("Resources")/TEXT("nga_cutoff_cursor"), FVector2D(0, 0)));
		}

		return true;
	}
	return false;
}


//trigger alt-mouse down event when move to cut off wires.
//to do:no undo if no wire cut.
void NGAInputProcessor::TryProcessAsCutOffWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (HasBegunCuttingWire)
	{
		if (IsTryingToCutOffWire(MouseEvent))
		{
			FArrangedWidget arrangedGraphPanel = GetArrangedGraphPanel(MouseEvent);
			SGraphPanel* graphPanel = static_cast<SGraphPanel*>(&arrangedGraphPanel.Widget.Get());
			if (!graphPanel)
			{
				return;
			}

			FPointerEvent mouseEvent(
				MouseEvent.GetPointerIndex(),
				MouseEvent.GetScreenSpacePosition(),
				MouseEvent.GetLastScreenSpacePosition(),
				TSet<FKey>(),
				EKeys::LeftMouseButton,
				0,
				FModifierKeysState(false, false, false, false, true, true, false, false, false));
			graphPanel->OnMouseButtonDown(arrangedGraphPanel.Geometry, mouseEvent);

			if (SlateApp.GetPlatformCursor().IsValid())
			{
				SlateApp.GetPlatformCursor()->SetTypeShape(EMouseCursor::Custom, CutOffCusor->GetHandle());
				SlateApp.GetPlatformCursor()->SetType(EMouseCursor::Custom);
			}

			return;
		}
		else
		{
			TryProcessAsEndCutOffWireEvent(SlateApp, MouseEvent);
			SlateApp.OnMouseUp(EMouseButtons::Middle);
		}
	}
	return;
}


//end undo transaction.
void NGAInputProcessor::TryProcessAsEndCutOffWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (HasBegunCuttingWire && GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
	{
		HasBegunCuttingWire = false;
		if (GEditor->IsTransactionActive())
		{
			GEditor->EndTransaction();

			//its possible cut off wire action will create too many transactions that put undo on hold that cause undo to be unusable.
			for (int i = 0; i < static_cast<UTransBuffer*>(GEditor->Trans)->ActiveCount; i++)
			{
				static_cast<UTransBuffer*>(GEditor->Trans)->End();
			}
		}
		if (SlateApp.GetPlatformCursor().IsValid())
		{
			SlateApp.GetPlatformCursor()->SetType(EMouseCursor::Default);
		}
	}
	return;
}


//try to link selected nodes' inputs to their outputs, and attempt to remove nodes.
void NGAInputProcessor::TryProcessAsBypassNodesEvent()
{
	SGraphPanel* graphPanel = GetCurrentGraphPanel();
	if (!graphPanel)
	{
		return;
	}
	if (graphPanel->GetGraphObj()->IsA(UAnimationStateMachineGraph::StaticClass()))
	{
		return;
	}
	if (graphPanel->GetGraphObj()->GetFName().ToString().Contains("ReferenceViewer"))
	{
		return;
	}
	if (graphPanel->GetGraphObj()->GetFName().ToString().Contains("PhysicsAssetGraph"))
	{
		return;
	}

	TArray<UEdGraphNode*> selectedNodes;
	for (auto selectedNode : graphPanel->SelectionManager.SelectedNodes)
	{
		UEdGraphNode* graphNode = Cast<UEdGraphNode>(selectedNode);
		if (!graphNode)
		{
			continue;
		}
		selectedNodes.Add(graphNode);
	}
	if (selectedNodes.Num() < 1)
	{
		return;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("NodeGraphAssistant", "BypassNodes", "Bypass node"));

	if (!FNodeHelper::BypassNodes(selectedNodes))
	{
		FNotificationInfo Info(NSLOCTEXT("NodeGraphAssistant","BypassNodes", "Can not fully bypass node"));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
	return;
}


bool NGAInputProcessor::TryProcessAsInsertNodeEvent(const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && !MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		if (IsDraggingConnection())
		{
			return false;
		}
		if (!IsCursorOnEmptySpace(MouseEvent))
		{
			return false;
		}
		if ((MouseEvent.GetScreenSpacePosition() - LastMouseDownScreenPos).Size() > 5)
		{
			return false;
		}

		FArrangedWidget arrangedGraphPanel = GetArrangedGraphPanel(MouseEvent);
		SGraphPanel* graphPanel = static_cast<SGraphPanel*>(&arrangedGraphPanel.Widget.Get());
		if (!graphPanel)
		{
			return false;
		}

		if (graphPanel->GetGraphObj()->IsA(UAnimationStateMachineGraph::StaticClass()))
		{
			return false;
		}

		TWeakPtr<SGraphPin> tempPin = graphPanel->MarkedPin;
		graphPanel->MarkedPin.Reset();

		TSet<FKey> keys;
		keys.Add(EKeys::LeftMouseButton);
		FPointerEvent fakeMouseEvent(
			MouseEvent.GetPointerIndex(),
			MouseEvent.GetScreenSpacePosition(),
			MouseEvent.GetLastScreenSpacePosition(),
			keys,
			EKeys::LeftMouseButton,
			0,
			FModifierKeysState(true, false, false, false, false, false, false, false, false)
		);

		FReply reply = graphPanel->OnMouseButtonUp(arrangedGraphPanel.Geometry, fakeMouseEvent);
		if (!reply.IsEventHandled() || !graphPanel->MarkedPin.IsValid())
		{
			graphPanel->MarkedPin = tempPin;
			return false;
		}

		if (GetDefault<UNodeGraphAssistantConfig>()->InsertNodeOnlyIfWireSelected && HighLightedPins.Array().Find(graphPanel->MarkedPin) == INDEX_NONE)
		{
			graphPanel->MarkedPin = tempPin;
			return false;
		}

		if (graphPanel->MarkedPin.Pin().Get()->GetPinObj()->LinkedTo.Num() == 0)
		{
			return false;
		}

		SGraphPin* markedPinA = graphPanel->MarkedPin.Pin().Get();
		SGraphPin* markedPinB = graphPanel->GetNodeWidgetFromGuid(markedPinA->GetPinObj()->LinkedTo[0]->GetOwningNode()->NodeGuid)->FindWidgetForPin(markedPinA->GetPinObj()->LinkedTo[0]).Get();
		graphPanel->MarkedPin = tempPin;

		if (markedPinA->GetPinObj()->LinkedTo.Num() > 1)
		{
			for (auto linkedPin : markedPinA->GetPinObj()->LinkedTo)
			{
				markedPinB = graphPanel->GetNodeWidgetFromGuid(linkedPin->GetOwningNode()->NodeGuid)->FindWidgetForPin(linkedPin).Get();
				//should be good enough.
				if (FNodeHelper::GetWirePointHitResult(arrangedGraphPanel, markedPinA, markedPinB, MouseEvent.GetScreenSpacePosition(), 7, GetDefault<UGraphEditorSettings>()))
				{
					break;
				}
			}
		}

		UEdGraphPin* pinA;
		UEdGraphPin* pinB;
		if (markedPinA->GetDirection() == EGPD_Output && markedPinB->GetDirection() == EGPD_Input)
		{
			pinA = markedPinA->GetPinObj();
			pinB = markedPinB->GetPinObj();
		}
		else if (markedPinA->GetDirection() == EGPD_Input && markedPinB->GetDirection() == EGPD_Output)
		{
			pinA = markedPinB->GetPinObj();
			pinB = markedPinA->GetPinObj();
		}
		else { return false; }

		UEdGraphNode* selectedNode = nullptr;
		if (graphPanel->SelectionManager.SelectedNodes.Num() == 1)
		{
			UEdGraphNode* graphNode = Cast<UEdGraphNode>(graphPanel->SelectionManager.SelectedNodes.Array()[0]);
			if (graphNode)
			{
				selectedNode = graphNode;
			}
		}

		const FVector2D nodeAddPosition = ScreenPosToGraphPos(arrangedGraphPanel,MouseEvent.GetScreenSpacePosition());
		TArray<UEdGraphPin*> sourcePins;
		sourcePins.Add(pinA);
		TSharedPtr<SWidget> WidgetToFocus = graphPanel->SummonContextMenu(MouseEvent.GetScreenSpacePosition(), nodeAddPosition, nullptr, nullptr, sourcePins);
		if (!WidgetToFocus.IsValid())
		{
			return false;
		}
		FSlateApplication::Get().SetUserFocus(MouseEvent.GetUserIndex(), WidgetToFocus);

		if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
		{
			GEditor->BeginTransaction(TEXT(""), NSLOCTEXT("NodeGraphAssistant", "InsertNode", "Insert node"), NULL);
		}

		int* tryTimes = new int(2);
		NGADeferredEventDele deferredDele;
		deferredDele.BindLambda([selectedNode, graphPanel, pinA, pinB](int* tryTimes)
		{
			//only exit
			if (*tryTimes == 0)
			{
				if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
				{
					if (GEditor->IsTransactionActive())
					{
						GEditor->EndTransaction();
					}
				}
				return true;
			}
			if (!FSlateApplication::Get().AnyMenusVisible())
			{
				if (graphPanel && graphPanel->SelectionManager.SelectedNodes.Num() == 1)
				{
					//selection set might still be the old one(blueprint graph),so don't do it right away.
					if (*tryTimes == 2)
					{
						(*tryTimes)--;
						return false;
					}
					UEdGraphNode* newSelectedNode = Cast<UEdGraphNode>(graphPanel->SelectionManager.SelectedNodes.Array()[0]);
					if (newSelectedNode && newSelectedNode != selectedNode)
					{
						bool linkSuccess = false;
						for (auto newNodePin : newSelectedNode->Pins)
						{
							if (newNodePin->Direction == EGPD_Output)
							{
								if (newSelectedNode->GetSchema()->TryCreateConnection(newNodePin, pinB))
								{
									linkSuccess = true;
									FNodeHelper::BreakSinglePinLink(pinA, pinB);
									break;
								}
							}
						}
						if (!linkSuccess)
						{
							FNotificationInfo Info(NSLOCTEXT("NodeGraphAssistant", "InsertNode", "Can not insert this node"));
							Info.ExpireDuration = 3.0f;
							FSlateNotificationManager::Get().AddNotification(Info);
						}
					}
				}
				(*tryTimes)--;
			}
			return false;
		}, tryTimes);
		TickEventListener.Add(deferredDele);

		CancelDraggingReset(MouseEvent.GetUserIndex());
		return true;
	}

	return false;
}


//rearrange selected nodes into grid based structure.
//see documentation.
void NGAInputProcessor::TryProcessAsRearrangeNodesEvent()
{
	SGraphPanel* graphPanel = GetCurrentGraphPanel();
	if (graphPanel)
	{
		if (graphPanel->GetGraphObj()->IsA(UAnimationStateMachineGraph::StaticClass()))
		{
			FNotificationInfo Info(NSLOCTEXT("NodeGraphAssistant", "StateMachineGraphRearrange", "Can not rearrange nodes for state machine graph"));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
			return;
		}
		if (graphPanel->GetGraphObj()->GetFName().ToString().Contains("ReferenceViewer"))
		{
			FNotificationInfo Info(NSLOCTEXT("NodeGraphAssistant", "RederenceGraphRearrange", "Can not rearrange nodes for referencer graph"));
			Info.ExpireDuration = 3.0f;
			FSlateNotificationManager::Get().AddNotification(Info);
			return;
		}

		const FScopedTransaction Transaction(NSLOCTEXT("NodeGraphAssistant", "RearrangeNodes", "Rearrange nodes"));
		if (graphPanel->GetGraphObj()->IsA(UAIGraph::StaticClass()))
		{
			FIntPoint rearrangeSpacing = GetDefault<UNodeGraphAssistantConfig>()->NodesRearrangeSpacingAIGraph;
			FNodeHelper::RearrangeSelectedNodes_AIGraph(GetCurrentGraphPanel(), rearrangeSpacing, GetDefault<UNodeGraphAssistantConfig>()->NodesRearrangeSpacingRelaxFactor);
		}
		else
		{
			FIntPoint rearrangeSpacing = GetDefault<UNodeGraphAssistantConfig>()->NodesRearrangeSpacing;
			if (graphPanel->GetGraphObj()->GetClass() == UEdGraph::StaticClass())
			{
				//ed graph nodes become smaller when zoomed out,cause nodes to overlap when zoom in after rearrange.
				if (GetCurrentGraphPanel()->GetZoomAmount() < 0.22)
				{
					rearrangeSpacing.X += 100;
					rearrangeSpacing.Y += 30;
				}
				else if (GetCurrentGraphPanel()->GetZoomAmount() < 0.37)
				{
					rearrangeSpacing.X += 5;
					rearrangeSpacing.Y += 30;
				}
			}
			//todo
			FNodeHelper::RearrangeSelectedNodes(GetCurrentGraphPanel(), rearrangeSpacing, GetDefault<UNodeGraphAssistantConfig>()->NodesRearrangeSpacingRelaxFactor);
			FNodeHelper::RearrangeSelectedNodes(GetCurrentGraphPanel(), rearrangeSpacing, GetDefault<UNodeGraphAssistantConfig>()->NodesRearrangeSpacingRelaxFactor);
		}
	}
	return;
}


//todo:no undo if no wire connected.
void NGAInputProcessor::TryProcessAsBeginLazyConnectEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && MouseEvent.IsShiftDown())
	{
		if (IsDraggingConnection())
		{
			if (!HasBegunLazyConnect)
			{
				HasBegunLazyConnect = true;

				if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
				{
					GEditor->BeginTransaction(TEXT(""), NSLOCTEXT("NodeGraphAssistant", "LazyConnect", "Lazy connect"), NULL);
				}
			}
		}
	}
}


//when cursor is above a node while dragging,auto snap connection wire to closest connectible pin of the node.
//will make actual connection if any connect gesture was comitted.
void NGAInputProcessor::TryProcessAsLazyConnectEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EnableLazyConnect)
	{
		return;
	}
	if (IsDraggingConnection())
	{
		SGraphPanel* graphPanel = GetGraphPanel(MouseEvent);
		if (!graphPanel)
		{
			return;
		}
		if (graphPanel->GetGraphObj()->IsA(UAnimationStateMachineGraph::StaticClass()) || graphPanel->GetGraphObj()->IsA(UAIGraph::StaticClass()))
		{
			return;
		}

		FArrangedWidget arrangedNode = GetArrangedGraphNode(MouseEvent);
		if (!(arrangedNode == FArrangedWidget::NullWidget))
		{
			SGraphNode* sGraphnode = static_cast<SGraphNode*>(&arrangedNode.Widget.Get());
			if (!sGraphnode)
			{
				return;
			}
	
			TArray<UEdGraphPin*> draggingPins;
			StaticCastSharedPtr<FDragConnection>(SlateApp.GetDragDroppingContent())->ValidateGraphPinList(draggingPins);

			//not detect self pins.
			bool isStartNode = false;
			for (auto draggingPin : draggingPins)
			{
				if (draggingPin->GetOwningNode() == sGraphnode->GetNodeObj())
				{
					isStartNode = true;
					break;
				}
			}

			if (!isStartNode)
			{
				TSet< TSharedRef<SWidget> > allPins;
				sGraphnode->GetPins(allPins); 
				if (allPins.Num() > 0)
				{
					FNGAGraphPinConnectionFactoryPayLoadData payloadData;
					payloadData.GraphPanel = GetGraphPanel(MouseEvent);
					payloadData.MouseEvent = MouseEvent;
					payloadData.DraggingPins = draggingPins;
					payloadData.NodePins = allPins;
					payloadData.NodeGeometry = arrangedNode.Geometry;
					payloadData.HasMouseUpAfterShiftDrag = HasMouseUpAfterDrag;
					MyGraphPinConnectionFactory->SetPayloadData(payloadData);
					return;
				}
			}
		}
	}
	if ((!MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) ||!MouseEvent.IsShiftDown()) && HasBegunLazyConnect)
	{
		TryProcessAsEndLazyConnectEvent(SlateApp, MouseEvent);
	}

	MyGraphPinConnectionFactory->ResetPayloadData();
	return;
}


void NGAInputProcessor::TryProcessAsEndLazyConnectEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (HasBegunLazyConnect && GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
	{
		HasBegunLazyConnect = false;
		if (GEditor->IsTransactionActive())
		{
			GEditor->EndTransaction();

			////its possible cut off wire action will create too many transactions that put undo on hold that cause undo to be unusable.
			//for (int i = 0; i < static_cast<UTransBuffer*>(GEditor->Trans)->ActiveCount; i++)
			//{
			//	static_cast<UTransBuffer*>(GEditor->Trans)->End();
			//}
		}
	}
}


//store node being dragged
void NGAInputProcessor::TryProcessAsAutoConnectMouseDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (IsCursorInsidePanel(MouseEvent) && !IsDraggingConnection())
		{
			if (!SlateApp.HasAnyMouseCaptor())
			{
				FArrangedWidget arrangedGraphNode = GetArrangedGraphNode(MouseEvent);
				if (!(arrangedGraphNode == FArrangedWidget::NullWidget))
				{
					if (SlateApp.GetPlatformCursor().IsValid())
					{
						//when cursor is CardinalCross,its draggible part of the node
						auto cursorIcon = SlateApp.GetPlatformCursor()->GetType();
						if (cursorIcon == EMouseCursor::CardinalCross || cursorIcon == EMouseCursor::Default)
						{
							NodeBeingDrag = TWeakPtr<SGraphNode>(StaticCastSharedRef<SGraphNode>(arrangedGraphNode.Widget));
						}
					}
				}
			}
		}
	}
}


//when dragging a node,auto detect surrounding pins it can linked to.
//will make connection when release drag.
void NGAInputProcessor::TryProcessAsAutoConnectMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EnableAutoConnect)
	{
		return;
	}
	if (GetDefault<UNodeGraphAssistantConfig>()->AutoConnectModifier == EAutoConnectModifier::Alt && !MouseEvent.IsAltDown())
	{
		return;
	}
	if (GetDefault<UNodeGraphAssistantConfig>()->AutoConnectModifier != EAutoConnectModifier::Alt && MouseEvent.IsAltDown())
	{
		return;
	}
	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !IsDraggingConnection())
	{
		SGraphPanel* graphPanel = GetGraphPanel(MouseEvent);

		if (!graphPanel)
		{
			return;
		}
		if (graphPanel->GetGraphObj()->IsA(UAnimationStateMachineGraph::StaticClass()) || graphPanel->GetGraphObj()->IsA(UAIGraph::StaticClass()))
		{
			return;
		}

		if (graphPanel->HasMouseCaptureByUser(MouseEvent.GetUserIndex()))
		{
			FArrangedWidget arrangedGraphNode = GetArrangedGraphNode(MouseEvent);
			if (NodeBeingDrag.IsValid())
			{
				SlateApp.SetAllowTooltips(false);


				auto curNode = NodeBeingDrag.Pin();
				TSet <TSharedRef<SWidget>> curNodePins;
				curNode->GetPins(curNodePins);
				
				TArray<UEdGraphNode*> alreadyLinkedNodes;
				for (auto pin : curNodePins)
				{
					for (auto linkedPin : StaticCastSharedRef<SGraphPin>(pin)->GetPinObj()->LinkedTo)
					{
						alreadyLinkedNodes.AddUnique(linkedPin->GetOwningNode());
					}
				}

				TArray<TSharedRef<SGraphNode>> targetNodes;
				FChildren* visibleChildren = graphPanel->GetChildren();
				for (int i = 0; i < visibleChildren->Num(); i++)
				{
					auto graphNode = StaticCastSharedRef<SGraphNode>(visibleChildren->GetChildAt(i));
					//not detect already linked node.
					if (!alreadyLinkedNodes.Contains(graphNode->GetNodeObj()))
					{
						if (!graphPanel->SelectionManager.SelectedNodes.Contains(graphNode->GetNodeObj()))
						{
							if (graphNode->GetNodeObj()!= curNode->GetNodeObj())
							{
								targetNodes.AddUnique(graphNode);
							}
						}
					}
				}
				//try to connect pin from top to bottom
				targetNodes.Sort([](TSharedRef<SGraphNode> A, TSharedRef<SGraphNode> B) {return A->GetPosition().Y < B->GetPosition().Y; });

				TArray<TSharedRef<SWidget>> startGraphPinsTemp;
				TArray<TSharedRef<SWidget>> endGraphPinsTemp;
				TArray<float> minDistsTemp;

				for (auto curNodePinWidget : curNodePins)
				{
					TSharedRef<SGraphPin> curNodePin = StaticCastSharedRef<SGraphPin>(curNodePinWidget);
					float minDist = GetDefault<UNodeGraphAssistantConfig>()->AutoConnectRadius;
					bool matched = false;
					FVector2D curPinPos = curNodePin->GetNodeOffset() + curNode->GetPosition();
					curPinPos = curPinPos + FVector2D(curNodePin->GetDirection() == EEdGraphPinDirection::EGPD_Output ? curNodePin->GetDesiredSize().X : 0, curNodePin->GetDesiredSize().Y / 2);
					TSharedRef<SWidget> matchedTargetNodePin = curNodePin;

					for (auto targetNode : targetNodes)
					{
						//cull distant node
						if (!FBox2D(curNode->GetPosition()- FVector2D(minDist, minDist), curNode->GetPosition() + curNode->GetDesiredSize()+ FVector2D(minDist, minDist)).Intersect(FBox2D(targetNode->GetPosition(), targetNode->GetPosition() + targetNode->GetDesiredSize())))
						{
							continue;
						}

						TSet <TSharedRef<SWidget>> targetNodePins;
						targetNode->GetPins(targetNodePins);
						for (auto targetNodePinWidget : targetNodePins)
						{
							TSharedRef<SGraphPin> targetNodePin = StaticCastSharedRef<SGraphPin>(targetNodePinWidget);
							if (curNodePin->GetDirection() == targetNodePin->GetDirection())
							{
								continue;
							}

							FVector2D targetNodePinPos = targetNodePin->GetNodeOffset() + targetNode->GetPosition();
							targetNodePinPos = targetNodePinPos + FVector2D(targetNodePin->GetDirection() == EEdGraphPinDirection::EGPD_Output ? targetNodePin->GetDesiredSize().X : 0, targetNodePin->GetDesiredSize().Y / 2);
							
							if (curNodePin->GetDirection() == EEdGraphPinDirection::EGPD_Input && targetNodePinPos.X > curPinPos.X)
							{
								continue;
							}
							if (curNodePin->GetDirection() == EEdGraphPinDirection::EGPD_Output && targetNodePinPos.X < curPinPos.X)
							{
								continue;
							}

							float curDist = FVector2D::Distance(curPinPos, targetNodePinPos);
							if (curDist < minDist)
							{
								//seems CanCreateConnection slow down performance,so make it last.
								if (graphPanel->GetGraphObj()->GetSchema()->CanCreateConnection(curNodePin->GetPinObj(), targetNodePin->GetPinObj()).Response == ECanCreateConnectionResponse::CONNECT_RESPONSE_MAKE)
								{
									minDist = curDist;
									matchedTargetNodePin = targetNodePinWidget;
									matched = true;
								}
							}
						}
					}
					if (matched)
					{
						//if closest pin to this pin is already possessed by higher brother pin,only replace when its closer. 
						if (endGraphPinsTemp.Contains(matchedTargetNodePin))
						{
							int index = endGraphPinsTemp.Find(matchedTargetNodePin);
							if (minDist < minDistsTemp[index])
							{
								startGraphPinsTemp.RemoveAt(index);
								endGraphPinsTemp.RemoveAt(index);
								minDistsTemp.RemoveAt(index);

								startGraphPinsTemp.Add(curNodePinWidget);
								endGraphPinsTemp.Add(matchedTargetNodePin);
								minDistsTemp.Add(minDist);
							}
						}
						else
						{
							startGraphPinsTemp.Add(curNodePinWidget);
							endGraphPinsTemp.Add(matchedTargetNodePin);
							minDistsTemp.Add(minDist);
						}
					}
				}
				TSet<TSharedRef<SWidget>> startGraphPins;
				TSet<TSharedRef<SWidget>> endGraphPins;
				TSet<float> minDists;
				for (int i = 0; i < startGraphPinsTemp.Num(); i++)
				{
					startGraphPins.Add(startGraphPinsTemp[i]);
					endGraphPins.Add(endGraphPinsTemp[i]);
					minDists.Add(minDistsTemp[i]);
				}

				if (startGraphPins.Num() > 0)
				{
					FNGAGraphPinConnectionFactoryPayLoadData payloadData;
					payloadData.GraphPanel = graphPanel;
					payloadData.autoConnectStartGraphPins = startGraphPins;
					payloadData.autoConnectEndGraphPins = endGraphPins;
					MyGraphPinConnectionFactory->SetPayloadData(payloadData);
					return;
				}
			}
		}
	}
}


void NGAInputProcessor::TryProcessAsAutoConnectMouseUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (GetDefault<UNodeGraphAssistantConfig>()->AutoConnectModifier == EAutoConnectModifier::Alt && !MouseEvent.IsAltDown())
	{
		return;
	}
	if (GetDefault<UNodeGraphAssistantConfig>()->AutoConnectModifier != EAutoConnectModifier::Alt && MouseEvent.IsAltDown())
	{
		return;
	}
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		SlateApp.SetAllowTooltips(true);
		SGraphPanel* graphPanel = GetGraphPanel(MouseEvent);
		if (graphPanel && graphPanel->HasMouseCaptureByUser(MouseEvent.GetUserIndex()))
		{
			FArrangedWidget arrangedGraphNode = GetArrangedGraphNode(MouseEvent);
			if (!(arrangedGraphNode == FArrangedWidget::NullWidget))
			{
				for (int i = 0; i < MyGraphPinConnectionFactory->PayLoadData.autoConnectStartGraphPins.Num(); i++)
				{
					//todo:fix close placed node messed up issue.
					if (MyGraphPinConnectionFactory->PayLoadData.autoConnectStartGraphPins.Num() - 1 < i)
					{
						break;
					}
					if (MyGraphPinConnectionFactory->PayLoadData.autoConnectEndGraphPins.Num() - 1 < i)
					{
						break;
					}
					TSharedRef<SGraphPin> startPin = StaticCastSharedRef<SGraphPin>(MyGraphPinConnectionFactory->PayLoadData.autoConnectStartGraphPins.Array()[i]);
					TSharedRef<SGraphPin> endPin = StaticCastSharedRef<SGraphPin>(MyGraphPinConnectionFactory->PayLoadData.autoConnectEndGraphPins.Array()[i]);
					
					graphPanel->GetGraphObj()->GetSchema()->TryCreateConnection(startPin->GetPinObj(), endPin->GetPinObj());
				}
				MyGraphPinConnectionFactory->ResetPayloadData();
			}
		}
	}
}


void NGAInputProcessor::OnSelectLinkedNodes(bool IsDownStream, bool bUpStream)
{
	SGraphPanel* graphPanel = GetCurrentGraphPanel();
	if (!graphPanel)
	{
		return;
	}

	TArray<UEdGraphNode*> selectedNodes;
	for (auto selectedNode : graphPanel->SelectionManager.SelectedNodes)
	{
		UEdGraphNode* graphNode = Cast<UEdGraphNode>(selectedNode);
		if (!graphNode)
		{
			continue;
		}
		selectedNodes.Add(graphNode);
	}

	for (auto nodeToSelect : FNodeHelper::GetLinkedNodes(selectedNodes, IsDownStream, bUpStream))
	{
		graphPanel->SelectionManager.SetNodeSelection(nodeToSelect, true);
	}
}


void NGAInputProcessor::CycleWireDrawStyle()
{
	if (GetDefault<UGraphEditorSettings>()->ForwardSplineTangentFromHorizontalDelta.X != 0)
	{
		const_cast<UGraphEditorSettings*>(GetDefault<UGraphEditorSettings>())->ForwardSplineTangentFromHorizontalDelta = FVector2D(0, 0);
		const_cast<UGraphEditorSettings*>(GetDefault<UGraphEditorSettings>())->ForwardSplineTangentFromVerticalDelta = FVector2D(0, 0);
		const_cast<UGraphEditorSettings*>(GetDefault<UGraphEditorSettings>())->BackwardSplineTangentFromHorizontalDelta = FVector2D(0, 0);
		const_cast<UGraphEditorSettings*>(GetDefault<UGraphEditorSettings>())->BackwardSplineTangentFromVerticalDelta = FVector2D(0, 0);
		const_cast<UGraphEditorSettings*>(GetDefault<UGraphEditorSettings>())->SaveConfig();
	}
	else
	{
		if (!GetDefault<UNodeGraphAssistantConfig>()->WireStyleStraight)
		{
			const_cast<UNodeGraphAssistantConfig*>(GetDefault<UNodeGraphAssistantConfig>())->WireStyleStraight = true;
			const_cast<UNodeGraphAssistantConfig*>(GetDefault<UNodeGraphAssistantConfig>())->SaveConfig();
		}
		else
		{
			const_cast<UGraphEditorSettings*>(GetDefault<UGraphEditorSettings>())->ForwardSplineTangentFromHorizontalDelta = FVector2D(1, 0);
			const_cast<UGraphEditorSettings*>(GetDefault<UGraphEditorSettings>())->ForwardSplineTangentFromVerticalDelta = FVector2D(1, 0);
			const_cast<UGraphEditorSettings*>(GetDefault<UGraphEditorSettings>())->BackwardSplineTangentFromHorizontalDelta = FVector2D(3, 0);
			const_cast<UGraphEditorSettings*>(GetDefault<UGraphEditorSettings>())->BackwardSplineTangentFromVerticalDelta = FVector2D(1.5, 0);
			const_cast<UNodeGraphAssistantConfig*>(GetDefault<UNodeGraphAssistantConfig>())->WireStyleStraight = false;

			const_cast<UGraphEditorSettings*>(GetDefault<UGraphEditorSettings>())->SaveConfig();
			const_cast<UNodeGraphAssistantConfig*>(GetDefault<UNodeGraphAssistantConfig>())->SaveConfig();
		}
	}
}


void NGAInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
	MouseUpDeltaTime += DeltaTime;

	for (int i = 0; i < TickEventListener.Num(); i++)
	{
		if (!TickEventListener[i].IsBound() || (TickEventListener[i].IsBound() && TickEventListener[i].Execute()))
		{
			TickEventListener.RemoveAt(i);
			i--;
		}
	}
}


bool NGAInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	LastMouseDownScreenPos = MouseEvent.GetScreenSpacePosition();

	if (TryProcessAsBeginDragNPanEvent(MouseEvent))
	{
		return true;
	}

	if (TryProcessAsDupliWireEvent(SlateApp,MouseEvent))
	{
		HasMouseUpAfterDrag = false;
		return true;
	}

	TryProcessAsBeginCutOffWireEvent(SlateApp, MouseEvent);  //do not block

	TryProcessAsAutoConnectMouseDownEvent(SlateApp, MouseEvent);

	TryProcessAsBeginLazyConnectEvent(SlateApp, MouseEvent);


	if (!IsDraggingConnection())
	{
		HasMouseUpAfterDrag = false;
	}
	

	if (IsCursorOnEmptySpace(MouseEvent) && !IsDraggingConnection())
	{
		//engine will use last factory in factory array when creation connection draw policy
		//this will override previous factory even if the last one can not actual create correct factory
		//we need to make sure its always the last factory,and it need to include all function of any other factory

		FEdGraphUtilities::UnregisterVisualPinConnectionFactory(MyGraphPinConnectionFactory);
		FEdGraphUtilities::RegisterVisualPinConnectionFactory(MyGraphPinConnectionFactory);
	}

	return false;
}


bool NGAInputProcessor::HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	float deltaTime = MouseUpDeltaTime;
	MouseUpDeltaTime = 0.f;

	float deltaPos = (LastMouseUpScreenPos - MouseEvent.GetScreenSpacePosition()).Size();
	LastMouseUpScreenPos = MouseEvent.GetScreenSpacePosition();
	NodeBeingDrag.Reset();

	if (MouseButtonDownResponsivnessThrottle.IsValid())
	{
		FSlateThrottleManager::Get().LeaveResponsiveMode(MouseButtonDownResponsivnessThrottle);
	}

	if (deltaTime < 0.5f && deltaPos < 5)
	{
		TryProcessAsSelectStreamEvent(MouseEvent);
	}

	if (deltaTime < 0.8f && deltaPos < 5)
	{
		TryProcessAsClusterHighlightEvent(MouseEvent);
	}

	if (TryProcessAsMultiConnectEvent(SlateApp,MouseEvent))
	{
		return true;
	}

	if (TryProcessAsSingleHighlightEvent(MouseEvent))
	{
		return true;
	}

	if (TryProcessAsInsertNodeEvent(MouseEvent))
	{
		return true;
	}

	//clean up cut off actions.
	TryProcessAsEndCutOffWireEvent(SlateApp, MouseEvent);

	TryProcessAsAutoConnectMouseUpEvent(SlateApp, MouseEvent);

	TryProcessAsEndLazyConnectEvent(SlateApp, MouseEvent);

	//end pan event
	//update visual elements.
	if (CanBeginOrEndPan(MouseEvent))
	{
		if (SlateApp.GetPlatformCursor().IsValid())
		{
			SlateApp.GetPlatformCursor()->Show(true);
		}
		SlateApp.GetDragDroppingContent()->SetDecoratorVisibility(true);
		if (SlateApp.HasUserMouseCapture(MouseEvent.GetUserIndex()))
		{
			SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
		}
		//move cursor inside safe frame where cursor wont get graph to pan.
		SlateApp.SetCursorPos(LastGraphCursorScreenPosClamped);

		return true;
	}

	return false;
}


bool NGAInputProcessor::HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	TryProcessAsBeingDragNPanEvent(MouseEvent);//first

	TryProcessAsLazyConnectEvent(SlateApp, MouseEvent);

	TryProcessAsAutoConnectMouseMoveEvent(SlateApp, MouseEvent);

	//if slate lose focus while drag and panning,the mouse will not be able to show up,so make a check here. 
	if (DidIHideTheCursor && !IsDragNPanning(MouseEvent))
	{
		if (SlateApp.GetPlatformCursor().IsValid())
		{
			SlateApp.GetPlatformCursor()->Show(true);
			DidIHideTheCursor = false;
		}
	}

	//send highlighted pins that we keep track of to slate,so slate can draw them.
	if (!IsDraggingConnection() && !IsDragNPanning(MouseEvent))
	{
		FArrangedWidget arrangedGraphPanel = GetArrangedGraphPanel(MouseEvent);
		SGraphPanel* graphPanel = static_cast<SGraphPanel*>(&arrangedGraphPanel.Widget.Get());
		if (graphPanel)
		{
			for(auto highlightedPin : HighLightedPins)
			{
				if (highlightedPin.IsValid())
				{
					graphPanel->AddPinToHoverSet(highlightedPin.Pin()->GetPinObj());
				}
			}
		}
	}

	//update visual elements(decorator window) for dragging.
	if(IsDraggingConnection() && !IsDragNPanning(MouseEvent))
	{
		if (SlateApp.GetPlatformCursor().IsValid())
		{
			SlateApp.GetPlatformCursor()->Show(true);
		}
		SlateApp.GetDragDroppingContent()->SetDecoratorVisibility(true);

		//change decorator display text.
		//lets update the widget if editor is running smooth.
		if(FPlatformTime::Seconds() - (int)FPlatformTime::Seconds() < 0.1f)
		{
			if (MouseEvent.IsShiftDown() || !MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
			{
				if (IsCursorOnEmptySpace(MouseEvent))
				{
					FDragConnection* dragConnection = static_cast<FDragConnection*>(SlateApp.GetDragDroppingContent().Get());
					if (dragConnection)
					{
						FSlateBrush* slateBrush = const_cast<FSlateBrush*>(FEditorStyle::GetBrush(TEXT("AnimNotifyEditor.BranchingPoint")));
						slateBrush->ImageSize = FVector2D(16,16);

						if (MouseEvent.IsShiftDown())
						{
							dragConnection->SetSimpleFeedbackMessage(
								slateBrush,
								FLinearColor::White,
								NSLOCTEXT("NodeGraphAssistant", "MultiDrop", "Release shift cancel "));
						}
						else
						{
							dragConnection->SetSimpleFeedbackMessage(
								slateBrush,
								FLinearColor::White,
								NSLOCTEXT("NodeGraphAssistant", "MultiDrop", "Right click cancel "));
						}
					}
				}
			}
		}
	}

	TryProcessAsCutOffWireEvent(SlateApp,MouseEvent);

	return false;
}


bool NGAInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (UICommandList->ProcessCommandBindings(InKeyEvent.GetKey(), FSlateApplication::Get().GetModifierKeys(), InKeyEvent.IsRepeat()))
	{
		return true;
	}

	if (InKeyEvent.GetKey() == EKeys::Escape && InKeyEvent.GetUserIndex() >= 0)
	{
		if (IsDraggingConnection())
		{
			CancelDraggingReset(InKeyEvent.GetUserIndex());
			return true;
		}
	}
	
	return false;
}


bool NGAInputProcessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	//shift cancel multi-drop
	if ((InKeyEvent.GetKey() == EKeys::LeftShift || InKeyEvent.GetKey() == EKeys::RightShift) && InKeyEvent.GetUserIndex() >= 0)
	{
		if (IsDraggingConnection())
		{
			CancelDraggingReset(InKeyEvent.GetUserIndex());
			return true;
		}
	}

	if (InKeyEvent.GetKey() == EKeys::LeftAlt)
	{
		TryProcessAsEndCutOffWireEvent(SlateApp,FPointerEvent());
	}

	return false;
}
//#pragma optimize("", on)