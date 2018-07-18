// Copyright 2018 yangxiangyun
// All Rights Reserved

#include "NGAGraphPinConnectionFactory.h"

#include "NGAMaterialGraphConnectionDrawingPolicy.h"
#include "NGABlueprintConnectionDrawingPolicy.h"
#include "NGASoundCueConnectionDrawingPolicy.h"
#include "NGAAnimGraphConnectionDrawingPolicy.h"

#include "MaterialGraph/MaterialGraphSchema.h"
#include "EdGraphSchema_K2.h"
#include "AnimationStateMachineSchema.h"

#ifdef NGA_With_MatDrawPolicy_API
#include "StateMachineConnectionDrawingPolicy.cpp"
#include "AnimationStateNodes/SGraphNodeAnimTransition.cpp"
#else
#include "../_ImportModulAPI/StateMachineConnectionDrawingPolicy.cpp"
#include "../_ImportModulAPI/SGraphNodeAnimTransition.cpp"
#endif



//engine will use last factory in factory array when creation connection draw policy
//this will override previous factory even if the last one can not actual create correct factory
//we need to make sure its always the last factory,and it need to include all function of any other factory
FConnectionDrawingPolicy* FNGAGraphPinConnectionFactory::CreateConnectionPolicy(const class UEdGraphSchema* Schema, int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const class FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const
{
	if (Schema->IsA(UMaterialGraphSchema::StaticClass()))
	{
		return new FNGAMaterialGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj, PayLoadData);
	}
	else if (Schema->IsA(UAnimationGraphSchema::StaticClass()))
	{
		return new FNGAAnimGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj,PayLoadData);
	}
	else if (Schema->IsA(UEdGraphSchema_K2::StaticClass()))
	{
		return new FNGABPGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj, PayLoadData);
	}
	else if (Schema->IsA(UAnimationStateMachineSchema::StaticClass()))
	{
		return new FStateMachineConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj);
	}
	else if (Schema->IsA(USoundCueGraphSchema::StaticClass()))
	{
		return new FNGASoundCueGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj,PayLoadData);
	}

	return nullptr;
}


void FNGAGraphPinConnectionFactory::SetPayloadData(FNGAGraphPinConnectionFactoryPayLoadData InData)
{
	PayLoadData = InData;
}

void FNGAGraphPinConnectionFactory::ResetPayloadData()
{
	PayLoadData = FNGAGraphPinConnectionFactoryPayLoadData();
}