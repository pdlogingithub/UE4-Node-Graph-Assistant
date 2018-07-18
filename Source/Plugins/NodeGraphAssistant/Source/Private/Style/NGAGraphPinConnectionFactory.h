// Copyright 2018 yangxiangyun
// All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

struct FNGAGraphPinConnectionFactoryPayLoadData
{
	FNGAGraphPinConnectionFactoryPayLoadData() { GraphPanel = nullptr; };
	FPointerEvent MouseEvent;
	SGraphPanel* GraphPanel;
	TArray<UEdGraphPin*> DraggingPins;
	TSet<TSharedRef<SWidget>> NodePins;
	FGeometry NodeGeometry;
	bool HasMouseUpAfterShiftDrag = false;

	TSet<TSharedRef<SWidget>> autoConnectStartGraphPins;
	TSet<TSharedRef<SWidget>> autoConnectEndGraphPins;
};

struct FNGAGraphPinConnectionFactory : public FGraphPanelPinConnectionFactory
{
public:

	virtual FConnectionDrawingPolicy* CreateConnectionPolicy(const class UEdGraphSchema* Schema, int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const class FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const override;

	void SetPayloadData(FNGAGraphPinConnectionFactoryPayLoadData InData);
	void ResetPayloadData();


	FNGAGraphPinConnectionFactoryPayLoadData PayLoadData;
};