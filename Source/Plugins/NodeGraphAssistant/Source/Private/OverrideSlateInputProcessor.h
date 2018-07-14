// Copyright 2018 yangxiangyun
// All Rights Reserved

#pragma once

#include "Framework/Application/IInputProcessor.h"
#include "CoreMinimal.h"

#include "SGraphPanel.h"

class FOverrideSlateInputProcessor : public IInputProcessor
{
public:
	FOverrideSlateInputProcessor() {}
	virtual ~FOverrideSlateInputProcessor() {}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;

	//is the widget under screen space position given by the input event belongs to a graph panel widget.
	bool IsCursorInsidePanel(const FPointerEvent& MouseEvent);

	//just a helper function,get the graph panel and its geometry at the screen space position given by input pointer event.
	FArrangedWidget GetArrangedGraphPanelForEvent(const FPointerEvent& MouseEvent);

	//is event pointer above graph panel or inside comment node.
	bool IsCursorOnEmptySpace(const FPointerEvent& MouseEvent);

	//is dragging a pin connection wire.
	bool IsDraggingConnection();

	//multi-drop triggered from right click or left click while dragging a connection wire.
	bool IsTryingToMultiDrop(const FPointerEvent& MouseEvent);

	//is the panning button according to graph editor setting gets pressed.
	bool CanBeginOrEndPan(const FPointerEvent& MouseEvent);

	//is the panning button according to graph editor setting pressed.
	bool IsPanning(const FPointerEvent& MouseEvent);

	FVector2D SoftwareCursorPanelPos;
	FVector2D SoftwareCursorScreenPosClamp;

	FVector2D LastMouseDownScreenPos = FVector2D::ZeroVector;
	FVector2D LastMouseUpScreenPos = FVector2D::ZeroVector;

	float MouseUpDeltaTime = 0.f;

	bool DidIHideTheCursor = false;

	DECLARE_DELEGATE_RetVal(bool, NGADeferredEventDele) //NGA:node graph assistant
	TArray<NGADeferredEventDele> DeferredEventDeles;

	//if should keep separate collection for multiple different graph editor? 
	TSet<TWeakPtr<SGraphPin>> HighLightedPins; 

	FThrottleRequest MouseButtonDownResponsivnessThrottle;
};