// Copyright 2019 yangxiangyun
// All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "BlueprintConnectionDrawingPolicy.h"

#include "NGAGraphConnectionDrawingPolicyCommon.h"


class FNGABPGraphConnectionDrawingPolicy : public FKismetConnectionDrawingPolicy, public FNGAGraphConnectionDrawingPolicyCommon
{
public:
	FNGABPGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj, TSharedPtr<FNGAGraphPinConnectionFactoryPayLoadData> InPayLoadData)
		:FKismetConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj)
		, FNGAGraphConnectionDrawingPolicyCommon(this, InBackLayerID, MidpointImage, BubbleImage, Settings, ZoomFactor, InClippingRect, InDrawElements, PinGeometries, LocalMousePosition, InPayLoadData, InGraphObj)
	{}

	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes) override;
	virtual void DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) override;
};
