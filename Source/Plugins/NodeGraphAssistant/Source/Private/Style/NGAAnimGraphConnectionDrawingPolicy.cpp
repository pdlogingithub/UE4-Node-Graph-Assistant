// Copyright 2018 yangxiangyun
// All Rights Reserved

#include "NGAAnimGraphConnectionDrawingPolicy.h"

#include "SGraphPanel.h"
#include "../NodeGraphAssistantConfig.h"

#include "Rendering/DrawElements.h"


void FNGAAnimGraphConnectionDrawingPolicy::DrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	//delay till we have PinGeometries data ready to use.
	DelayDrawPreviewStart.Add(StartPoint);
	DelayDrawPreviewEnd.Add(EndPoint);
	DelayDrawPreviewPins.Add(Pin);
}


void FNGAAnimGraphConnectionDrawingPolicy::Draw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes)
{
	FAnimGraphConnectionDrawingPolicy::Draw(InPinGeometries, ArrangedNodes);
	DelayDrawPreviewConnector();
}


//move a lot node logic here,because here we have most accurate pin position that I can not calculated in input processor
//todo:build better communication between draw policy and input processor.
void FNGAAnimGraphConnectionDrawingPolicy::DelayDrawPreviewConnector()
{
	for (int i = 0; i < DelayDrawPreviewStart.Num(); i++)
	{
		auto startPoint = DelayDrawPreviewStart[i];
		auto endPoint = DelayDrawPreviewEnd[i];
		auto Pin = DelayDrawPreviewPins[i];

		//make sure its the same graph that draw this panel.
		if (MyPayLoadData.GraphPanel  && MyPayLoadData.GraphPanel->GetGraphObj() == GraphObj)
		{
			FVector2D cusorPos;
			if (Pin->Direction == EEdGraphPinDirection::EGPD_Input)
			{
				cusorPos = startPoint;
			}
			else if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
			{
				cusorPos = endPoint;
			}
			FVector2D minPos = cusorPos + FVector2D(500, 500);

			TArray<UEdGraphPin*> connectablePins;
			UEdGraphPin* closestPin = nullptr;

			for (auto aPin : MyPayLoadData.NodePins)
			{
				auto aGraphpin = StaticCastSharedRef<SGraphPin>(aPin);
				if (!PinGeometries->Find(aGraphpin))
				{
					continue;
				}
				if (*PinGeometries->Find(aGraphpin) == FArrangedWidget::NullWidget)
				{
					continue;
				}

				ECanCreateConnectionResponse response = MyPayLoadData.DraggingPins[0]->GetSchema()->CanCreateConnection(MyPayLoadData.DraggingPins[0], aGraphpin->GetPinObj()).Response;
				if (response != ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW && response != ECanCreateConnectionResponse::CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE)
				{
					FVector2D curPinPos = PinGeometries->Find(aGraphpin)->Geometry.AbsolutePosition;
					if (aGraphpin->GetDirection() == EEdGraphPinDirection::EGPD_Input)
					{
						curPinPos = curPinPos + FVector2D(3, PinGeometries->Find(aGraphpin)->Geometry.GetDrawSize().Y / 2);
					}
					else if (aGraphpin->GetDirection() == EEdGraphPinDirection::EGPD_Output)
					{
						curPinPos = curPinPos + FVector2D(PinGeometries->Find(aGraphpin)->Geometry.GetDrawSize().X - 3, PinGeometries->Find(aGraphpin)->Geometry.GetDrawSize().Y / 2);
					}

					if (FMath::Abs(cusorPos.Y - curPinPos.Y) < FMath::Abs(cusorPos.Y - minPos.Y))
					{
						closestPin = aGraphpin->GetPinObj();
						if (aGraphpin->GetDirection() == EEdGraphPinDirection::EGPD_Input)
						{
							minPos = curPinPos;
							endPoint = minPos;
						}
						else if (aGraphpin->GetDirection() == EEdGraphPinDirection::EGPD_Output)
						{
							minPos = curPinPos;
							startPoint = minPos;
						}
					}
				}
			}
			if (MyPayLoadData.MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && closestPin)
			{
				for (auto draggingPin : MyPayLoadData.DraggingPins)
				{
					MySchema->TryCreateConnection(draggingPin, closestPin);
				}
			}

			if (MyPayLoadData.MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && MyPayLoadData.MouseEvent.IsShiftDown() && closestPin && MyPayLoadData.HasMouseUpAfterShiftDrag)
			{
				for (auto draggingPin : MyPayLoadData.DraggingPins)
				{
					MySchema->TryCreateConnection(draggingPin, closestPin);
				}
			}
		}


		FConnectionParams Params;
		DetermineWiringStyle(Pin, nullptr, /*inout*/ Params);

		DrawSplineWithArrow(startPoint, endPoint, Params);
	}

	if (MyPayLoadData.GraphPanel  && MyPayLoadData.GraphPanel->GetGraphObj() == GraphObj)
	{
		for (int i = 0; i < MyPayLoadData.autoConnectEndGraphPins.Num(); i++)
		{
			TSharedRef<SGraphPin> startPin = StaticCastSharedRef<SGraphPin>(MyPayLoadData.autoConnectStartGraphPins.Array()[i]);
			TSharedRef<SGraphPin> endPin = StaticCastSharedRef<SGraphPin>(MyPayLoadData.autoConnectEndGraphPins.Array()[i]);

			if (!PinGeometries->Find(startPin) || !PinGeometries->Find(endPin))
			{
				continue;
			}
			FVector2D startPos = PinGeometries->Find(startPin)->Geometry.AbsolutePosition;
			FVector2D endPos = PinGeometries->Find(endPin)->Geometry.AbsolutePosition;

			if (startPin->GetDirection() == EEdGraphPinDirection::EGPD_Input)
			{
				startPos = startPos + FVector2D(3, PinGeometries->Find(startPin)->Geometry.GetDrawSize().Y / 2);
				endPos = endPos + FVector2D(PinGeometries->Find(endPin)->Geometry.GetDrawSize().X - 3, PinGeometries->Find(endPin)->Geometry.GetDrawSize().Y / 2);
				auto tempPos = startPos;
				startPos = endPos;
				endPos = tempPos;
			}
			else if (startPin->GetDirection() == EEdGraphPinDirection::EGPD_Output)
			{
				startPos = startPos + FVector2D(PinGeometries->Find(startPin)->Geometry.GetDrawSize().X - 3, PinGeometries->Find(startPin)->Geometry.GetDrawSize().Y / 2);
				endPos = endPos + FVector2D(3, PinGeometries->Find(endPin)->Geometry.GetDrawSize().Y / 2);
			}
			FConnectionParams Params;
			DetermineWiringStyle(startPin->GetPinObj(), nullptr, /*inout*/ Params);

			if (GetDefault<UNodeGraphAssistantConfig>()->OverrideAutoConnectPreviewWireColor)
			{
				Params.WireColor = GetDefault<UNodeGraphAssistantConfig>()->AutoConnectPreviewWireColor;
				Params.WireThickness = 3;
			}

			DrawSplineWithArrow(startPos, endPos, Params);
		}
	}
}


void FNGAAnimGraphConnectionDrawingPolicy::DrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->WireStyleStraight)
	{
		FConnectionDrawingPolicy::DrawConnection(LayerId, Start, End, Params);
	}
	else
	{
		float middle = (Start.X + End.X) / 2;
		FVector2D middleStart = FVector2D(middle, Start.Y);
		FVector2D middleEnd = FVector2D(middle, End.Y);
		TArray<FVector2D> points;
		points.Add(Start);
		points.Add(middleStart);
		points.Add(middleEnd);
		points.Add(End);

		float ClosestDistanceSquared = FLT_MAX;
		float QueryDistanceTriggerThreshold = Settings->SplineHoverTolerance + Params.WireThickness * 0.5f;
		float QueryDistanceTriggerThresholdSquared = FMath::Square(QueryDistanceTriggerThreshold);

		FBox2D BoundsStart(ForceInit);
		BoundsStart += FVector2D(Start.X, Start.Y - QueryDistanceTriggerThreshold);
		BoundsStart += FVector2D(middleStart.X, middleStart.Y + QueryDistanceTriggerThreshold);
		FBox2D BoundsMiddle(ForceInit);
		BoundsMiddle += FVector2D(middleStart.X - QueryDistanceTriggerThreshold, middleStart.Y);
		BoundsMiddle += FVector2D(middleEnd.X + QueryDistanceTriggerThreshold, middleEnd.Y);
		FBox2D BoundsEnd(ForceInit);
		BoundsEnd += FVector2D(middleEnd.X, middleEnd.Y - QueryDistanceTriggerThreshold);
		BoundsEnd += FVector2D(End.X, End.Y + QueryDistanceTriggerThreshold);
		ClosestDistanceSquared = FMath::Min3(BoundsStart.ComputeSquaredDistanceToPoint(LocalMousePosition), BoundsMiddle.ComputeSquaredDistanceToPoint(LocalMousePosition), BoundsEnd.ComputeSquaredDistanceToPoint(LocalMousePosition));

		if (ClosestDistanceSquared < QueryDistanceTriggerThreshold && ClosestDistanceSquared < SplineOverlapResult.GetDistanceSquared())
		{
			const float SquaredDistToPin1 = (Params.AssociatedPin1 != nullptr) ? (Start - LocalMousePosition).SizeSquared() : FLT_MAX;
			const float SquaredDistToPin2 = (Params.AssociatedPin2 != nullptr) ? (End - LocalMousePosition).SizeSquared() : FLT_MAX;
			SplineOverlapResult = FGraphSplineOverlapResult(Params.AssociatedPin1, Params.AssociatedPin2, ClosestDistanceSquared, SquaredDistToPin1, SquaredDistToPin2);
		}

		FLinearColor newColor = Params.WireColor;
		newColor.A = FMath::Min(Params.WireColor.A,0.84f);
		FSlateDrawElement::MakeLines(
			DrawElementsList,
			LayerId,
			FPaintGeometry(),
			points,
			ESlateDrawEffect::None,
			newColor,
			false,//no aa,its straight line, and aa causes line breaks
			FMath::Min(2.2f, Params.WireThickness)
		);


		if (Params.bDrawBubbles)
		{
			const float line1 = FMath::Abs(middleStart.X - Start.X);
			const float line2 = FMath::Abs(middleEnd.Y - middleStart.Y);
			const float line3 = FMath::Abs(End.X - middleEnd.X);
			const float SplineLength = line1 + line2 + line3;
			const float BubbleSpacing = 64.f * ZoomFactor;
			const float BubbleSpeed = 192.f * ZoomFactor;
			const FVector2D BubbleSize = FMath::Min(FVector2D(25, 25), BubbleImage->ImageSize * ZoomFactor * 0.1f * Params.WireThickness);

			float Time = (FPlatformTime::Seconds() - GStartTime);
			const float BubbleOffset = FMath::Fmod(Time * BubbleSpeed, BubbleSpacing);
			const int32 NumBubbles = FMath::CeilToInt(SplineLength / BubbleSpacing);
			for (int32 i = 0; i < NumBubbles; ++i)
			{
				const float Distance = ((float)i * BubbleSpacing) + BubbleOffset;
				if (Distance < SplineLength)
				{
					FVector2D BubblePos = FMath::Lerp(Start, middleStart, Distance / line1);
					if (Distance > line1)
					{
						BubblePos = FMath::Lerp(middleStart, middleEnd, (Distance - line1) / line2);
					}
					if (Distance > line1 + line2)
					{
						BubblePos = FMath::Lerp(middleEnd, End, (Distance - line1 - line2) / line3);
					}
					BubblePos -= (BubbleSize * 0.5f);

					FSlateDrawElement::MakeBox(
						DrawElementsList,
						LayerId,
						FPaintGeometry(BubblePos, BubbleSize, ZoomFactor),
						BubbleImage,
						ESlateDrawEffect::None,
						Params.WireColor
					);
				}
			}
		}
		if (MidpointImage != nullptr)
		{
			if (FMath::Abs(middleStart.X - Start.X) > float(MidpointImage->ImageSize.X) * ZoomFactor)
			{
				FSlateDrawElement::MakeRotatedBox(
					DrawElementsList,
					LayerId,
					FPaintGeometry((middleStart + Start) / 2 - MidpointRadius, MidpointImage->ImageSize * ZoomFactor, ZoomFactor),
					MidpointImage,
					ESlateDrawEffect::None,
					middleStart.X > Start.X ? 0 : 3.14,
					TOptional<FVector2D>(),
					FSlateDrawElement::RelativeToElement,
					Params.WireColor
				);
			}
			if (FMath::Abs(middleEnd.Y - middleStart.Y) > MidpointImage->ImageSize.Y * ZoomFactor)
			{
				FSlateDrawElement::MakeRotatedBox(
					DrawElementsList,
					LayerId,
					FPaintGeometry((middleEnd + middleStart) / 2 - MidpointRadius, MidpointImage->ImageSize * ZoomFactor, ZoomFactor),
					MidpointImage,
					ESlateDrawEffect::None,
					middleEnd.Y > middleStart.Y ? 1.57 : 4.71,
					TOptional<FVector2D>(),
					FSlateDrawElement::RelativeToElement,
					Params.WireColor
				);
			}
			if (FMath::Abs(End.X - middleEnd.X) > MidpointImage->ImageSize.X * ZoomFactor)
			{
				FSlateDrawElement::MakeRotatedBox(
					DrawElementsList,
					LayerId,
					FPaintGeometry((End + middleEnd) / 2 - MidpointRadius, MidpointImage->ImageSize * ZoomFactor, ZoomFactor),
					MidpointImage,
					ESlateDrawEffect::None,
					End.X > middleEnd.X ? 0 : 3.14,
					TOptional<FVector2D>(),
					FSlateDrawElement::RelativeToElement,
					Params.WireColor
				);
			}
		}
	}
}