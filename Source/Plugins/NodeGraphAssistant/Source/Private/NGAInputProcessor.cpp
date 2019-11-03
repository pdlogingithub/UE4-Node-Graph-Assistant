// Copyright 2019 yangxiangyun
// All Rights Reserved

#include "NGAInputProcessor.h"
#include "CoreMinimal.h"

#ifdef NGA_WITH_ENGINE_CPP 
#include "DragConnection.cpp"
#else
#include "EngineCppFiles/DragConnection.cpp"
#endif

#include "NodeGraphAssistantConfig.h"
#include "NodeHelper.h"

#include "SlateApplication.h"
#include "SGraphPanel.h"
#include "SGraphEditorImpl.h"
#include "Editor.h"
#include "EditorStyleSet.h"
#include "EdGraph.h"
#include "GraphEditorSettings.h"
#include "EdGraphUtilities.h"

#include "ScopedTransaction.h"
#include "Editor/TransBuffer.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "MaterialEditorModule.h"
#include "BlueprintEditorModule.h"
#include "GraphEditorModule.h"
#include "IAnimationBlueprintEditorModule.h"
#include "IPluginManager.h"
#include "Modules/ModuleManager.h"

#include "MultiBox/MultiBoxBuilder.h"
#include "K2Node_EditablePinBase.h"


#pragma optimize("", off)

NGAInputProcessor::NGAInputProcessor()
{
#if ENGINE_MINOR_VERSION > 18
	MyPinFactory = MakeShareable(new FNGAGraphPinConnectionFactory());
	FEdGraphUtilities::RegisterVisualPinConnectionFactory(MyPinFactory);
#endif

	ShakeOffNodeTracker.Reserve(3);
	HighLightedPins.Reserve(10);


	FNodeGraphAssistantCommands::Register();
	UICommandList = MakeShared<FUICommandList>();
	const FNodeGraphAssistantCommands& Commands = FNodeGraphAssistantCommands::Get();

	UICommandList->MapAction(Commands.BypassNodes,
		FExecuteAction::CreateRaw(this, &NGAInputProcessor::BypassSelectedNodes,false));
	UICommandList->MapAction(Commands.SelectDownStreamNodes,
		FExecuteAction::CreateRaw(this, &NGAInputProcessor::OnSelectLinkedNodes, false, true));
	UICommandList->MapAction(Commands.SelectUpStreamNodes,
		FExecuteAction::CreateRaw(this, &NGAInputProcessor::OnSelectLinkedNodes, true, false));
	UICommandList->MapAction(Commands.RearrangeNode,
		FExecuteAction::CreateRaw(this, &NGAInputProcessor::RearrangeNodes));
	UICommandList->MapAction(Commands.CycleWireDrawStyle,
		FExecuteAction::CreateRaw(this, &NGAInputProcessor::CycleWireDrawStyle));
	UICommandList->MapAction(Commands.BypassAndKeepNodes,
		FExecuteAction::CreateRaw(this, &NGAInputProcessor::BypassSelectedNodes, true));
	UICommandList->MapAction(Commands.ConnectNodes,
		FExecuteAction::CreateRaw(this, &NGAInputProcessor::ConnectNodes));

	UICommandList->MapAction(Commands.ToggleAutoConnect,
		FExecuteAction::CreateLambda([] 
	{
		const_cast<UNodeGraphAssistantConfig*>(GetDefault<UNodeGraphAssistantConfig>())->EnableAutoConnect = !GetDefault<UNodeGraphAssistantConfig>()->EnableAutoConnect;
		const_cast<UNodeGraphAssistantConfig*>(GetDefault<UNodeGraphAssistantConfig>())->SaveConfig();
	}),
	FCanExecuteAction(),
	FIsActionChecked::CreateLambda([] {return GetDefault<UNodeGraphAssistantConfig>()->EnableAutoConnect;}));

	UICommandList->MapAction(Commands.ToggleInsertNode,
		FExecuteAction::CreateLambda([]
	{
		const_cast<UNodeGraphAssistantConfig*>(GetDefault<UNodeGraphAssistantConfig>())->EnableInsertNodeOnWire = !GetDefault<UNodeGraphAssistantConfig>()->EnableInsertNodeOnWire;
		const_cast<UNodeGraphAssistantConfig*>(GetDefault<UNodeGraphAssistantConfig>())->SaveConfig();
	}),
		FCanExecuteAction(),
		FIsActionChecked::CreateLambda([] {return GetDefault<UNodeGraphAssistantConfig>()->EnableInsertNodeOnWire; }));

	UICommandList->MapAction(Commands.DuplicateNodeWithInput,
		FExecuteAction::CreateRaw(this, &NGAInputProcessor::DupliNodeWithWire));
	UICommandList->MapAction(Commands.ExchangeWires,
		FExecuteAction::CreateRaw(this, &NGAInputProcessor::ExchangeWire, this));

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
				ToolbarBuilder.AddToolBarButton(FNodeGraphAssistantCommands::Get().ToggleAutoConnect);
				ToolbarBuilder.AddToolBarButton(FNodeGraphAssistantCommands::Get().ToggleInsertNode);
			}
			ToolbarBuilder.EndSection();
		}
		static void FillRightClickMenu(FMenuBuilder& MenuBuilder)
		{
			MenuBuilder.BeginSection("NGA");
			{
				MenuBuilder.AddMenuEntry(FNodeGraphAssistantCommands::Get().CycleWireDrawStyle);
			}
			MenuBuilder.EndSection();
		}
	};
	if (GetDefault<UNodeGraphAssistantConfig>()->ToolBarButton)
	{
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

		ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension(
			"Debugging",
			EExtensionHook::After,
			UICommandList,
			FToolBarExtensionDelegate::CreateStatic(&Local::FillToolbar)
		);
		IAnimationBlueprintEditorModule* AnimBPEditorModule = &FModuleManager::LoadModuleChecked<IAnimationBlueprintEditorModule>("AnimationBlueprintEditor");
		AnimBPEditorModule->GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
		
	}

	/*FGraphEditorModule& GraphEditorModule = FModuleManager::GetModuleChecked<FGraphEditorModule>(TEXT("GraphEditor"));
	GraphEditorModule.GetAllGraphEditorContextMenuExtender().Add(FGraphEditorModule::FGraphEditorMenuExtender_SelectedNode::CreateLambda([this](const TSharedRef<FUICommandList>, const UEdGraph*, const UEdGraphNode*, const UEdGraphPin*, bool)
	{
		TSharedRef<FExtender> graphEditorRightClickMenuExtender(new FExtender());
		graphEditorRightClickMenuExtender->AddMenuExtension(
			"ContextMenu",
			EExtensionHook::After,
			UICommandList,
			FMenuExtensionDelegate::CreateStatic(&Local::FillRightClickMenu));
		return graphEditorRightClickMenuExtender;
	}
	));*/

#if ENGINE_MINOR_VERSION < 19
	MyPinFactory = MakeShareable(new FNGAGraphPinConnectionFactory());
	FEdGraphUtilities::RegisterVisualPinConnectionFactory(MyPinFactory);
#endif
}

NGAInputProcessor::~NGAInputProcessor()
{
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().RearrangeNode);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().SelectUpStreamNodes);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().SelectDownStreamNodes);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().BypassNodes);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().CycleWireDrawStyle);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().ToggleAutoConnect);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().DuplicateNodeWithInput);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().ExchangeWires);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().ToggleInsertNode);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().BypassAndKeepNodes);
	UICommandList->UnmapAction(FNodeGraphAssistantCommands::Get().ConnectNodes);
	UICommandList.Reset();
	FNodeGraphAssistantCommands::Unregister(); 

	FEdGraphUtilities::UnregisterVisualPinConnectionFactory(MyPinFactory);
}


FNGAEventContex NGAInputProcessor::InitEventContex(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	FNGAEventContex ctx;

	ctx.IsDraggingConnection = SlateApp.IsDragDropping() && SlateApp.GetDragDroppingContent()->IsOfType<FDragConnection>();

	if ((MouseEvent.GetScreenSpacePosition() - LastMouseDownScreenPos).Size() < 10)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			ctx.IsClickGesture = true;
		}
		else if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && !MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
		{
			ctx.IsClickGesture = true;
		}
		else if (MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton && !MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
		{
			ctx.IsClickGesture = true;
		}
	}

	FWidgetPath widgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
	FScopedSwitchWorldHack SwitchWorld(widgetsUnderCursor);
	for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
	{
		FString widgetName = widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString();
		if (widgetName == "SGraphPanel")
		{
			ctx.IsCursorInsidePanel = true;
			ctx.GraphPanel = StaticCastSharedRef<SGraphPanel>(widgetsUnderCursor.Widgets[i].Widget);
			ctx.PanelGeometry = widgetsUnderCursor.Widgets[i].Geometry;

			if (i == widgetsUnderCursor.Widgets.Num()-1)
			{
				ctx.IsCursorOnPanelEmptySpace = true;
			}

			auto graphClassName = ctx.GraphPanel->GetGraphObj()->GetClass()->GetFName();
			if (graphClassName == "NiagaraGraph")
			{
				ctx.GraphType = EGraphObjType::EGT_Niagara;
			}
			else if (graphClassName == "AIGraph" || graphClassName == "UBehaviorTreeGraph" || graphClassName == "UEnvironmentQueryGraph")
			{
				ctx.GraphType = EGraphObjType::EGT_AI;
			}
			else if (graphClassName == "AnimationStateMachineGraph")
			{
				ctx.GraphType = EGraphObjType::EGT_AnimStateMachine;
			}
			else if (graphClassName == "EdGraph_ReferenceViewer")
			{
				ctx.GraphType = EGraphObjType::EGT_ReferenceViewer;
			}
			else if (graphClassName == "PhysicsAssetGraph")
			{
				ctx.GraphType = EGraphObjType::EGT_PhysicsAsset;
			}
			else if (graphClassName == "SoundCueGraph" || graphClassName == "SoundClassGraph")
			{
				ctx.GraphType = EGraphObjType::EGT_Sound;
			}
		}

		if (widgetName == "SGraphNodeComment" || widgetName == "SGraphNodeMaterialComment")
		{
			//if SComment is the second last widget,the cursor must be inside the blank area of the comment frame.
			if (i == widgetsUnderCursor.Widgets.Num()-2)
			{
				ctx.IsCursorOnPanelEmptySpace = true;
			}
		}

		if (widgetName.Contains("GraphNode") || widgetName == "SNiagaraGraphParameterMapGetNode" || widgetName == "SReferenceNode")
		{
			if (widgetName != "SGraphNodeComment" && widgetName != "SGraphNodeMaterialComment")
			{
				//physics node
				if (!widgetName.Contains("OutputPin") && !widgetName.Contains("InputPin"))
				{
					ctx.GraphNode = StaticCastSharedRef<SGraphNode>(widgetsUnderCursor.Widgets[i].Widget);
					ctx.NodeGeometry = widgetsUnderCursor.Widgets[i].Geometry;
				}
			}
			else
			{
				ctx.CommentNode = StaticCastSharedRef<SGraphNode>(widgetsUnderCursor.Widgets[i].Widget);
			}
		}

		if (widgetName == "SLevelOfDetailBranchNode")
		{
			ctx.IsNodeTitle = true;
		}

		if (widgetName.Contains("EditableText") || widgetName.Contains("NumericEntryBox"))
		{
			ctx.IsInPinEditableBox = true;
		}

		if (widgetName.Contains("GraphPin") || widgetName.Contains("BehaviorTreePin") || widgetName.Contains("OutputPin") || widgetName.Contains("SEnvironmentQueryPin"))
		{
			ctx.GraphPin = StaticCastSharedRef<SGraphPin>(widgetsUnderCursor.Widgets[i].Widget);
			ctx.PinGeometry = widgetsUnderCursor.Widgets[i].Geometry;

			ctx.IsNodeTitle = false;
		}
	}

	return ctx;
}


bool NGAInputProcessor::IsCursorInsidePanel() const
{
	FSlateApplication& slateApp = FSlateApplication::Get();
	FWidgetPath widgetsUnderCursor = slateApp.LocateWindowUnderMouse(slateApp.GetCursorPos(), slateApp.GetInteractiveTopLevelWindows());
	for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
	{
		if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString() == "SGraphPanel")
		{
			return true;
		}
	}
	return false;
}


bool NGAInputProcessor::IsCursorOnEmptySpace() const
{
	FSlateApplication& slateApp = FSlateApplication::Get();
	FWidgetPath widgetsUnderCursor = slateApp.LocateWindowUnderMouse(slateApp.GetCursorPos(), slateApp.GetInteractiveTopLevelWindows());
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


FSlateRect NGAInputProcessor::GetWidgetRect(FGeometry Geometry)
{
#if ENGINE_MINOR_VERSION > 16
	return Geometry.GetLayoutBoundingRect();
#else
	return Geometry.GetClippingRect();
#endif
}


TSharedPtr<SGraphPanel> NGAInputProcessor::GetCurrentGraphPanel()
{
	FSlateApplication& slateApp = FSlateApplication::Get();
	FWidgetPath widgetsUnderCursor = slateApp.LocateWindowUnderMouse(slateApp.GetCursorPos(), slateApp.GetInteractiveTopLevelWindows());
	FScopedSwitchWorldHack SwitchWorld(widgetsUnderCursor);
	for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
	{
		if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString() == "SGraphPanel")
		{
			return TSharedPtr<SGraphPanel>(StaticCastSharedRef<SGraphPanel>(widgetsUnderCursor.Widgets[i].Widget));
		}
	}
	return nullptr;
}


FVector2D NGAInputProcessor::GraphPosToScreenPos(TSharedRef<SGraphPanel> GraphPanel, FGeometry Geometry, FVector2D PanelPos)
{
	PanelPos = (PanelPos - GraphPanel->GetViewOffset()) * GraphPanel->GetZoomAmount();
	return Geometry.LocalToAbsolute(PanelPos);
}


FVector2D NGAInputProcessor::ScreenPosToGraphPos(TSharedRef<SGraphPanel> GraphPanel, FGeometry Geometry, FVector2D ScreenPos)
{
	auto ZoomStartOffset = Geometry.AbsoluteToLocal(ScreenPos);
	return GraphPanel->PanelCoordToGraphCoord(ZoomStartOffset);
}


bool NGAInputProcessor::IsDraggingConnection() const
{
	FSlateApplication& slateApp = FSlateApplication::Get();
	return slateApp.IsDragDropping() && slateApp.GetDragDroppingContent()->IsOfType<FDragConnection>();
}


//remember to call when block mouse up event.
void NGAInputProcessor::CancelDraggingReset(int32 UserIndex)
{
	FSlateApplication& slateApp = FSlateApplication::Get();
	TSharedPtr< FDragDropOperation > LocalDragDropContent = slateApp.GetDragDroppingContent();
	if (LocalDragDropContent.IsValid())
	{
		slateApp.CancelDragDrop();
		LocalDragDropContent->OnDrop(true, FPointerEvent());
	}
	slateApp.ReleaseMouseCaptureForUser(UserIndex);
	slateApp.GetPlatformApplication()->SetCapture(nullptr);
	if (slateApp.GetPlatformCursor().IsValid())
	{
		slateApp.GetPlatformCursor()->Show(true);
	}
}

bool NGAInputProcessor::IsPanningButton(const FKey& Key) const
{
#if ENGINE_MINOR_VERSION > 16
	if (GetDefault<UGraphEditorSettings>()->PanningMouseButton == EGraphPanningMouseButton::Both)
	{
		if (Key == EKeys::RightMouseButton)
		{
			return true;
		}
		if (Key == EKeys::MiddleMouseButton)
		{
			return true;
		}
	}
	else if (GetDefault<UGraphEditorSettings>()->PanningMouseButton == EGraphPanningMouseButton::Right)
	{
		if (Key == EKeys::RightMouseButton)
		{
			return true;
		}
	}
	else if (GetDefault<UGraphEditorSettings>()->PanningMouseButton == EGraphPanningMouseButton::Middle)
	{
		if (Key == EKeys::MiddleMouseButton)
		{
			return true;
		}
	}
#else
	if (Key == EKeys::RightMouseButton)
	{
		return true;
	}
#endif
	return false;
}


bool NGAInputProcessor::IsPanningButtonDown(const FPointerEvent& MouseEvent) const
{
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
	return false;
}


bool NGAInputProcessor::IsCutoffButton(const FKey& Key) const
{
	if (GetDefault<UNodeGraphAssistantConfig>()->DragCutOffWireMouseButton == ECutOffMouseButton::Middle)
	{
		if (Key == EKeys::MiddleMouseButton)
		{
			return true;
		}
	}
	if (GetDefault<UNodeGraphAssistantConfig>()->DragCutOffWireMouseButton == ECutOffMouseButton::Left)
	{
		if (Key == EKeys::LeftMouseButton)
		{
			return true;
		}
	}
	return false;
}


bool NGAInputProcessor::IsCutoffButtonDown(const FPointerEvent& MouseEvent) const
{
	if (GetDefault<UNodeGraphAssistantConfig>()->DragCutOffWireMouseButton == ECutOffMouseButton::Middle)
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
		{
			return true;
		}
	}
	if (GetDefault<UNodeGraphAssistantConfig>()->DragCutOffWireMouseButton == ECutOffMouseButton::Left)
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			return true;
		}
	}
	return false;
}


//shift click to replicate connections.
FNGAEventReply NGAInputProcessor::TryProcessAsDupliWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (Ctx.GraphType == EGT_ReferenceViewer)
	{
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.GraphType == EGT_AnimStateMachine)
	{
		return FNGAEventReply::UnHandled();
	}

	if (Ctx.IsCursorInsidePanel && !SlateApp.IsDragDropping())
	{
		if (MouseEvent.IsShiftDown() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			if (Ctx.GraphPin.IsValid() && Ctx.GraphPanel.IsValid())
			{
				TArray<FGraphPinHandle> linkedPins;

				for (auto linkedPin : Ctx.GraphPin->GetPinObj()->LinkedTo)
				{
					linkedPins.AddUnique(linkedPin);
				}
				if (linkedPins.Num() > 0)
				{
					TSharedRef<SGraphPanel> graphPanel = Ctx.GraphPanel.ToSharedRef();
					TSharedPtr<FDragDropOperation> DragEvent = FDragConnection::New(graphPanel, linkedPins);
					FWidgetPath widgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
					SlateApp.ProcessReply(widgetsUnderCursor, FReply::Handled().BeginDragDrop(DragEvent.ToSharedRef()), &widgetsUnderCursor, &MouseEvent, MouseEvent.GetUserIndex());
					return FNGAEventReply::BlockSlateInput();
				}
			}
		}
	}
	return FNGAEventReply::UnHandled();
}


//by default panning is bypassed when dragging connection(slate.cpp 5067 4.18), to be able to pan while dragging,need to manually route the pointer down event to graph panel.
FNGAEventReply NGAInputProcessor::TryProcessAsBeginDragNPanEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (Ctx.IsCursorInsidePanel && (Ctx.IsDraggingConnection || NodeBeingDrag.IsValid() || CommentNodeBeingDrag.IsValid()) && IsPanningButton(MouseEvent.GetEffectingButton()) && MouseEvent.IsMouseButtonDown(MouseEvent.GetEffectingButton()))
	{
		//make sure no widget captured the mouse so we can pan now.
		if (Ctx.GraphPanel.IsValid() && (NodeBeingDrag.IsValid() || CommentNodeBeingDrag.IsValid() ||!SlateApp.HasUserMouseCapture(MouseEvent.GetUserIndex())))
		{
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
				//todo,remove this,route to panel directly.
				if (reply.ShouldThrottle() && !MouseButtonDownResponsivnessThrottle.IsValid())
				{
					MouseButtonDownResponsivnessThrottle = FSlateThrottleManager::Get().EnterResponsiveMode();
				}
				else if (!reply.ShouldThrottle() && MouseButtonDownResponsivnessThrottle.IsValid())
				{
					// Leave responsive mode if a widget choose not to throttle
					FSlateThrottleManager::Get().LeaveResponsiveMode(MouseButtonDownResponsivnessThrottle);
				}

				LastPanelScreenSpaceRect = GetWidgetRect(Ctx.PanelGeometry);

				//[correct wire position]software cursor is in panel space,but preview connection wire is in screen space,when drag and panning,their positions will become mismatched.
				//to fix that we need to cache the software cursor panel position,so we can convert it to correct screen space position when drag and panning to keep connection wire aligned with it.
				LastGraphCursorGraphPos = ScreenPosToGraphPos(Ctx.GraphPanel.ToSharedRef(), Ctx.PanelGeometry, MouseEvent.GetScreenSpacePosition());

				return FNGAEventReply::BlockSlateInput();
			}
		}
	}
	return FNGAEventReply::UnHandled();
}


//update visual elements(wire.cursor icon...) for drag and panning,or they will be at mismatched position.
FNGAEventReply NGAInputProcessor::TryProcessAsBeingDragNPanEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if ((Ctx.IsDraggingConnection || NodeBeingDrag.IsValid() || CommentNodeBeingDrag.IsValid()) && IsPanningButtonDown(MouseEvent))
	{
		//when dragging and panning towards outside of the panel fast,the mouse move event will be picked up by another widget rather than graph panel.
		//check if still inside same panel.
		if (Ctx.GraphPanel.IsValid() && GetWidgetRect(Ctx.PanelGeometry) == LastPanelScreenSpaceRect)
		{
			LastGraphCursorScreenPosClamped = GraphPosToScreenPos(Ctx.GraphPanel.ToSharedRef(), Ctx.PanelGeometry, LastGraphCursorGraphPos);
		}
		else
		{
			LastGraphCursorScreenPosClamped = MouseEvent.GetScreenSpacePosition();
		}
		
		//when cursor is getting close to the edge of the panel,drag operation will send deferred pan request to accelerate pan speed, no need that.
		//do not check if it is outside of rect,doing correction anyway,or problem will occurs.
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
			LastGraphCursorScreenPosClamped- MouseEvent.GetCursorDelta(),
			MouseEvent.GetCursorDelta(),//original delta is needed for panning.
			keys,
			FModifierKeysState()
		);

		//call OnDragOver with correct screen space software cursor position to update connection wire endpoint position.
		if (Ctx.GraphPanel.IsValid() && Ctx.IsDraggingConnection)
		{
			Ctx.GraphPanel->OnDragOver(Ctx.PanelGeometry, FDragDropEvent(mouseEvent, SlateApp.GetDragDroppingContent()));
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

		if (Ctx.IsDraggingConnection)
		{
			SlateApp.GetDragDroppingContent()->SetDecoratorVisibility(false);
		}
		return FNGAEventReply::Handled();
	}

	return FNGAEventReply::UnHandled();
}

//clean up for drag and pan event,release capture,set visibility,clamp cursor position...
FNGAEventReply NGAInputProcessor::TryProcessAsEndDragNPanEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (Ctx.IsDraggingConnection && Ctx.IsCursorInsidePanel)
	{
		if (IsPanningButton(MouseEvent.GetEffectingButton()))
		{
			if (!Ctx.IsClickGesture)
			{
				SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
				if (SlateApp.GetPlatformCursor().IsValid())
				{
					SlateApp.GetPlatformCursor()->Show(true);
				}
				SlateApp.GetDragDroppingContent()->SetDecoratorVisibility(true);
				SlateApp.SetCursorPos(LastGraphCursorScreenPosClamped);  //panel cursor to screen space position.
			}
			else if(!Ctx.GraphPin.IsValid())
			{
				CancelDraggingReset(MouseEvent.GetUserIndex());

#if PLATFORM_MAC
				TSet<FKey> keys;
				keys.Add(EKeys::LeftMouseButton);
				FPointerEvent fakeMouseEvent(
					MouseEvent.GetPointerIndex(),
					MouseEvent.GetScreenSpacePosition(),
					MouseEvent.GetLastScreenSpacePosition(),
					keys,
					EKeys::LeftMouseButton,
					0,
					FModifierKeysState(false, false, false, false, false, false, false, false, false)
				);

				FWidgetPath WidgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
				SlateApp.RoutePointerUpEvent(WidgetsUnderCursor, fakeMouseEvent);
#endif
			}
			return FNGAEventReply::BlockSlateInput();
		}
	}
	//drag node and pan.
	if ((NodeBeingDrag.IsValid() || CommentNodeBeingDrag.IsValid()) && IsPanningButton(MouseEvent.GetEffectingButton()) && Ctx.GraphPanel.IsValid())
	{
		//at this moment,left mouse is not pressed in slate's state(see begin drag n pan),add it back,otherwise dragging node has no effect.
		TSet<FKey> keys;
		keys.Add(EKeys::LeftMouseButton);
		FPointerEvent mouseEvent(
			MouseEvent.GetPointerIndex(),
			MouseEvent.GetScreenSpacePosition(),
			MouseEvent.GetLastScreenSpacePosition(),
			keys,
			MouseEvent.GetEffectingButton(),
			0,
			FModifierKeysState()
		);
		const_cast<FPointerEvent&>(MouseEvent) = mouseEvent;
		SlateApp.SetCursorPos(LastGraphCursorScreenPosClamped);
		Ctx.GraphPanel->OnMouseButtonUp(Ctx.PanelGeometry, mouseEvent);
		if (SlateApp.GetPlatformCursor().IsValid())
		{
			SlateApp.GetPlatformCursor()->Show(true);
		}

		return FNGAEventReply::BlockSlateInput(); 
	}
	return FNGAEventReply::UnHandled();
}


//middle mouse double click select linked node.
FNGAEventReply NGAInputProcessor::TryProcessAsSelectStreamEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EnableSelectStream)
	{
		return FNGAEventReply::UnHandled();
	}

	if (Ctx.IsDoubleClickGesture && MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton && !Ctx.IsCursorOnPanelEmptySpace)
	{
		if (!Ctx.GraphPin.IsValid() && Ctx.GraphNode.IsValid() && Ctx.GraphPanel.IsValid())
		{
			UEdGraphNode* clickedNode = Ctx.GraphNode->GetNodeObj();

			if (clickedNode)
			{
				Ctx.GraphPanel->SelectionManager.SetNodeSelection(clickedNode, true);

#if ENGINE_MINOR_VERSION > 16
				FSlateRect nodeRect = Ctx.NodeGeometry.GetLayoutBoundingRect();
#else
				FSlateRect nodeRect = Ctx.NodeGeometry.GetClippingRect();
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
					Ctx.GraphPanel->SelectionManager.SetNodeSelection(nodeToSelect, true);
				}
				return FNGAEventReply::Handled();
			}
		}
	}
	return FNGAEventReply::UnHandled();
}


//multi-connect,by routing drop event without destroying drag-drop operation
FNGAEventReply NGAInputProcessor::TryProcessAsMultiConnectEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (Ctx.GraphType == EGT_ReferenceViewer)
	{
		return FNGAEventReply::UnHandled();
	}

	if (Ctx.IsDraggingConnection && Ctx.IsCursorInsidePanel && !MouseEvent.IsAltDown())
	{
		TSharedPtr<FDragConnection> dragConnection = StaticCastSharedPtr<FDragConnection>(SlateApp.GetDragDroppingContent());
		//right click multi connect.
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			if (Ctx.IsClickGesture)
			{
				if (Ctx.GraphPin.IsValid())
				{
					Ctx.GraphPin->OnDrop(Ctx.PinGeometry, FDragDropEvent(MouseEvent, dragConnection));
					SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
					return FNGAEventReply::BlockSlateInput();
				}
			}
		}
		else if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			if (Ctx.IsClickGesture || (!Ctx.IsClickGesture && MouseEvent.IsShiftDown()))
			{
				//decide what to do depend on click on different widget.
				//[editable text pin]blueprint node pin can have editable text box area,do not multi connect when inside it.
				if (Ctx.GraphPin.IsValid() && !Ctx.IsInPinEditableBox)
				{
					if (!GetDefault<UNodeGraphAssistantConfig>()->EnableLeftClickMultiConnect && !MouseEvent.IsShiftDown())
					{
						TArray<UEdGraphPin*> draggingPins;
						dragConnection->ValidateGraphPinList(draggingPins);
						for (UEdGraphPin* draggingPin : draggingPins)
						{
							if (Ctx.GraphPin->GetPinObj()->GetSchema()->CanCreateConnection(Ctx.GraphPin->GetPinObj(), draggingPin).Response != CONNECT_RESPONSE_DISALLOW)
							{
								Ctx.GraphPin->OnDrop(Ctx.PinGeometry, FDragDropEvent(MouseEvent, dragConnection));
								CancelDraggingReset(MouseEvent.GetUserIndex());
								return FNGAEventReply::BlockSlateInput();
							}
						}
					}

					Ctx.GraphPin->OnDrop(Ctx.PinGeometry, FDragDropEvent(MouseEvent, dragConnection));
					return FNGAEventReply::BlockSlateInput();
				}
				else if (Ctx.GraphNode.IsValid())
				{
					TWeakPtr<SGraphPin> lazyConnectiblePin = MyPinFactory->GetLazyConnectiblePin();
					if (lazyConnectiblePin.IsValid())
					{
						if (!GetDefault<UNodeGraphAssistantConfig>()->EnableLeftClickMultiConnect && !MouseEvent.IsShiftDown())
						{
							dragConnection->SetHoveredPin(lazyConnectiblePin.Pin()->GetPinObj());
							TArray<UEdGraphPin*> draggingPins;
							dragConnection->ValidateGraphPinList(draggingPins);
							for (UEdGraphPin* draggingPin : draggingPins)
							{
								if (lazyConnectiblePin.Pin()->GetPinObj()->GetSchema()->CanCreateConnection(lazyConnectiblePin.Pin()->GetPinObj(), draggingPin).Response != CONNECT_RESPONSE_DISALLOW)
								{
									lazyConnectiblePin.Pin()->OnDrop(Ctx.PinGeometry, FDragDropEvent(MouseEvent, dragConnection));
									CancelDraggingReset(MouseEvent.GetUserIndex());
									return FNGAEventReply::BlockSlateInput();
								}
							}
						}

						dragConnection->SetHoveredPin(lazyConnectiblePin.Pin()->GetPinObj());
						lazyConnectiblePin.Pin()->OnDrop(Ctx.PinGeometry, FDragDropEvent(MouseEvent, dragConnection));
						dragConnection->SetHoveredPin(nullptr);//otherwise will always show pin info.
						dragConnection->SetSimpleFeedbackMessage(
							FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK")),
							FLinearColor::White,
							NSLOCTEXT("NodeGraphAssistant", "LazyConnect", ""));
						return FNGAEventReply::BlockSlateInput();
					}
				}
				//decide what to do when drop on empty space,cancel or summon menu.
				else if (Ctx.GraphPanel.IsValid())
				{
					if (Ctx.IsClickGesture)
					{
						return FNGAEventReply::UnHandled();
					}
					else
					{
						return FNGAEventReply::BlockSlateInput();
					}
				}
			}
			else if (!Ctx.IsClickGesture && !MouseEvent.IsShiftDown())
			{
				if (Ctx.GraphPin.IsValid())
				{
					return FNGAEventReply::UnHandled();
				}
				else if (Ctx.GraphNode.IsValid())
				{
					TWeakPtr<SGraphPin> lazyConnectiblePin = MyPinFactory->GetLazyConnectiblePin();
					if (lazyConnectiblePin.IsValid())
					{
						StaticCastSharedPtr<FDragConnection>(SlateApp.GetDragDroppingContent())->SetHoveredPin(lazyConnectiblePin.Pin()->GetPinObj());
						lazyConnectiblePin.Pin()->OnDrop(Ctx.PinGeometry, FDragDropEvent(MouseEvent, SlateApp.GetDragDroppingContent()));
						CancelDraggingReset(MouseEvent.GetUserIndex());
						return FNGAEventReply::BlockSlateInput();
					}
				}
			}
			else if (!Ctx.IsClickGesture && MouseEvent.IsControlDown())
			{
				if (Ctx.GraphPin.IsValid())
				{
					return FNGAEventReply::UnHandled();
				}
				else if (Ctx.GraphNode.IsValid())
				{
					TWeakPtr<SGraphPin> lazyConnectiblePin = MyPinFactory->GetLazyConnectiblePin();
					if (lazyConnectiblePin.IsValid())
					{
						StaticCastSharedPtr<FDragConnection>(SlateApp.GetDragDroppingContent())->SetHoveredPin(lazyConnectiblePin.Pin()->GetPinObj());
						lazyConnectiblePin.Pin()->OnDrop(Ctx.PinGeometry, FDragDropEvent(MouseEvent, SlateApp.GetDragDroppingContent()));
						CancelDraggingReset(MouseEvent.GetUserIndex());
						return FNGAEventReply::BlockSlateInput();
					}
				}
			}
		}
	}
	return FNGAEventReply::UnHandled();
}


// double click on pin to highlight all wires that connect to this pin.
FNGAEventReply NGAInputProcessor::TryProcessAsClusterHighlightEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (Ctx.IsDoubleClickGesture && MouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		//add the double clicked graph pin to our collection.
		if (Ctx.GraphPanel.IsValid() && Ctx.GraphPin.IsValid())
		{
			auto sourcePin = TWeakPtr<SGraphPin>(Ctx.GraphPin);
			HighLightedPins.Add(sourcePin);
			Ctx.GraphPanel->MarkedPin = sourcePin;

			return FNGAEventReply::Handled();
		}
	}
	return FNGAEventReply::UnHandled();
}


//todo:use connection factory to set pin highlight.
FNGAEventReply NGAInputProcessor::TryProcessAsSingleHighlightEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (!Ctx.IsDraggingConnection && Ctx.IsDoubleClickGesture && (Ctx.IsCursorOnPanelEmptySpace || (Ctx.GraphNode.IsValid() && Ctx.GraphNode->GetTypeAsString() == "SGraphNodeKnot")))
	{
		Ctx.GraphPanel->MarkedPin.Reset();
		HighLightedPins.Empty();
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.IsCursorOnPanelEmptySpace && !Ctx.IsDraggingConnection && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) && !MouseEvent.IsAltDown())
	{
		if (Ctx.IsClickGesture && !BlockNextClick && PressedCharKey ==0)
		{
			if (Ctx.GraphPanel.IsValid())
			{
				Ctx.GraphPanel->MarkedPin.Reset();

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

				FWidgetPath WidgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
				SlateApp.RoutePointerDownEvent(WidgetsUnderCursor, fakeMouseEvent);
				FReply reply = SlateApp.RoutePointerUpEvent(WidgetsUnderCursor, fakeMouseEvent);

				if (reply.IsEventHandled())
				{
					//graph panel can support constant highlight for one wire and tempt hover highlight for multiple wires,
					//in order to constantly highlight multiple wires,I have to keep a list of constant highlighted wires,and squeeze them into graph panel's hover set.
					if (Ctx.GraphPanel->MarkedPin.IsValid())// wire detected
					{
						if (MouseEvent.IsShiftDown())
						{
							if (HighLightedPins.Contains(Ctx.GraphPanel->MarkedPin))
							{
								HighLightedPins.Remove(Ctx.GraphPanel->MarkedPin);
								//keep marked pin reference alive,so we can have instant hover highlight.
								if (HighLightedPins.Num() > 0)
								{
									Ctx.GraphPanel->MarkedPin = HighLightedPins.Array().Last();
								}
								else
								{
									Ctx.GraphPanel->MarkedPin.Reset();
								}
							}
							else
							{
								HighLightedPins.Add(Ctx.GraphPanel->MarkedPin);
							}
						}
						else
						{
							for (auto pin : HighLightedPins)
							{
								if (pin.IsValid())
								{
									Ctx.GraphPanel->RemovePinFromHoverSet(pin.Pin()->GetPinObj());
								}
							}
							HighLightedPins.Empty();
							HighLightedPins.Add(Ctx.GraphPanel->MarkedPin);
						}
					}
					else
					{
						//click on empty space remove all highlighted wires.
						TSet< TSharedRef<SWidget>> allPins;
						Ctx.GraphPanel->GetAllPins(allPins);
						for (auto pin : allPins)
						{
							Ctx.GraphPanel->RemovePinFromHoverSet(StaticCastSharedRef<SGraphPin>(pin)->GetPinObj());
						}
						//make sure clear pin from other graph from current graph
						//if unrecognized pin was send to graph,graph will dim other wires.
						for (auto pin : HighLightedPins)
						{
							if (pin.IsValid())
							{
								Ctx.GraphPanel->RemovePinFromHoverSet(pin.Pin()->GetPinObj());
							}
						}
						HighLightedPins.Empty();
					}

					//click on wire will create mouse capture,causing graph pin to be not clickable at first click,so release capture.
					SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
					SlateApp.GetPlatformApplication()->SetCapture(nullptr);
					return FNGAEventReply::BlockSlateInput();
				}
			}
		}
	}
	return FNGAEventReply::UnHandled();
}


FNGAEventReply NGAInputProcessor::TryProcessAsBeginCutOffWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EnableCutoffWire)
	{
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.GraphType == EGT_ReferenceViewer)
	{
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.GraphType == EGT_PhysicsAsset)
	{
		return FNGAEventReply::UnHandled();
	}

	//check if inside panel first,otherwise user will not be able to alt+drag to duplicate actor or alt+drag to orbit around actor in level editor viewport,
	//because FEditorViewportClient::Tick gets skipped because entering responsive mode in below code.
	if (!HasBegunCuttingWire && !Ctx.IsDraggingConnection && Ctx.IsCursorOnPanelEmptySpace)
	{
		if (IsCutoffButton(MouseEvent.GetEffectingButton()) && MouseEvent.IsAltDown() && !MouseEvent.IsShiftDown() && !MouseEvent.IsControlDown())
		{
			//alt+drag to orbit in viewport can get cursor to move inside panel;todo remove this
			if (SlateApp.GetPlatformCursor().IsValid() && SlateApp.GetPlatformCursor()->GetType() != EMouseCursor::None)
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

#if ENGINE_MINOR_VERSION > 22
				if (SlateApp.GetPlatformCursor().IsValid())
				{
					CusorResource_Scissor = SlateApp.GetPlatformCursor()->CreateCursorFromFile(IPluginManager::Get().FindPlugin(TEXT("NodeGraphAssistant"))->GetBaseDir() / TEXT("Resources") / TEXT("nga_cutoff_cursor"), FVector2D(0, 0));
				}
#else
				if (!CusorResource_Scissor.IsValid())
				{
					CusorResource_Scissor = MakeShareable(new FHardwareCursor(IPluginManager::Get().FindPlugin(TEXT("NodeGraphAssistant"))->GetBaseDir() / TEXT("Resources") / TEXT("nga_cutoff_cursor"), FVector2D(0, 0)));
				}
#endif

				return FNGAEventReply::BlockSlateInput();  //need to block,otherwise custom cursor icon will flicker if cutoff button is left mouse due to capture.
			}
		}
	}
	return FNGAEventReply::UnHandled();
}


//trigger alt-mouse down event when move to cut off wires.
//to do:no undo if no wire cut.
FNGAEventReply NGAInputProcessor::TryProcessAsBeingCutOffWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (Ctx.IsCursorInsidePanel && HasBegunCuttingWire && IsCutoffButtonDown(MouseEvent))
	{
		if (MouseEvent.IsAltDown() && !MouseEvent.IsShiftDown() && !MouseEvent.IsControlDown())
		{
			if (Ctx.GraphPanel.IsValid())
			{
				MyPinFactory->PayLoadData->CursorDeltaSquared = (MouseEvent.GetScreenSpacePosition() - MouseEvent.GetLastScreenSpacePosition()).SizeSquared();
				if (MyPinFactory->PayLoadData->OutHoveredInputPins.Num() > 0)
				{
					for (int i = 0; i < MyPinFactory->PayLoadData->OutHoveredInputPins.Num(); i++)
					{
						if (MyPinFactory->PayLoadData->OutHoveredInputPins[i]->GetOwningNodeUnchecked() && MyPinFactory->PayLoadData->OutHoveredOutputPins[i]->GetOwningNodeUnchecked())
						{
							FNodeHelper::BreakSinglePinLink(MyPinFactory->PayLoadData->OutHoveredInputPins[i], MyPinFactory->PayLoadData->OutHoveredOutputPins[i]);
						}
					}
					//these code doesn't trigger "mark material dirty" for material editor
					//switch to graph schema's interface above,it has nullptr issue as I remembered,keep watching
					/*TArray<UEdGraphNode*> notifyNodes;
					while (MyPinFactory->PayLoadData->OutHoveredInputPins.Num() > 0)
					{
						MyPinFactory->PayLoadData->OutHoveredInputPins[0]->BreakLinkTo(MyPinFactory->PayLoadData->OutHoveredOutputPins[0]);

						auto inputNode = MyPinFactory->PayLoadData->OutHoveredInputPins[0]->GetOwningNodeUnchecked();
						auto outputNode = MyPinFactory->PayLoadData->OutHoveredOutputPins[0]->GetOwningNodeUnchecked();
						if (inputNode && outputNode)
						{
							inputNode->PinConnectionListChanged(MyPinFactory->PayLoadData->OutHoveredInputPins[0]);
							outputNode->PinConnectionListChanged(MyPinFactory->PayLoadData->OutHoveredOutputPins[0]);
						}
						notifyNodes.AddUnique(inputNode);
						notifyNodes.AddUnique(outputNode);

						MyPinFactory->PayLoadData->OutHoveredInputPins.RemoveAt(0);
						MyPinFactory->PayLoadData->OutHoveredOutputPins.RemoveAt(0);
					}
					for (auto notifyNode : notifyNodes)
					{
						if (notifyNode)
						{
							notifyNode->NodeConnectionListChanged();
						}
					}*/
					return FNGAEventReply::Handled();
				}

				FPointerEvent mouseEvent(
					MouseEvent.GetPointerIndex(),
					MouseEvent.GetLastScreenSpacePosition(),
					MouseEvent.GetLastScreenSpacePosition(),
					TSet<FKey>(),
					EKeys::LeftMouseButton,
					0,
					FModifierKeysState(false, false, false, false, true, true, false, false, false));
				Ctx.GraphPanel->OnMouseButtonDown(Ctx.PanelGeometry, mouseEvent);

				if (SlateApp.GetPlatformCursor().IsValid())
				{
#if ENGINE_MINOR_VERSION > 22
					if (CusorResource_Scissor)
					{
						SlateApp.GetPlatformCursor()->SetTypeShape(EMouseCursor::Custom, CusorResource_Scissor);
					}
#else
					SlateApp.GetPlatformCursor()->SetTypeShape(EMouseCursor::Custom, CusorResource_Scissor->GetHandle());
#endif
					SlateApp.GetPlatformCursor()->SetType(EMouseCursor::Custom);
				}
			}

			return FNGAEventReply::Handled();
		}
	}
	return FNGAEventReply::UnHandled();
}


//end undo transaction.
FNGAEventReply NGAInputProcessor::TryProcessAsEndCutOffWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (HasBegunCuttingWire)
	{
		if (!IsCutoffButtonDown(MouseEvent) || !MouseEvent.IsAltDown())
		{
			HasBegunCuttingWire = false;

			MyPinFactory->PayLoadData->CursorDeltaSquared = 0;
			SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
			SlateApp.GetPlatformApplication()->SetCapture(nullptr);

			if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
			{
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
			return FNGAEventReply::BlockSlateInput();
		}
	}

	return FNGAEventReply::UnHandled();
}


//try to link selected nodes' inputs to their outputs, and attempt to remove nodes.
void NGAInputProcessor::BypassSelectedNodes(bool ForceKeepNode)
{
	TSharedPtr<SGraphPanel> graphPanel = GetCurrentGraphPanel();

	if (!graphPanel.IsValid())
	{
		return;
	}
	if (graphPanel->GetGraphObj()->GetClass()->GetFName() == "AnimationStateMachineGraph")
	{
		return;
	}
	if (graphPanel->GetGraphObj()->GetClass()->GetFName() == "EdGraph_ReferenceViewer")
	{
		return;
	}
	if (graphPanel->GetGraphObj()->GetClass()->GetFName() == "PhysicsAssetGraph")
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

	FScopedTransaction Transaction(NSLOCTEXT("NodeGraphAssistant", "BypassNodes", "Bypass node"));

	if (!ForceKeepNode && GetDefault<UNodeGraphAssistantConfig>()->BypassAndCopyNodes)
	{
		FSlateApplication& slateApp = FSlateApplication::Get();
		FWidgetPath widgetsUnderCursor = slateApp.LocateWindowUnderMouse(slateApp.GetCursorPos(), slateApp.GetInteractiveTopLevelWindows());
		FScopedSwitchWorldHack SwitchWorld(widgetsUnderCursor);
		for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
		{
			FString widgetName = widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString();
			if (widgetName == "SGraphEditorImpl")
			{
				TSharedRef<SGraphEditorImpl> PanelImpl = StaticCastSharedRef<SGraphEditorImpl>(widgetsUnderCursor.Widgets[i].Widget);
				PanelImpl->OnKeyDown(
					FGeometry(),
#if PLATFORM_MAC
					FKeyEvent(EKeys::C, FModifierKeysState(false, false, false, false, false, false, true, false, false), 0, false, 0, 0)
#else
					FKeyEvent(EKeys::C, FModifierKeysState(false, false, true, false, false, false, false, false, false), 0, false, 0, 0)
#endif
				);
				break;
			}
		}
	}

	bool BypassSuccess = FNodeHelper::BypassNodes(graphPanel->GetGraphObj(), selectedNodes, GetDefault<UNodeGraphAssistantConfig>()->BypassNodeAnyway, ForceKeepNode);
	if (!BypassSuccess)
	{
		FNotificationInfo Info(NSLOCTEXT("NodeGraphAssistant","BypassNodes", "Can not fully bypass node"));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		//do not cancel transaction here,cause node can be changed if even return false.
	}
	return;
}


FNGAEventReply NGAInputProcessor::TryProcessAsCreateNodeOnWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EnableCreateNodeOnWire)
	{
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.GraphType == EGT_AnimStateMachine)
	{
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.GraphType == EGT_ReferenceViewer)
	{
		return FNGAEventReply::UnHandled();
	}

	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && !MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
	{
		if (!Ctx.IsDraggingConnection && Ctx.IsCursorOnPanelEmptySpace && Ctx.IsClickGesture)
		{
			TSharedPtr<SGraphPanel> graphPanel = Ctx.GraphPanel;

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

			FReply reply = graphPanel->OnMouseButtonUp(Ctx.PanelGeometry, fakeMouseEvent);
			if (!reply.IsEventHandled() || !graphPanel->MarkedPin.IsValid())
			{
				graphPanel->MarkedPin = tempPin;
				return FNGAEventReply::UnHandled();
			}

			if (GetDefault<UNodeGraphAssistantConfig>()->CreateNodeOnlyOnSelectedWire && HighLightedPins.Array().Find(graphPanel->MarkedPin) == INDEX_NONE)
			{
				graphPanel->MarkedPin = tempPin;
				return FNGAEventReply::UnHandled();
			}

			if (graphPanel->MarkedPin.Pin().Get()->GetPinObj()->LinkedTo.Num() == 0)
			{
				return FNGAEventReply::UnHandled();
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
					if (FNodeHelper::GetWirePointHitResult(FArrangedWidget(graphPanel.ToSharedRef(), Ctx.PanelGeometry), markedPinA, markedPinB, MouseEvent.GetScreenSpacePosition(), 7, GetDefault<UGraphEditorSettings>()))
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
			else { return FNGAEventReply::UnHandled(); }

			UEdGraphNode* selectedNode = nullptr;
			if (graphPanel->SelectionManager.SelectedNodes.Num() > 0)
			{
				UEdGraphNode* graphNode = Cast<UEdGraphNode>(graphPanel->SelectionManager.SelectedNodes.Array()[0]);
				if (graphNode)
				{
					selectedNode = graphNode;
				}
			}

			const FVector2D nodeAddPosition = ScreenPosToGraphPos(graphPanel.ToSharedRef(), Ctx.PanelGeometry, MouseEvent.GetScreenSpacePosition());
			TArray<UEdGraphPin*> sourcePins;
			sourcePins.Add(pinA);
			TSharedPtr<SWidget> WidgetToFocus = graphPanel->SummonContextMenu(MouseEvent.GetScreenSpacePosition(), nodeAddPosition, nullptr, nullptr, sourcePins);
			if (!WidgetToFocus.IsValid())
			{
				return FNGAEventReply::UnHandled();
			}
			SlateApp.SetUserFocus(MouseEvent.GetUserIndex(), WidgetToFocus);

			//begin transaction here,before create new node may happen next.
			//need transaction for break single pin link.
			int32 tranIndex = INDEX_NONE;
			if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
			{
				tranIndex = GEditor->BeginTransaction(TEXT(""), NSLOCTEXT("NodeGraphAssistant", "InsertNode", "Insert node"), NULL);
			}
			BlockNextClick = true; //possible mouse up event will clear selection before we query it.

			int* tryTimes = new int(2);
			NGADeferredEventDele deferredDele;
			deferredDele.BindLambda([selectedNode, graphPanel, pinA, pinB, tranIndex,this](int* tryTimes)
			{
				//only exit
				if (*tryTimes == 0)
				{
					//maybe node not created,cancel
					if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
					{
						if (GEditor->IsTransactionActive() && tranIndex)
						{
							GEditor->CancelTransaction(tranIndex);//tranIndex equal INDEX_NONE will cause assertion in vs 
						}
					}
					BlockNextClick = false;
					return true;
				}
				if (!FSlateApplication::Get().AnyMenusVisible())
				{
					if (graphPanel.IsValid() && graphPanel->SelectionManager.SelectedNodes.Num() > 0)  //>0, possible auto conversion node.
					{
						//selection set might still be the old one(blueprint graph),so don't do it right away.
						if (*tryTimes > 1)
						{
							(*tryTimes)--;
							return false;
						}
						UEdGraphNode* newSelectedNode = Cast<UEdGraphNode>(graphPanel->SelectionManager.SelectedNodes.Array()[0]);
						if (newSelectedNode && newSelectedNode != selectedNode)
						{
							bool insertSuccess = false;
							for (auto newNodePin : newSelectedNode->Pins)
							{
								if (newNodePin->Direction == EGPD_Output)
								{
									if (newSelectedNode->GetSchema()->CanCreateConnection(newNodePin, pinB).Response != CONNECT_RESPONSE_DISALLOW)
									{
										newSelectedNode->GetSchema()->TryCreateConnection(newNodePin, pinB);
										insertSuccess = true;
										FNodeHelper::BreakSinglePinLink(pinA, pinB);
										break;
									}
								}
							}
							if (!insertSuccess)
							{
								FNotificationInfo Info(NSLOCTEXT("NodeGraphAssistant", "InsertNode", "Can not insert this node"));
								Info.ExpireDuration = 3.0f;
								FSlateNotificationManager::Get().AddNotification(Info);
							}
							//new node must be created now,end.
							if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
							{
								if (GEditor->IsTransactionActive())
								{
									GEditor->EndTransaction();
								}
							}
						}
					}
					(*tryTimes)--;
				}
				return false;
			}, tryTimes);
			TickEventListener.Add(deferredDele);

			SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
			SlateApp.GetPlatformApplication()->SetCapture(nullptr);

			return FNGAEventReply::BlockSlateInput();
		}
	}

	return FNGAEventReply::UnHandled();
}


//rearrange selected nodes into grid based structure.
void NGAInputProcessor::RearrangeNodes()
{
	TSharedPtr<SGraphPanel> graphPanel = GetCurrentGraphPanel();

	if (!graphPanel.IsValid())
	{
		return;
	}

	auto graphClassName = graphPanel->GetGraphObj()->GetClass()->GetFName();

	if (graphClassName == "AnimationStateMachineGraph")
	{
		FNotificationInfo Info(NSLOCTEXT("NodeGraphAssistant", "StateMachineGraphRearrange", "Can not rearrange nodes for state machine graph"));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}
	if (graphClassName == "EdGraph_ReferenceViewer")
	{
		FNotificationInfo Info(NSLOCTEXT("NodeGraphAssistant", "RederenceGraphRearrange", "Can not rearrange nodes for referencer graph"));
		Info.ExpireDuration = 3.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
		return;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("NodeGraphAssistant", "RearrangeNodes", "Rearrange nodes"));
	if (graphClassName == "BehaviorTreeGraph" || graphClassName == "EnvironmentQueryGraph")
	{
		FIntPoint rearrangeSpacing = GetDefault<UNodeGraphAssistantConfig>()->NodesRearrangeSpacingAIGraph;
		FNodeHelper::RearrangeSelectedNodes_AIGraph(graphPanel.Get(), rearrangeSpacing, GetDefault<UNodeGraphAssistantConfig>()->NodesRearrangeSpacingRelaxFactor);
	}
	else
	{
		FIntPoint rearrangeSpacing = GetDefault<UNodeGraphAssistantConfig>()->NodesRearrangeSpacing;
		if (graphPanel->GetGraphObj()->GetClass() == UEdGraph::StaticClass())
		{
			//ed graph nodes become smaller when zoomed out,cause nodes to overlap when zoom in after rearrange.
			if (graphPanel.Get()->GetZoomAmount() < 0.22)
			{
				rearrangeSpacing.X += 100;
				rearrangeSpacing.Y += 30;
			}
			else if (graphPanel->GetZoomAmount() < 0.37)
			{
				rearrangeSpacing.X += 5;
				rearrangeSpacing.Y += 30;
			}
		}
		//todo
		FNodeHelper::RearrangeSelectedNodes(graphPanel.Get(), rearrangeSpacing, GetDefault<UNodeGraphAssistantConfig>()->NodesRearrangeSpacingRelaxFactor);
		FNodeHelper::RearrangeSelectedNodes(graphPanel.Get(), rearrangeSpacing, GetDefault<UNodeGraphAssistantConfig>()->NodesRearrangeSpacingRelaxFactor);
	}
	return;
}


//!before create node event.
FNGAEventReply NGAInputProcessor::TryProcessAsEndCreateNodeOnWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EnableCreateNodeOnWire)
	{
		return FNGAEventReply::UnHandled();
	}

	if (BlockNextClick && !MouseEvent.IsMouseButtonDown(MouseEvent.GetEffectingButton()))
	{
		if (Ctx.IsClickGesture && !Ctx.IsDraggingConnection && Ctx.IsCursorOnPanelEmptySpace)
		{
			BlockNextClick = false;
			return FNGAEventReply::BlockSlateInput();
		}
	}
	return FNGAEventReply::UnHandled();		
}

//todo:no undo if no wire connected.
FNGAEventReply NGAInputProcessor::TryProcessAsBeginLazyConnectEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EnableLazyConnect)
	{
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.GraphType != EGT_Default)
	{
		return FNGAEventReply::UnHandled();
	}

	if (Ctx.IsDraggingConnection)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && MouseEvent.IsShiftDown())
		{
			if (!HasBegunLazyConnect)
			{
				HasBegunLazyConnect = true;

				if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
				{
					GEditor->BeginTransaction(TEXT(""), NSLOCTEXT("NodeGraphAssistant", "LazyConnect", "Lazy connect"), NULL);
					return FNGAEventReply::Handled();
				}
			}
		}
	}

	return FNGAEventReply::UnHandled();
}

//when cursor is above a node while dragging,auto snap connection wire to closest connectible pin of the node.
//will make actual connection if any connect gesture was committed.
FNGAEventReply NGAInputProcessor::TryProcessAsLazyConnectMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, FNGAEventContex Ctx)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EnableLazyConnect)
	{
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.GraphType != EGT_Default)
	{
		return FNGAEventReply::UnHandled();
	}

	if (Ctx.IsDraggingConnection && Ctx.IsCursorInsidePanel && !IsPanningButtonDown(MouseEvent))
	{
		if (Ctx.GraphPanel.IsValid() && Ctx.GraphNode.IsValid())
		{
			TSharedPtr<FDragConnection> dragConnection = StaticCastSharedPtr<FDragConnection>(SlateApp.GetDragDroppingContent());
			if (dragConnection.IsValid())
			{
				TArray<UEdGraphPin*> draggingPins;
				dragConnection->ValidateGraphPinList(draggingPins);
				if (draggingPins.Num() > 0)
				{
					bool isStartNode = false;
					for (auto draggingPin : draggingPins)
					{
						//not detect self pins.
						if (draggingPin->GetOwningNode() == Ctx.GraphNode->GetNodeObj())
						{
							isStartNode = true;
							break;
						}
					}
					if (!isStartNode)
					{
						UK2Node_EditablePinBase* editableNode = Cast<UK2Node_EditablePinBase>(Ctx.GraphNode->GetNodeObj());
						FText aText;
						bool canCreateUserPin = editableNode && editableNode->CanCreateUserDefinedPin(draggingPins.Last()->PinType, (draggingPins.Last()->Direction == EGPD_Input) ? EGPD_Output : EGPD_Input, aText);
						TWeakPtr<SGraphPin> lazyConnectiblePin = MyPinFactory->GetLazyConnectiblePin();

						if (!(Ctx.IsNodeTitle && canCreateUserPin))
						{
							MyPinFactory->SetLazyConnectPayloadData(Ctx.GraphNode, draggingPins);
							if (Ctx.GraphPin.IsValid())
							{
								ECanCreateConnectionResponse response = Ctx.GraphPanel->GetGraphObj()->GetSchema()->CanCreateConnection(draggingPins[0], Ctx.GraphPin->GetPinObj()).Response;
								if (response == ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW || response == ECanCreateConnectionResponse::CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE)
								{
									//dont show lazy connectible pin,if hovered pin is not direct connectible.so user can see the info.
									MyPinFactory->ResetLazyConnectPayloadData();
								}
							}

							if (lazyConnectiblePin.IsValid())
							{
								if (!Ctx.GraphPin.IsValid())
								{
									dragConnection->SetSimpleFeedbackMessage(
										FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK")),
										FLinearColor::White,
										NSLOCTEXT("NodeGraphAssistant", "LazyConnect", ""));
								}

								if (HasBegunLazyConnect && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && MouseEvent.IsShiftDown())
								{
									dragConnection->SetHoveredPin(lazyConnectiblePin.Pin()->GetPinObj());
									lazyConnectiblePin.Pin()->OnDrop(Ctx.PinGeometry, FDragDropEvent(MouseEvent, dragConnection));
									dragConnection->SetHoveredPin(nullptr);
									dragConnection->SetSimpleFeedbackMessage(
										FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK")),
										FLinearColor::White,
										NSLOCTEXT("NodeGraphAssistant", "LazyConnect", "Lazy connect"));
								}
							}
						}
						if (Ctx.IsNodeTitle && canCreateUserPin)
						{
							dragConnection->HoverTargetChanged();
							//clear data,otherwise when click,will connect pin rather than create user pin.
							MyPinFactory->ResetLazyConnectPayloadData();
						}
						if (!Ctx.IsNodeTitle && canCreateUserPin && !Ctx.GraphPin.IsValid())
						{
							dragConnection->SetSimpleFeedbackMessage(
								lazyConnectiblePin.IsValid()? FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OK")) : nullptr,
								FLinearColor::White,
								NSLOCTEXT("NodeGraphAssistant", "LazyConnect", "Hover to node title to see more options"));
						}
						return FNGAEventReply::Handled();
					}
				}
			}
		}
	}

	MyPinFactory->ResetLazyConnectPayloadData();
	return FNGAEventReply::UnHandled();
}

FNGAEventReply NGAInputProcessor::TryProcessAsEndLazyConnectEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, FNGAEventContex Ctx)
{
	if (HasBegunLazyConnect)
	{
		if (!MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) || !MouseEvent.IsShiftDown() || !Ctx.IsDraggingConnection)
		{
			HasBegunLazyConnect = false;
			if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
			{
				if (GEditor->IsTransactionActive())
				{
					GEditor->EndTransaction();
					return FNGAEventReply::Handled();
				}
			}
		}
	}
	return FNGAEventReply::UnHandled();
}

//when dragging a node,auto detect surrounding pins it can linked to.
//will make connection when release drag.
FNGAEventReply NGAInputProcessor::TryProcessAsAutoConnectMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EnableAutoConnect)
	{
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.GraphType != EGT_Default)
	{
		return FNGAEventReply::UnHandled();
	}
	if (GetDefault<UNodeGraphAssistantConfig>()->AutoConnectModifier == EAutoConnectModifier::Alt && !MouseEvent.IsAltDown())
	{
		MyPinFactory->ResetAutoConnectPayloadData();
		return FNGAEventReply::UnHandled();
	}
	if (GetDefault<UNodeGraphAssistantConfig>()->AutoConnectModifier != EAutoConnectModifier::Alt && MouseEvent.IsAltDown())
	{
		MyPinFactory->ResetAutoConnectPayloadData();
		return FNGAEventReply::UnHandled();
	}
	

	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && NodeBeingDrag.IsValid() && !Ctx.IsDraggingConnection)
	{
		if (!MyPinFactory->PayLoadData->OutInsertableNodePinInfo.InputPin)
		{
			if (Ctx.GraphPanel.IsValid())
			{
				if (Ctx.GraphPanel->HasMouseCaptureByUser(MouseEvent.GetUserIndex()))
				{
					float autoConnectRadius = FMath::Min(GetDefault<UNodeGraphAssistantConfig>()->AutoConnectRadius, 500.f);
					TArray<TSharedRef<SGraphNode>> sourceNodes;
					TArray<TSharedRef<SGraphNode>> targetNodes;

					FChildren* visibleNodes = Ctx.GraphPanel->GetChildren();
					if (visibleNodes->Num() > 100)
					{
						return FNGAEventReply::UnHandled();
					}
					for (int i = 0; i < visibleNodes->Num(); i++)
					{
						auto visibleNode = StaticCastSharedRef<SGraphNode>(visibleNodes->GetChildAt(i));
						if (Ctx.GraphPanel->SelectionManager.SelectedNodes.Contains(visibleNode->GetNodeObj()))
						{
							sourceNodes.AddUnique(visibleNode);
						}
						else
						{
							targetNodes.AddUnique(visibleNode);
						}
					}
					TArray<TWeakPtr<SGraphPin>> startGraphPins;
					TArray<TWeakPtr<SGraphPin>> endGraphPins;

					FNodeHelper::GetAutoConnectablePins(Ctx.GraphPanel->GetGraphObj()->GetSchema(), autoConnectRadius, sourceNodes, targetNodes, startGraphPins, endGraphPins);

					if (startGraphPins.Num() > 0)
					{
						MyPinFactory->SetAutoConnectPayloadData(startGraphPins, endGraphPins);
						return FNGAEventReply::Handled();
					}
				}
			}
		}
	}
	MyPinFactory->ResetAutoConnectPayloadData();

	return FNGAEventReply::UnHandled();
}

FNGAEventReply NGAInputProcessor::TryProcessAsAutoConnectMouseUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EnableAutoConnect)
	{
		return FNGAEventReply::UnHandled();
	}
	if (GetDefault<UNodeGraphAssistantConfig>()->AutoConnectModifier == EAutoConnectModifier::Alt && !MouseEvent.IsAltDown())
	{
		return FNGAEventReply::UnHandled();
	}
	if (GetDefault<UNodeGraphAssistantConfig>()->AutoConnectModifier != EAutoConnectModifier::Alt && MouseEvent.IsAltDown())
	{
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.GraphType != EGT_Default)
	{
		return FNGAEventReply::UnHandled();
	}

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (NodeBeingDrag.IsValid() && Ctx.GraphPanel.IsValid())
		{
			TSet<UEdGraphNode*> NodeList;
			for (int i = 0; i < MyPinFactory->PayLoadData->AutoConnectStartPins.Num(); i++)
			{
				if (MyPinFactory->PayLoadData->AutoConnectStartPins[i].IsValid() && MyPinFactory->PayLoadData->AutoConnectEndPins[i].IsValid())
				{
					TSharedRef<SGraphPin> startPin = MyPinFactory->PayLoadData->AutoConnectStartPins[i].Pin().ToSharedRef();
					TSharedRef<SGraphPin> endPin = MyPinFactory->PayLoadData->AutoConnectEndPins[i].Pin().ToSharedRef();

					Ctx.GraphPanel->GetGraphObj()->GetSchema()->TryCreateConnection(startPin->GetPinObj(), endPin->GetPinObj());
					if (startPin->GetPinObj()->GetOwningNodeUnchecked())
					{
						NodeList.Add(startPin->GetPinObj()->GetOwningNodeUnchecked());
						
					}
					if (endPin->GetPinObj()->GetOwningNodeUnchecked())
					{
						NodeList.Add(endPin->GetPinObj()->GetOwningNodeUnchecked());
					}
				}
			}
			for (auto It = NodeList.CreateConstIterator(); It; ++It)
			{
				UEdGraphNode* Node = (*It);
				if (Node)
				{
					Node->NodeConnectionListChanged();
				}
			}
			MyPinFactory->ResetAutoConnectPayloadData();
			return FNGAEventReply::Handled();
		}
	}

	return FNGAEventReply::UnHandled();
}

FNGAEventReply NGAInputProcessor::TryProcessAsShakeNodeOffWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, FNGAEventContex Ctx)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EnableShakeNodeOffWire)
	{
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.GraphType == EGT_ReferenceViewer || Ctx.GraphType == EGT_PhysicsAsset || Ctx.GraphType == EGT_AnimStateMachine)
	{
		return FNGAEventReply::UnHandled();
	}

	if (Ctx.IsCursorInsidePanel && !IsPanningButtonDown(MouseEvent))//if drag pan node,
	{
		if (!NodeBeingDrag.IsValid())
		{
			ShakeOffNodeTracker.Empty();
		}
		else
		{
			float moveDIst = FVector2D::DistSquared(MouseEvent.GetScreenSpacePosition(), MouseEvent.GetLastScreenSpacePosition());
			if (moveDIst > 4 && moveDIst < 2048)
			{
				FVector2D moveDirection = MouseEvent.GetScreenSpacePosition() - MouseEvent.GetLastScreenSpacePosition();
				moveDirection.Normalize();
				ShakeOffNodeTrackigInfo newTrackingInfo = {SlateApp.GetCurrentTime(), moveDirection};

				if (ShakeOffNodeTracker.Num() == 0)
				{
					ShakeOffNodeTracker.Add(newTrackingInfo);
				}
				//clear buffer if after some time,otherwise have to shake the same direction as first shake.
				else if (newTrackingInfo.MouseMoveTime - ShakeOffNodeTracker.Last().MouseMoveTime >  GetDefault<UNodeGraphAssistantConfig>()->ShakeNodeOffWireTimeWindow)
				{
					ShakeOffNodeTracker.Empty();
					ShakeOffNodeTracker.Add(newTrackingInfo);
				}
				else
				{
					if (FVector2D::DotProduct(newTrackingInfo.MouseMoveDirection, ShakeOffNodeTracker.Last().MouseMoveDirection) < -0.9)
					{
						/*UE_LOG(LogTemp, Warning, TEXT("--------Shake node--------"));
						UE_LOG(LogTemp, Warning, TEXT("%f,%f"), MouseEvent.GetScreenSpacePosition().X, MouseEvent.GetScreenSpacePosition().Y);
						UE_LOG(LogTemp, Warning, TEXT("%f,%f"), MouseEvent.GetLastScreenSpacePosition().X, MouseEvent.GetLastScreenSpacePosition().Y)*/
						ShakeOffNodeTracker.Add(newTrackingInfo);
						if (ShakeOffNodeTracker.Num() == 3)
						{
							if (newTrackingInfo.MouseMoveTime - ShakeOffNodeTracker[0].MouseMoveTime < GetDefault<UNodeGraphAssistantConfig>()->ShakeNodeOffWireTimeWindow)
							{
								BypassSelectedNodes(true);
								ShakeOffNodeTracker.Empty();
							}
							else
							{
								ShakeOffNodeTracker.RemoveAt(0);
							}
						}
					}
				}
				return FNGAEventReply::Handled();
			}
		}
	}
	return FNGAEventReply::UnHandled();
}

FNGAEventReply NGAInputProcessor::TryProcessAsInsertNodeMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EnableInsertNodeOnWire)
	{
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.GraphType != EGT_Default)
	{
		return FNGAEventReply::UnHandled();
	}
	if (GetDefault<UNodeGraphAssistantConfig>()->AutoConnectModifier == EAutoConnectModifier::Alt && !MouseEvent.IsAltDown())
	{
		MyPinFactory->ResetInsertNodePayloadData();
		return FNGAEventReply::UnHandled();
	}
	if (GetDefault<UNodeGraphAssistantConfig>()->AutoConnectModifier != EAutoConnectModifier::Alt && MouseEvent.IsAltDown())
	{
		MyPinFactory->ResetInsertNodePayloadData();
		return FNGAEventReply::UnHandled();
	}


	if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && NodeBeingDrag.IsValid() && !Ctx.IsDraggingConnection)
	{
		if (Ctx.GraphPanel.IsValid())
		{
			TArray<TSharedRef<SGraphPin>> nodeInputPins;
			TArray<TSharedRef<SGraphPin>> nodeOutputPins;

			TArray<TSharedRef<SWidget>> graphNodePinWidgets;
			NodeBeingDrag.Pin()->GetPins(graphNodePinWidgets);
			for (auto graphNodePinWidget : graphNodePinWidgets)
			{
				TSharedRef<SGraphPin> pin = StaticCastSharedRef<SGraphPin>(graphNodePinWidget);
				if (pin->GetDirection() == EGPD_Input)
				{
					nodeInputPins.Add(pin);
				}
				else
				{
					nodeOutputPins.Add(pin);
				}
			}
			auto nodeBondMin = GraphPosToScreenPos(Ctx.GraphPanel.ToSharedRef(), Ctx.PanelGeometry, NodeBeingDrag.Pin()->GetPosition()) - MouseEvent.GetScreenSpacePosition();
			auto nodeBondMax = GraphPosToScreenPos(Ctx.GraphPanel.ToSharedRef(), Ctx.PanelGeometry, NodeBeingDrag.Pin()->GetPosition() + NodeBeingDrag.Pin()->GetDesiredSize()) - MouseEvent.GetScreenSpacePosition();
			MyPinFactory->PayLoadData->NodeBoundMinRelToCursor = nodeBondMin;
			MyPinFactory->PayLoadData->NodeBoundMaxRelToCursor = nodeBondMax;
			for (auto inputPin : nodeInputPins)
			{
				auto inPos = GraphPosToScreenPos(Ctx.GraphPanel.ToSharedRef(), Ctx.PanelGeometry, NodeBeingDrag.Pin()->GetPosition() + inputPin->GetNodeOffset()) - MouseEvent.GetScreenSpacePosition();
				for (auto outputPin : nodeOutputPins)
				{
					if (inputPin->GetPinObj()->LinkedTo.Num() == 0 && outputPin->GetPinObj()->LinkedTo.Num() == 0)
					{
						auto outPos = GraphPosToScreenPos(Ctx.GraphPanel.ToSharedRef(), Ctx.PanelGeometry, NodeBeingDrag.Pin()->GetPosition() + outputPin->GetNodeOffset()) - MouseEvent.GetScreenSpacePosition();
						MyPinFactory->PayLoadData->InsertNodePinInfos.Add({ inputPin,inPos, outputPin,outPos });
					}
				}
			}
			return FNGAEventReply::Handled();
		}
	}

	MyPinFactory->ResetInsertNodePayloadData();
	return FNGAEventReply::UnHandled();
}

FNGAEventReply NGAInputProcessor::TryProcessAsEndInsertNodeEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EnableInsertNodeOnWire)
	{
		return FNGAEventReply::UnHandled();
	}
	if (GetDefault<UNodeGraphAssistantConfig>()->AutoConnectModifier == EAutoConnectModifier::Alt && !MouseEvent.IsAltDown())
	{
		return FNGAEventReply::UnHandled();
	}
	if (GetDefault<UNodeGraphAssistantConfig>()->AutoConnectModifier != EAutoConnectModifier::Alt && MouseEvent.IsAltDown())
	{
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.GraphType != EGT_Default)
	{
		return FNGAEventReply::UnHandled();
	}

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && NodeBeingDrag.IsValid())
	{
		if (Ctx.GraphPanel.IsValid())
		{
			if (MyPinFactory->PayLoadData->OutInsertableNodePinInfo.InputPin && MyPinFactory->PayLoadData->OutInsertableNodePinInfo.OutputPin)
			{
				Ctx.GraphPanel->GetGraphObj()->GetSchema()->TryCreateConnection(MyPinFactory->PayLoadData->OutInsertableNodePinInfo.Params.AssociatedPin1, MyPinFactory->PayLoadData->OutInsertableNodePinInfo.InputPin);
				Ctx.GraphPanel->GetGraphObj()->GetSchema()->TryCreateConnection(MyPinFactory->PayLoadData->OutInsertableNodePinInfo.OutputPin, MyPinFactory->PayLoadData->OutInsertableNodePinInfo.Params.AssociatedPin2);
				MyPinFactory->ResetInsertNodePayloadData();
				return FNGAEventReply::Handled();
			}
		}
	}
	return FNGAEventReply::UnHandled();
}

FNGAEventReply NGAInputProcessor::TryProcessAsCreateNodeOnWireWithHotkeyEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->EnableCreateNodeOnWire)
	{
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.GraphType == EGT_AnimStateMachine)
	{
		return FNGAEventReply::UnHandled();
	}
	if (Ctx.GraphType == EGT_ReferenceViewer)
	{
		return FNGAEventReply::UnHandled();
	}

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		if (!Ctx.IsDraggingConnection && Ctx.IsCursorOnPanelEmptySpace && Ctx.IsClickGesture && PressedCharKey)
		{
			TSharedPtr<SGraphPanel> graphPanel = Ctx.GraphPanel;

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

			FReply reply = graphPanel->OnMouseButtonUp(Ctx.PanelGeometry, fakeMouseEvent);
			if (!reply.IsEventHandled() || !graphPanel->MarkedPin.IsValid())
			{
				graphPanel->MarkedPin = tempPin;
				return FNGAEventReply::UnHandled();
			}

			if (GetDefault<UNodeGraphAssistantConfig>()->CreateNodeOnlyOnSelectedWire && HighLightedPins.Array().Find(graphPanel->MarkedPin) == INDEX_NONE)
			{
				graphPanel->MarkedPin = tempPin;
				return FNGAEventReply::UnHandled();
			}

			if (graphPanel->MarkedPin.Pin().Get()->GetPinObj()->LinkedTo.Num() == 0)
			{
				return FNGAEventReply::UnHandled();
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
					if (FNodeHelper::GetWirePointHitResult(FArrangedWidget(graphPanel.ToSharedRef(), Ctx.PanelGeometry), markedPinA, markedPinB, MouseEvent.GetScreenSpacePosition(), 7, GetDefault<UGraphEditorSettings>()))
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
			else { return FNGAEventReply::UnHandled(); }

			UEdGraphNode* selectedNode = nullptr;
			if (graphPanel->SelectionManager.SelectedNodes.Num() > 0)
			{
				UEdGraphNode* graphNode = Cast<UEdGraphNode>(graphPanel->SelectionManager.SelectedNodes.Array()[0]);
				if (graphNode)
				{
					selectedNode = graphNode;
				}
			}

			//begin transaction here,before create new node may happen next.
			//need transaction for break single pin link.
			int32 tranIndex = INDEX_NONE;
			if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
			{
				tranIndex = GEditor->BeginTransaction(TEXT(""), NSLOCTEXT("NodeGraphAssistant", "CreateNode", "Create node"), NULL);
			}

			int* tryTimes = new int(2);
			NGADeferredEventDele deferredDele;
			deferredDele.BindLambda([selectedNode, graphPanel, pinA, pinB, tranIndex, this](int* tryTimes)
			{
				//only exit
				if (*tryTimes == 0)
				{
					if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
					{
						if (GEditor->IsTransactionActive() && tranIndex)
						{
							GEditor->CancelTransaction(tranIndex);//tranIndex equal INDEX_NONE will cause assertion in vs 
						}
					}
					return false;
				}
				if (!FSlateApplication::Get().AnyMenusVisible())
				{
					if (graphPanel.IsValid() && graphPanel->SelectionManager.SelectedNodes.Num() > 0)  //>0, possible auto conversion node.
					{
						//selection set might still be the old one(blueprint graph),so don't do it right away.
						if (*tryTimes > 1)
						{
							(*tryTimes)--;
							return false;
						}
						UEdGraphNode* newSelectedNode = Cast<UEdGraphNode>(graphPanel->SelectionManager.SelectedNodes.Array()[0]);
						if (newSelectedNode && newSelectedNode != selectedNode)
						{
							bool insertSuccess = true;
							for (auto newNodePin : newSelectedNode->Pins)
							{
								if (newNodePin->Direction == EGPD_Output)
								{
									if (newSelectedNode->GetSchema()->CanCreateConnection(newNodePin, pinB).Response != CONNECT_RESPONSE_DISALLOW)
									{
										insertSuccess &= newSelectedNode->GetSchema()->TryCreateConnection(newNodePin, pinB);
										break;
									}
								}
							}
							for (auto newNodePin : newSelectedNode->Pins)
							{
								if (newNodePin->Direction == EGPD_Input)
								{
									if (newSelectedNode->GetSchema()->CanCreateConnection(newNodePin, pinA).Response != CONNECT_RESPONSE_DISALLOW)
									{
										insertSuccess &= newSelectedNode->GetSchema()->TryCreateConnection(newNodePin, pinA);
										break;
									}
								}
							}
							if (insertSuccess)
							{
								FNodeHelper::BreakSinglePinLink(pinA, pinB);
							}
							else
							{
								FNotificationInfo Info(NSLOCTEXT("NodeGraphAssistant", "InsertNode", "Can not insert this node"));
								Info.ExpireDuration = 3.0f;
								FSlateNotificationManager::Get().AddNotification(Info);
							}
							//new node must be created now,end.
							if (GEditor && GEditor->Trans && !GEditor->bIsSimulatingInEditor)
							{
								if (GEditor->IsTransactionActive())
								{
									GEditor->EndTransaction();
								}
							}
						}
					}
					(*tryTimes)--;
				}
				return false;
			}, tryTimes);
			TickEventListener.Add(deferredDele);
		}
	}

	return FNGAEventReply::UnHandled();
}

void NGAInputProcessor::ExchangeWire(NGAInputProcessor* InputProcessor)
{
	TSharedPtr<SGraphPanel> graphPanel;
	FGeometry panelGeometry;

	FSlateApplication& slateApp = FSlateApplication::Get();
	FWidgetPath widgetsUnderCursor = slateApp.LocateWindowUnderMouse(slateApp.GetCursorPos(), slateApp.GetInteractiveTopLevelWindows());
	for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
	{
		if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString() == "SGraphPanel")
		{
			graphPanel = StaticCastSharedRef<SGraphPanel>(widgetsUnderCursor.Widgets[i].Widget);
			panelGeometry = widgetsUnderCursor.Widgets[i].Geometry;
		}
	}

	if (!graphPanel.IsValid())
	{
		return;
	}
	if (graphPanel->GetGraphObj()->GetClass()->GetFName() == "AnimationStateMachineGraph")
	{
		return;
	}
	if (graphPanel->GetGraphObj()->GetClass()->GetFName() == "EdGraph_ReferenceViewer")
	{
		return;
	}
	if (graphPanel->GetGraphObj()->GetClass()->GetFName() == "PhysicsAssetGraph")
	{
		return;
	}

	//if two pin on this graph is highlighted,use it first.
	TMap<UEdGraphPin*, TArray<UEdGraphPin*>> highlightPinMap;
	TSet<TSharedRef<SWidget>> allPanelPins;
	graphPanel->GetAllPins(allPanelPins);
	for (auto highlightedPin : InputProcessor->HighLightedPins)
	{
		if (!highlightedPin.IsValid())
		{
			continue;
		}
		auto highlightedPinPin = highlightedPin.Pin();
		for (auto panelPin : allPanelPins)
		{
			if (highlightedPinPin->GetPinObj() == StaticCastSharedRef<SGraphPin>(panelPin)->GetPinObj())
			{
				if (highlightedPinPin->GetPinObj()->LinkedTo.Num() > 0)
				{
					highlightPinMap.Add(highlightedPinPin->GetPinObj(), highlightedPinPin->GetPinObj()->LinkedTo);
				}
			}
		}
	}
	for (auto pinPair1 : highlightPinMap)
	{
		for (auto pinPair2 : highlightPinMap)
		{
			if (pinPair1 != pinPair2 && pinPair1.Key && pinPair1.Value.Num() > 0 && pinPair2.Key && pinPair2.Value.Num() > 0)
			{
				TArray<UEdGraphPin*> startA;
				startA.Add(pinPair1.Key);
				TArray<UEdGraphPin*> endA = pinPair1.Value;
				TArray<UEdGraphPin*> startB;
				startB.Add(pinPair2.Key);
				TArray<UEdGraphPin*> endB = pinPair2.Value;
				if (pinPair1.Key->Direction != pinPair2.Key->Direction)
				{
					startB = pinPair2.Value;
					endB.Empty();
					endB.Add(pinPair2.Key);
				}

				ECanCreateConnectionResponse Response;
				Response = graphPanel->GetGraphObj()->GetSchema()->CanCreateConnection(startA[0], endB[0]).Response;
				if (Response != CONNECT_RESPONSE_DISALLOW && Response != CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE)
				{
					const FScopedTransaction Transaction(NSLOCTEXT("NodeGraphAssistant", "ExchangeWires", "Exchange wires"));
					for (auto pin1 : startA)
					{
						for (auto pin2 : endB)
						{
							graphPanel->GetGraphObj()->GetSchema()->TryCreateConnection(pin1, pin2);
						}
					}
					for (auto pin1 : startB)
					{
						for (auto pin2 : endA)
						{
							graphPanel->GetGraphObj()->GetSchema()->TryCreateConnection(pin1, pin2);
						}
					}

					for (auto pin : endA)
					{
						pin->BreakLinkTo(startA[0]);
					}
					if (pinPair1.Key->Direction == pinPair2.Key->Direction)
					{
						for (auto pin : endB)
						{
							pin->BreakLinkTo(startB[0]);
						}
					}
					else
					{
						for (auto pin : startB)
						{
							endB[0]->BreakLinkTo(pin);
						}
					}
					return;
				}
			}
		}
	}

	//if can not find two pair of highlighted pins,find inside selected node instead.
	if(graphPanel->SelectionManager.SelectedNodes.Num() > 0)
	{
		TMap<UEdGraphPin*, TArray<UEdGraphPin*>> selectNodePinMapInput;
		TMap<UEdGraphPin*, TArray<UEdGraphPin*>> selectNodePinMapOutput;
		float nodeAvePosX = 0;
		for (auto selectedNode : graphPanel->SelectionManager.SelectedNodes)
		{
			UEdGraphNode* graphNode = Cast<UEdGraphNode>(selectedNode);
			if (!graphNode)
			{
				continue;
			}
			nodeAvePosX += graphNode->NodePosX;
			for (auto pin : graphNode->Pins)
			{
				if (pin->LinkedTo.Num() > 0)
				{
					if (pin->Direction == EGPD_Input)
					{
						selectNodePinMapInput.Add(pin, pin->LinkedTo);
					}
					else
					{
						selectNodePinMapOutput.Add(pin, pin->LinkedTo);
					}
				}
			}
		}
		nodeAvePosX /= graphPanel->SelectionManager.SelectedNodes.Num();

		TMap<UEdGraphPin*, TArray<UEdGraphPin*>> selectNodePinMap;
		if (selectNodePinMapInput.Num() > 1 && selectNodePinMapOutput.Num() < 2)
		{
			selectNodePinMap = selectNodePinMapInput;
		}
		else if (selectNodePinMapOutput.Num() > 1 && selectNodePinMapInput.Num() < 2)
		{
			selectNodePinMap = selectNodePinMapOutput;
		}
		else
		{
			if (graphPanel->PanelCoordToGraphCoord(panelGeometry.AbsoluteToLocal(slateApp.GetCursorPos())).X > nodeAvePosX)
			{
				selectNodePinMap = selectNodePinMapOutput;
			}
			else
			{
				selectNodePinMap = selectNodePinMapInput;
			}
		}

		for (auto pinPair1 : selectNodePinMap)
		{
			for (auto pinPair2 : selectNodePinMap)
			{
				if (pinPair1 != pinPair2 && pinPair1.Key && pinPair1.Value.Num() > 0 && pinPair2.Key && pinPair2.Value.Num() > 0)
				{
					TArray<UEdGraphPin*> startA;
					startA.Add(pinPair1.Key);
					TArray<UEdGraphPin*> endA = pinPair1.Value;
					TArray<UEdGraphPin*> startB;
					startB.Add(pinPair2.Key);
					TArray<UEdGraphPin*> endB = pinPair2.Value;
					if (pinPair1.Key->Direction != pinPair2.Key->Direction)
					{
						startB = pinPair2.Value;
						endB.Empty();
						endB.Add(pinPair2.Key);
					}

					ECanCreateConnectionResponse Response;
					Response = graphPanel->GetGraphObj()->GetSchema()->CanCreateConnection(startA[0], endB[0]).Response;
					if (Response != CONNECT_RESPONSE_DISALLOW && Response != CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE)
					{
						const FScopedTransaction Transaction(NSLOCTEXT("NodeGraphAssistant", "ExchangeWires", "Exchange wires"));
						for (auto pin1 : startA)
						{
							for (auto pin2 : endB)
							{
								graphPanel->GetGraphObj()->GetSchema()->TryCreateConnection(pin1, pin2);
							}
						}
						for (auto pin1 : startB)
						{
							for (auto pin2 : endA)
							{
								graphPanel->GetGraphObj()->GetSchema()->TryCreateConnection(pin1, pin2);
							}
						}

						for (auto pin : endA)
						{
							pin->BreakLinkTo(startA[0]);
						}
						if (pinPair1.Key->Direction == pinPair2.Key->Direction)
						{
							for (auto pin : endB)
							{
								pin->BreakLinkTo(startB[0]);
							}
						}
						else
						{
							for (auto pin : startB)
							{
								endB[0]->BreakLinkTo(pin);
							}
						}
						return;
					}
				}
			}
		}
	}
	
	FNotificationInfo Info(NSLOCTEXT("NodeGraphAssistant", "ExchangeWires", "Did not find exchangeable wires."));
	Info.ExpireDuration = 3.0f;
	FSlateNotificationManager::Get().AddNotification(Info);
}


void NGAInputProcessor::DupliNodeWithWire()
{
	TSharedPtr<SGraphPanel> graphPanel = GetCurrentGraphPanel();
	if (!graphPanel.IsValid())
	{
		return;
	}
	if (graphPanel->GetGraphObj()->GetClass()->GetFName() == "AnimationStateMachineGraph")
	{
		return;
	}
	if (graphPanel->GetGraphObj()->GetClass()->GetFName() == "EdGraph_ReferenceViewer")
	{
		return;
	}
	if (graphPanel->GetGraphObj()->GetClass()->GetFName() == "PhysicsAssetGraph")
	{
		return;
	}

	TArray<UEdGraphNode*> selectedNodes;
	for (auto selectedNode : graphPanel->SelectionManager.SelectedNodes)
	{
		UEdGraphNode* graphNode = Cast<UEdGraphNode>(selectedNode);
		if (graphNode && graphNode->CanDuplicateNode())
		{
			selectedNodes.Add(graphNode);
		}
	}
	if (selectedNodes.Num() > 0)
	{
		FSlateApplication& SlateApp = FSlateApplication::Get();
		auto keyEvent = FKeyEvent(EKeys::W,
#if PLATFORM_MAC
			FModifierKeysState(false, false, false, false, false, false, true, false, false),
#else
			FModifierKeysState(false, false, true, false, false, false, false, false, false),
#endif
			0,
			false,
			0,
			0
		);
		SlateApp.ProcessKeyDownEvent(keyEvent);

		int* timer = new int(1);
		NGADeferredEventDele deferredDele;
		deferredDele.BindLambda([timer, graphPanel, selectedNodes]()
		{
			if (*timer == 0)
			{
				delete timer;

				if (!graphPanel.IsValid())
				{
					return true;
				}
				TArray<UEdGraphNode*> newSelectedNodes;
				for (auto selectedNode : graphPanel->SelectionManager.SelectedNodes)
				{
					UEdGraphNode* graphNode = Cast<UEdGraphNode>(selectedNode);
					if (!graphNode)
					{
						continue;
					}
					newSelectedNodes.Add(graphNode);
				}
				if (newSelectedNodes.Num() != selectedNodes.Num())
				{
					return true;
				}
				for (int nodeIndex = 0; nodeIndex < selectedNodes.Num(); nodeIndex++)
				{
					for (int pinIndex = 0; pinIndex < selectedNodes[nodeIndex]->Pins.Num(); pinIndex++)
					{
						if (selectedNodes[nodeIndex]->Pins[pinIndex]->Direction == EGPD_Input)
						{
							auto selectNodePinLinkto = selectedNodes[nodeIndex]->Pins[pinIndex]->LinkedTo;
							for (auto linkedto : selectNodePinLinkto)
							{
								if (!selectedNodes.Contains(linkedto->GetOwningNode()))
								{
									if (graphPanel->GetGraphObj()->GetSchema()->CanCreateConnection(linkedto, newSelectedNodes[nodeIndex]->Pins[pinIndex]).Response == CONNECT_RESPONSE_MAKE)
									{
										graphPanel->GetGraphObj()->GetSchema()->TryCreateConnection(linkedto, newSelectedNodes[nodeIndex]->Pins[pinIndex]);
									}
								}
							}
						}
					}
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
	}
}


void NGAInputProcessor::OnSelectLinkedNodes(bool IsDownStream, bool bUpStream)
{
	TSharedPtr<SGraphPanel> graphPanel = GetCurrentGraphPanel();
	if (!graphPanel.IsValid())
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

void NGAInputProcessor::ConnectNodes()
{
	TSharedPtr<SGraphPanel> graphPanel = GetCurrentGraphPanel();
	if (!graphPanel.IsValid())
	{
		return;
	}
	if (graphPanel->GetGraphObj()->GetClass()->GetFName() == "AnimationStateMachineGraph")
	{
		return;
	}
	if (graphPanel->GetGraphObj()->GetClass()->GetFName() == "EdGraph_ReferenceViewer")
	{
		return;
	}
	if (graphPanel->GetGraphObj()->GetClass()->GetFName() == "PhysicsAssetGraph")
	{
		return;
	}

	TArray<TSharedRef<SGraphNode>> targetNodes;
	FChildren* allNodes = graphPanel->GetChildren();
	for (int i = 0; i < allNodes->Num(); i++)
	{
		auto node = StaticCastSharedRef<SGraphNode>(allNodes->GetChildAt(i));
		if (graphPanel->SelectionManager.SelectedNodes.Contains(node->GetNodeObj()))
		{
			targetNodes.AddUnique(node);
		}
	}

	const FScopedTransaction Transaction(NSLOCTEXT("NodeGraphAssistant", "ConnectNodes", "Connect Nodes"));
	for (TSharedRef<SGraphNode> targetNode : targetNodes)
	{
		TArray<TSharedRef<SGraphNode>> sourceNodes;
		sourceNodes.Add(targetNode);
		TArray<TWeakPtr<SGraphPin>> startGraphPins;
		TArray<TWeakPtr<SGraphPin>> endGraphPins;
		FNodeHelper::GetAutoConnectablePins(graphPanel->GetGraphObj()->GetSchema(), -1, sourceNodes, targetNodes, startGraphPins, endGraphPins);

		for (int i = 0; i < startGraphPins.Num(); i++)
		{
			if (startGraphPins[i].IsValid() && endGraphPins[i].IsValid())
			{
				TSharedRef<SGraphPin> startPin = startGraphPins[i].Pin().ToSharedRef();
				TSharedRef<SGraphPin> endPin = endGraphPins[i].Pin().ToSharedRef();
				graphPanel->GetGraphObj()->GetSchema()->TryCreateConnection(startPin->GetPinObj(), endPin->GetPinObj());
			}
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

	FNGAEventContex ctx = InitEventContex(SlateApp, MouseEvent);

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (ctx.IsCursorInsidePanel && !ctx.IsDraggingConnection)
		{
			if (!SlateApp.HasAnyMouseCaptor())
			{
				if (ctx.GraphNode.IsValid() || ctx.CommentNode.IsValid())
				{
					if (SlateApp.GetPlatformCursor().IsValid())
					{
						//when cursor is CardinalCross,its draggable part of the node
						auto cursorIcon = SlateApp.GetPlatformCursor()->GetType();
						if (cursorIcon == EMouseCursor::CardinalCross || cursorIcon == EMouseCursor::Default)
						{
							NodeBeingDrag = ctx.GraphNode;
							CommentNodeBeingDrag = ctx.CommentNode;
							if (GetDefault<UNodeGraphAssistantConfig>()->HideToolTipWhenDraggingNode)
							{
								//todo,use cvar
#if ENGINE_MINOR_VERSION > 16
								SlateApp.SetAllowTooltips(false);
#endif
							}				
						}
					}
				}
			}
		}
	}

	if (ctx.IsCursorOnPanelEmptySpace && !ctx.IsDraggingConnection)
	{
		//engine will use last factory in factory array when creation connection draw policy
		//this will override previous factory even if the last one can not actual create correct factory
		//we need to make sure its always the last factory,and it need to include all function of any other factory
#if ENGINE_MINOR_VERSION < 19
		FEdGraphUtilities::UnregisterVisualPinConnectionFactory(MyPinFactory);
		FEdGraphUtilities::RegisterVisualPinConnectionFactory(MyPinFactory);
#endif
	}


	FNGAEventReply reply;

	reply.Append(TryProcessAsBeginDragNPanEvent(SlateApp, MouseEvent, ctx));

	reply.Append(TryProcessAsDupliWireEvent(SlateApp, MouseEvent, ctx));

	reply.Append(TryProcessAsBeginLazyConnectEvent(SlateApp, MouseEvent, ctx));

	reply.Append(TryProcessAsBeginCutOffWireEvent(SlateApp, MouseEvent, ctx));
	
	if (reply.ShouldBlockInput)
	{
		return true;
	}

	return false;
}


bool NGAInputProcessor::HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	FNGAEventContex ctx = InitEventContex(SlateApp, MouseEvent);

	float deltaTime = MouseUpDeltaTime;
	MouseUpDeltaTime = 0.f;
	float deltaPos = (LastMouseUpScreenPos - MouseEvent.GetScreenSpacePosition()).Size();
	LastMouseUpScreenPos = MouseEvent.GetScreenSpacePosition();
	if (deltaTime < 0.5f && deltaPos < 5)
	{
		ctx.IsDoubleClickGesture = true;
	}

	if (MouseButtonDownResponsivnessThrottle.IsValid())
	{
		FSlateThrottleManager::Get().LeaveResponsiveMode(MouseButtonDownResponsivnessThrottle);
	}


	FNGAEventReply reply;

	reply.Append(TryProcessAsClusterHighlightEvent(SlateApp, MouseEvent, ctx));

	reply.Append(TryProcessAsSelectStreamEvent(SlateApp, MouseEvent, ctx));

	reply.Append(TryProcessAsEndDragNPanEvent(SlateApp, MouseEvent, ctx));

	reply.Append(TryProcessAsMultiConnectEvent(SlateApp, MouseEvent, ctx));

	reply.Append(TryProcessAsSingleHighlightEvent(SlateApp, MouseEvent, ctx));

	reply.Append(TryProcessAsEndCreateNodeOnWireEvent(SlateApp, MouseEvent, ctx));//!before create node event,todo

	reply.Append(TryProcessAsCreateNodeOnWireEvent(SlateApp, MouseEvent, ctx));

	reply.Append(TryProcessAsAutoConnectMouseUpEvent(SlateApp, MouseEvent, ctx));

	reply.Append(TryProcessAsEndCutOffWireEvent(SlateApp, MouseEvent, ctx));

	reply.Append(TryProcessAsEndLazyConnectEvent(SlateApp, MouseEvent, ctx));

	reply.Append(TryProcessAsEndInsertNodeEvent(SlateApp, MouseEvent, ctx));

	reply.Append(TryProcessAsCreateNodeOnWireWithHotkeyEvent(SlateApp, MouseEvent, ctx));

	
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && (NodeBeingDrag.IsValid() || CommentNodeBeingDrag.IsValid()))
	{
		NodeBeingDrag.Reset();
		CommentNodeBeingDrag.Reset();
		if (GetDefault<UNodeGraphAssistantConfig>()->HideToolTipWhenDraggingNode)
		{
			RefreshToolTipWhenMouseMove = true;
		}
	}

	if (reply.ShouldBlockInput)
	{
		return true;
	}

	return false;
}


bool NGAInputProcessor::HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	FNGAEventContex ctx = InitEventContex(SlateApp, MouseEvent);

	//comment
	if (!MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) &&(NodeBeingDrag.IsValid() || CommentNodeBeingDrag.IsValid()))
	{
		NodeBeingDrag.Reset();
		CommentNodeBeingDrag.Reset();
		if (GetDefault<UNodeGraphAssistantConfig>()->HideToolTipWhenDraggingNode)
		{
			RefreshToolTipWhenMouseMove = true;
		}
	}
	if (RefreshToolTipWhenMouseMove)
	{
		RefreshToolTipWhenMouseMove = false;
		SlateApp.SpawnToolTip(SlateApp.MakeToolTip(FText::FromString(" ")), FVector2D(0, 0)); //force refresh,otherwise tooltip wont show up until hover widget change.
#if ENGINE_MINOR_VERSION > 16
		SlateApp.SetAllowTooltips(true);
#endif
	}

	//send highlighted pins that we keep track of to slate,so slate can draw them.
	if (ctx.GraphPanel.IsValid())
	{
		for (auto highlightedPin : HighLightedPins)
		{
			if (highlightedPin.IsValid())
			{
				ctx.GraphPanel->AddPinToHoverSet(highlightedPin.Pin()->GetPinObj());
			}
		}
	}

	//refresh communication with pin factory when mouse move.
	MyPinFactory->PayLoadData->HoveredGraphPanel = ctx.GraphPanel;
	

	TryProcessAsBeingDragNPanEvent(SlateApp, MouseEvent, ctx);//first

	TryProcessAsLazyConnectMouseMoveEvent(SlateApp, MouseEvent, ctx);

	TryProcessAsAutoConnectMouseMoveEvent(SlateApp, MouseEvent, ctx);

	TryProcessAsShakeNodeOffWireEvent(SlateApp, MouseEvent, ctx);

	TryProcessAsBeingCutOffWireEvent(SlateApp, MouseEvent, ctx);

	TryProcessAsEndLazyConnectEvent(SlateApp, MouseEvent, ctx);

	TryProcessAsEndCutOffWireEvent(SlateApp, MouseEvent, ctx);

	TryProcessAsInsertNodeMouseMoveEvent(SlateApp, MouseEvent, ctx);

	//if slate lose focus while drag and panning,the mouse will not be able to show up,so make a check here. 
	if (DidIHideTheCursor && !ctx.IsDraggingConnection && !IsPanningButtonDown(MouseEvent))
	{
		if (SlateApp.GetPlatformCursor().IsValid())
		{
			SlateApp.GetPlatformCursor()->Show(true);
			DidIHideTheCursor = false;
		}
	}

	//update visual elements(decorator window) for dragging.
	if(ctx.IsDraggingConnection && !IsPanningButtonDown(MouseEvent))
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
				if (ctx.IsCursorOnPanelEmptySpace)
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
								IsPanningButton(EKeys::RightMouseButton)? NSLOCTEXT("NodeGraphAssistant", "MultiDrop", "Right click cancel ") :NSLOCTEXT("NodeGraphAssistant", "MultiDrop", "Middle click cancel "));
						}
					}
				}
			}
		}
	}

	return false;
}


#if ENGINE_MINOR_VERSION >= 21
bool NGAInputProcessor::HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	FNGAEventContex ctx = InitEventContex(SlateApp, MouseEvent);
	ctx.IsDoubleClickGesture = true;

	TryProcessAsSingleHighlightEvent(SlateApp, MouseEvent, ctx);

	return false;
}
#endif


bool NGAInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetCharacter() > 0)
	{
		PressedCharKey = InKeyEvent.GetCharacter();
	}

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
	PressedCharKey = 0;

	//shift cancel multi-drop
	if ((InKeyEvent.GetKey() == EKeys::LeftShift || InKeyEvent.GetKey() == EKeys::RightShift) && InKeyEvent.GetUserIndex() >= 0)
	{
		if (IsDraggingConnection())
		{
			CancelDraggingReset(InKeyEvent.GetUserIndex());
			return true;
		}
	}

	return false;
}
#pragma optimize("", on)