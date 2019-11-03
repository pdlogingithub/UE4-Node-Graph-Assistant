// Copyright 2019 yangxiangyun
// All Rights Reserved

#include "NGABlueprintConnectionDrawingPolicy.h"


void FNGABPGraphConnectionDrawingPolicy::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	OverrideDrawPreviewConnector(PinGeometry, StartPoint, EndPoint, Pin);
}


void FNGABPGraphConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes)
{
	ResetPayloadData();
	FKismetConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
	OverrideDraw(InPinGeometries, ArrangedNodes);
}


void FNGABPGraphConnectionDrawingPolicy::DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	OverrideDrawConnection(LayerId, Start, End, Params);
}