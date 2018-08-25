// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#ifndef NGA_WITH_ENGINE_CPP

#include "AnimGraphConnectionDrawingPolicy.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AnimationGraphSchema.h"
#include "AnimGraphNode_Base.h"

/////////////////////////////////////////////////////
// FAnimGraphConnectionDrawingPolicy

FAnimGraphConnectionDrawingPolicy::FAnimGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FKismetConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj)
{
}

bool FAnimGraphConnectionDrawingPolicy::TreatWireAsExecutionPin(UEdGraphPin* InputPin, UEdGraphPin* OutputPin) const
{
	const UAnimationGraphSchema* Schema = GetDefault<UAnimationGraphSchema>();

	return (InputPin != NULL) && (Schema->IsPosePin(OutputPin->PinType));
}

void FAnimGraphConnectionDrawingPolicy::BuildExecutionRoadmap()
{
	UAnimBlueprint* TargetBP = CastChecked<UAnimBlueprint>(FBlueprintEditorUtils::FindBlueprintForGraphChecked(GraphObj));
	UAnimBlueprintGeneratedClass* AnimBlueprintClass = (UAnimBlueprintGeneratedClass*)(*(TargetBP->GeneratedClass));

	if (TargetBP->GetObjectBeingDebugged() == NULL)
	{
		return;
	}

	TMap<UProperty*, UObject*> PropertySourceMap;
	AnimBlueprintClass->GetDebugData().GenerateReversePropertyMap(/*out*/ PropertySourceMap);

	FAnimBlueprintDebugData& DebugInfo = AnimBlueprintClass->GetAnimBlueprintDebugData();
	for (auto VisitIt = DebugInfo.UpdatedNodesThisFrame.CreateIterator(); VisitIt; ++VisitIt)
	{
		const FAnimBlueprintDebugData::FNodeVisit& VisitRecord = *VisitIt;

		if ((VisitRecord.SourceID >= 0) && (VisitRecord.SourceID < AnimBlueprintClass->AnimNodeProperties.Num()) && (VisitRecord.TargetID >= 0) && (VisitRecord.TargetID < AnimBlueprintClass->AnimNodeProperties.Num()))
		{
			if (UAnimGraphNode_Base* SourceNode = Cast<UAnimGraphNode_Base>(PropertySourceMap.FindRef(AnimBlueprintClass->AnimNodeProperties[VisitRecord.SourceID])))
			{
				if (UAnimGraphNode_Base* TargetNode = Cast<UAnimGraphNode_Base>(PropertySourceMap.FindRef(AnimBlueprintClass->AnimNodeProperties[VisitRecord.TargetID])))
				{
					UEdGraphPin* PoseNet = NULL;

					UAnimationGraphSchema const* AnimSchema = GetDefault<UAnimationGraphSchema>();
					for (int32 PinIndex = 0; PinIndex < TargetNode->Pins.Num(); ++PinIndex)
					{
						UEdGraphPin* Pin = TargetNode->Pins[PinIndex];
						check(Pin);
						if (AnimSchema->IsPosePin(Pin->PinType) && (Pin->Direction == EGPD_Output))
						{
							PoseNet = Pin;
							break;
						}
					}

					if (PoseNet != NULL)
					{
						//@TODO: Extend the rendering code to allow using the recorded blend weight instead of faked exec times
						FExecPairingMap& Predecessors = PredecessorPins.FindOrAdd((UEdGraphNode*)SourceNode);
						FTimePair& Timings = Predecessors.FindOrAdd(PoseNet);
						Timings.PredExecTime = 0.0;
						Timings.ThisExecTime = VisitRecord.Weight;
					}
				}
			}
		}
	}
}

void FAnimGraphConnectionDrawingPolicy::DetermineStyleOfExecWire(float& Thickness, FLinearColor& WireColor, bool& bDrawBubbles, const FTimePair& Times)
{
	// It's a followed link, make it strong and yellowish but fading over time
	const double BlendWeight = Times.ThisExecTime;

	const float HeavyBlendThickness = AttackWireThickness;
	const float LightBlendThickness = SustainWireThickness;

	Thickness = FMath::Lerp<float>(LightBlendThickness, HeavyBlendThickness, BlendWeight);
	WireColor = WireColor * (BlendWeight * 0.5f + 0.5f);//FMath::Lerp<FLinearColor>(SustainColor, AttackColor, BlendWeight);

	bDrawBubbles = true;
}

#endif