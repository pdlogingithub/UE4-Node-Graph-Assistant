// Copyright 2019 yangxiangyun
// All Rights Reserved

#include "NGAGraphPinConnectionFactory.h"

#include "ConnectionDrawingPolicy/NGAMaterialGraphConnectionDrawingPolicy.h"
#include "ConnectionDrawingPolicy/NGABlueprintConnectionDrawingPolicy.h"
#include "ConnectionDrawingPolicy/NGASoundCueConnectionDrawingPolicy.h"
#include "ConnectionDrawingPolicy/NGAAnimGraphConnectionDrawingPolicy.h"

#include "MaterialGraph/MaterialGraphSchema.h"
#include "EdGraphSchema_K2.h"
#include "AnimationStateMachineSchema.h"

#ifdef NGA_WITH_ENGINE_CPP
#include "StateMachineConnectionDrawingPolicy.cpp"
#include "AnimationStateNodes/SGraphNodeAnimTransition.cpp"
#else
#include "EngineCppFiles/StateMachineConnectionDrawingPolicy.cpp"
#include "EngineCppFiles/SGraphNodeAnimTransition.cpp"
#endif

#include "Version.h"



//before 419 engine will use last factory in factory array when creation connection draw policy
//this will override previous factory even if the last one can not actual create correct factory
//we need to make sure its always the last factory,and it need to include all function of any other factory
FConnectionDrawingPolicy* FNGAGraphPinConnectionFactory::CreateConnectionPolicy(const class UEdGraphSchema* Schema, int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const class FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	TSharedPtr<FNGAGraphPinConnectionFactoryPayLoadData> payLoadData;
	if (PayLoadData->HoveredGraphPanel.IsValid() && PayLoadData->HoveredGraphPanel.Pin()->GetGraphObj() == InGraphObj)
	{
		payLoadData = PayLoadData;
	}

	if (Schema->IsA(UMaterialGraphSchema::StaticClass()))
	{
		return new FNGAMaterialGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj, payLoadData);
	}
	else if (Schema->IsA(UAnimationGraphSchema::StaticClass()))
	{
		return new FNGAAnimGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj, payLoadData);
	}
	else if (Schema->IsA(UEdGraphSchema_K2::StaticClass()))
	{
		return new FNGABPGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj, payLoadData);
	}
#if ENGINE_MINOR_VERSION < 19
	else if (Schema->IsA(USoundCueGraphSchema::StaticClass()))
	{
		return new FNGASoundCueGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj, payLoadData);
	}
	else if (Schema->IsA(UAnimationStateMachineSchema::StaticClass()))
	{
		return new FStateMachineConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj);
	}
#endif

	return nullptr;
}


void FNGAGraphPinConnectionFactory::SetLazyConnectPayloadData(TWeakPtr<SGraphNode> InHoveredNode, TArray<UEdGraphPin*> InDraggingPins)
{
	PayLoadData->HoveredNode = InHoveredNode;
	PayLoadData->DraggingPins = InDraggingPins;
}


void FNGAGraphPinConnectionFactory::SetAutoConnectPayloadData(TArray<TWeakPtr<SGraphPin>> InAutoConnectStartPins, TArray<TWeakPtr<SGraphPin>> InAutoConnectEndPins)
{
	PayLoadData->AutoConnectStartPins = InAutoConnectStartPins;
	PayLoadData->AutoConnectEndPins = InAutoConnectEndPins;
}


void FNGAGraphPinConnectionFactory::ResetLazyConnectPayloadData()
{
	PayLoadData->HoveredNode.Reset();
	PayLoadData->DraggingPins.Empty();
}


void FNGAGraphPinConnectionFactory::ResetAutoConnectPayloadData()
{
	PayLoadData->AutoConnectStartPins.Empty();
	PayLoadData->AutoConnectEndPins.Empty();
}


void FNGAGraphPinConnectionFactory::ResetInsertNodePayloadData()
{
	PayLoadData->InsertNodePinInfos.Empty();
}