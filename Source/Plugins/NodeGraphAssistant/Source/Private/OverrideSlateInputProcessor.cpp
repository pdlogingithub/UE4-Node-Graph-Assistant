// Copyright 2018 yangxiangyun
// All Rights Reserved

#include "OverrideSlateInputProcessor.h"
#include "CoreMinimal.h"

#include "SlateApplication.h"
#include "DragConnection.h"
#include "SGraphPanel.h"
#include "GraphEditorSettings.h"

#include "EditorStyleSet.h"

#include "EdGraphNode.h"

//check to make sure we don't touch anything we are not interested.
bool FOverrideSlateInputProcessor::IsCursorInsidePanel(const FPointerEvent& MouseEvent)
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


//get the graph panel at the screen space position given by input pointer event.
FArrangedWidget FOverrideSlateInputProcessor::GetArrangedGraphPanelForEvent(const FPointerEvent& MouseEvent)
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


//is cursor above graph panel or inside comment node.
bool FOverrideSlateInputProcessor::IsCursorOnEmptySpace(const FPointerEvent& MouseEvent)
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


//is dragging a pin connection wire.
bool FOverrideSlateInputProcessor::IsDraggingConnection()
{
	FSlateApplication& slateApp = FSlateApplication::Get();
	return slateApp.IsDragDropping() && slateApp.GetDragDroppingContent()->IsOfType<FDragConnection>();
}


//multi-drop triggered from right click or left click (down and up) while dragging a connection wire.
bool FOverrideSlateInputProcessor::IsTryingToMultiDrop(const FPointerEvent& MouseEvent)
{
	if (IsCursorInsidePanel(MouseEvent))
	{
		if (IsDraggingConnection())
		{
			if (!MouseEvent.IsAltDown() && !MouseEvent.IsControlDown())
			{
				//if user is releasing mouse while panning,don't drop,even if cursor is above widget,this is for preventing accidentally drop on graph pin while drag and panning.
				if ((MouseEvent.GetScreenSpacePosition() - LastMouseDownScreenPos).Size() < 10)
				{
					if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton || MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
					{
						return true;
					}
				}
				//if the connection wire is created by left drag and shift is down,will consider as a multi-drop even is not a click gesture. 
				else if (MouseEvent.IsShiftDown() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					return true;
				}
			}
		}
	}
	return false;
}


//is the panning button according to graph editor setting gets pressed.
bool FOverrideSlateInputProcessor::CanBeginOrEndPan(const FPointerEvent& MouseEvent)
{
	if (IsCursorInsidePanel(MouseEvent))
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


//is the panning button according to graph editor setting pressed.
bool FOverrideSlateInputProcessor::IsPanning(const FPointerEvent & MouseEvent)
{
	if (!MouseEvent.GetEffectingButton().IsValid())
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

	}
	return false;
}


void FOverrideSlateInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
	MouseUpDeltaTime += DeltaTime;

	for (int i = 0; i < DeferredEventDeles.Num(); i++)
	{
		if (!DeferredEventDeles[i].IsBound() || (DeferredEventDeles[i].IsBound() && DeferredEventDeles[i].Execute()))
		{
			DeferredEventDeles.RemoveAt(i);
			i--;
		}
	}
}


bool FOverrideSlateInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	LastMouseDownScreenPos = MouseEvent.GetScreenSpacePosition();


	//shift click to replicate connection.
	//by faking a ctrl click event to pluck a connection then immediately do a multi-drop to plug it back on without losing current drag-drop operation.
	if (IsCursorInsidePanel(MouseEvent) && !SlateApp.IsDragDropping())
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && MouseEvent.IsShiftDown())
		{
			FWidgetPath widgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
			FScopedSwitchWorldHack SwitchWorld(widgetsUnderCursor);

			for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
			{
				//graph pin has multiple inheritances,generally contain a word "GraphPin".
				if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString().Contains("GraphPin"))
				{
					TSet<FKey> keys;
					keys.Add(MouseEvent.GetEffectingButton());

					FPointerEvent mouseEvent(
						MouseEvent.GetPointerIndex(),
						MouseEvent.GetScreenSpacePosition(),
						MouseEvent.GetLastScreenSpacePosition(),
						keys,
						MouseEvent.GetEffectingButton(),
						0,
						FModifierKeysState(false, false, true, false, false, false, false, false, false));//faking ctrl click

					bool ret = true;
					ret &= SlateApp.ProcessMouseButtonDownEvent(TSharedPtr<FGenericWindow>(), mouseEvent);

					LastMouseUpScreenPos -= FVector2D(50, 50); //[cluster-highlight]prevent this fake event from triggering double click
					ret &= SlateApp.ProcessMouseButtonUpEvent(const_cast<FPointerEvent&>(MouseEvent));
					LastMouseUpScreenPos -= FVector2D(50, 50); //[cluster-highlight]prevent this fake event from triggering double click

					if (ret)
					{
						//reset marked pin, otherwise it will cause crash when duplication triggers reconstruction of a big graph.
						//during which graph panel wont recognize marked pin.
						FArrangedWidget arrangedGraphPanel = GetArrangedGraphPanelForEvent(MouseEvent);
						SGraphPanel* graphPanel = static_cast<SGraphPanel*>(&arrangedGraphPanel.Widget.Get());
						if (graphPanel)
						{
							graphPanel->MarkedPin.Reset();
							for (auto pin : HighLightedPins)
							{
								if (pin.IsValid())
								{
									graphPanel->RemovePinFromHoverSet(pin.Pin()->GetPinObj());
								}
							}
						}
						HighLightedPins.Empty();
						LastMouseDownScreenPos += FVector2D(50,50);//prevent right click menu from popping up when all pins are being reconstructed and can not receive event.

						//for some types of pin,when connection break,it will reconstruct,and graph will refresh async.
						//so we cant reconnect it here,because the pin handle will be invalid this tick.
						//send a deferred mouse click event to connect it back after few ticks when pin is reconstructed and graph is refreshed.

						int* timer = new int(2);
						NGADeferredEventDele deferredDele;
						deferredDele.BindLambda([MouseEvent,this](int* timer)
						{
							//send the event after second tick,so the graph is already refreshed and can accept pin connection.
							if (!timer)
							{
								return true;
							}
							else if (*timer > 0)
							{
								(*timer)--;
								return false;
							}
							else
							{
								delete timer;

								FPointerEvent mouseEvent(
									MouseEvent.GetUserIndex(),
									MouseEvent.GetScreenSpacePosition(),
									MouseEvent.GetScreenSpacePosition(),
									TSet<FKey>(),
									EKeys::Invalid,
									0,
									FModifierKeysState()
								);
								//send move event,otherwise drag operation wont know which pin to connect.
								FSlateApplication::Get().ProcessMouseMoveEvent(mouseEvent);

								LastMouseUpScreenPos -= FVector2D(50,50);//[cluster-highlight]prevent this fake event from triggering double click
								FSlateApplication::Get().OnMouseUp(EMouseButtons::Left, mouseEvent.GetScreenSpacePosition());
								LastMouseUpScreenPos -= FVector2D(50, 50);//[cluster-highlight]prevent this fake event from triggering double click

								return true;
							}
						}, timer);
						DeferredEventDeles.Add(deferredDele);
					}

					return ret;
				}
			}
		}
	}


	//by default panning is bypassed when dragging connection(slate.cpp 5067 4.18), to be able to pan while dragging,need to manually route the pointer down event to graph panel.
	//make sure no widget captured the mouse so we can pan now.
	if (IsDraggingConnection() && CanBeginOrEndPan(MouseEvent) && !SlateApp.HasUserMouseCapture(MouseEvent.GetUserIndex()))
	{
		FWidgetPath widgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
		FScopedSwitchWorldHack SwitchWorld(widgetsUnderCursor);

		for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
		{
			if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString() == "SGraphPanel")
			{
				//[correct wire position]software cursor is in panel space,but preview connection wire is in screen space,when drag and panning,their positions will become mismatched.
				//to fix that we need to cache the software cursor panel position,so we can convert it to correct screen space position when drag and panning to keep connection wire aligned with it.
				auto ZoomStartOffset = widgetsUnderCursor.Widgets[i].Geometry.AbsoluteToLocal(MouseEvent.GetLastScreenSpacePosition());
				SoftwareCursorPanelPos = static_cast<SGraphPanel*>(&widgetsUnderCursor.Widgets[i].Widget.Get())->PanelCoordToGraphCoord(ZoomStartOffset);


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

					return true;
				}

				break;
			}
		}
	}


	//delay a bit to show decorator window,so if user is double clicking,they wont see the decoration window between two clicks.
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !IsDraggingConnection() && !MouseEvent.IsAltDown() && !MouseEvent.IsControlDown())
	{
		FWidgetPath widgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
		FScopedSwitchWorldHack SwitchWorld(widgetsUnderCursor);

		for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
		{
			if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString().Contains("GraphPin"))
			{
				int* timer = new int(30);
				NGADeferredEventDele deferredDele;
				deferredDele.BindLambda([](int* timer)
				{
					if (!timer)
					{
						return true;
					}
					else if (*timer > 0)
					{
						(*timer)--;

						if (FSlateApplication::Get().IsDragDropping() && FSlateApplication::Get().GetDragDroppingContent()->IsOfType<FDragConnection>())
						{
							FSlateApplication::Get().GetDragDroppingContent().Get()->SetDecoratorVisibility(false);
						}

						return false;
					}
					else
					{
						delete timer;

						if (FSlateApplication::Get().IsDragDropping() && FSlateApplication::Get().GetDragDroppingContent()->IsOfType<FDragConnection>())
						{
							FSlateApplication::Get().GetDragDroppingContent().Get()->SetDecoratorVisibility(true);
						}
						return true;
					}
				}, timer);
				DeferredEventDeles.Add(deferredDele);

				break;
			}
		}
	}

	return false;
}


bool FOverrideSlateInputProcessor::HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	float deltaTime = MouseUpDeltaTime;
	MouseUpDeltaTime = 0.f;

	float deltaPos = (LastMouseUpScreenPos - MouseEvent.GetScreenSpacePosition()).Size();
	LastMouseUpScreenPos = MouseEvent.GetScreenSpacePosition();


	if (MouseButtonDownResponsivnessThrottle.IsValid())
	{
		FSlateThrottleManager::Get().LeaveResponsiveMode(MouseButtonDownResponsivnessThrottle);
	}

	//select connected nodes.
	if (deltaTime < 0.6f && deltaPos < 30 && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !IsCursorOnEmptySpace(MouseEvent))
	{
		FWidgetPath widgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
		FScopedSwitchWorldHack SwitchWorld(widgetsUnderCursor);

		for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
		{
			if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString().Contains("GraphPin"))
			{
				break;
			}

			if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString().Contains("GraphNode"))
			{

#if ENGINE_MINOR_VERSION > 16
				FSlateRect nodeRect = widgetsUnderCursor.Widgets[i].Geometry.GetLayoutBoundingRect();
#else
				FSlateRect nodeRect = widgetsUnderCursor.Widgets[i].Geometry.GetClippingRect();
#endif
		
				//select up stream or down stream depending on which side of the node cursor is on.
				bool isDownStream = false;
				if (MouseEvent.GetScreenSpacePosition().X - nodeRect.Left > nodeRect.Right - MouseEvent.GetScreenSpacePosition().X)
				{
					isDownStream = true;
				}

				FArrangedWidget arrangedGraphPanel = GetArrangedGraphPanelForEvent(MouseEvent);
				SGraphPanel* graphPanel = static_cast<SGraphPanel*>(&arrangedGraphPanel.Widget.Get());
				if (graphPanel)
				{
					TArray<UEdGraphNode*> nodesToCheck;
					TArray<UEdGraphNode*> nodesToSkip;
					TArray<UEdGraphNode*> nodesToSelect;

					for (auto* selectedNode : graphPanel->SelectionManager.SelectedNodes)
					{
						UEdGraphNode* sourceNode = Cast<UEdGraphNode>(selectedNode);
						if (!sourceNode)
						{
							continue;
						}
						nodesToCheck.Add(sourceNode);

						while (nodesToCheck.Num() > 0)
						{
							UEdGraphNode* currentNode = nodesToCheck.Last();
							TArray<UEdGraphPin*> currentNodePins = currentNode->Pins;

							for (int32 Index = 0; Index < currentNodePins.Num(); ++Index)
							{
								if (!isDownStream && currentNodePins[Index]->Direction == EGPD_Input || isDownStream && currentNodePins[Index]->Direction == EGPD_Output)
								{
									for (int32 LinkIndex = 0; LinkIndex < currentNodePins[Index]->LinkedTo.Num(); ++LinkIndex)
									{
										UEdGraphNode* LinkedNode = currentNodePins[Index]->LinkedTo[LinkIndex]->GetOwningNode();
										if (LinkedNode)
										{
											int32 FoundIndex = -1;
											nodesToSkip.Find(LinkedNode, FoundIndex);

											if (FoundIndex < 0)
											{
												nodesToSelect.Add(LinkedNode);
												nodesToCheck.Add(LinkedNode);
											}
										}
									}
								}
							}

							// This graph node has now been examined
							nodesToSkip.Add(currentNode);
							nodesToCheck.Remove(currentNode);
						}
					}

					for (int32 Index = 0; Index < nodesToSelect.Num(); ++Index)
					{
						graphPanel->SelectionManager.SetNodeSelection(nodesToSelect[Index], true);
					}
				}
				return true;
			}
		}
	}


	//multi-drop
	//by routing drop event without destroying drag-drop operation
	if (IsTryingToMultiDrop(MouseEvent))
	{
		FWidgetPath widgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
		FScopedSwitchWorldHack SwitchWorld(widgetsUnderCursor);

		for (int i = widgetsUnderCursor.Widgets.Num() - 1; i >= 0; i--)
		{
			//decide what to do when drop on empty space,cancel or summon menu.
			if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString() == "SGraphPanel")
			{
				//if not dropping on any pin,right mouse button cancel dragging,left mouse button summon context menu.
				if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					//prevent pop up menu from appearing when dragging a connection.
					if (MouseEvent.IsShiftDown() && (MouseEvent.GetScreenSpacePosition() - LastMouseDownScreenPos).Size() > 10)
					{
						return true;
					}
					else
					{
						if (SlateApp.HasUserMouseCapture(MouseEvent.GetUserIndex()))
						{
							SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
						}

						return false;
					}
				}
				if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
				{
					TSharedPtr< FDragDropOperation > LocalDragDropContent = SlateApp.GetDragDroppingContent();
					SlateApp.CancelDragDrop();
					LocalDragDropContent->OnDrop(true, FPointerEvent());

					if (SlateApp.HasUserMouseCapture(MouseEvent.GetUserIndex()))
					{
						SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
					}
					if(SlateApp.GetPlatformCursor().IsValid())
					{
						SlateApp.GetPlatformCursor()->Show(true);
					}

					return true;
				}
			}
			//do nothing when click inside a node.
			if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString().Contains("GraphNode") && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				FString widgetName = widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString();
				if (widgetName != "SGraphNodeComment" && widgetName != "SGraphNodeMaterialComment")
				{
					if (SlateApp.HasUserMouseCapture(MouseEvent.GetUserIndex()))
					{
						SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
					}
					return true;
				}
			}

			//[cluster-highlight] add the double clicked graph pin to our collection.
			if (widgetsUnderCursor.Widgets[i].Widget->GetTypeAsString().Contains("GraphPin"))
			{
				if (deltaTime < 0.8f && deltaPos < 8.f && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
				{
					FArrangedWidget arrangedGraphPanel = GetArrangedGraphPanelForEvent(MouseEvent);
					SGraphPanel* graphPanel = static_cast<SGraphPanel*>(&arrangedGraphPanel.Widget.Get());
					if (graphPanel)
					{
						auto sourcePin = TWeakPtr<SGraphPin>(StaticCastSharedRef<SGraphPin>(widgetsUnderCursor.Widgets[i].Widget));
						for (auto pin : HighLightedPins)
						{
							if (pin.IsValid())
							{
								graphPanel->RemovePinFromHoverSet(pin.Pin()->GetPinObj());
							}
						}
						HighLightedPins.Empty();
						HighLightedPins.Add(sourcePin);
						graphPanel->MarkedPin = sourcePin;

						if (IsDraggingConnection())
						{
							TSharedPtr< FDragDropOperation > LocalDragDropContent = SlateApp.GetDragDroppingContent();
							SlateApp.CancelDragDrop();
							LocalDragDropContent->OnDrop(true, FPointerEvent());
						}

						return true;
					}
				}
			}

			//dropping,just like slate route event with bubble policy.
			FReply Reply = widgetsUnderCursor.Widgets[i].Widget->OnDrop(widgetsUnderCursor.Widgets[i].Geometry, FDragDropEvent(MouseEvent, SlateApp.GetDragDroppingContent()));
			if (Reply.IsEventHandled())
			{
				//this is for when its right mouse button up,because when right mouse button down we bypass slate and manually call RoutePointerDownEvent for drag and panning,
				//so we need to do some clean up to cancel out the pointer event we called and release capture that is created by it,otherwise weird things will happen.
				if (SlateApp.HasUserMouseCapture(MouseEvent.GetUserIndex()))
				{
					SlateApp.ReleaseMouseCaptureForUser(MouseEvent.GetUserIndex());
				}

				return true;//return true keep dragging alive.
			}
		}
	}


	//[single-highlight] pick wire for highlight
	if (IsCursorOnEmptySpace(MouseEvent) && !IsDraggingConnection() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
	{
		if ((MouseEvent.GetScreenSpacePosition() - LastMouseDownScreenPos).Size() < 10)
		{
			FArrangedWidget arrangedGraphPanel = GetArrangedGraphPanelForEvent(MouseEvent);
			SGraphPanel* graphPanel = static_cast<SGraphPanel*>(&arrangedGraphPanel.Widget.Get());
			if (graphPanel)
			{
				graphPanel->MarkedPin.Reset();

				//convert regular click on empty space to shift click,so graph panel will set connection wire under cursor as marked and highlight it,so we don't need to press "shift".
				TSet<FKey> keys;
				keys.Add(MouseEvent.GetEffectingButton());
				FPointerEvent fakeMouseEvent(
					MouseEvent.GetPointerIndex(),
					MouseEvent.GetScreenSpacePosition(),
					MouseEvent.GetLastScreenSpacePosition(),
					keys,
					MouseEvent.GetEffectingButton(),
					0,
					FModifierKeysState(true, false, false, false, false, false, false, false, false)
				);

				FWidgetPath WidgetsUnderCursor = SlateApp.LocateWindowUnderMouse(MouseEvent.GetScreenSpacePosition(), SlateApp.GetInteractiveTopLevelWindows());
				FReply reply = SlateApp.RoutePointerUpEvent(WidgetsUnderCursor, fakeMouseEvent);

				if (reply.IsEventHandled())
				{
					//graph panel can support constant highlight for one wire and temp hover highlight for multiple wires,
					//in order to constantly highlight multiple wires,I have to keep a list of constant highlighted wires,and squeeze them into graphpanel's hover set.
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
						for (auto pin : HighLightedPins)
						{
							if (pin.IsValid())
							{
								graphPanel->RemovePinFromHoverSet(pin.Pin()->GetPinObj());
							}
						}
						HighLightedPins.Empty();
					}

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


	//update visual elements.
	if (IsDraggingConnection() && CanBeginOrEndPan(MouseEvent))
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
		SlateApp.SetCursorPos(SoftwareCursorScreenPosClamp);

		return true;
	}

	return false;
}


bool FOverrideSlateInputProcessor::HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	//update visual elements(wire.cursor icon...) for drag and panning,or they will be at mismatched position.
	if (IsDraggingConnection() && IsPanning(MouseEvent))
	{
		FArrangedWidget arrangedGraphPanel = GetArrangedGraphPanelForEvent(MouseEvent);
		SGraphPanel* graphPanel = static_cast<SGraphPanel*>(&arrangedGraphPanel.Widget.Get());
		if (graphPanel)
		{
			//[correct wire position]convert cached software cursor position to correct screen space.
			SoftwareCursorScreenPosClamp = (SoftwareCursorPanelPos - graphPanel->GetViewOffset()) * graphPanel->GetZoomAmount();
			SoftwareCursorScreenPosClamp = arrangedGraphPanel.Geometry.LocalToAbsolute(SoftwareCursorScreenPosClamp);

#if ENGINE_MINOR_VERSION > 16
			FSlateRect panelScreenSpaceRect = arrangedGraphPanel.Geometry.GetLayoutBoundingRect();
#else
			FSlateRect panelScreenSpaceRect = arrangedGraphPanel.Geometry.GetClippingRect();
#endif

			//[drag-pan]when cursor is getting closer to the edge of the panel,drag operation will send deferred pan request to accelerate pan speed(SNodePanle.cpp line:347),
			//this feature is for when dragging connection,but is not wanted when we drag and panning,so clamp the cursor position so accelerate amount will be zero.
			SoftwareCursorScreenPosClamp = FVector2D
			(
				FMath::Clamp(SoftwareCursorScreenPosClamp.X, panelScreenSpaceRect.Left + 50.0f, panelScreenSpaceRect.Right - 50.0f),
				FMath::Clamp(SoftwareCursorScreenPosClamp.Y, panelScreenSpaceRect.Top + 50.0f, panelScreenSpaceRect.Bottom - 50.0f)
			);
		}

		//[drag-pan]its possible that when dragging and panning towards outside of the panel,the mouse move event will be picked up by another widget rather than graph panel.
		//in that case,the position of the cursor wont have a chance to get clamped here,and the graph will start flying around
		//the solution is cache the last clamped position,and keep clamping the cursor using it if mouse event is not received by graph panel.

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
			SoftwareCursorScreenPosClamp, //[drag-pan]use clamped position,so deferred pan amount will be 0.
			SoftwareCursorScreenPosClamp,
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


		SlateApp.SetCursorPos(SoftwareCursorScreenPosClamp);
		if (SlateApp.GetPlatformCursor().IsValid())
		{
			SlateApp.GetPlatformCursor()->Show(false);
		}

		DidIHideTheCursor = true;
		SlateApp.GetDragDroppingContent()->SetDecoratorVisibility(false);
	}


	//if slate lose focus while drag and panning,the mouse will not be able to show up,so make a check here. 
	if (!IsPanning(MouseEvent) && DidIHideTheCursor)
	{
		if (SlateApp.GetPlatformCursor().IsValid())
		{
			SlateApp.GetPlatformCursor()->Show(true);
		}
		DidIHideTheCursor = false;
	}

	//[highlight]
	if (!IsDraggingConnection() && !IsPanning(MouseEvent))
	{
		FArrangedWidget arrangedGraphPanel = GetArrangedGraphPanelForEvent(MouseEvent);
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
	if(IsDraggingConnection() && !IsPanning(MouseEvent))
	{
		if (SlateApp.GetPlatformCursor().IsValid())
		{
			SlateApp.GetPlatformCursor()->Show(true);
		}
		SlateApp.GetDragDroppingContent()->SetDecoratorVisibility(true);

		//change decorator display text.
		//lets update the widget if editor is running smooth.
		if(FPlatformTime::Seconds() - (int)FPlatformTime::Seconds() < 0.03f)
		{
			if (MouseEvent.IsShiftDown() || !MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
			{
				if (IsCursorOnEmptySpace(MouseEvent))
				{
					FDragConnection* dragConnection = static_cast<FDragConnection*>(SlateApp.GetDragDroppingContent().Get());
					if (dragConnection)
					{
						dragConnection->SetSimpleFeedbackMessage(
							FEditorStyle::GetBrush(TEXT("Graph.ConnectorFeedback.OKWarn")),
							FLinearColor::White,
							NSLOCTEXT("GraphEditor.Feedback", "MultiDrop", "Multi-drop enabled."));
					}
				}
			}
		}
	}


	//hold down alt and drag to break connection wire.
	//if user alt-drag in level editor viewport,invisible cursor may enter panel.
	if (IsCursorInsidePanel(MouseEvent) && SlateApp.GetPlatformCursor()->GetType() != EMouseCursor::None)
	{
		if (!IsDraggingConnection() && MouseEvent.IsAltDown() && MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && !MouseEvent.IsMouseButtonDown(EKeys::RightMouseButton))
		{
			if (!MouseButtonDownResponsivnessThrottle.IsValid())
			{
				MouseButtonDownResponsivnessThrottle = FSlateThrottleManager::Get().EnterResponsiveMode();
			}


			if (IsCursorOnEmptySpace(MouseEvent))
			{
				//fill all the gaps between two mouse move event to ensure click event is passed to graph panel along the way.
				//but it seems some connection wire wont get cut off when dragging fast.
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
	}


	return false;
}


//shift cancel multi-drop
bool FOverrideSlateInputProcessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	if ((InKeyEvent.GetKey() == EKeys::LeftShift || InKeyEvent.GetKey() == EKeys::RightShift) && InKeyEvent.GetUserIndex() >= 0)
	{
		if (IsDraggingConnection() )
		{
			TSharedPtr< FDragDropOperation > LocalDragDropContent = SlateApp.GetDragDroppingContent();
			SlateApp.CancelDragDrop();
			LocalDragDropContent->OnDrop(true, FPointerEvent());
			if (SlateApp.GetPlatformCursor().IsValid())
			{
				SlateApp.GetPlatformCursor()->Show(true);
			}
			return true;
		}
	}

	return false;
}