// Copyright 2019 yangxiangyun
// All Rights Reserved

#include "NGAMaterialGraphConnectionDrawingPolicy.h"

#ifdef NGA_WITH_ENGINE_CPP
#include "MaterialGraphConnectionDrawingPolicy.cpp"
#else
#include "../EngineCppFiles/MaterialGraphConnectionDrawingPolicy.cpp"
#endif


void FNGAMaterialGraphConnectionDrawingPolicy::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	OverrideDrawPreviewConnector(PinGeometry, StartPoint, EndPoint, Pin);
}


void FNGAMaterialGraphConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes)
{
	ResetPayloadData();
	FMaterialGraphConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
	OverrideDraw(InPinGeometries, ArrangedNodes);
}


void FNGAMaterialGraphConnectionDrawingPolicy::DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	OverrideDrawConnection(LayerId, Start, End, Params);
}