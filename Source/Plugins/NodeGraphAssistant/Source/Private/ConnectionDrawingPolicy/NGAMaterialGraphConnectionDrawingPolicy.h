// Copyright 2019 yangxiangyun
// All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#ifdef NGA_WITH_ENGINE_CPP
	#include "MaterialGraphConnectionDrawingPolicy.cpp"
#else
	#include "../EngineCppFiles/MaterialGraphConnectionDrawingPolicy.cpp"
#endif

#include "NGAGraphConnectionDrawingPolicyCommon.h"


class FNGAMaterialGraphConnectionDrawingPolicy : public FMaterialGraphConnectionDrawingPolicy,public FNGAGraphConnectionDrawingPolicyCommon
{
public:
	FNGAMaterialGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj, TSharedPtr<FNGAGraphPinConnectionFactoryPayLoadData> InPayLoadData)
		:FMaterialGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj),
		FNGAGraphConnectionDrawingPolicyCommon(this, InBackLayerID, MidpointImage, BubbleImage, Settings, ZoomFactor, InClippingRect, InDrawElements, PinGeometries, LocalMousePosition, InPayLoadData, InGraphObj)
	{
	}


	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes) override;
	virtual void DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) override;
	//virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params) override;
};
