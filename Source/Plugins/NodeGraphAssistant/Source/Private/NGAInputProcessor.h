// Copyright 2018 yangxiangyun
// All Rights Reserved

#pragma once

#include "Framework/Application/IInputProcessor.h"
#include "CoreMinimal.h"

#include "SGraphPanel.h"
#include "NodeGraphAssistantCommands.h"
#include "Style/NGAGraphPinConnectionFactory.h"
#include "HardwareCursor.h"

class NGAInputProcessor : public IInputProcessor
{
public:
	NGAInputProcessor();
	virtual ~NGAInputProcessor();


	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;


	//if cursor is inside node graph.
	bool IsCursorInsidePanel(const FPointerEvent& MouseEvent) const;

	//get the graph panel and its geometry.
	FArrangedWidget GetArrangedGraphPanel(const FPointerEvent& MouseEvent);
	FArrangedWidget GetArrangedGraphNode(const FPointerEvent& MouseEvent);

	//is name of pin class
	bool IsPinName(const FString& InName) const;

	//get the graph panel.
	SGraphPanel* GetGraphPanel(const FPointerEvent& MouseEvent);

	FSlateRect GetWidgetRect(FGeometry Geometry);

	//get the graph panel currently under cursor.
	SGraphPanel* GetCurrentGraphPanel();

	FVector2D GraphPosToScreenPos(FArrangedWidget ArrangedPanel, FVector2D PanelPos);
	FVector2D ScreenPosToGraphPos(FArrangedWidget ArrangedPanel, FVector2D ScreenPos);

	//is cursor not above any regular nodes or inside comment node.
	bool IsCursorOnEmptySpace(const FPointerEvent& MouseEvent) const;

	//is dragging a node connection wire.
	bool IsDraggingConnection() const;

	void CancelDraggingReset(int32 UserIndex);


	//is the panning button according to graph editor setting gets pressed.
	bool CanBeginOrEndPan(const FPointerEvent& MouseEvent) const;

	//is the panning button according to graph editor setting pressed.
	bool IsDragNPanning(const FPointerEvent& MouseEvent) const;

	bool IsTryingToCutOffWire(const FPointerEvent& MouseEvent) const;


	bool TryProcessAsDupliWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
	bool TryProcessAsBeginDragNPanEvent(const FPointerEvent& MouseEvent);
	void TryProcessAsBeingDragNPanEvent(const FPointerEvent& MouseEvent);
	bool TryProcessAsSelectStreamEvent(const FPointerEvent& MouseEvent);
	bool TryProcessAsMultiConnectEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
	bool TryProcessAsClusterHighlightEvent(const FPointerEvent& MouseEvent);
	bool TryProcessAsSingleHighlightEvent(const FPointerEvent& MouseEvent);
	bool TryProcessAsBeginCutOffWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
	void TryProcessAsCutOffWireEvent(FSlateApplication& SlateApp,const FPointerEvent& MouseEvent);
	void TryProcessAsEndCutOffWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
	void TryProcessAsBypassNodesEvent();
	bool TryProcessAsInsertNodeEvent(const FPointerEvent& MouseEvent);
	void TryProcessAsRearrangeNodesEvent();
	void TryProcessAsBeginLazyConnectEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
	void TryProcessAsLazyConnectEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
	void TryProcessAsEndLazyConnectEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
	void TryProcessAsAutoConnectMouseDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
	void TryProcessAsAutoConnectMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);
	void TryProcessAsAutoConnectMouseUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);


	//for ui command.
	void OnSelectLinkedNodes(bool bDownStream, bool bUpStream);
	void CycleWireDrawStyle();

private:
	FVector2D LastGraphCursorGraphPos;
	FVector2D LastGraphCursorScreenPosClamped;
	FSlateRect LastPanelScreenSpaceRect;

	FVector2D LastMouseDownScreenPos = FVector2D::ZeroVector;
	FVector2D LastMouseUpScreenPos = FVector2D::ZeroVector;
	float MouseUpDeltaTime = 0.f;

	bool DidIHideTheCursor = false;
	bool HasBegunCuttingWire = false;
	bool HasBegunLazyConnect = false;
	bool IsSelecting = false;
	//prevent connect pins when mouse move if user are duplicating wire,
	bool HasMouseUpAfterDrag = false;

	TWeakPtr<SGraphNode> NodeBeingDrag;

	DECLARE_DELEGATE_RetVal(bool, NGADeferredEventDele)
	TArray<NGADeferredEventDele> TickEventListener;

	//DECLARE_DELEGATE_RetVal_TwoParams(ProcessResult, MouseEventListenerDele, FSlateApplication&, const FPointerEvent&)
	//TMap<FName, MouseEventListenerDele> OnMouseMoveListeners;
	//TMap<FName, MouseEventListenerDele> OnMouseButtonDownListeners;
	//TMap<FName, MouseEventListenerDele> OnMouseButtonUpListeners;


	//to communicate with draw event
	TSharedPtr<FNGAGraphPinConnectionFactory> MyGraphPinConnectionFactory;

	//todo,hold pins for each graph panel instance.
	TSet<TWeakPtr<SGraphPin>> HighLightedPins; 

	//use when need to boost slate performance.
	FThrottleRequest MouseButtonDownResponsivnessThrottle;

	//for setting input schem in editor ui;
	TSharedPtr<FUICommandList> UICommandList;

	TSharedPtr<FHardwareCursor> CutOffCusor;
};