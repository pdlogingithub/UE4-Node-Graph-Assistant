// Copyright 2018 yangxiangyun
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

	//
	static float GetWirePointDistanceIfClose(const FArrangedWidget& ArrangedGraphPanel, SGraphPin* APin, SGraphPin* BPin, FVector2D HitPosition, const UGraphEditorSettings* Settings);

	//if given position can hit the spline defined by two give pins.
	static bool GetWirePointHitResult(const FArrangedWidget& ArrangedGraphPanel, SGraphPin* APin, SGraphPin* BPin, FVector2D HitPosition, float WireThickness, const UGraphEditorSettings* Settings);

	//try to remove nodes but keep wire flow.
	static bool BypassNodes(TArray<UEdGraphNode*> sourceNodes);

	//
	static TArray<TArray<UEdGraphNode*>> CalculateNodeshierarchy(TArray<UEdGraphNode*> sourceNodes, EAlignDirection AlignDirection);

	//reorder nodes' position into grid like structure.
	static bool RearrangeSelectedNodes(SGraphPanel* graphPanel ,FIntPoint Spacing, float SpacingRelax);

	//for ai graph.
	static bool RearrangeSelectedNodes_AIGraph(SGraphPanel* graphPanel, FIntPoint Spacing, float SpacingRelax);

private:
	static bool TryBypassOneToNConnection(UEdGraphPin* ALinkedPin, UEdGraphPin* APin, UEdGraphPin* BPin, TArray<UEdGraphPin*> BLinkedPins);
};
	