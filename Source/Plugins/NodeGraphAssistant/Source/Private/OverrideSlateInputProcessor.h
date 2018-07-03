// Copyright 2018 yangxiangyun
// All Rights Reserved

#pragma once

#include "Framework/Application/IInputProcessor.h"
#include "CoreMinimal.h"


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

	bool IsCursorInsidePanel(const FPointerEvent& MouseEvent);
	bool IsDraggingConnection();
	bool IsTryingToMultiDrop(const FPointerEvent& MouseEvent);
	bool CanBeginOrEndPan(const FPointerEvent& MouseEvent);
	bool IsPanning(const FPointerEvent& MouseEvent);

	FVector2D SoftwareCursorPanelPosCached;
	FVector2D LastMouseDownScreenPos;
	int LastUpdateDecoratorTime;
	bool DidIHideTheCursor = false;

	bool SendDeferredMouseEvent;
	float DeferredMouseEventTimer = 0.f;
	FPointerEvent DeferredMouseEvent;

	FThrottleRequest MouseButtonDownResponsivnessThrottle;
};