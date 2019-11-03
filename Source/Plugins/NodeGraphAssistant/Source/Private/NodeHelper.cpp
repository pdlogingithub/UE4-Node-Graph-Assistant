// Copyright 2019 yangxiangyun
// All Rights Reserved

#include "NodeHelper.h"

#include "CoreMinimal.h"
#include "Algo/Reverse.h"

#include "EdGraphNode_Comment.h"
#include "EdGraph/EdGraph.h"

#include "GraphEditorSettings.h"
#include "NodeGraphAssistantConfig.h"

#include "Version.h"

//#pragma optimize("", off)


void FNodeHelper::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin)
{
#if ENGINE_MINOR_VERSION > 18

	SourcePin->GetSchema()->BreakSinglePinLink(SourcePin, TargetPin);
#else
	const_cast<UEdGraphSchema*>(SourcePin->GetSchema())->BreakSinglePinLink(SourcePin, TargetPin);
#endif
}


TArray<UEdGraphNode*> FNodeHelper::GetLinkedNodes(TArray<UEdGraphNode*> SourceNodes, bool bDownStream, bool bUpStream)
{
	TArray<UEdGraphNode*> nodesToCheck;
	TArray<UEdGraphNode*> nodesToSkip;
	TArray<UEdGraphNode*> nodesToReturn;

	for (auto* sourceNode : SourceNodes)
	{
		if (!sourceNode)
		{
			continue;
		}
		nodesToCheck.AddUnique(sourceNode);
		nodesToSkip.AddUnique(sourceNode);
	}

	while (nodesToCheck.Num() > 0)
	{
		UEdGraphNode* currentNode = nodesToCheck.Last();
		nodesToCheck.Remove(currentNode);

		TArray<UEdGraphPin*> currentNodePins = currentNode->Pins;
		for (int32 Index = 0; Index < currentNodePins.Num(); ++Index)
		{
			if (bUpStream && currentNodePins[Index]->Direction == EGPD_Input || bDownStream && currentNodePins[Index]->Direction == EGPD_Output)
			{
				for (int32 LinkIndex = 0; LinkIndex < currentNodePins[Index]->LinkedTo.Num(); ++LinkIndex)
				{
					UEdGraphNode* LinkedNode = currentNodePins[Index]->LinkedTo[LinkIndex]->GetOwningNode();
					if (LinkedNode)
					{
						if (nodesToSkip.Find(LinkedNode) < 0)
						{
							nodesToReturn.AddUnique(LinkedNode);
							nodesToCheck.AddUnique(LinkedNode);
							nodesToSkip.AddUnique(LinkedNode);
						}
					}
				}
			}
		}
	}

	return nodesToReturn;
}


FVector2D FNodeHelper::GetSplinePointDistanceIfClose(FVector2D SplineStart, FVector2D SplineEnd, FVector2D PointPosition, const UGraphEditorSettings* Settings)
{
	FVector2D P0 = SplineStart;
	FVector2D P1 = SplineEnd;
	FVector2D SplineTangent;

	const FVector2D DeltaPos = P1 - P0;
	const bool bGoingForward = DeltaPos.X >= 0.0f;
	const float ClampedTensionX = FMath::Min<float>(FMath::Abs<float>(DeltaPos.X), bGoingForward ? Settings->ForwardSplineHorizontalDeltaRange : Settings->BackwardSplineHorizontalDeltaRange);
	const float ClampedTensionY = FMath::Min<float>(FMath::Abs<float>(DeltaPos.Y), bGoingForward ? Settings->ForwardSplineVerticalDeltaRange : Settings->BackwardSplineVerticalDeltaRange);

	if (bGoingForward)
	{
		SplineTangent = (ClampedTensionX * Settings->ForwardSplineTangentFromHorizontalDelta) + (ClampedTensionY * Settings->ForwardSplineTangentFromVerticalDelta);
	}
	else
	{
		SplineTangent = (ClampedTensionX * Settings->BackwardSplineTangentFromHorizontalDelta) + (ClampedTensionY * Settings->BackwardSplineTangentFromVerticalDelta);
	}

	const FVector2D P0Tangent = SplineTangent;
	const FVector2D P1Tangent = SplineTangent;

	bool bCloseToSpline = false;
	{
		// The curve will include the endpoints but can extend out of a tight bounds because of the tangents
		// P0Tangent coefficient maximizes to 4/27 at a=1/3, and P1Tangent minimizes to -4/27 at a=2/3.
		const float MaximumTangentContribution = 4.0f / 27.0f;
		FBox2D Bounds(ForceInit);

		Bounds += FVector2D(P0);
		Bounds += FVector2D(P0 + MaximumTangentContribution * P0Tangent);
		Bounds += FVector2D(P1);
		Bounds += FVector2D(P1 - MaximumTangentContribution * P1Tangent);

		bCloseToSpline = Bounds.ComputeSquaredDistanceToPoint(PointPosition) < 0.1;
	}

	if (bCloseToSpline)
	{
		// Find the closest approach to the spline
		FVector2D ClosestPoint(ForceInit);
		float ClosestDistanceSquared = FLT_MAX;

		const int32 NumStepsToTest = 16;
		const float StepInterval = 1.0f / (float)NumStepsToTest;
		FVector2D Point1 = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, 0.0f);
		for (float TestAlpha = 0.0f; TestAlpha < 1.0f; TestAlpha += StepInterval)
		{
			const FVector2D Point2 = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, TestAlpha + StepInterval);

			const FVector2D ClosestPointToSegment = FMath::ClosestPointOnSegment2D(PointPosition, Point1, Point2);
			const float DistanceSquared = (PointPosition - ClosestPointToSegment).SizeSquared();

			if (DistanceSquared < ClosestDistanceSquared)
			{
				ClosestDistanceSquared = DistanceSquared;
				ClosestPoint = ClosestPointToSegment;
			}

			Point1 = Point2;
		}

		return ClosestPoint - PointPosition;
	}

	return FVector2D(0,0);
}


float FNodeHelper::GetWirePointDistanceIfClose(const FArrangedWidget& ArrangedGraphPanel, SGraphPin* APin, SGraphPin* BPin, FVector2D HitPosition, const UGraphEditorSettings* Settings)
{
	SGraphPanel* graphPanel = static_cast<SGraphPanel*>(&ArrangedGraphPanel.Widget.Get());
	if (!graphPanel)
	{
		return -1;
	}

	FVector2D P0 = APin->GetNodeOffset() + FVector2D(APin->GetPinObj()->GetOwningNode()->NodePosX, APin->GetPinObj()->GetOwningNode()->NodePosY);
	FVector2D P1 = BPin->GetNodeOffset() + FVector2D(BPin->GetPinObj()->GetOwningNode()->NodePosX, BPin->GetPinObj()->GetOwningNode()->NodePosY);

	P0 = (P0 - graphPanel->GetViewOffset()) * graphPanel->GetZoomAmount();
	P0 = ArrangedGraphPanel.Geometry.LocalToAbsolute(P0);
	P1 = (P1 - graphPanel->GetViewOffset()) * graphPanel->GetZoomAmount();
	P1 = ArrangedGraphPanel.Geometry.LocalToAbsolute(P1);

	FVector2D SplineTangent;

	const FVector2D DeltaPos = P1 - P0;
	const bool bGoingForward = DeltaPos.X >= 0.0f;
	const float ClampedTensionX = FMath::Min<float>(FMath::Abs<float>(DeltaPos.X), bGoingForward ? Settings->ForwardSplineHorizontalDeltaRange : Settings->BackwardSplineHorizontalDeltaRange);
	const float ClampedTensionY = FMath::Min<float>(FMath::Abs<float>(DeltaPos.Y), bGoingForward ? Settings->ForwardSplineVerticalDeltaRange : Settings->BackwardSplineVerticalDeltaRange);

	if (bGoingForward)
	{
		SplineTangent = (ClampedTensionX * Settings->ForwardSplineTangentFromHorizontalDelta) + (ClampedTensionY * Settings->ForwardSplineTangentFromVerticalDelta);
	}
	else
	{
		SplineTangent = (ClampedTensionX * Settings->BackwardSplineTangentFromHorizontalDelta) + (ClampedTensionY * Settings->BackwardSplineTangentFromVerticalDelta);
	}

	const FVector2D P0Tangent = (APin->GetDirection() == EGPD_Output) ? SplineTangent : -SplineTangent;
	const FVector2D P1Tangent = (BPin->GetDirection() == EGPD_Input) ? SplineTangent : -SplineTangent;

	bool bCloseToSpline = false;
	{
		// The curve will include the endpoints but can extend out of a tight bounds because of the tangents
		// P0Tangent coefficient maximizes to 4/27 at a=1/3, and P1Tangent minimizes to -4/27 at a=2/3.
		const float MaximumTangentContribution = 4.0f / 27.0f;
		FBox2D Bounds(ForceInit);

		Bounds += FVector2D(P0);
		Bounds += FVector2D(P0 + MaximumTangentContribution * P0Tangent);
		Bounds += FVector2D(P1);
		Bounds += FVector2D(P1 - MaximumTangentContribution * P1Tangent);

		bCloseToSpline = Bounds.ComputeSquaredDistanceToPoint(HitPosition) < 0.1;
	}

	if (bCloseToSpline)
	{
		// Find the closest approach to the spline
		FVector2D ClosestPoint(ForceInit);
		float ClosestDistanceSquared = FLT_MAX;

		const int32 NumStepsToTest = 16;
		const float StepInterval = 1.0f / (float)NumStepsToTest;
		FVector2D Point1 = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, 0.0f);
		for (float TestAlpha = 0.0f; TestAlpha < 1.0f; TestAlpha += StepInterval)
		{
			const FVector2D Point2 = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, TestAlpha + StepInterval);

			const FVector2D ClosestPointToSegment = FMath::ClosestPointOnSegment2D(HitPosition, Point1, Point2);
			const float DistanceSquared = (HitPosition - ClosestPointToSegment).SizeSquared();

			if (DistanceSquared < ClosestDistanceSquared)
			{
				ClosestDistanceSquared = DistanceSquared;
				ClosestPoint = ClosestPointToSegment;
			}

			Point1 = Point2;
		}

		return ClosestDistanceSquared;
	}

	return -1;
}


bool FNodeHelper::GetWirePointHitResult(const FArrangedWidget& ArrangedGraphPanel, SGraphPin* APin, SGraphPin* BPin, FVector2D HitPosition, float WireThickness, const UGraphEditorSettings* Settings)
{
	float hitDist = GetWirePointDistanceIfClose(ArrangedGraphPanel, APin, BPin, HitPosition, Settings);
	if (hitDist != -1 && hitDist < FMath::Square(Settings->SplineHoverTolerance + WireThickness * 0.5f))
	{
		return true;
	}
	return false;
}

struct BypassPinInfo
{
	UEdGraphPin* NodePin;
	TArray<UEdGraphPin*> LinkedPins;
	FORCEINLINE friend bool operator==(const BypassPinInfo& A, const BypassPinInfo& B) { return A.NodePin == B.NodePin && A.LinkedPins == B.LinkedPins; }
};

//todo,for each node block.
bool FNodeHelper::BypassNodes(UEdGraph* EdGraph, TArray<UEdGraphNode*> TargetNodes, bool ForceBypass, bool ForceKeepNode)
{
	TargetNodes.Sort([](UEdGraphNode& A, UEdGraphNode& B) {return A.NodePosY < B.NodePosY; });
	TArray<BypassPinInfo> bypassPinInfosInput;
	TArray<BypassPinInfo> bypassPinInfosOutput;

	for (auto targetNode : TargetNodes)
	{
		for (auto targetNodePin : targetNode->Pins)
		{
			TArray<UEdGraphPin*> targetNodePinLinkOut;
			for (auto targetNodePinLinkedto : targetNodePin->LinkedTo)
			{
				if (!TargetNodes.Contains(targetNodePinLinkedto->GetOwningNode()))
				{
					targetNodePinLinkOut.Add(targetNodePinLinkedto);
				}
			}
			if (targetNodePinLinkOut.Num() > 0)
			{
				targetNodePinLinkOut.Sort([](UEdGraphPin& A, UEdGraphPin& B) {return A.GetOwningNode()->NodePosY > B.GetOwningNode()->NodePosY; });
				if (targetNodePin->Direction == EGPD_Input)
				{
					bypassPinInfosInput.Add({ targetNodePin ,targetNodePinLinkOut });
				}
				else if (targetNodePin->Direction == EGPD_Output)
				{
					bypassPinInfosOutput.Add({ targetNodePin ,targetNodePinLinkOut });
				}
			}
		}
	}
	if (bypassPinInfosInput.Num() == 0 || bypassPinInfosOutput.Num() == 0)
	{
		for (auto bypassInput : bypassPinInfosInput)
		{
			for (auto bypassInputPin : bypassInput.LinkedPins)
			{
				bypassInputPin->BreakLinkTo(bypassInput.NodePin);
			}
		}
		for (auto bypassOutput : bypassPinInfosOutput)
		{
			for (auto bypassOutputPin : bypassOutput.LinkedPins)
			{
				bypassOutputPin->BreakLinkTo(bypassOutput.NodePin);
			}
		}
		if (!ForceKeepNode)
		{
			for (auto targetNode : TargetNodes)
			{
				EdGraph->RemoveNode(targetNode);
			}
		}
		return true;
	}

	//if one side is single pin,connect it to all other side pin.
	//if neither side is single pin,connect one by one.
	//reverse,so top pin will be last.
	bool isOneToMultiBypass = bypassPinInfosInput.Num() == 1 || bypassPinInfosOutput.Num() == 1;
	if (isOneToMultiBypass)
	{
		Algo::Reverse(bypassPinInfosInput);
		Algo::Reverse(bypassPinInfosOutput);
	}

	TArray<BypassPinInfo> pinInfosInput = bypassPinInfosInput;
	TArray<BypassPinInfo> pinInfosOutput = bypassPinInfosOutput;
	//first pass,bypass connectible,ignore force bypass.
	for (auto pinInfoInput : pinInfosInput)
	{
		for (auto pinInfoOutput : pinInfosOutput)
		{
			const ECanCreateConnectionResponse Response = EdGraph->GetSchema()->CanCreateConnection(pinInfoInput.LinkedPins[0], pinInfoOutput.LinkedPins[0]).Response;
			if (Response != CONNECT_RESPONSE_DISALLOW && Response != CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE)
			{
				bypassPinInfosInput.Remove(pinInfoInput);
				bypassPinInfosOutput.Remove(pinInfoOutput);

				for (auto inputLink : pinInfoInput.LinkedPins)
				{
					for (auto outputlink : pinInfoOutput.LinkedPins)
					{
						pinInfoInput.NodePin->BreakLinkTo(inputLink);
						pinInfoOutput.NodePin->BreakLinkTo(outputlink);
						UEdGraphNode* linkSourcePinNode = inputLink->GetOwningNode();
						UEdGraphNode* linkTargetPinNode = outputlink->GetOwningNode();
						EdGraph->GetSchema()->TryCreateConnection(inputLink, outputlink);
						linkSourcePinNode->NodeConnectionListChanged();
						linkTargetPinNode->NodeConnectionListChanged();
					}
				}

				if (!isOneToMultiBypass)
				{
					pinInfosOutput.Remove(pinInfoOutput);
					break;
				}
			}
		}
	}

	//todo,how to deal with multi to one auto conversion
	bool isMultiToOneConversion = isOneToMultiBypass && bypassPinInfosOutput.Num() == 1;
	if (isMultiToOneConversion)
	{
		Algo::Reverse(bypassPinInfosInput);
		Algo::Reverse(bypassPinInfosOutput);
	}
	pinInfosInput = bypassPinInfosInput;
	pinInfosOutput = bypassPinInfosOutput;
	//second pass,bypass possible auto conversion pin,or force bypass.
	for (auto pinInfoInput : pinInfosInput)
	{
		for (auto pinInfoOutput : pinInfosOutput)
		{
			const ECanCreateConnectionResponse Response = EdGraph->GetSchema()->CanCreateConnection(pinInfoInput.LinkedPins[0], pinInfoOutput.LinkedPins[0]).Response;
			if (ForceBypass || Response != CONNECT_RESPONSE_DISALLOW)
			{
				bypassPinInfosInput.Remove(pinInfoInput);
				bypassPinInfosOutput.Remove(pinInfoOutput);

				for (auto inputLink : pinInfoInput.LinkedPins)
				{
					for (auto outputlink : pinInfoOutput.LinkedPins)
					{
						pinInfoInput.NodePin->BreakLinkTo(inputLink);
						pinInfoOutput.NodePin->BreakLinkTo(outputlink);
						UEdGraphNode* linkSourcePinNode = inputLink->GetOwningNode();
						UEdGraphNode* linkTargetPinNode = outputlink->GetOwningNode();
						EdGraph->GetSchema()->TryCreateConnection(inputLink, outputlink);
						linkSourcePinNode->NodeConnectionListChanged();
						linkTargetPinNode->NodeConnectionListChanged();
					}
				}

				if (!isOneToMultiBypass || isMultiToOneConversion)
				{
					pinInfosOutput.Remove(pinInfoOutput);
					break;
				}
			}
		}
	}

	bool success = bypassPinInfosInput.Num() == 0 && bypassPinInfosOutput.Num() == 0;
	if ((success || ForceBypass) && !ForceKeepNode)
	{
		for (auto targetNode : TargetNodes)
		{
			EdGraph->RemoveNode(targetNode);
		}
		return true;
	}

	return success;
}


TArray<TArray<UEdGraphNode*>> FNodeHelper::CalculateNodeshierarchy(TArray<UEdGraphNode*> sourceNodes, EAlignDirection AlignDirection)
{
	TArray<UEdGraphNode*> validatedNodes;
	for (auto sourceNode : sourceNodes)
	{
		if (!sourceNode)
		{
			continue;
		}
		validatedNodes.AddUnique(sourceNode);
	}

	sourceNodes = validatedNodes;
	TArray<TArray<UEdGraphNode*>> arrangedNodes;
	arrangedNodes.Add(validatedNodes);

	//find the grid position for every node.
	for (auto sourceNode : sourceNodes)
	{
		for (int i = 0; i < arrangedNodes.Num(); i++)
		{
			TArray<UEdGraphNode*> colume = arrangedNodes[i];
			if (colume.Find(sourceNode) != INDEX_NONE)
			{
				//find upstream,downstream,side nodes in the colume that the node resides.
				TArray<UEdGraphNode*> nodesToCheck;
				TArray<UEdGraphNode*> nodesToSkip;

				TArray<UEdGraphNode*> newDownStreamColume;
				TArray<UEdGraphNode*> newUpStreamColume;

				nodesToCheck.Add(sourceNode);
				nodesToSkip.Add(sourceNode);
				while (nodesToCheck.Num() > 0)
				{
					for (auto pinToCheck : nodesToCheck[0]->Pins)
					{
						if (pinToCheck->Direction == EGPD_Input)
						{
							for (auto linkedPin : pinToCheck->LinkedTo)
							{
								UEdGraphNode* linkedNode = linkedPin->GetOwningNode();

								if (nodesToSkip.Find(linkedNode) == INDEX_NONE)
								{
									nodesToSkip.AddUnique(linkedNode);
									if (colume.Find(linkedNode) != INDEX_NONE)
									{
										newDownStreamColume.AddUnique(linkedNode);
										nodesToCheck.AddUnique(linkedNode);
									}
								}
							}
						}
					}
					nodesToSkip.Add(nodesToCheck[0]);
					nodesToCheck.RemoveAt(0);
				}
				nodesToSkip.Empty();
				nodesToCheck.Add(sourceNode);
				nodesToSkip.Add(sourceNode);
				while (nodesToCheck.Num() > 0)
				{
					for (auto pinToCheck : nodesToCheck[0]->Pins)
					{
						if (pinToCheck->Direction == EGPD_Output)
						{
							for (auto linkedPin : pinToCheck->LinkedTo)
							{
								UEdGraphNode* linkedNode = linkedPin->GetOwningNode();

								if (nodesToSkip.Find(linkedNode) == INDEX_NONE)
								{
									nodesToSkip.AddUnique(linkedNode);
									if (colume.Find(linkedNode) != INDEX_NONE)
									{
										newUpStreamColume.AddUnique(linkedNode);
										nodesToCheck.AddUnique(linkedNode);
									}
								}
							}
						}
					}
					nodesToSkip.Add(nodesToCheck[0]);
					nodesToCheck.RemoveAt(0);
				}

				//subdivide current colume.
				if (newUpStreamColume.Num() > 0)//upstream before downstream,so the array order wont change.
				{
					arrangedNodes.Insert(newUpStreamColume, i + 1);
					for (auto oldNode : newUpStreamColume)
					{
						arrangedNodes[i].Remove(oldNode);
					}
				}
				if (newDownStreamColume.Num() > 0)
				{
					arrangedNodes.Insert(newDownStreamColume, i);
					for (auto oldNode : newDownStreamColume)
					{
						arrangedNodes[i + 1].Remove(oldNode);
					}
				}

				break;
			}
		}
	}

	//push to align direction to get rid of empty slots
	if(AlignDirection == AD_OutputDirection)
	{
		for (int i = arrangedNodes.Num() - 2; i > -1; i--)
		{
			for (int j = arrangedNodes[i].Num() - 1; j > -1; j--) //remember to do it backward!
			{
				UEdGraphNode* currentNode = arrangedNodes[i][j];
				int availableColIndex = i;

				//try to push to end colume.
				while (availableColIndex < arrangedNodes.Num() - 1)
				{
					availableColIndex++;
					for (auto pinToCheck : currentNode->Pins)
					{
						if (pinToCheck->Direction == EGPD_Output)
						{
							for (auto linkedPin : pinToCheck->LinkedTo)
							{
								UEdGraphNode* linkedNode = linkedPin->GetOwningNode();

								//if next colume has no nodes that are linked to this node,this node can go to next colume.
								if (arrangedNodes[availableColIndex].Find(linkedNode) != INDEX_NONE)
								{
									availableColIndex--;
									goto outOfLoop1;
								}
							}
						}
					}
				}
			outOfLoop1:
				if (availableColIndex > i)
				{
					arrangedNodes[availableColIndex].Add(currentNode);
					arrangedNodes[i].Remove(currentNode);
				}
			}
		}
	}
	else
	{
		for (int i = 1; i < arrangedNodes.Num(); i++)
		{
			for (int j = arrangedNodes[i].Num() - 1; j > -1; j--)
			{
				UEdGraphNode* currentNode = arrangedNodes[i][j];
				int availableColIndex = i;

				//try to push to end colume.
				while (availableColIndex > 0)
				{
					availableColIndex--;
					for (auto pinToCheck : currentNode->Pins)
					{
						if (pinToCheck->Direction == EGPD_Input)
						{
							for (auto linkedPin : pinToCheck->LinkedTo)
							{
								UEdGraphNode* linkedNode = linkedPin->GetOwningNode();

								//if next colume has no nodes that are linked to this node,this node can go to next colume.
								if (arrangedNodes[availableColIndex].Find(linkedNode) != INDEX_NONE)
								{
									availableColIndex++;
									goto outOfLoop2;
								}
							}
						}
					}
				}
			outOfLoop2:
				if (availableColIndex < i)
				{
					arrangedNodes[availableColIndex].Add(currentNode);
					arrangedNodes[i].Remove(currentNode);
				}
			}
		}
	}

	//clean up nodes.
	for (int i = 0; i < arrangedNodes.Num(); i++)
	{
		if (arrangedNodes[i].Num() < 1)
		{
			arrangedNodes.RemoveAt(i);
			if (arrangedNodes.Num() > 1)
			{
				i--;
			}
		}
	}

	return arrangedNodes;
}


//	       |node|\
//	|node|-|node|-|node|-|node| 
//	|node|-|node|-|node|	
//	       |node|/
//  col0	col1   col2   col3
bool FNodeHelper::RearrangeSelectedNodes(SGraphPanel* graphPanel, FIntPoint Spacing, float SpacingRelax)
{
	if (!graphPanel)
	{
		return false;
	}

	TArray<UEdGraphNode*> selectedNodes;
	TArray<UEdGraphNode*> nodesToUnselect;;
	for (auto selectedNode : graphPanel->SelectionManager.SelectedNodes)
	{
		UEdGraphNode* graphNode = Cast<UEdGraphNode>(selectedNode);
		if (!graphNode)
		{
			continue;
		}

		if (Cast<UEdGraphNode_Comment>(graphNode))
		{
			nodesToUnselect.Add(graphNode);
			continue;
		}
		selectedNodes.Add(graphNode);
	}
	for (auto nodeToUnselect : nodesToUnselect)
	{
		graphPanel->SelectionManager.SetNodeSelection(nodeToUnselect, false);
	}

	if (selectedNodes.Num() < 2)
	{
		return false;
	}

	TArray<TArray<UEdGraphNode*>> arrangedNodes;
	arrangedNodes = CalculateNodeshierarchy(selectedNodes, AD_OutputDirection);

	//calculate node position.
	//every nodes' position is determined by its linked output pins height,then reorder colume nodes by this value.
	//except for the fist colume nodes.

	TArray<FIntPoint> columePos;
	TArray<FIntPoint> columeSize;

	for (int i = arrangedNodes.Num() - 1; i > -1; i--)
	{
		int32 columeHeight = 0;
		int32 columeWidth = 0;

		//special arrangement for head colume as start point.
		if (i == arrangedNodes.Num() - 1)
		{
			for (int j = 0; j < arrangedNodes[i].Num(); j++)
			{
				auto nodeWidget = graphPanel->GetNodeWidgetFromGuid(arrangedNodes[i][j]->NodeGuid);
				if (!nodeWidget.IsValid())
				{
					return false;
				}
				FVector2D nodeSizeF = nodeWidget->GetDesiredSize();
				FIntPoint nodeSize = FIntPoint((int32)nodeSizeF.X, (int32)nodeSizeF.Y);

				columeHeight += nodeSize.Y;
				columeHeight = j > 0 ? columeHeight + Spacing.Y : columeHeight;
				columeWidth = FMath::Max(columeWidth, nodeSize.X);
			}

			arrangedNodes[i].Sort([](UEdGraphNode& A, UEdGraphNode& B) {return A.NodePosY < B.NodePosY; });

			FVector2D headNodeSize = graphPanel->GetNodeWidgetFromGuid(arrangedNodes[i][0]->NodeGuid)->GetDesiredSize();

			columePos.Insert({ arrangedNodes[i][0]->NodePosX,arrangedNodes[i][0]->NodePosY }, 0);
			columeSize.Insert({ columeWidth, columeHeight }, 0);
		}
		else
		{
			TMap<UEdGraphNode*, int32 > nodeDesirePosYMap;

			int32 columeTempPosY = 0;
			int32 columeTempPosX = 0;

			int32 columePinsPosYMax = TNumericLimits< int32 >::Min();
			int32 columePinsPosYMin = TNumericLimits< int32 >::Max();
			int32 columeLinkedPinsPosYMax = TNumericLimits< int32 >::Min();
			int32 columeLinkedPinsPosYMin = TNumericLimits< int32 >::Max();

			//calculate colume info
			for (int j = 0; j < arrangedNodes[i].Num(); j++)
			{
				auto nodeWidget = graphPanel->GetNodeWidgetFromGuid(arrangedNodes[i][j]->NodeGuid);
				if (!nodeWidget.IsValid())
				{
					return false;
				}
				FVector2D nodeSizeF = nodeWidget->GetDesiredSize();
				FIntPoint nodeSize = FIntPoint((int32)nodeSizeF.X, (int32)nodeSizeF.Y);

				columeHeight += nodeSize.Y;
				columeHeight = j > 0 ? columeHeight + Spacing.Y : columeHeight;
				columeWidth = FMath::Max(columeWidth, nodeSize.X);

				int linkedPinNum = 0;
				float linkedPinPosYSum = 0;

				for (auto pinToCheck : arrangedNodes[i][j]->Pins)
				{
					if (pinToCheck->Direction != EGPD_Output)
					{
						continue;
					}
					for (auto linkedPin : pinToCheck->LinkedTo)
					{
						UEdGraphNode* linkedNode = linkedPin->GetOwningNode();
						if (arrangedNodes[i + 1].Find(linkedNode) != INDEX_NONE)
						{
							TSharedPtr<SGraphPin> sSourcePin = graphPanel->GetNodeWidgetFromGuid(arrangedNodes[i][j]->NodeGuid)->FindWidgetForPin(pinToCheck);
							TSharedPtr<SGraphPin> sLinkedPin = graphPanel->GetNodeWidgetFromGuid(linkedNode->NodeGuid)->FindWidgetForPin(linkedPin);

							//special cases like anim state graph, check to make sure
							if (!sSourcePin.IsValid() || !sLinkedPin.IsValid())
							{
								linkedPinPosYSum += linkedNode->NodePosY;
								linkedPinNum++;
								continue;
							}

							linkedPinPosYSum += linkedNode->NodePosY + sLinkedPin->GetNodeOffset().Y;
							linkedPinNum++;

							//to improve,calculate true colume pos y.
							columePinsPosYMax = FMath::Max(arrangedNodes[i][j]->NodePosY + int32(sSourcePin->GetNodeOffset().Y + sSourcePin->GetDesiredSize().Y), columePinsPosYMax);
							columePinsPosYMin = FMath::Min(arrangedNodes[i][j]->NodePosY + int32(sSourcePin->GetNodeOffset().Y), columePinsPosYMin);
							columeLinkedPinsPosYMax = FMath::Max(linkedNode->NodePosY + int32(sLinkedPin->GetNodeOffset().Y + sLinkedPin->GetDesiredSize().Y), columeLinkedPinsPosYMax);
							columeLinkedPinsPosYMin = FMath::Min(linkedNode->NodePosY + int32(sLinkedPin->GetNodeOffset().Y), columeLinkedPinsPosYMin);
						}
					}
				}

				//calculate average linked pin height can make wires more relaxed.
				float aveLinkedPosY = linkedPinNum != 0 ? linkedPinPosYSum / linkedPinNum : arrangedNodes[i][j]->NodePosY;
				nodeDesirePosYMap.Add(arrangedNodes[i][j], aveLinkedPosY);
				columeTempPosY += aveLinkedPosY;
			}
			arrangedNodes[i].Sort([nodeDesirePosYMap](UEdGraphNode& A, UEdGraphNode& B)
			{
				if (nodeDesirePosYMap[&A] != nodeDesirePosYMap[&B])
				{
					return nodeDesirePosYMap[&A] < nodeDesirePosYMap[&B];
				}
				//if two nodes connect to one pin,node with high Y will appear on top.
				else
				{
					return A.NodePosY < B.NodePosY;
				}
			});
			nodeDesirePosYMap.Empty();

			columeTempPosY /= arrangedNodes[i].Num();
			columeTempPosY -= columeHeight / 2;
			columeTempPosX = columePos[0].X - columeWidth - Spacing.X;

			if (columePinsPosYMax > columePinsPosYMin && columeLinkedPinsPosYMax > columeLinkedPinsPosYMin)
			{
				columeTempPosX -= FMath::Max(columePinsPosYMax - columePinsPosYMin, columeLinkedPinsPosYMax - columeLinkedPinsPosYMin) / FMath::Max(30,FMath::Min(columePinsPosYMax - columePinsPosYMin, columeLinkedPinsPosYMax - columeLinkedPinsPosYMin)) * SpacingRelax;
			}

			columePos.Insert({ columeTempPosX,columeTempPosY }, 0);
			columeSize.Insert({ columeWidth, columeHeight }, 0);
		}

		//move nodes on this colume to new position.
		FVector2D currentColumePos = columePos[0];

		if (i < arrangedNodes.Num() - 1)
		{
			if (arrangedNodes[i][0]->Pins[0]->PinType.PinCategory == TEXT("exec"))
			{
				if (arrangedNodes[i + 1][0]->Pins[0]->PinType.PinCategory == TEXT("exec"))
				{
					currentColumePos.Y = columePos[1].Y;
					columePos[0].Y = columePos[1].Y;
				}
			}

			//make node straight line,to improve
			if (arrangedNodes[i].Num() == 1 && arrangedNodes[i + 1].Num() == 1)
			{
				if (columeSize[0].Y == columeSize[1].Y)
				{
					currentColumePos.Y = columePos[1].Y;
					columePos[0].Y = columePos[1].Y;
				}
			}
		}

		for (int j = 0; j < arrangedNodes[i].Num(); j++)
		{
			FVector2D nodeSize = graphPanel->GetNodeWidgetFromGuid(arrangedNodes[i][j]->NodeGuid)->GetDesiredSize();

			float moveToX = currentColumePos.X + (columeWidth-nodeSize.X) / 2;
			float moveToY = currentColumePos.Y;

			if (i == 0)
			{
				moveToX = currentColumePos.X + (columeWidth - nodeSize.X);
			}

			if (i == arrangedNodes.Num() - 1)
			{
				moveToX = currentColumePos.X;
			}

			TSet<TWeakPtr<SNodePanel::SNode>> nodeFilter;
			graphPanel->GetNodeWidgetFromGuid(arrangedNodes[i][j]->NodeGuid)->MoveTo({ moveToX , moveToY }, nodeFilter);
			currentColumePos.Y += nodeSize.Y + Spacing.Y;
		}
	}

	return true;
}


//	       |node| 			    col0
//			/   \
//	   |node|   |node|          col1
//       |       /   \
//	 |node|  |node| |node|		col2
//             |
//	         |node|			    col3
bool FNodeHelper::RearrangeSelectedNodes_AIGraph(SGraphPanel* graphPanel, FIntPoint Spacing, float SpacingRelax)
{
	if (!graphPanel)
	{
		return false;
	}

	TArray<UEdGraphNode*> selectedNodes;
	TArray<UEdGraphNode*> nodesToUnselect;
	for (auto selectedNode : graphPanel->SelectionManager.SelectedNodes)
	{
		UEdGraphNode* graphNode = Cast<UEdGraphNode>(selectedNode);
		if (!graphNode)
		{
			continue;
		}

		if (Cast<UEdGraphNode_Comment>(graphNode))
		{
			nodesToUnselect.Add(graphNode);
			continue;
		}
		selectedNodes.Add(graphNode);
	}
	for (auto nodeToUnselect : nodesToUnselect)
	{
		graphPanel->SelectionManager.SetNodeSelection(nodeToUnselect, false);
	}
	if (selectedNodes.Num() < 2)
	{
		return false;
	}

	TArray<TArray<UEdGraphNode*>> arrangedNodes;
	arrangedNodes = CalculateNodeshierarchy(selectedNodes, AD_InputDirection);

	//calculate node position.
	//every nodes' position is determined by its linked output pins height,then reorder colume nodes by this value.
	//except for the fist colume nodes.

	TArray<FIntPoint> columePos;
	TArray<FIntPoint> columeSize;

	for (int i = 0; i < arrangedNodes.Num(); i++)
	{
		int32 columeHeight = 0;
		int32 columeWidth = 0;

		if (i == 0)
		{
			for (int j = 0; j < arrangedNodes[i].Num(); j++)
			{
				auto nodeWidget = graphPanel->GetNodeWidgetFromGuid(arrangedNodes[i][j]->NodeGuid);
				if (!nodeWidget.IsValid())
				{
					return false;
				}
				FVector2D nodeSizeF = graphPanel->GetNodeWidgetFromGuid(arrangedNodes[i][j]->NodeGuid)->GetDesiredSize();
				FIntPoint nodeSize = FIntPoint((int32)nodeSizeF.X, (int32)nodeSizeF.Y);

				columeWidth += nodeSize.X;
				columeWidth = j > 0 ? columeWidth + Spacing.X : columeWidth;
				columeHeight = FMath::Max(columeHeight, nodeSize.Y);
			}

			arrangedNodes[i].Sort([](UEdGraphNode& A, UEdGraphNode& B) {return A.NodePosX <  B.NodePosX; });

			FVector2D headNodeSize = graphPanel->GetNodeWidgetFromGuid(arrangedNodes[i][0]->NodeGuid)->GetDesiredSize();

			columePos.Insert({ arrangedNodes[i][0]->NodePosX, arrangedNodes[i][0]->NodePosY + (columeHeight - (int32)headNodeSize.Y) / 2 }, 0);
			columeSize.Insert({ columeWidth, columeHeight }, 0);
		}
		else
		{
			TMap<UEdGraphNode*, int32 > nodeDesirePosXMap;

			int32 columeTempPosY = 0;
			int32 columeTempPosX = 0;

			int32 columePinsPosXMax = TNumericLimits< int32 >::Min();
			int32 columePinsPosXMin = TNumericLimits< int32 >::Max();
			int32 columeLinkedPinsPosXMax = TNumericLimits< int32 >::Min();
			int32 columeLinkedPinsPosXMin = TNumericLimits< int32 >::Max();

			for (int j = 0; j < arrangedNodes[i].Num(); j++)
			{
				auto nodeWidget = graphPanel->GetNodeWidgetFromGuid(arrangedNodes[i][j]->NodeGuid);
				if (!nodeWidget.IsValid())
				{
					return false;
				}
				FVector2D nodeSizeF = graphPanel->GetNodeWidgetFromGuid(arrangedNodes[i][j]->NodeGuid)->GetDesiredSize();
				FIntPoint nodeSize = FIntPoint((int32)nodeSizeF.X, (int32)nodeSizeF.Y);

				columeWidth += nodeSize.X;
				columeWidth = j > 0 ? columeWidth += Spacing.X : columeWidth;
				columeHeight = FMath::Max(columeHeight, nodeSize.Y);

				int linkedPinNum = 0;
				float linkedPinPosXSum = 0;

				for (auto pinToCheck : arrangedNodes[i][j]->Pins)
				{
					if (pinToCheck->Direction != EGPD_Input)
					{
						continue;
					}
					for (auto linkedPin : pinToCheck->LinkedTo)
					{
						UEdGraphNode* linkedNode = linkedPin->GetOwningNode();
						if (arrangedNodes[i - 1].Find(linkedNode) != INDEX_NONE)
						{
							TSharedPtr<SGraphPin> sSourcePin = graphPanel->GetNodeWidgetFromGuid(arrangedNodes[i][j]->NodeGuid)->FindWidgetForPin(pinToCheck);
							TSharedPtr<SGraphPin> sLinkedPin = graphPanel->GetNodeWidgetFromGuid(linkedNode->NodeGuid)->FindWidgetForPin(linkedPin);

							if (!sSourcePin.IsValid() || !sLinkedPin.IsValid())
							{
								linkedPinPosXSum += linkedNode->NodePosX;
								linkedPinNum++;
								continue;
							}

							linkedPinPosXSum += linkedNode->NodePosX + sLinkedPin->GetNodeOffset().X + sLinkedPin->GetDesiredSize().X/2; //ai graph node has wide pin.
							linkedPinNum++;

							columePinsPosXMax = FMath::Max(arrangedNodes[i][j]->NodePosX + int32(sSourcePin->GetNodeOffset().X + sSourcePin->GetDesiredSize().X), columePinsPosXMax);
							columePinsPosXMin = FMath::Min(arrangedNodes[i][j]->NodePosX + int32(sSourcePin->GetNodeOffset().X), columePinsPosXMin);
							columeLinkedPinsPosXMax = FMath::Max(linkedNode->NodePosX + int32(sLinkedPin->GetNodeOffset().X  +sLinkedPin->GetDesiredSize().X), columeLinkedPinsPosXMax);
							columeLinkedPinsPosXMin = FMath::Min(linkedNode->NodePosX + int32(sLinkedPin->GetNodeOffset().X), columeLinkedPinsPosXMin);
						}
					}
				}

				float aveLinkedPosX = linkedPinNum != 0 ? linkedPinPosXSum / linkedPinNum : arrangedNodes[i][j]->NodePosX;

				nodeDesirePosXMap.Add(arrangedNodes[i][j], aveLinkedPosX);
				columeTempPosX += aveLinkedPosX;
			}

			arrangedNodes[i].Sort([nodeDesirePosXMap](UEdGraphNode& A, UEdGraphNode& B)
			{
				if (nodeDesirePosXMap[&A] != nodeDesirePosXMap[&B])
				{
					return nodeDesirePosXMap[&A] < nodeDesirePosXMap[&B];
				}
				else
				{
					return A.NodePosX < B.NodePosX;
				}
				
			});
			nodeDesirePosXMap.Empty();

			columeTempPosX /= arrangedNodes[i].Num();
			columeTempPosX -= columeWidth / 2;
			columeTempPosY = columePos[0].Y + columeSize[0].Y + Spacing.Y;

			if (columePinsPosXMax > columePinsPosXMin && columeLinkedPinsPosXMax > columeLinkedPinsPosXMin)
			{
				columeTempPosY += FMath::Max(columePinsPosXMax- columePinsPosXMin, columeLinkedPinsPosXMax- columeLinkedPinsPosXMin) / FMath::Max(30,FMath::Min(columePinsPosXMax - columePinsPosXMin, columeLinkedPinsPosXMax - columeLinkedPinsPosXMin)) * SpacingRelax;
			}

			columePos.Insert({ columeTempPosX,columeTempPosY }, 0);
			columeSize.Insert({ columeWidth, columeHeight }, 0);
		}

		//move nodes on this colume to new position.
		FVector2D currentColumePos = columePos[0];
		for (int j = 0; j < arrangedNodes[i].Num(); j++)
		{
			FVector2D nodeSize = graphPanel->GetNodeWidgetFromGuid(arrangedNodes[i][j]->NodeGuid)->GetDesiredSize();

			float moveToX = currentColumePos.X;
			float moveToY = currentColumePos.Y + (columeHeight - nodeSize.Y) / 2;

			if (i == 0)
			{
				moveToY = currentColumePos.Y + columeHeight - nodeSize.Y;
			}

			TSet<TWeakPtr<SNodePanel::SNode>> nodeFilter;
			graphPanel->GetNodeWidgetFromGuid(arrangedNodes[i][j]->NodeGuid)->MoveTo({ moveToX  , moveToY }, nodeFilter);
			currentColumePos.X += nodeSize.X + Spacing.X;
		}
	}

	return true;
}


TArray<TSharedRef<SGraphPin>> FNodeHelper::GetPins(TSharedRef<SGraphNode> GraphNode)
{
	TArray<TSharedRef<SGraphPin>> retGraphPins;

	TArray<TSharedRef<SWidget>> graphNodePinWidgets;
	GraphNode->GetPins(graphNodePinWidgets);
	for (auto graphNodePinWidget : graphNodePinWidgets)
	{
		retGraphPins.AddUnique(StaticCastSharedRef<SGraphPin>(graphNodePinWidget));
	}
	return retGraphPins;
}

void FNodeHelper::GetAutoConnectablePins(const UEdGraphSchema* GraphSchema, float MaxConnectionDistance, TArray<TSharedRef<SGraphNode>> InSourceNodes, TArray<TSharedRef<SGraphNode>> InTargetNodes, TArray<TWeakPtr<SGraphPin>>& OutStartPins, TArray<TWeakPtr<SGraphPin>>& OutEndPins)
{
	OutStartPins.Empty();
	OutEndPins.Empty();

	//cull distant node
	if (MaxConnectionDistance > 0)
	{
		TArray<TSharedRef<SGraphNode>> connectableTargetNodes;
		for (auto sourceNode : InSourceNodes)
		{
			for (auto targetNode : InTargetNodes)
			{
				if (!connectableTargetNodes.Contains(targetNode))
				{
					if (FBox2D(sourceNode->GetPosition() - FVector2D(MaxConnectionDistance, MaxConnectionDistance), sourceNode->GetPosition() + sourceNode->GetDesiredSize() + FVector2D(MaxConnectionDistance, MaxConnectionDistance)).Intersect(FBox2D(targetNode->GetPosition(), targetNode->GetPosition() + targetNode->GetDesiredSize())))
					{
						connectableTargetNodes.Add(targetNode);
					}
				}
			}
		}
		InTargetNodes = connectableTargetNodes;
	}
	else
	{
		MaxConnectionDistance = FLT_MAX;
	}

	//try to connect pin from top to bottom
	InSourceNodes.Sort([](TSharedRef<SGraphNode> A, TSharedRef<SGraphNode> B) {return A->GetPosition().Y < B->GetPosition().Y; });
	InTargetNodes.Sort([](TSharedRef<SGraphNode> A, TSharedRef<SGraphNode> B) {return A->GetPosition().Y < B->GetPosition().Y; });

	TArray<float> startEndPinDist;

	for (auto sourceNode : InSourceNodes)
	{
		for (auto sourceNodePin : FNodeHelper::GetPins(sourceNode))
		{
			float closestDistTotargetPin = MaxConnectionDistance;
			TSharedRef<SGraphPin> closestTargetPin = sourceNodePin;
			for (auto targetNode : InTargetNodes)
			{
				bool isConnected = false;
				for (auto pinIsConnected : FNodeHelper::GetPins(sourceNode))
				{
					for (auto linkToIsConnected : pinIsConnected->GetPinObj()->LinkedTo)
					{
						if (linkToIsConnected->GetOwningNode() == targetNode->GetNodeObj())
						{
							isConnected = true;
							break;
						}
					}
				}
				if (isConnected)
				{
					continue;
				}

				for (auto targetNodePin : FNodeHelper::GetPins(targetNode))
				{
					if (sourceNodePin->GetDirection() != targetNodePin->GetDirection())
					{
						FVector2D sourceNodePinPos = sourceNodePin->GetNodeOffset() + sourceNode->GetPosition();
						sourceNodePinPos = sourceNodePinPos + FVector2D(sourceNodePin->GetDirection() == EEdGraphPinDirection::EGPD_Output ? sourceNodePin->GetDesiredSize().X : 0, sourceNodePin->GetDesiredSize().Y / 2);
						FVector2D targetNodePinPos = targetNodePin->GetNodeOffset() + targetNode->GetPosition();
						targetNodePinPos = targetNodePinPos + FVector2D(targetNodePin->GetDirection() == EEdGraphPinDirection::EGPD_Output ? targetNodePin->GetDesiredSize().X : 0, targetNodePin->GetDesiredSize().Y / 2);

						if (sourceNodePin->GetDirection() == EEdGraphPinDirection::EGPD_Input && targetNodePinPos.X > sourceNodePinPos.X)
						{
							continue;
						}
						if (sourceNodePin->GetDirection() == EEdGraphPinDirection::EGPD_Output && targetNodePinPos.X < sourceNodePinPos.X)
						{
							continue;
						}

						float pinDist = FVector2D::Distance(sourceNodePinPos, targetNodePinPos);
						if (pinDist < closestDistTotargetPin)
						{
							//seems CanCreateConnection slow down performance,so make it last.
							if (GraphSchema->CanCreateConnection(sourceNodePin->GetPinObj(), targetNodePin->GetPinObj()).Response == ECanCreateConnectionResponse::CONNECT_RESPONSE_MAKE)
							{
								closestDistTotargetPin = pinDist;
								closestTargetPin = targetNodePin;
							}
						}
					}
				}
			}
			if (closestDistTotargetPin < MaxConnectionDistance)
			{
				//if target pin was already matched with other source pin,replace it if it's closer.
				if (OutEndPins.Contains(closestTargetPin))
				{
					int index = OutEndPins.Find(closestTargetPin);
					if (closestDistTotargetPin < startEndPinDist[index])
					{
						OutStartPins.RemoveAt(index);
						OutEndPins.RemoveAt(index);
						startEndPinDist.RemoveAt(index);

						OutStartPins.Add(TWeakPtr<SGraphPin>(sourceNodePin));
						OutEndPins.Add(TWeakPtr<SGraphPin>(closestTargetPin));
						startEndPinDist.Add(closestDistTotargetPin);
					}
				}
				else
				{
					OutStartPins.Add(TWeakPtr<SGraphPin>(sourceNodePin));
					OutEndPins.Add(TWeakPtr<SGraphPin>(closestTargetPin));
					startEndPinDist.Add(closestDistTotargetPin);
				}
			}
		}
	}
}

//#pragma optimize("", on)