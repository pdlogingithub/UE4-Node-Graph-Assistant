// Copyright 2018 yangxiangyun
// All Rights Reserved

#pragma once

#include "CoreMinimal.h"

#ifdef NGA_With_MatDrawPolicy_API
#include "MaterialGraphConnectionDrawingPolicy.cpp"
#else
#include "../_ImportModulAPI/MaterialGraphConnectionDrawingPolicy.cpp"
#endif

#include "NGAGraphPinConnectionFactory.h"



class FNGAMaterialGraphConnectionDrawingPolicy : public FMaterialGraphConnectionDrawingPolicy
{
public:

	FNGAGraphPinConnectionFactoryPayLoadData MyPayLoadData;
	UEdGraph* MyGraphObject;

	TArray<FVector2D> DelayDrawPreviewStart;
	TArray<FVector2D> DelayDrawPreviewEnd;
	TArray<UEdGraphPin*> DelayDrawPreviewPins;

	FNGAMaterialGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj, FNGAGraphPinConnectionFactoryPayLoadData InPayLoadData)
		:FMaterialGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj)
		,MyPayLoadData(InPayLoadData)
		,MyGraphObject(InGraphObj)
	{
	}


	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes) override;
	virtual void DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) override;
	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params) override;

	void DelayDrawPreviewConnector();

private:

};
