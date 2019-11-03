// Copyright 2019 yangxiangyun
// All Rights Reserved

//check list
//overridden function change
//private module api change

#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"
#include "ConnectionDrawingPolicy.h"

struct InsertNodePinInfo
{
	TWeakPtr<SGraphPin> InputPin;
	FVector2D InputPinPosRelToCursor;
	TWeakPtr<SGraphPin> OutputPin;
	FVector2D OutputPinPosRelToCursor;
};

struct InsertableNodePinInfo
{
	float MinPinDist = 100;
	FConnectionParams Params;
	UEdGraphPin* InputPin = nullptr;
	UEdGraphPin* OutputPin = nullptr;
	FVector2D Pin1Pos;
	FVector2D Pin1Tangent;
	FVector2D Pin2Pos;
	FVector2D Pin2Tangent;
	FVector2D InputPinPos;
	FVector2D OutputPinPos;
};

struct FNGAGraphPinConnectionFactoryPayLoadData
{
	TWeakPtr<SGraphPanel> HoveredGraphPanel;

	TWeakPtr<SGraphNode> HoveredNode;
	TArray<UEdGraphPin*> DraggingPins;
	TWeakPtr<SGraphPin> OutLazyConnectiblePin;

	TArray<TWeakPtr<SGraphPin>> AutoConnectStartPins;
	TArray<TWeakPtr<SGraphPin>> AutoConnectEndPins;

	FVector2D NodeBoundMinRelToCursor;
	FVector2D NodeBoundMaxRelToCursor;
	TArray<InsertNodePinInfo> InsertNodePinInfos;
	InsertableNodePinInfo OutInsertableNodePinInfo;

	TArray<UEdGraphPin*> OutHoveredInputPins;
	TArray<UEdGraphPin*> OutHoveredOutputPins;

	float CursorDeltaSquared;
};


struct FNGAGraphPinConnectionFactory : public FGraphPanelPinConnectionFactory
{
public:
	FNGAGraphPinConnectionFactory()
	{
		PayLoadData = MakeShareable(new FNGAGraphPinConnectionFactoryPayLoadData());
	}
	virtual FConnectionDrawingPolicy* CreateConnectionPolicy(const class UEdGraphSchema* Schema, int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const class FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, class UEdGraph* InGraphObj) const override;


	void SetLazyConnectPayloadData(TWeakPtr<SGraphNode> InHoveredNode, TArray<UEdGraphPin*> InDraggingPins);
	void SetAutoConnectPayloadData(TArray<TWeakPtr<SGraphPin>> InAutoConnectStartPins, TArray<TWeakPtr<SGraphPin>> InAutoConnectEndPins);
	void ResetLazyConnectPayloadData();
	void ResetAutoConnectPayloadData();
	void ResetInsertNodePayloadData();

	TWeakPtr<SGraphPin> GetLazyConnectiblePin()
	{
		return PayLoadData->OutLazyConnectiblePin;
	}

	TSharedPtr<FNGAGraphPinConnectionFactoryPayLoadData> PayLoadData;
};
