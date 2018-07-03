// Copyright 2018 yangxiangyun
// All Rights Reserved

#include "OverrideSlateInputProcessor.h"
#include "CoreMinimal.h"

#include "SlateApplication.h"
#include "DragConnection.h"
#include "SGraphPanel.h"
#include "GraphEditorSettings.h"

#include "EditorStyleSet.h"


//check to make sure we dont touch anything we are not interested.
bool FOverrideSlateInputProcessor::IsCursorInsidePanel(const FPointerEvent& MouseEvent)
{
	FSlateApplication& slateApp = FSlateApplication::Get();
	FWidgetPath WidgetsUnderCursor = slateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), slateApp.GetInteractiveTopLevelWindows());
	for (int i = WidgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
	{
		if (WidgetsUnderCursor.Widgets[i].Widget->GetTypeAsString() == "SGraphPanel")
		{
			return true;
		}
	}
	return false;
}


bool FOverrideSlateInputProcessor::IsDraggingConnection()
{
	FSlateApplication& slateApp = FSlateApplication::Get();
	return slateApp.IsDragDropping() && slateApp.GetDragDroppingContent()->IsOfType<FDragConnection>();
}


bool FOverrideSlateInputProcessor::IsTryingToMultiDrop(const FPointerEvent& MouseEvent)
{
	if (IsCursorInsidePanel(MouseEvent))
	{
		if (IsDraggingConnection())
		{
			if (!MouseEvent.IsMouseButtonDown(EKeys::AnyKey))
			{
				//if user is releasing mouse while panning,dont drop,even if cursor is above widget,this is for preventing accidentally drop on graphpin while drag and panning.
				if ((MouseEvent.GetScreenSpacePosition() - LastMouseDownScreenPos).SizeSquared() < 8)
				{
					if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
					{
						return true;
					}
					if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && CanBeginOrEndPan(MouseEvent))
					{
						return true;
					}
				}
				else if (MouseEvent.IsShiftDown() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					return true;
				}
			}
		}
	}
	return false;
}


bool FOverrideSlateInputProcessor::CanBeginOrEndPan(const FPointerEvent& MouseEvent)
{
	if (IsCursorInsidePanel(MouseEvent))
	{
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
	}
	return false;
}


bool FOverrideSlateInputProcessor::IsPanning(const FPointerEvent & MouseEvent)
{
	if (IsCursorInsidePanel(MouseEvent))
	{
		if (!MouseEvent.GetEffectingButton().IsValid())
		{
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
		}
	}
	return false;
}


void FOverrideSlateInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
	//send a deferred event to connect a pin at specific screen position,see line:206 for more.
	if (SendDeferredMouseEvent)
	{
		DeferredMouseEventTimer += DeltaTime;
		//send the event after second tick,so the graph is already refreshed and can accept pin connection.
		if (DeferredMouseEventTimer > 2 * DeltaTime)
		{
			DeferredMouseEventTimer = 0;
			SendDeferredMouseEvent = false;

			FPointerEvent MouseEvent(
				DeferredMouseEvent.GetUserIndex(),
				DeferredMouseEvent.GetScreenSpacePosition(),
				DeferredMouseEvent.GetScreenSpacePosition(),
				TSet<FKey>(),
				EKeys::Invalid,
				0,
				FModifierKeysState()
			);
			//send move event,otherwise drag operation wont know which pin to connect.
			SlateApp.ProcessMouseMoveEvent(MouseEvent);
			LastMouseDownScreenPos = DeferredMouseEvent.GetScreenSpacePosition();
			SlateApp.OnMouseUp(EMouseButtons::Left, DeferredMouseEvent.GetScreenSpacePosition());
		}
	}
}


bool FOverrideSlateInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	LastMouseDownScreenPos = MouseEvent.GetScreenSpacePosition();

	//shift click to replicate connection.
	//by faking a ctrl click event to pluck a connection then immediately do a multi-drop to plug it back on without losing current dragdrop operation.
	if (IsCursorInsidePanel(MouseEvent))
	{
		if (!SlateApp.IsDragDropping() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && MouseEvent.IsShiftDown())
		{
			FWidgetPath WidgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
			FScopedSwitchWorldHack SwitchWorld(WidgetsUnderCursor);

			for (int i = WidgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
			{
				//graphpin has multiple inheritances,generally contain a word "GraphPin".
				//TODO:better type check,or just dont check.
				if (WidgetsUnderCursor.Widgets[i].Widget->GetTypeAsString().Contains("GraphPin"))
				{
					TSet<FKey> fakeKeys;
					fakeKeys.Add(MouseEvent.GetEffectingButton());

					FPointerEvent fakeMouseEvent(
						MouseEvent.GetPointerIndex(),
						MouseEvent.GetScreenSpacePosition(),
						MouseEvent.GetLastScreenSpacePosition(),
						fakeKeys,
						MouseEvent.GetEffectingButton(),
						0,
						FModifierKeysState(false, false, true, false, false, false, false, false, false));//faking ctrl click

					bool ret = true;
					ret &= SlateApp.ProcessMouseButtonDownEvent(TSharedPtr<FGenericWindow>(), fakeMouseEvent);
					ret &= SlateApp.ProcessMouseButtonUpEvent(const_cast<FPointerEvent&>(MouseEvent));

					//for some types of pin,when connection break,it will reconstruct,and graph will refresh async.
					//so we cant reconnect it here,because the pin handle will be invalid this tick.
					//send a deferred mouse click event to connect it back after few ticks when pin is reconstructed and graph is refreshed.
					if (ret)
					{
						SendDeferredMouseEvent = true;
						DeferredMouseEvent = MouseEvent;
					}

					return ret;
				}
			}
		}
	}

	//by default panning is bypassed when dragging connection(slate.cpp 5067), to be able to pan while dragging,need to manually route the pointer down event to graph panel.
	//make sure no widget captured the mouse so we can pan now.
	if (IsDraggingConnection() && CanBeginOrEndPan(MouseEvent) && !SlateApp.HasUserMouseCapture(MouseEvent.GetUserIndex()))
	{
		FWidgetPath WidgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
		FScopedSwitchWorldHack SwitchWorld(WidgetsUnderCursor);

		for (int i = WidgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
		{
			if (WidgetsUnderCursor.Widgets[i].Widget->GetTypeAsString() == "SGraphPanel")
			{
				//software cursor is in panel space,but preview connection wire is in screen space,when drag and paning,their positions will become mismatched.
				//to fix that we need to cache the software cursor panel position,so we can convert it to correct screen space position when drag and paning to keep connection wire aligned with it.
				auto ZoomStartOffset = WidgetsUnderCursor.Widgets[i].Geometry.AbsoluteToLocal(MouseEvent.GetLastScreenSpacePosition());
				SoftwareCursorPanelPosCached = static_cast<SGraphPanel*>(&WidgetsUnderCursor.Widgets[i].Widget.Get())->PanelCoordToGraphCoord(ZoomStartOffset);

				//this fake event exclude left mouse button as pressed,so node panel wont display software currsor as "up-down" icon(node panel thinks we are going to zoom,but we are not).
				TSet<FKey> fakeKeys;
				fakeKeys.Add(MouseEvent.GetEffectingButton());
				FPointerEvent fakeMouseEvent(
					MouseEvent.GetPointerIndex(),
					MouseEvent.GetScreenSpacePosition(),
					MouseEvent.GetLastScreenSpacePosition(),
					fakeKeys,
					MouseEvent.GetEffectingButton(),
					0,
					FModifierKeysState()
				);

				FReply reply = SlateApp.RoutePointerDownEvent(WidgetsUnderCursor, fakeMouseEvent);
				if (reply.IsEventHandled())//directly route event to node panel.
				{
					if (reply.ShouldThrottle() && !MouseButtonDownResponsivnessThrottle.IsValid())
					{
						MouseButtonDownResponsivnessThrottle = FSlateThrottleManager::Get().EnterResponsiveMode();
					}
					else if (!reply.ShouldThrottle() && MouseButtonDownResponsivnessThrottle.IsValid())
					{
						// Leave responsive mode if a widget chose not to throttle
						FSlateThrottleManager::Get().LeaveResponsiveMode(MouseButtonDownResponsivnessThrottle);
					}

					return true;
				}
				break;
			}
		}
	}

	return false;
}


bool FOverrideSlateInputProcessor::HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (MouseButtonDownResponsivnessThrottle.IsValid())
	{
		FSlateThrottleManager::Get().LeaveResponsiveMode(MouseButtonDownResponsivnessThrottle);
	}

	//multi-drop
	//by routing drop event without destroying dragdrop operation
	if (IsTryingToMultiDrop(MouseEvent))
	{
		FWidgetPath WidgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
		FScopedSwitchWorldHack SwitchWorld(WidgetsUnderCursor);

		for (int i = WidgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
		{
			if (WidgetsUnderCursor.Widgets[i].Widget->GetTypeAsString() == "SGraphPanel")
			{
				//prevent pop up menu from appearing when click on empty space.
				if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					return true;
				}

				//right click on empty space ends drag-drop operation.
				if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
				{
					TSharedPtr< FDragDropOperation > LocalDragDropContent = SlateApp.GetDragDroppingContent();
					SlateApp.CancelDragDrop();
					LocalDragDropContent->OnDrop(true, FPointerEvent());

					if (SlateApp.HasUserMouseCapture(MouseEvent.GetUserIndex()))
					{
						SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
					}

					SlateApp.GetPlatformCursor()->Show(true);
					return true;
				}
			}

			//dropping
			FReply Reply = WidgetsUnderCursor.Widgets[i].Widget->OnDrop(WidgetsUnderCursor.Widgets[i].Geometry, FDragDropEvent(MouseEvent, SlateApp.GetDragDroppingContent()));
			if (Reply.IsEventHandled())
			{
				//this is for when its right mouse button up,because when right mouse button down we bypass slate and manually call RoutePointerDownEvent for drag and panning,
				//so we need to do some clean up to cancel out the pointer event we called and release capture that is created by it,otherwise wierd things will happen.
				if (SlateApp.HasUserMouseCapture(MouseEvent.GetUserIndex()))
				{
					SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
				}

				return true;
			}
		}
	}

	//convert regular click on empty space to shift click,so graph panel will set connection wire under cursor as marked and highlight it,so we dont need to press "shift".
	if (IsCursorInsidePanel(MouseEvent) && !IsDraggingConnection() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !MouseEvent.IsLeftShiftDown())
	{
		if (MouseEvent.GetScreenSpacePosition() == LastMouseDownScreenPos)
		{
			FWidgetPath WidgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
			//panel is the last widget means click on empty space.
			if (WidgetsUnderCursor.Widgets.Num() > 0 && WidgetsUnderCursor.Widgets.Last().Widget->GetTypeAsString() == "SGraphPanel")
			{
				SGraphPanel* graphPanel = static_cast<SGraphPanel*>(&WidgetsUnderCursor.Widgets.Last().Widget.Get());
				graphPanel->MarkedPin.Reset();

				TSet<FKey> fakeKeys;
				fakeKeys.Add(MouseEvent.GetEffectingButton());

				FPointerEvent fakeMouseEvent(
					MouseEvent.GetPointerIndex(),
					MouseEvent.GetScreenSpacePosition(),
					MouseEvent.GetLastScreenSpacePosition(),
					fakeKeys,
					MouseEvent.GetEffectingButton(),
					0,
					FModifierKeysState(true, false, false, false, false, false, false, false, false)
				);
				if (SlateApp.RoutePointerUpEvent(WidgetsUnderCursor, fakeMouseEvent).IsEventHandled())
				{
					//click on wire will create mouse capture,causing graphpin to be not clickable at first click,so release capture.
					if (SlateApp.HasUserMouseCapture(MouseEvent.GetUserIndex()))
					{
						SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
					}

					return true;
				}
			}
		}
	}

	if (IsDraggingConnection() && CanBeginOrEndPan(MouseEvent))
	{
		SlateApp.GetPlatformCursor()->Show(true);
		SlateApp.GetDragDroppingContent()->SetDecoratorVisibility(true);
	}

	return false;
}

bool FOverrideSlateInputProcessor::HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	if (IsDraggingConnection() && IsPanning(MouseEvent))
	{
		FWidgetPath WidgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
		FScopedSwitchWorldHack SwitchWorld(WidgetsUnderCursor);

		for (int i = WidgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
		{
			if (WidgetsUnderCursor.Widgets[i].Widget->GetTypeAsString() == "SGraphPanel")
			{
				//convert cached software cursor position to correct screen space.
				auto nodePanel = static_cast<SNodePanel*>(&WidgetsUnderCursor.Widgets[i].Widget.Get());
				FVector2D softwareCursorScreenrPos = (SoftwareCursorPanelPosCached - nodePanel->GetViewOffset()) * nodePanel->GetZoomAmount();
				softwareCursorScreenrPos = WidgetsUnderCursor.Widgets[i].Geometry.LocalToAbsolute(softwareCursorScreenrPos);

				//clamp the cursor inside graph panel,but it seems not necessary.
				//FSlateRect ThisPanelScreenSpaceRect = WidgetsUnderCursor.Widgets[i].Geometry.GetLayoutBoundingRect();
				//FIntPoint softwareCursorScreenrPosClamp(
				//	FMath::RoundToInt(FMath::Clamp(softwareCursorScreenrPos.X, ThisPanelScreenSpaceRect.Left, ThisPanelScreenSpaceRect.Right)),
				//	FMath::RoundToInt(FMath::Clamp(softwareCursorScreenrPos.Y, ThisPanelScreenSpaceRect.Top, ThisPanelScreenSpaceRect.Bottom))
				//);

				SlateApp.SetCursorPos(softwareCursorScreenrPos);

				TSet<FKey> fakeKeysForWidget;
				if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
				{
					fakeKeysForWidget.Add(EKeys::RightMouseButton);
				}
				if (MouseEvent.IsMouseButtonDown(EKeys::MiddleMouseButton))
				{
					fakeKeysForWidget.Add(EKeys::MiddleMouseButton);
				}
				FPointerEvent fakeEventForWidget(
					MouseEvent.GetPointerIndex(),
					softwareCursorScreenrPos,
					softwareCursorScreenrPos,
					fakeKeysForWidget,
					FKey(),
					0,
					FModifierKeysState()
				);
				//call OnDragOver with correct screen space software cursor position to update connection wire endpoint position.
				WidgetsUnderCursor.Widgets[i].Widget->OnDragOver(WidgetsUnderCursor.Widgets[i].Geometry, FDragDropEvent(fakeEventForWidget, SlateApp.GetDragDroppingContent()));


				//by default both mouse button down will zoom panel view,which we dont want when drag and panning,so override the event to trick graph panel.
				//this may causes some wrong stat in slate after stop paning,but should be fine.
				if (MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton) && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
				{
					if (GetDefault<UGraphEditorSettings>()->PanningMouseButton == EGraphPanningMouseButton::Both || GetDefault<UGraphEditorSettings>()->PanningMouseButton == EGraphPanningMouseButton::Right)
					{
						TSet<FKey> fakeKeysForSlate;
						fakeKeysForSlate.Add(EKeys::RightMouseButton);

						FPointerEvent fakeMouseEvent(
							MouseEvent.GetPointerIndex(),
							MouseEvent.GetScreenSpacePosition(),
							MouseEvent.GetLastScreenSpacePosition(),
							fakeKeysForSlate,
							FKey(),
							0,
							FModifierKeysState()
						);
						const_cast<FPointerEvent&>(MouseEvent) = fakeMouseEvent;
					}
				}

				break;
			}
		}

		SlateApp.GetPlatformCursor()->Show(false);
		DidIHideTheCursor = true;
		SlateApp.GetDragDroppingContent()->SetDecoratorVisibility(false);
	}

	//if slate lose focus while drag and panning,the mouse will not be able to show up,so make a check here. 
	if (!IsPanning(MouseEvent) && DidIHideTheCursor)
	{
		SlateApp.GetPlatformCursor()->Show(true);
		DidIHideTheCursor = false;
	}

	if(IsDraggingConnection() && !IsPanning(MouseEvent))
	{
		//update visual
		SlateApp.GetPlatformCursor()->Show(true);
		SlateApp.GetDragDroppingContent()->SetDecoratorVisibility(true);

		//change decorator display text.
		//dont do it every frame for performance.
		if (LastUpdateDecoratorTime < FPlatformTime::Seconds())
		{
			LastUpdateDecoratorTime = (int)FPlatformTime::Seconds()+1;

			if (MouseEvent.IsShiftDown() || !MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
			{
				FWidgetPath WidgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
				if (WidgetsUnderCursor.Widgets.Num() > 0 && WidgetsUnderCursor.Widgets.Last().Widget->GetTypeAsString() == "SGraphPanel")
				{
					static_cast<FDragConnection*>(SlateApp.GetDragDroppingContent().Get())->SetSimpleFeedbackMessage(
						FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OKWarn")),
						FLinearColor::White,
						NSLOCTEXT("GraphEditor.Feedback", "MultiDrop", "Multi-drop enabled."));
				}
			}
		}
	}

	//hold down alt and drag to break connection wire.
	if (!IsDraggingConnection() && MouseEvent.IsAltDown() && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		if (!MouseButtonDownResponsivnessThrottle.IsValid())
		{
			MouseButtonDownResponsivnessThrottle = FSlateThrottleManager::Get().EnterResponsiveMode();
		}

		FWidgetPath WidgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());

		if (WidgetsUnderCursor.Widgets.Num() > 0 && WidgetsUnderCursor.Widgets.Last().Widget->GetTypeAsString() == "SGraphPanel")
		{
			//fill all the gaps between two mouse move event to ensure click event is passed to graph panel along the way.
			float mouseDelta = MouseEvent.GetCursorDelta().Size();
			float stepSize = 1.f;
			if (mouseDelta > stepSize)
			{
				//UE_LOG(LogTemp, Warning, TEXT("delta:%f,%f"), MouseEvent.GetCursorDelta().X, MouseEvent.GetCursorDelta().Y);
				int steps = mouseDelta / stepSize;
				for (int i = 1; i < steps; i++)
				{
					FVector2D interpolatedPos = FMath::Lerp(MouseEvent.GetLastScreenSpacePosition(), MouseEvent.GetScreenSpacePosition(), (i * stepSize) / mouseDelta);
					SlateApp.OnMouseDown(TSharedPtr< FGenericWindow >(), EMouseButtons::Left, interpolatedPos);
					//UE_LOG(LogTemp, Warning, TEXT("lerp:%f,%f"), interpolatedPos.X, interpolatedPos.Y);
				}
			}
			SlateApp.OnMouseDown(TSharedPtr< FGenericWindow >(), EMouseButtons::Left);
		}
	}

	return false;
}

//shift cancel multi-drop
bool FOverrideSlateInputProcessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if ((InKeyEvent.GetKey() == EKeys::LeftShift || InKeyEvent.GetKey() == EKeys::RightShift) && InKeyEvent.GetUserIndex() >=0 && IsDraggingConnection())
	{
		TSharedPtr< FDragDropOperation > LocalDragDropContent = SlateApp.GetDragDroppingContent(); 
		SlateApp.CancelDragDrop();
		LocalDragDropContent->OnDrop(true, FPointerEvent());
		SlateApp.GetPlatformCursor()->Show(true);
		return true;
	}
	return false;
}