// Copyright 2019 yangxiangyun
// All Rights Reserved

#pragma once

#include "Framework/Application/IInputProcessor.h"
#include "CoreMinimal.h"
#include "Version.h"

#include "Application/ThrottleManager.h"
#include "SGraphPanel.h"

#if ENGINE_MINOR_VERSION < 23
#include "HardwareCursor.h"
#endif

#include "NodeGraphAssistantCommands.h"
#include "NGAGraphPinConnectionFactory.h"


enum EGraphObjType
{
	EGT_Default,
	EGT_AnimStateMachine,
	EGT_Niagara,
	EGT_AI,
	EGT_ReferenceViewer,
	EGT_PhysicsAsset,
	EGT_Sound
};

struct FNGAEventContex
{
	bool IsClickGesture = false;
	bool IsDoubleClickGesture = false;

	bool IsCursorInsidePanel = false;
	bool IsCursorOnPanelEmptySpace = false;

	bool IsDraggingConnection = false;
	bool IsDragDropping = false;
	bool IsDraggingNode = false;
	bool IsPanning = false;
	bool IsBoxSelecting = false;
	
	TSharedPtr<SGraphPanel> GraphPanel = nullptr; //hovered graph panel.
	FGeometry PanelGeometry;
	TSharedPtr<SGraphNode> GraphNode = nullptr; //hovered graph node.
	TSharedPtr<SGraphNode> CommentNode = nullptr; 
	FGeometry NodeGeometry;
	bool IsNodeTitle = false;
	TSharedPtr<SGraphPin> GraphPin = nullptr; //hovered graph pin.
	FGeometry PinGeometry;
	bool IsInPinEditableBox = false; //[editable text pin]
	EGraphObjType GraphType = EGT_Default;
};

struct FNGAEventReply
{
	bool IsHandled = false;
	bool ShouldBlockInput = false;

	static FNGAEventReply BlockSlateInput()
	{
		FNGAEventReply reply;
		reply.IsHandled = true;
		reply.ShouldBlockInput = true;
		return reply;
	};

	static FNGAEventReply UnHandled()
	{
		return FNGAEventReply();
	}
	static FNGAEventReply Handled()
	{
		FNGAEventReply reply;
		reply.IsHandled = true;
		return reply;
	}

	void Append(FNGAEventReply AdditionalReply)
	{
		IsHandled |= AdditionalReply.IsHandled;
		ShouldBlockInput |= AdditionalReply.ShouldBlockInput;
	}
};


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
#if ENGINE_MINOR_VERSION >= 21
	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;
#endif

	FNGAEventContex InitEventContex(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent);

	//if cursor is inside node graph.
	bool IsCursorInsidePanel() const;

	//is cursor not above any regular nodes or inside comment node.
	bool IsCursorOnEmptySpace() const;

	//is dragging a node connection wire.
	bool IsDraggingConnection() const;

	FSlateRect GetWidgetRect(FGeometry Geometry);

	//get the graph panel currently under cursor.
	TSharedPtr<SGraphPanel> GetCurrentGraphPanel();

	FVector2D GraphPosToScreenPos(TSharedRef<SGraphPanel> GraphPanel, FGeometry Geometry, FVector2D PanelPos);
	FVector2D ScreenPosToGraphPos(TSharedRef<SGraphPanel> GraphPanel, FGeometry Geometry, FVector2D ScreenPos);

	void CancelDraggingReset(int32 UserIndex);

	bool IsPanningButton(const FKey& Key) const;
	bool IsPanningButtonDown(const FPointerEvent& MouseEvent) const;
	bool IsCutoffButton(const FKey& Key) const;
	bool IsCutoffButtonDown(const FPointerEvent& MouseEvent) const;

	FNGAEventReply TryProcessAsDupliWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsBeginDragNPanEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsBeingDragNPanEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsEndDragNPanEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsSelectStreamEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsMultiConnectEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsClusterHighlightEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsSingleHighlightEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsBeginCutOffWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsBeingCutOffWireEvent(FSlateApplication& SlateApp,const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsEndCutOffWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsCreateNodeOnWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsEndCreateNodeOnWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsBeginLazyConnectEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsLazyConnectMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, FNGAEventContex Ctx);
	FNGAEventReply TryProcessAsEndLazyConnectEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, FNGAEventContex Ctx);
	FNGAEventReply TryProcessAsAutoConnectMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsAutoConnectMouseUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsShakeNodeOffWireEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, FNGAEventContex Ctx);
	FNGAEventReply TryProcessAsInsertNodeMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsEndInsertNodeEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);
	FNGAEventReply TryProcessAsCreateNodeOnWireWithHotkeyEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent, const FNGAEventContex& Ctx);

	void BypassSelectedNodes(bool ForceKeepNode);
	void RearrangeNodes();
	void OnSelectLinkedNodes(bool bDownStream, bool bUpStream);
	void CycleWireDrawStyle();
	void DupliNodeWithWire();
	void ExchangeWire(NGAInputProcessor* InputProcessor);
	void ConnectNodes();

private:
	struct ShakeOffNodeTrackigInfo
	{
		double MouseMoveTime;
		FVector2D MouseMoveDirection;
	};
	TArray<ShakeOffNodeTrackigInfo> ShakeOffNodeTracker;

	FVector2D LastGraphCursorGraphPos;
	FVector2D LastGraphCursorScreenPosClamped;
	FSlateRect LastPanelScreenSpaceRect;

	FVector2D LastMouseDownScreenPos = FVector2D::ZeroVector;
	FVector2D LastMouseUpScreenPos = FVector2D::ZeroVector;
	float MouseUpDeltaTime = 0.f;

	bool DidIHideTheCursor = false;
	bool HasBegunCuttingWire = false;
	bool HasBegunLazyConnect = false;
	bool RefreshToolTipWhenMouseMove = false;
	bool BlockNextClick = false;

	TWeakPtr<SGraphNode> NodeBeingDrag;
	TWeakPtr<SGraphNode> CommentNodeBeingDrag;

	DECLARE_DELEGATE_RetVal(bool, NGADeferredEventDele)
	TArray<NGADeferredEventDele> TickEventListener;

	//to communicate with draw event
	TSharedPtr<FNGAGraphPinConnectionFactory> MyPinFactory;

	TSet<TWeakPtr<SGraphPin>> HighLightedPins; 

	//use when need to boost slate performance.
	FThrottleRequest MouseButtonDownResponsivnessThrottle;

	//for setting input schem in editor ui;
	TSharedPtr<FUICommandList> UICommandList;

#if ENGINE_MINOR_VERSION > 22
	void* CusorResource_Scissor;
#else
	TSharedPtr<FHardwareCursor> CusorResource_Scissor;
#endif

	int32 PressedCharKey = 0;
};