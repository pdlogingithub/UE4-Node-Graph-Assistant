// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#ifndef NGA_WITH_ENGINE_CPP

#include "AnimationStateNodes/SGraphNodeAnimTransition.h"
#include "AnimStateTransitionNode.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SToolTip.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimBlueprint.h"
#include "SGraphPanel.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "SKismetLinearExpression.h"

#include "ConnectionDrawingPolicy.h"
#include "IDocumentation.h"

#include "AnimationTransitionGraph.h"
#include "AnimGraphNode_TransitionResult.h"
#include "Animation/AnimNode_StateMachine.h"
#include "AnimGraphNode_StateMachineBase.h"
#include "AnimationStateMachineGraph.h"

#define LOCTEXT_NAMESPACE "TransitionNodes"

/////////////////////////////////////////////////////
// SGraphNodeAnimTransition

void SGraphNodeAnimTransition::Construct(const FArguments& InArgs, UAnimStateTransitionNode* InNode)
{
	this->GraphNode = InNode;
	this->UpdateGraphNode();
}

void SGraphNodeAnimTransition::GetNodeInfoPopups(FNodeInfoContext* Context, TArray<FGraphInformationPopupInfo>& Popups) const
{
}

void SGraphNodeAnimTransition::MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter)
{
	// Ignored; position is set by the location of the attached state nodes
}

bool SGraphNodeAnimTransition::RequiresSecondPassLayout() const
{
	return true;
}

void SGraphNodeAnimTransition::PerformSecondPassLayout(const TMap< UObject*, TSharedRef<SNode> >& NodeToWidgetLookup) const
{
	UAnimStateTransitionNode* TransNode = CastChecked<UAnimStateTransitionNode>(GraphNode);

	// Find the geometry of the state nodes we're connecting
	FGeometry StartGeom;
	FGeometry EndGeom;

	int32 TransIndex = 0;
	int32 NumOfTrans = 1;

	UAnimStateNodeBase* PrevState = TransNode->GetPreviousState();
	UAnimStateNodeBase* NextState = TransNode->GetNextState();
	if ((PrevState != NULL) && (NextState != NULL))
	{
		const TSharedRef<SNode>* pPrevNodeWidget = NodeToWidgetLookup.Find(PrevState);
		const TSharedRef<SNode>* pNextNodeWidget = NodeToWidgetLookup.Find(NextState);
		if ((pPrevNodeWidget != NULL) && (pNextNodeWidget != NULL))
		{
			const TSharedRef<SNode>& PrevNodeWidget = *pPrevNodeWidget;
			const TSharedRef<SNode>& NextNodeWidget = *pNextNodeWidget;

			StartGeom = FGeometry(FVector2D(PrevState->NodePosX, PrevState->NodePosY), FVector2D::ZeroVector, PrevNodeWidget->GetDesiredSize(), 1.0f);
			EndGeom = FGeometry(FVector2D(NextState->NodePosX, NextState->NodePosY), FVector2D::ZeroVector, NextNodeWidget->GetDesiredSize(), 1.0f);

			TArray<UAnimStateTransitionNode*> Transitions;
			PrevState->GetTransitionList(Transitions);

			Transitions = Transitions.FilterByPredicate([NextState](const UAnimStateTransitionNode* InTransition) -> bool
			{
				return InTransition->GetNextState() == NextState;
			});

			TransIndex = Transitions.IndexOfByKey(TransNode);
			NumOfTrans = Transitions.Num();
		}
	}

	//Position Node
	PositionBetweenTwoNodesWithOffset(StartGeom, EndGeom, TransIndex, NumOfTrans);
}

TSharedRef<SWidget> SGraphNodeAnimTransition::GenerateRichTooltip()
{
	UAnimStateTransitionNode* TransNode = CastChecked<UAnimStateTransitionNode>(GraphNode);

	if (TransNode->BoundGraph == NULL)
	{
		return SNew(STextBlock).Text(LOCTEXT("NoAnimGraphBoundToNodeMessage", "Error: No graph"));
	}

	// Find the expression hooked up to the can execute pin of the transition node
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* CanExecPin = NULL;

	if (UAnimationTransitionGraph* TransGraph = Cast<UAnimationTransitionGraph>(TransNode->BoundGraph))
	{
		if (UAnimGraphNode_TransitionResult* ResultNode = TransGraph->GetResultNode())
		{
			CanExecPin = ResultNode->FindPin(TEXT("bCanEnterTransition"));
		}
	}

	TSharedRef<SVerticalBox> Widget = SNew(SVerticalBox);

	const FText TooltipDesc = GetPreviewCornerText(false);

	
	// Transition rule linearized
	Widget->AddSlot()
		.AutoHeight()
		.Padding( 2.0f )
		[
			SNew(STextBlock)
			.TextStyle( FEditorStyle::Get(), TEXT("Graph.TransitionNode.TooltipName") )
			.Text(TooltipDesc)
		];

	Widget->AddSlot()
		.AutoHeight()
		.Padding( 2.0f )
		[
			SNew(STextBlock)
			.TextStyle( FEditorStyle::Get(), TEXT("Graph.TransitionNode.TooltipRule") )
			.Text(LOCTEXT("AnimGraphNodeTransitionRule_ToolTip", "Transition Rule (in words)"))
		];

	Widget->AddSlot()
		.AutoHeight()
		.Padding( 2.0f )
		[
			SNew(SKismetLinearExpression, CanExecPin)
		];

	Widget->AddSlot()
		.AutoHeight()
		.Padding( 2.0f )
		[
			IDocumentation::Get()->CreateToolTip(FText::FromString("Documentation"), NULL, TransNode->GetDocumentationLink(), TransNode->GetDocumentationExcerptName())
		];
			
	return Widget;
}

TSharedPtr<SToolTip> SGraphNodeAnimTransition::GetComplexTooltip()
{
	return SNew(SToolTip)
		[
			GenerateRichTooltip()
		];
}

void SGraphNodeAnimTransition::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();

	// Reset variables that are going to be exposed, in case we are refreshing an already setup node.
	RightNodeBox.Reset();
	LeftNodeBox.Reset();

	this->ContentScale.Bind( this, &SGraphNode::GetContentScale );
	this->GetOrAddSlot( ENodeZone::Center )
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image( FEditorStyle::GetBrush("Graph.TransitionNode.Body") )
			]
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image( FEditorStyle::GetBrush("Graph.TransitionNode.ColorSpill") )
				.ColorAndOpacity( this, &SGraphNodeAnimTransition::GetTransitionColor )
			]
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image( FEditorStyle::GetBrush("Graph.TransitionNode.Icon") )
			]
			+SOverlay::Slot()
			[
				SNew(SImage)
				.Image( FEditorStyle::GetBrush("Graph.TransitionNode.Gloss") )
			]
		];
}

FText SGraphNodeAnimTransition::GetPreviewCornerText(bool bReverse) const
{
	UAnimStateTransitionNode* TransNode = CastChecked<UAnimStateTransitionNode>(GraphNode);

	UAnimStateNodeBase* PrevState = (bReverse ? TransNode->GetNextState() : TransNode->GetPreviousState());
	UAnimStateNodeBase* NextState = (bReverse ? TransNode->GetPreviousState() : TransNode->GetNextState());

	FText Result = LOCTEXT("BadTransition", "Bad transition (missing source or target)");

	// Show the priority if there is any ambiguity
	if (PrevState != NULL)
	{
		if (NextState != NULL)
		{
			TArray<UAnimStateTransitionNode*> TransitionFromSource;
			PrevState->GetTransitionList(/*out*/ TransitionFromSource);

			bool bMultiplePriorities = false;
			if (TransitionFromSource.Num() > 1)
			{
				// See if the priorities differ
				for (int32 Index = 0; (Index < TransitionFromSource.Num()) && !bMultiplePriorities; ++Index)
				{
					const bool bDifferentPriority = (TransitionFromSource[Index]->PriorityOrder != TransNode->PriorityOrder);
					bMultiplePriorities |= bDifferentPriority;
				}
			}

			if (bMultiplePriorities)
			{
				Result = FText::Format(LOCTEXT("TransitionXToYWithPriority", "{0} to {1} (Priority {2})"), FText::FromString(PrevState->GetStateName()), FText::FromString(NextState->GetStateName()), FText::AsNumber(TransNode->PriorityOrder));
			}
			else
			{
				Result = FText::Format(LOCTEXT("TransitionXToY", "{0} to {1}"), FText::FromString(PrevState->GetStateName()), FText::FromString(NextState->GetStateName()));
			}
		}
	}

	return Result;
}

FLinearColor SGraphNodeAnimTransition::StaticGetTransitionColor(UAnimStateTransitionNode* TransNode, bool bIsHovered)
{
	//@TODO: Make configurable by styling
	const FLinearColor ActiveColor(1.0f, 0.4f, 0.3f, 1.0f);
	const FLinearColor HoverColor(0.724f, 0.256f, 0.0f, 1.0f);
	FLinearColor BaseColor(0.9f, 0.9f, 0.9f, 1.0f);

	// Display various types of debug data
	UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(FBlueprintEditorUtils::FindBlueprintForNodeChecked(TransNode));
	check(AnimBlueprint);
	UAnimInstance* ActiveObject = Cast<UAnimInstance>(AnimBlueprint->GetObjectBeingDebugged());
	UAnimBlueprintGeneratedClass* Class = AnimBlueprint->GetAnimBlueprintGeneratedClass();
	UEdGraph* StateMachineGraph = TransNode->GetGraph();

	if ((ActiveObject != NULL) && (Class != NULL))
	{
		if (FStateMachineDebugData* DebugInfo = Class->GetAnimBlueprintDebugData().StateMachineDebugData.Find(StateMachineGraph))
		{
			if (int32* pTransIndex = DebugInfo->NodeToTransitionIndex.Find(TransNode))
			{
				const int32 TransIndex = *pTransIndex;

				if (Class->AnimNodeProperties.Num())
				{
					UAnimationStateMachineGraph* TypedGraph = CastChecked<UAnimationStateMachineGraph>(StateMachineGraph);

					if (FAnimNode_StateMachine* CurrentInstance = Class->GetPropertyInstance<FAnimNode_StateMachine>(ActiveObject, TypedGraph->OwnerAnimGraphNode))
					{
						if (CurrentInstance->IsTransitionActive(TransIndex))
						{
							// We're active!
							return ActiveColor;
						}
					}
				}
			}
		}
	}

	//@TODO: ANIMATION: Sort out how to display this
	// 			if (TransNode->SharedCrossfadeIdx != INDEX_NONE)
	// 			{
	// 				WireColor.R = (TransNode->SharedCrossfadeIdx & 1 ? 1.0f : 0.15f);
	// 				WireColor.G = (TransNode->SharedCrossfadeIdx & 2 ? 1.0f : 0.15f);
	// 				WireColor.B = (TransNode->SharedCrossfadeIdx & 4 ? 1.0f : 0.15f);
	// 			}


	// If shared transition, show different color
	if (TransNode->bSharedRules)
	{
		BaseColor = TransNode->SharedColor;
	}

	return bIsHovered ? HoverColor : BaseColor;
}

FSlateColor SGraphNodeAnimTransition::GetTransitionColor() const
{
	UAnimStateTransitionNode* TransNode = CastChecked<UAnimStateTransitionNode>(GraphNode);
	return StaticGetTransitionColor(TransNode, IsHovered());
}

FString SGraphNodeAnimTransition::GetCurrentDuration() const
{
	UAnimStateTransitionNode* TransNode = CastChecked<UAnimStateTransitionNode>(GraphNode);

	return FString::Printf(TEXT("%.2f seconds"), TransNode->CrossfadeDuration);
}

void SGraphNodeAnimTransition::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	UAnimStateTransitionNode* TransNode = CastChecked<UAnimStateTransitionNode>(GraphNode);
	if (UEdGraphPin* Pin = TransNode->GetInputPin())
	{
		GetOwnerPanel()->AddPinToHoverSet(Pin);
	}

	SGraphNode::OnMouseEnter(MyGeometry, MouseEvent);
}

void SGraphNodeAnimTransition::OnMouseLeave(const FPointerEvent& MouseEvent)
{	
	UAnimStateTransitionNode* TransNode = CastChecked<UAnimStateTransitionNode>(GraphNode);
	if (UEdGraphPin* Pin = TransNode->GetInputPin())
	{
		GetOwnerPanel()->RemovePinFromHoverSet(Pin);
	}

	SGraphNode::OnMouseLeave(MouseEvent);
}

void SGraphNodeAnimTransition::PositionBetweenTwoNodesWithOffset(const FGeometry& StartGeom, const FGeometry& EndGeom, int32 NodeIndex, int32 MaxNodes) const
{
	// Get a reasonable seed point (halfway between the boxes)
	const FVector2D StartCenter = FGeometryHelper::CenterOf(StartGeom);
	const FVector2D EndCenter = FGeometryHelper::CenterOf(EndGeom);
	const FVector2D SeedPoint = (StartCenter + EndCenter) * 0.5f;

	// Find the (approximate) closest points between the two boxes
	const FVector2D StartAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(StartGeom, SeedPoint);
	const FVector2D EndAnchorPoint = FGeometryHelper::FindClosestPointOnGeom(EndGeom, SeedPoint);

	// Position ourselves halfway along the connecting line between the nodes, elevated away perpendicular to the direction of the line
	const float Height = 30.0f;

	const FVector2D DesiredNodeSize = GetDesiredSize();

	FVector2D DeltaPos(EndAnchorPoint - StartAnchorPoint);

	if (DeltaPos.IsNearlyZero())
	{
		DeltaPos = FVector2D(10.0f, 0.0f);
	}

	const FVector2D Normal = FVector2D(DeltaPos.Y, -DeltaPos.X).GetSafeNormal();

	const FVector2D NewCenter = StartAnchorPoint + (0.5f * DeltaPos) + (Height * Normal);

	FVector2D DeltaNormal = DeltaPos.GetSafeNormal();
	
	// Calculate node offset in the case of multiple transitions between the same two nodes
	// MultiNodeOffset: the offset where 0 is the centre of the transition, -1 is 1 <size of node>
	// towards the PrevStateNode and +1 is 1 <size of node> towards the NextStateNode.

	const float MutliNodeSpace = 0.2f; // Space between multiple transition nodes (in units of <size of node> )
	const float MultiNodeStep = (1.f + MutliNodeSpace); //Step between node centres (Size of node + size of node spacer)

	const float MultiNodeStart = -((MaxNodes - 1) * MultiNodeStep) / 2.f;
	const float MultiNodeOffset = MultiNodeStart + (NodeIndex * MultiNodeStep);

	// Now we need to adjust the new center by the node size, zoom factor and multi node offset
	const FVector2D NewCorner = NewCenter - (0.5f * DesiredNodeSize) + (DeltaNormal * MultiNodeOffset * DesiredNodeSize.Size());

	GraphNode->NodePosX = NewCorner.X;
	GraphNode->NodePosY = NewCorner.Y;
}

/////////////////////////////////////////////////////

#undef LOCTEXT_NAMESPACE

#endif