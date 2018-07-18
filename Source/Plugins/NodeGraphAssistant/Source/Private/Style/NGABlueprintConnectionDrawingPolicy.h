// Copyright 2018 yangxiangyun
// All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "BlueprintConnectionDrawingPolicy.h"
#include "EdGraphSchema_K2.h"

#include "NGAGraphPinConnectionFactory.h"



class FNGABPGraphConnectionDrawingPolicy : public FKismetConnectionDrawingPolicy
{
public:
	const UEdGraphSchema_K2* MySchema;
	FNGAGraphPinConnectionFactoryPayLoadData MyPayLoadData;

	TArray<FVector2D> DelayDrawPreviewStart;
	TArray<FVector2D> DelayDrawPreviewEnd;
	TArray<UEdGraphPin*> DelayDrawPreviewPins;

	FNGABPGraphConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj, FNGAGraphPinConnectionFactoryPayLoadData InPayLoadData)
		:FKismetConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj)
		, MyPayLoadData(InPayLoadData)
		, MySchema(static_cast<const UEdGraphSchema_K2*>(InGraphObj->GetSchema()))
	{
	}


	virtual void DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin) override;
	virtual void Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes) override;
	virtual void DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params) override;

	void DelayDrawPreviewConnector();

private:

};
