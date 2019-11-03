// Copyright 2019 yangxiangyun
// All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "ConnectionDrawingPolicy.h"

#include "../NGAGraphPinConnectionFactory.h"


class FNGAGraphConnectionDrawingPolicyCommon
{
public:
	FConnectionDrawingPolicy* Ref_EffectingPolicy;
	const int32 Ref_WireLayerID;
	const FSlateBrush*& Ref_MidpointImage;
	const FSlateBrush*& Ref_BubbleImage;
	const UGraphEditorSettings*& Ref_Settings;
	const float Ref_ZoomFactor;
	const FSlateRect& Ref_ClippingRect;
	FSlateWindowElementList& Ref_DrawElementsList;
	TMap<TSharedRef<SWidget>, FArrangedWidget>*& Ref_PinGeometries;
	const FVector2D& Ref_LocalMousePosition;

	TSharedPtr<FNGAGraphPinConnectionFactoryPayLoadData> MyPayLoadData;
	TArray<FVector2D> DelayDrawPreviewStart;
	TArray<FVector2D> DelayDrawPreviewEnd;
	TArray<UEdGraphPin*> DelayDrawPreviewPins;
	const UEdGraph* MyGraphObject;

	FNGAGraphConnectionDrawingPolicyCommon(
	FConnectionDrawingPolicy* InPolicy,
	const int32 InWireLayerID,
	const FSlateBrush*& MidpointImage,
	const FSlateBrush*& InBubbleImage,
	const UGraphEditorSettings*& InSettings,
	const float ZoomFactor,
	const FSlateRect& InClippingRect,
	FSlateWindowElementList& InDrawElements,
	TMap<TSharedRef<SWidget>, FArrangedWidget>*& InPinGeometries,
	const FVector2D& InLocalMousePosition, //need reference
	TSharedPtr<FNGAGraphPinConnectionFactoryPayLoadData> InPayLoadData,
	const UEdGraph* InGraphObj):
	Ref_EffectingPolicy(InPolicy),
	Ref_WireLayerID(InWireLayerID),
	Ref_MidpointImage(MidpointImage),
	Ref_BubbleImage(InBubbleImage),
	Ref_Settings(InSettings),
	Ref_ZoomFactor(ZoomFactor),
	Ref_ClippingRect(InClippingRect),
	Ref_DrawElementsList(InDrawElements),
	Ref_PinGeometries(InPinGeometries),
	Ref_LocalMousePosition(InLocalMousePosition),
	MyPayLoadData(InPayLoadData),
	MyGraphObject(InGraphObj)
	{}

	void OverrideDrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin);
	void OverrideDraw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes);
	void OverrideDrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params);
	void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params);

	void DelayDrawPreviewConnector();

	void ResetPayloadData()
	{
		if (MyPayLoadData.IsValid())
		{
			MyPayLoadData->OutLazyConnectiblePin.Reset();
			MyPayLoadData->OutInsertableNodePinInfo = InsertableNodePinInfo();
			MyPayLoadData->OutHoveredInputPins.Empty();
			MyPayLoadData->OutHoveredOutputPins.Empty();
		}
	}
};
