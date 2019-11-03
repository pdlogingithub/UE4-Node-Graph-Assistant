// Copyright 2019 yangxiangyun
// All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#include "SGraphPanel.h"


UENUM()
enum EAlignDirection
{
	AD_InputDirection,
	AD_OutputDirection,
};

struct FNodeHelper
{
	//pull#4006
	static void BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin);

	// get all nodes that linked to these nodes.
	static TArray<UEdGraphNode*> GetLinkedNodes(TArray<UEdGraphNode*> SourceNodes, bool bDownStream, bool bUpStream);

	static FVector2D GetSplinePointDistanceIfClose(FVector2D SplineStart, FVector2D SplineEnd, FVector2D PointPosition, const UGraphEditorSettings* Settings);

	//
	static float GetWirePointDistanceIfClose(const FArrangedWidget& ArrangedGraphPanel, SGraphPin* APin, SGraphPin* BPin, FVector2D HitPosition, const UGraphEditorSettings* Settings);


	//if given position can hit the spline defined by two give pins.
	static bool GetWirePointHitResult(const FArrangedWidget& ArrangedGraphPanel, SGraphPin* APin, SGraphPin* BPin, FVector2D HitPosition, float WireThickness, const UGraphEditorSettings* Settings);

	/*
	* try to remove nodes but keep wire flow.
	*
	* @param	ForceBypass	    break pin links event when can not bypass them(input pin type not compatible with output pin type).
	* @param	ForceKeepNode	do not remove node event it is bypassed.
	*/
	static bool BypassNodes(UEdGraph* Graph, TArray<UEdGraphNode*> TargetNodes, bool ForceBypass, bool ForceKeepNode);

	//
	static TArray<TArray<UEdGraphNode*>> CalculateNodeshierarchy(TArray<UEdGraphNode*> sourceNodes, EAlignDirection AlignDirection);

	//reorder nodes' position into grid like structure.
	static bool RearrangeSelectedNodes(SGraphPanel* graphPanel ,FIntPoint Spacing, float SpacingRelax);

	//for ai graph.
	static bool RearrangeSelectedNodes_AIGraph(SGraphPanel* graphPanel, FIntPoint Spacing, float SpacingRelax);

	static TArray<TSharedRef<SGraphPin>> GetPins(TSharedRef<SGraphNode> GraphNode);

	static void GetAutoConnectablePins(const UEdGraphSchema* GraphSchema, float MaxConnectionDistance, TArray<TSharedRef<SGraphNode>> InSourceNodes, TArray<TSharedRef<SGraphNode>> InTargetNodes, TArray<TWeakPtr<SGraphPin>>& OutStartPins, TArray<TWeakPtr<SGraphPin>>& OutEndPins);

private:
};
	