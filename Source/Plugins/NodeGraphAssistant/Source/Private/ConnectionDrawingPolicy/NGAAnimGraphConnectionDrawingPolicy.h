// Copyright 2019 yangxiangyun
// All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#ifdef NGA_WITH_ENGINE_CPP
	#include "AnimGraphConnectionDrawingPolicy.cpp"
#else
	#include "../EngineCppFiles/AnimGraphConnectionDrawingPolicy.cpp"
#endif

#include "NGAGraphConnectionDrawingPolicyCommon.h"


class FNGAAnimGraphConnectionDrawingPolicy : public FAnimGraphConnectionDrawingPolicy, public FNGAGraphConnectionDrawingPolicyCommon
{
public:
	FNGAAnimGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj, TSharedPtr<FNGAGraphPinConnectionFactoryPayLoadData> InPayLoadData)
		:FAnimGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj)
		, FNGAGraphConnectionDrawingPolicyCommon(this, InBackLayerID, MidpointImage, BubbleImage, Settings, ZoomFactor, InClippingRect, InDrawElements, PinGeometries, LocalMousePosition, InPayLoadData, InGraphObj)
	{}


	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes) override;
	virtual void DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) override;
};
