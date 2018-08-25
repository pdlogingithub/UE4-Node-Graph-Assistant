// Copyright 2018 yangxiangyun
// All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "SoundCueGraph/SoundCueGraphSchema.h"

#ifdef NGA_WITH_ENGINE_CPP
#include "SoundCueGraphConnectionDrawingPolicy.cpp"
#else
#include "../_ImportPrivateEngineAPI/SoundCueGraphConnectionDrawingPolicy.cpp"
#endif

#include "NGAGraphPinConnectionFactory.h"



class FNGASoundCueGraphConnectionDrawingPolicy : public FSoundCueGraphConnectionDrawingPolicy
{
public:
	TSharedPtr<FNGAGraphPinConnectionFactoryPayLoadData> MyPayLoadData;

	TArray<FVector2D> DelayDrawPreviewStart;
	TArray<FVector2D> DelayDrawPreviewEnd;
	TArray<UEdGraphPin*> DelayDrawPreviewPins;

	UEdGraph* MyGraphObject;

	FNGASoundCueGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj, TSharedPtr<FNGAGraphPinConnectionFactoryPayLoadData> InPayLoadData)
		:FSoundCueGraphConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj)
		, MyPayLoadData(InPayLoadData)
		, MyGraphObject(InGraphObj)
	{
	}


	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes) override;
	virtual void DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) override;

	void DelayDrawPreviewConnector();
};
