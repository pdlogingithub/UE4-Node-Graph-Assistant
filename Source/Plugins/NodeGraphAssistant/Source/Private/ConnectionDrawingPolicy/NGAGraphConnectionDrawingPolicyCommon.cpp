// Copyright 2019 yangxiangyun
// All Rights Reserved

#include "NGAGraphConnectionDrawingPolicyCommon.h"

#include "SGraphPanel.h"
#include "../NodeGraphAssistantConfig.h"

#include "Rendering/DrawElements.h"

#include "SlateApplication.h"

//#pragma optimize("", off)


void FNGAGraphConnectionDrawingPolicyCommon::OverrideDrawPreviewConnector(const FGeometry& PinGeometry, const FVector2D& StartPoint, const FVector2D& EndPoint, UEdGraphPin* Pin)
{
	//delay till we have PinGeometries data ready to use.
	DelayDrawPreviewStart.Add(StartPoint);
	DelayDrawPreviewEnd.Add(EndPoint);
	DelayDrawPreviewPins.Add(Pin);
}

void FNGAGraphConnectionDrawingPolicyCommon::OverrideDraw(TMap<TSharedRef<SWidget>, FArrangedWidget>& InPinGeometries, FArrangedChildren& ArrangedNodes)
{
	//reset payload data
	//call parent policy draw func.

	DelayDrawPreviewConnector();
}


void FNGAGraphConnectionDrawingPolicyCommon::DelayDrawPreviewConnector()
{
	if (MyPayLoadData.IsValid())
	{
		if (MyPayLoadData->HoveredNode.IsValid())
		{
			if (MyPayLoadData->DraggingPins.Num() > 0 && DelayDrawPreviewStart.Num() > 0)
			{
				auto startPoint = DelayDrawPreviewStart[0];
				auto endPoint = DelayDrawPreviewEnd[0];
				auto Pin = DelayDrawPreviewPins[0];

				FVector2D cursorPos;
				if (Pin->Direction == EEdGraphPinDirection::EGPD_Input)
				{
					cursorPos = startPoint;
				}
				else if (Pin->Direction == EEdGraphPinDirection::EGPD_Output)
				{
					cursorPos = endPoint;
				}
				FVector2D minPos = cursorPos + FVector2D(500, 500);

				TArray< TSharedRef<SWidget> > NodePinWdgets;
				MyPayLoadData->HoveredNode.Pin()->GetPins(NodePinWdgets);
				for (auto nodePinWidget : NodePinWdgets)
				{
					auto nodePin = StaticCastSharedRef<SGraphPin>(nodePinWidget);
					if (auto arrangedNodePin = Ref_PinGeometries->Find(nodePin))
					{
						FVector2D curPinPos = arrangedNodePin->Geometry.AbsolutePosition;
						if (nodePin->GetDirection() == EEdGraphPinDirection::EGPD_Input)
						{
							curPinPos = curPinPos + FVector2D(arrangedNodePin->Geometry.GetDrawSize().Y / 2, arrangedNodePin->Geometry.GetDrawSize().Y / 2);
						}
						else if (nodePin->GetDirection() == EEdGraphPinDirection::EGPD_Output)
						{
							curPinPos = curPinPos + FVector2D(arrangedNodePin->Geometry.GetDrawSize().X - arrangedNodePin->Geometry.GetDrawSize().Y / 2, arrangedNodePin->Geometry.GetDrawSize().Y / 2);
						}

						if (FMath::Abs(cursorPos.Y - curPinPos.Y) < FMath::Abs(cursorPos.Y - minPos.Y))
						{
							ECanCreateConnectionResponse response = MyPayLoadData->DraggingPins[0]->GetSchema()->CanCreateConnection(MyPayLoadData->DraggingPins[0], nodePin->GetPinObj()).Response;
							if (response != ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW && response != ECanCreateConnectionResponse::CONNECT_RESPONSE_MAKE_WITH_CONVERSION_NODE)
							{
								MyPayLoadData->OutLazyConnectiblePin = nodePin;
								minPos = curPinPos;

								if (nodePin->GetDirection() == EEdGraphPinDirection::EGPD_Input)
								{
									DelayDrawPreviewEnd.Init(minPos, DelayDrawPreviewStart.Num());
								}
								else if (nodePin->GetDirection() == EEdGraphPinDirection::EGPD_Output)
								{
									DelayDrawPreviewStart.Init(minPos, DelayDrawPreviewStart.Num());
								}
							}
						}
					}
				}
			}
		}

		for (int i = 0; i < MyPayLoadData->AutoConnectEndPins.Num(); i++)
		{
			TSharedRef<SGraphPin> startPin = MyPayLoadData->AutoConnectStartPins[i].Pin().ToSharedRef();
			TSharedRef<SGraphPin> endPin = MyPayLoadData->AutoConnectEndPins[i].Pin().ToSharedRef();

			if (Ref_PinGeometries->Find(startPin) && Ref_PinGeometries->Find(endPin))
			{
				FGeometry startGeometry = Ref_PinGeometries->Find(startPin)->Geometry;
				FGeometry endGeometry = Ref_PinGeometries->Find(endPin)->Geometry;
				FVector2D startPos = startGeometry.AbsolutePosition;
				FVector2D endPos = endGeometry.AbsolutePosition;

				if (startPin->GetDirection() == EEdGraphPinDirection::EGPD_Input)
				{
					startPos = startPos + FVector2D(startGeometry.GetDrawSize().Y / 2, startGeometry.GetDrawSize().Y / 2);
					endPos = endPos + FVector2D(endGeometry.GetDrawSize().X - startGeometry.GetDrawSize().Y / 2, endGeometry.GetDrawSize().Y / 2);
					auto tempPos = startPos;
					startPos = endPos;
					endPos = tempPos;
				}
				else if (startPin->GetDirection() == EEdGraphPinDirection::EGPD_Output)
				{
					startPos = startPos + FVector2D(startGeometry.GetDrawSize().X - startGeometry.GetDrawSize().Y / 2, startGeometry.GetDrawSize().Y / 2);
					endPos = endPos + FVector2D(endGeometry.GetDrawSize().Y / 2, endGeometry.GetDrawSize().Y / 2);
				}

				FConnectionParams Params;
				DetermineWiringStyle(startPin->GetPinObj(), nullptr, /*inout*/ Params);
				if (GetDefault<UNodeGraphAssistantConfig>()->OverrideAutoConnectPreviewWireColor)
				{
					Params.WireColor = GetDefault<UNodeGraphAssistantConfig>()->AutoConnectPreviewWireColor;
					Params.WireThickness = 3;
				}
				Ref_EffectingPolicy->DrawSplineWithArrow(startPos, endPos, Params);
			}
		}
		if (MyPayLoadData->OutInsertableNodePinInfo.InputPin && MyPayLoadData->OutInsertableNodePinInfo.OutputPin)
		{
			auto params = MyPayLoadData->OutInsertableNodePinInfo.Params;
			if (GetDefault<UNodeGraphAssistantConfig>()->InsertNodeShowDeletedWireAsRed)
			{
				params.WireColor = FColor::Red;
				OverrideDrawConnection(Ref_WireLayerID, MyPayLoadData->OutInsertableNodePinInfo.Pin1Pos, MyPayLoadData->OutInsertableNodePinInfo.Pin2Pos, params);
			}

			params = MyPayLoadData->OutInsertableNodePinInfo.Params;
			if (GetDefault<UNodeGraphAssistantConfig>()->OverrideAutoConnectPreviewWireColor)
			{
				params.WireColor = GetDefault<UNodeGraphAssistantConfig>()->AutoConnectPreviewWireColor;
				params.WireThickness = 3;
			}
			OverrideDrawConnection(Ref_WireLayerID, MyPayLoadData->OutInsertableNodePinInfo.Pin1Pos, MyPayLoadData->OutInsertableNodePinInfo.InputPinPos, params);
			OverrideDrawConnection(Ref_WireLayerID, MyPayLoadData->OutInsertableNodePinInfo.OutputPinPos, MyPayLoadData->OutInsertableNodePinInfo.Pin2Pos, params);
		}
	}
	for (int i = 0; i < DelayDrawPreviewStart.Num(); i++)
	{
		FConnectionParams Params;
		DetermineWiringStyle(DelayDrawPreviewPins[i], nullptr, /*inout*/ Params);
		Ref_EffectingPolicy->DrawSplineWithArrow(DelayDrawPreviewStart[i], DelayDrawPreviewEnd[i], Params);
	}
}


void FNGAGraphConnectionDrawingPolicyCommon::OverrideDrawConnection(int32 LayerId, const FVector2D& Start, const FVector2D& End, const FConnectionParams& Params)
{
	if (!GetDefault<UNodeGraphAssistantConfig>()->WireStyleStraight)
	{
		const FVector2D& P0 = Start;
		const FVector2D& P1 = End;

		const FVector2D SplineTangent = Ref_EffectingPolicy->ComputeSplineTangent(P0, P1);
		const FVector2D P0Tangent = (Params.StartDirection == EGPD_Output) ? SplineTangent : -SplineTangent;
		const FVector2D P1Tangent = (Params.EndDirection == EGPD_Input) ? SplineTangent : -SplineTangent;

		//NGA begin
		if (MyPayLoadData.IsValid() && MyPayLoadData->InsertNodePinInfos.Num() > 0)
		{
			auto nodeBoundMin = MyPayLoadData->NodeBoundMinRelToCursor + Ref_LocalMousePosition;
			auto nodeBoundMax = MyPayLoadData->NodeBoundMaxRelToCursor + Ref_LocalMousePosition;

			if ((FMath::Min(Start.X, End.X) < nodeBoundMin.X && nodeBoundMax.X < FMath::Max(Start.X, End.X)) || (FMath::Min(Start.Y, End.Y) < nodeBoundMin.Y && nodeBoundMax.Y < FMath::Max(Start.Y, End.Y)))
			{
				FBox2D splineBound(ForceInit);
				splineBound += Start;
				splineBound += (End + FVector2D(1, 1));
				if (FBox2D(nodeBoundMin, nodeBoundMax).Intersect(splineBound))
				{
					float alpha = FMath::Clamp((nodeBoundMin.X - Start.X) / (End.X - Start.X),0.f ,1.f);
					FVector2D intersectInput = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, alpha);
					alpha = FMath::Clamp((nodeBoundMax.X - Start.X) / (End.X - Start.X),0.f ,1.f);
					FVector2D intersectOutput = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, alpha);
					if ((intersectInput.Y < nodeBoundMax.Y && intersectOutput.Y > nodeBoundMin.Y) || (intersectInput.Y > nodeBoundMin.Y && intersectOutput.Y < nodeBoundMax.Y))
					{
						intersectInput.Y = FMath::Clamp(intersectInput.Y, nodeBoundMin.Y,nodeBoundMax.Y);
						intersectOutput.Y = FMath::Clamp(intersectInput.Y, nodeBoundMin.Y, nodeBoundMax.Y);
						auto preInfo = MyPayLoadData->OutInsertableNodePinInfo;
						for (auto inserInfo : MyPayLoadData->InsertNodePinInfos)
						{
							if (inserInfo.InputPin.IsValid() && inserInfo.OutputPin.IsValid())
							{
								float closestDist = FMath::Abs(intersectInput.Y - inserInfo.InputPinPosRelToCursor.Y - Ref_LocalMousePosition.Y) + FMath::Abs(intersectOutput.Y - inserInfo.OutputPinPosRelToCursor.Y - Ref_LocalMousePosition.Y);
								if (closestDist < MyPayLoadData->OutInsertableNodePinInfo.MinPinDist)
								{
									ECanCreateConnectionResponse response = MyGraphObject->GetSchema()->CanCreateConnection(Params.AssociatedPin1, inserInfo.InputPin.Pin()->GetPinObj()).Response;
									if (response != ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
									{
										response = MyGraphObject->GetSchema()->CanCreateConnection(Params.AssociatedPin2, inserInfo.OutputPin.Pin()->GetPinObj()).Response;
										if (response != ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
										{
											auto arrangedInput = Ref_PinGeometries->Find(inserInfo.InputPin.Pin().ToSharedRef());
											auto arrangedOutput = Ref_PinGeometries->Find(inserInfo.OutputPin.Pin().ToSharedRef());
											if (arrangedInput && arrangedOutput)
											{
												auto inputGeo = arrangedInput->Geometry;
												auto outputGeo = arrangedOutput->Geometry;
												FVector2D inputPinPos = inputGeo.AbsolutePosition + FVector2D(inputGeo.GetDrawSize().Y / 2, inputGeo.GetDrawSize().Y / 2);
												FVector2D outputPinPos = outputGeo.AbsolutePosition + FVector2D(outputGeo.GetDrawSize().X - outputGeo.GetDrawSize().Y / 2, outputGeo.GetDrawSize().Y / 2);

												MyPayLoadData->OutInsertableNodePinInfo.MinPinDist = closestDist;
												MyPayLoadData->OutInsertableNodePinInfo.InputPin = inserInfo.InputPin.Pin()->GetPinObj();
												MyPayLoadData->OutInsertableNodePinInfo.OutputPin = inserInfo.OutputPin.Pin()->GetPinObj();
												MyPayLoadData->OutInsertableNodePinInfo.InputPinPos = inputPinPos;
												MyPayLoadData->OutInsertableNodePinInfo.OutputPinPos = outputPinPos;
												MyPayLoadData->OutInsertableNodePinInfo.Params = Params;
												MyPayLoadData->OutInsertableNodePinInfo.Pin1Pos = Start;
												MyPayLoadData->OutInsertableNodePinInfo.Pin2Pos = End;
												MyPayLoadData->OutInsertableNodePinInfo.Pin1Tangent = P0Tangent;
												MyPayLoadData->OutInsertableNodePinInfo.Pin2Tangent = P1Tangent;
											}
										}
									}
								}
							}
						}
						if (MyPayLoadData->OutInsertableNodePinInfo.MinPinDist < preInfo.MinPinDist && !GetDefault<UNodeGraphAssistantConfig>()->InsertNodeShowDeletedWireAsRed)
						{
#if ENGINE_MINOR_VERSION > 16
							FSlateDrawElement::MakeDrawSpaceSpline(
								Ref_DrawElementsList,
								LayerId,
								preInfo.Pin1Pos, preInfo.Pin1Tangent,
								preInfo.Pin2Pos, preInfo.Pin2Tangent,
								preInfo.Params.WireThickness,
								ESlateDrawEffect::None,
								preInfo.Params.WireColor
							);
#else
							FSlateDrawElement::MakeDrawSpaceSpline(
								Ref_DrawElementsList,
								LayerId,
								preInfo.Pin1Pos, preInfo.Pin1Tangent,
								preInfo.Pin2Pos, preInfo.Pin2Tangent,
								Ref_ClippingRect,
								preInfo.Params.WireThickness,
								ESlateDrawEffect::None,
								preInfo.Params.WireColor
							);
#endif
							return;
						}
					}
				}
			}
		}

		if (Ref_Settings->bTreatSplinesLikePins && !FSlateApplication::Get().IsDragDropping())//[wire flickering]fix connection flickering when drag and pan.
		{
			// Distance to consider as an overlap
			float QueryDistanceTriggerThresholdSquared = FMath::Square(Ref_Settings->SplineHoverTolerance + Params.WireThickness * 0.5f);

			//if cursor delta is big,means cursor move fast,so adjust hit radius,
			if (MyPayLoadData.IsValid())
			{
				QueryDistanceTriggerThresholdSquared = FMath::Max(MyPayLoadData->CursorDeltaSquared, QueryDistanceTriggerThresholdSquared);
			}

			// Distance to pass the bounding box cull test (may want to expand this later on if we want to do 'closest pin' actions that don't require an exact hit)
			const float QueryDistanceToBoundingBoxSquared = QueryDistanceTriggerThresholdSquared;

			bool bCloseToSpline = false;
			{
				// The curve will include the endpoints but can extend out of a tight bounds because of the tangents
				// P0Tangent coefficient maximizes to 4/27 at a=1/3, and P1Tangent minimizes to -4/27 at a=2/3.
				const float MaximumTangentContribution = 4.0f / 27.0f;
				FBox2D Bounds(ForceInit);

				Bounds += FVector2D(P0);
				Bounds += FVector2D(P0 + MaximumTangentContribution * P0Tangent);
				Bounds += FVector2D(P1);
				Bounds += FVector2D(P1 - MaximumTangentContribution * P1Tangent);

				bCloseToSpline = Bounds.ComputeSquaredDistanceToPoint(Ref_LocalMousePosition) < QueryDistanceToBoundingBoxSquared;
			}

			if (bCloseToSpline)
			{
				// Find the closest approach to the spline
				FVector2D ClosestPoint(ForceInit);
				float ClosestDistanceSquared = FLT_MAX;

				const int32 NumStepsToTest = 16;
				const float StepInterval = 1.0f / (float)NumStepsToTest;
				FVector2D Point1 = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, 0.0f);
				for (float TestAlpha = 0.0f; TestAlpha < 1.0f; TestAlpha += StepInterval)
				{
					const FVector2D Point2 = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, TestAlpha + StepInterval);

					const FVector2D ClosestPointToSegment = FMath::ClosestPointOnSegment2D(Ref_LocalMousePosition, Point1, Point2);
					const float DistanceSquared = (Ref_LocalMousePosition - ClosestPointToSegment).SizeSquared();

					if (DistanceSquared < ClosestDistanceSquared)
					{
						ClosestDistanceSquared = DistanceSquared;
						ClosestPoint = ClosestPointToSegment;
					}

					Point1 = Point2;
				}

				// Record the overlap
				if (ClosestDistanceSquared < QueryDistanceTriggerThresholdSquared)
				{
					if (ClosestDistanceSquared < Ref_EffectingPolicy->SplineOverlapResult.GetDistanceSquared())
					{
						const float SquaredDistToPin1 = (Params.AssociatedPin1 != nullptr) ? (P0 - ClosestPoint).SizeSquared() : FLT_MAX;
						const float SquaredDistToPin2 = (Params.AssociatedPin2 != nullptr) ? (P1 - ClosestPoint).SizeSquared() : FLT_MAX;

						Ref_EffectingPolicy->SplineOverlapResult = FGraphSplineOverlapResult(Params.AssociatedPin1, Params.AssociatedPin2, ClosestDistanceSquared, SquaredDistToPin1, SquaredDistToPin2);
					}
					//NGA begin
					if (MyPayLoadData.IsValid())
					{
						MyPayLoadData->OutHoveredInputPins.Add(Params.AssociatedPin1);
						MyPayLoadData->OutHoveredOutputPins.Add(Params.AssociatedPin2);
					}
					//NGA end
				}
			}
		}

		// Draw the spline itself
#if ENGINE_MINOR_VERSION > 16
		FSlateDrawElement::MakeDrawSpaceSpline(
			Ref_DrawElementsList,
			LayerId,
			P0, P0Tangent,
			P1, P1Tangent,
			Params.WireThickness,
			ESlateDrawEffect::None,
			Params.WireColor
		);
#else
		FSlateDrawElement::MakeDrawSpaceSpline(
			Ref_DrawElementsList,
			LayerId,
			P0, P0Tangent,
			P1, P1Tangent,
			Ref_ClippingRect,
			Params.WireThickness,
			ESlateDrawEffect::None,
			Params.WireColor
		);
#endif

		if (Params.bDrawBubbles || (Ref_MidpointImage != nullptr))
		{
			// This table maps distance along curve to alpha
			FInterpCurve<float> SplineReparamTable;
			const float SplineLength = Ref_EffectingPolicy->MakeSplineReparamTable(P0, P0Tangent, P1, P1Tangent, SplineReparamTable);

			// Draw bubbles on the spline
			if (Params.bDrawBubbles)
			{
				const float BubbleSpacing = 64.f * Ref_ZoomFactor;
				const float BubbleSpeed = 192.f * Ref_ZoomFactor;
				const FVector2D BubbleSize = Ref_BubbleImage->ImageSize * Ref_ZoomFactor * 0.1f * Params.WireThickness;

				float Time = (FPlatformTime::Seconds() - GStartTime);
				const float BubbleOffset = FMath::Fmod(Time * BubbleSpeed, BubbleSpacing);
				const int32 NumBubbles = FMath::CeilToInt(SplineLength / BubbleSpacing);
				for (int32 i = 0; i < NumBubbles; ++i)
				{
					const float Distance = ((float)i * BubbleSpacing) + BubbleOffset;
					if (Distance < SplineLength)
					{
						const float Alpha = SplineReparamTable.Eval(Distance, 0.f);
						FVector2D BubblePos = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, Alpha);
						BubblePos -= (BubbleSize * 0.5f);

#if ENGINE_MINOR_VERSION > 16
						FSlateDrawElement::MakeBox(
							Ref_DrawElementsList,
							LayerId,
							FPaintGeometry(BubblePos, BubbleSize, Ref_ZoomFactor),
							Ref_BubbleImage,
							ESlateDrawEffect::None,
							Params.WireColor
						);
#else
						FSlateDrawElement::MakeBox(
							Ref_DrawElementsList,
							LayerId,
							FPaintGeometry(BubblePos, BubbleSize, Ref_ZoomFactor),
							Ref_BubbleImage,
							Ref_ClippingRect,
							ESlateDrawEffect::None,
							Params.WireColor
						);
#endif
					}
				}
			}

			// Draw the midpoint image
			if (Ref_MidpointImage != nullptr)
			{
				// Determine the spline position for the midpoint
				const float MidpointAlpha = SplineReparamTable.Eval(SplineLength * 0.5f, 0.f);
				const FVector2D Midpoint = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, MidpointAlpha);

				// Approximate the slope at the midpoint (to orient the midpoint image to the spline)
				const FVector2D MidpointPlusE = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, MidpointAlpha + KINDA_SMALL_NUMBER);
				const FVector2D MidpointMinusE = FMath::CubicInterp(P0, P0Tangent, P1, P1Tangent, MidpointAlpha - KINDA_SMALL_NUMBER);
				const FVector2D SlopeUnnormalized = MidpointPlusE - MidpointMinusE;

				// Draw the arrow
				const FVector2D MidpointDrawPos = Midpoint -Ref_EffectingPolicy->MidpointRadius;
				const float AngleInRadians = SlopeUnnormalized.IsNearlyZero() ? 0.0f : FMath::Atan2(SlopeUnnormalized.Y, SlopeUnnormalized.X);

#if ENGINE_MINOR_VERSION > 16
				FSlateDrawElement::MakeRotatedBox(
					Ref_DrawElementsList,
					LayerId,
					FPaintGeometry(MidpointDrawPos, Ref_MidpointImage->ImageSize * Ref_ZoomFactor, Ref_ZoomFactor),
					Ref_MidpointImage,
					ESlateDrawEffect::None,
					AngleInRadians,
					TOptional<FVector2D>(),
					FSlateDrawElement::RelativeToElement,
					Params.WireColor
				);
#else
				FSlateDrawElement::MakeRotatedBox(
					Ref_DrawElementsList,
					LayerId,
					FPaintGeometry(MidpointDrawPos, Ref_MidpointImage->ImageSize * Ref_ZoomFactor, Ref_ZoomFactor),
					Ref_MidpointImage,
					Ref_ClippingRect,
					ESlateDrawEffect::None,
					AngleInRadians,
					TOptional<FVector2D>(),
					FSlateDrawElement::RelativeToElement,
					Params.WireColor
				);
#endif
			}
		}
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
		float QueryDistanceTriggerThreshold = Ref_Settings->SplineHoverTolerance + Params.WireThickness * 0.5f;
		if (MyPayLoadData.IsValid())
		{
			QueryDistanceTriggerThreshold = FMath::Max(FMath::Sqrt(MyPayLoadData->CursorDeltaSquared), QueryDistanceTriggerThreshold);
		}
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

		//NGA begin
		if (MyPayLoadData.IsValid() && MyPayLoadData->InsertNodePinInfos.Num() > 0)
		{
			auto nodeBoundMin = MyPayLoadData->NodeBoundMinRelToCursor + Ref_LocalMousePosition;
			auto nodeBoundMax = MyPayLoadData->NodeBoundMaxRelToCursor + Ref_LocalMousePosition;

			if ((Start.X < nodeBoundMin.X && nodeBoundMax.X < End.X) || (End.X < nodeBoundMin.X && nodeBoundMax.X < Start.X))
			{
				FBox2D nodeBound = FBox2D(nodeBoundMin, nodeBoundMax);
				if (nodeBound.Intersect(BoundsStart) || nodeBound.Intersect(BoundsEnd) || nodeBound.Intersect(BoundsMiddle))
				{
					auto preInfo = MyPayLoadData->OutInsertableNodePinInfo;
					for (auto inserInfo : MyPayLoadData->InsertNodePinInfos)
					{
						if (inserInfo.InputPin.IsValid() && inserInfo.OutputPin.IsValid())
						{
							float closestDistStart = FMath::Abs(Start.Y - inserInfo.InputPinPosRelToCursor.Y - Ref_LocalMousePosition.Y) + FMath::Abs(Start.Y - inserInfo.OutputPinPosRelToCursor.Y - Ref_LocalMousePosition.Y);
							float closestDistEnd = FMath::Abs(End.Y - inserInfo.InputPinPosRelToCursor.Y - Ref_LocalMousePosition.Y) + FMath::Abs(End.Y - inserInfo.OutputPinPosRelToCursor.Y - Ref_LocalMousePosition.Y);
							float closestDist = FMath::Min(closestDistStart, closestDistEnd);
							if (closestDist < MyPayLoadData->OutInsertableNodePinInfo.MinPinDist)
							{
								ECanCreateConnectionResponse response = MyGraphObject->GetSchema()->CanCreateConnection(Params.AssociatedPin1, inserInfo.InputPin.Pin()->GetPinObj()).Response;
								if (response != ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
								{
									response = MyGraphObject->GetSchema()->CanCreateConnection(Params.AssociatedPin2, inserInfo.OutputPin.Pin()->GetPinObj()).Response;
									if (response != ECanCreateConnectionResponse::CONNECT_RESPONSE_DISALLOW)
									{
										auto arrangedInput = Ref_PinGeometries->Find(inserInfo.InputPin.Pin().ToSharedRef());
										auto arrangedOutput = Ref_PinGeometries->Find(inserInfo.OutputPin.Pin().ToSharedRef());
										if (arrangedInput && arrangedOutput)
										{
											auto inputGeo = arrangedInput->Geometry;
											auto outputGeo = arrangedOutput->Geometry;
											FVector2D inputPinPos = inputGeo.AbsolutePosition + FVector2D(inputGeo.GetDrawSize().Y / 2, inputGeo.GetDrawSize().Y / 2);
											FVector2D outputPinPos = outputGeo.AbsolutePosition + FVector2D(outputGeo.GetDrawSize().X - outputGeo.GetDrawSize().Y / 2, outputGeo.GetDrawSize().Y / 2);

											MyPayLoadData->OutInsertableNodePinInfo.MinPinDist = closestDist;
											MyPayLoadData->OutInsertableNodePinInfo.InputPin = inserInfo.InputPin.Pin()->GetPinObj();
											MyPayLoadData->OutInsertableNodePinInfo.OutputPin = inserInfo.OutputPin.Pin()->GetPinObj();
											MyPayLoadData->OutInsertableNodePinInfo.InputPinPos = inputPinPos;
											MyPayLoadData->OutInsertableNodePinInfo.OutputPinPos = outputPinPos;
											MyPayLoadData->OutInsertableNodePinInfo.Params = Params;
											MyPayLoadData->OutInsertableNodePinInfo.Pin1Pos = Start;
											MyPayLoadData->OutInsertableNodePinInfo.Pin2Pos = End;
										}
									}
								}
							}
						}
					}
					if (MyPayLoadData->OutInsertableNodePinInfo.MinPinDist < preInfo.MinPinDist && !GetDefault<UNodeGraphAssistantConfig>()->InsertNodeShowDeletedWireAsRed)
					{
						float preMiddle = (preInfo.Pin1Pos.X + preInfo.Pin2Pos.X) / 2;
						FVector2D preMiddleStart = FVector2D(preMiddle, preInfo.Pin1Pos.Y);
						FVector2D preMiddleEnd = FVector2D(preMiddle, preInfo.Pin2Pos.Y);
						TArray<FVector2D> prePoints;
						points.Add(preInfo.Pin1Pos);
						points.Add(preMiddleStart);
						points.Add(preMiddleEnd);
						points.Add(preInfo.Pin2Pos);
						preInfo.Params.WireColor.A *= 0.85;

#if ENGINE_MINOR_VERSION > 16
						FSlateDrawElement::MakeLines(
							Ref_DrawElementsList,
							LayerId,
							FPaintGeometry(),
							prePoints,
							ESlateDrawEffect::None,
							preInfo.Params.WireColor,
							false,
							FMath::Min(3.f, preInfo.Params.WireThickness)
						);
#else
						FSlateDrawElement::MakeLines(
							Ref_DrawElementsList,
							LayerId,
							FPaintGeometry(),
							prePoints,
							Ref_ClippingRect,
							ESlateDrawEffect::None,
							preInfo.Params.WireColor,
							false,
							FMath::Min(3.f, preInfo.Params.WireThickness)
						);
#endif
						return;
					}
				}
			}
		}

		ClosestDistanceSquared =FMath::Min3(BoundsStart.ComputeSquaredDistanceToPoint(Ref_LocalMousePosition), BoundsMiddle.ComputeSquaredDistanceToPoint(Ref_LocalMousePosition),BoundsEnd.ComputeSquaredDistanceToPoint(Ref_LocalMousePosition)) ;
		
		if (ClosestDistanceSquared < QueryDistanceTriggerThreshold && ClosestDistanceSquared < Ref_EffectingPolicy->SplineOverlapResult.GetDistanceSquared())
		{
			const float SquaredDistToPin1 = (Params.AssociatedPin1 != nullptr) ? (Start - Ref_LocalMousePosition).SizeSquared() : FLT_MAX;
			const float SquaredDistToPin2 = (Params.AssociatedPin2 != nullptr) ? (End - Ref_LocalMousePosition).SizeSquared() : FLT_MAX;
			Ref_EffectingPolicy->SplineOverlapResult = FGraphSplineOverlapResult(Params.AssociatedPin1, Params.AssociatedPin2, ClosestDistanceSquared, SquaredDistToPin1, SquaredDistToPin2);
		}
		if (ClosestDistanceSquared < QueryDistanceTriggerThreshold)
		{
			if (MyPayLoadData.IsValid())
			{
				MyPayLoadData->OutHoveredInputPins.Add(Params.AssociatedPin1);
				MyPayLoadData->OutHoveredOutputPins.Add(Params.AssociatedPin2);
			}
		}

		FLinearColor color = Params.WireColor;
		color.A *= 0.85;
#if ENGINE_MINOR_VERSION > 16
		FSlateDrawElement::MakeLines(
			Ref_DrawElementsList,
			LayerId,
			FPaintGeometry(),
			points,
			ESlateDrawEffect::None,
			color,
			false,//no aa,its straight line, and aa causes line breaks
			FMath::Min(3.f, Params.WireThickness)
		);
#else
		FSlateDrawElement::MakeLines(
			Ref_DrawElementsList,
			LayerId,
			FPaintGeometry(),
			points,
			Ref_ClippingRect,
			ESlateDrawEffect::None,
			color,
			false,//no aa,its straight line, and aa causes line breaks
			FMath::Min(3.f, Params.WireThickness)
		);
#endif

		if (Params.bDrawBubbles)
		{
			const float line1 = FMath::Abs(middleStart.X - Start.X);
			const float line2 = FMath::Abs(middleEnd.Y - middleStart.Y);
			const float line3 = FMath::Abs(End.X - middleEnd.X);
			const float SplineLength = line1 + line2 + line3;
			const float BubbleSpacing = 64.f * Ref_ZoomFactor;
			const float BubbleSpeed = 192.f * Ref_ZoomFactor;
			const FVector2D BubbleSize = FMath::Min(FVector2D(25, 25), Ref_BubbleImage->ImageSize * Ref_ZoomFactor * 0.1f * Params.WireThickness);

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
#if ENGINE_MINOR_VERSION > 16
					FSlateDrawElement::MakeBox(
						Ref_DrawElementsList,
						LayerId,
						FPaintGeometry(BubblePos, BubbleSize, Ref_ZoomFactor),
						Ref_BubbleImage,
						ESlateDrawEffect::None,
						Params.WireColor
					);
#else
					FSlateDrawElement::MakeBox(
						Ref_DrawElementsList,
						LayerId,
						FPaintGeometry(BubblePos, BubbleSize, Ref_ZoomFactor),
						Ref_BubbleImage,
						Ref_ClippingRect,
						ESlateDrawEffect::None,
						Params.WireColor
					);
#endif
				}
			}
		}
		if (Ref_MidpointImage != nullptr)
		{
#if ENGINE_MINOR_VERSION > 16
			if (FMath::Abs(middleStart.X - Start.X) > float(Ref_MidpointImage->ImageSize.X) * Ref_ZoomFactor)
			{
				FSlateDrawElement::MakeRotatedBox(
					Ref_DrawElementsList,
					LayerId,
					FPaintGeometry((middleStart + Start) / 2 - Ref_EffectingPolicy->MidpointRadius, Ref_MidpointImage->ImageSize * Ref_ZoomFactor, Ref_ZoomFactor),
					Ref_MidpointImage,
					ESlateDrawEffect::None,
					middleStart.X > Start.X ? 0 : 3.14,
					TOptional<FVector2D>(),
					FSlateDrawElement::RelativeToElement,
					Params.WireColor
				);
			}
			if (FMath::Abs(middleEnd.Y - middleStart.Y) > Ref_MidpointImage->ImageSize.Y * Ref_ZoomFactor)
			{
				FSlateDrawElement::MakeRotatedBox(
					Ref_DrawElementsList,
					LayerId,
					FPaintGeometry((middleEnd + middleStart) / 2 - Ref_EffectingPolicy->MidpointRadius, Ref_MidpointImage->ImageSize * Ref_ZoomFactor, Ref_ZoomFactor),
					Ref_MidpointImage,
					ESlateDrawEffect::None,
					middleEnd.Y > middleStart.Y ? 1.57 : 4.71,
					TOptional<FVector2D>(),
					FSlateDrawElement::RelativeToElement,
					Params.WireColor
				);
			}
			if (FMath::Abs(End.X - middleEnd.X) > Ref_MidpointImage->ImageSize.X * Ref_ZoomFactor)
			{
				FSlateDrawElement::MakeRotatedBox(
					Ref_DrawElementsList,
					LayerId,
					FPaintGeometry((End + middleEnd) / 2 - Ref_EffectingPolicy->MidpointRadius, Ref_MidpointImage->ImageSize * Ref_ZoomFactor, Ref_ZoomFactor),
					Ref_MidpointImage,
					ESlateDrawEffect::None,
					End.X > middleEnd.X ? 0 : 3.14,
					TOptional<FVector2D>(),
					FSlateDrawElement::RelativeToElement,
					Params.WireColor
				);
			}
#else
			if (FMath::Abs(middleStart.X - Start.X) > float(Ref_MidpointImage->ImageSize.X) * Ref_ZoomFactor)
			{
				FSlateDrawElement::MakeRotatedBox(
					Ref_DrawElementsList,
					LayerId,
					FPaintGeometry((middleStart + Start) / 2 - Ref_EffectingPolicy->MidpointRadius, Ref_MidpointImage->ImageSize * Ref_ZoomFactor, Ref_ZoomFactor),
					Ref_MidpointImage,
					Ref_ClippingRect,
					ESlateDrawEffect::None,
					middleStart.X > Start.X ? 0 : 3.14,
					TOptional<FVector2D>(),
					FSlateDrawElement::RelativeToElement,
					Params.WireColor
				);
			}
			if (FMath::Abs(middleEnd.Y - middleStart.Y) > Ref_MidpointImage->ImageSize.Y * Ref_ZoomFactor)
			{
				FSlateDrawElement::MakeRotatedBox(
					Ref_DrawElementsList,
					LayerId,
					FPaintGeometry((middleEnd + middleStart) / 2 - Ref_EffectingPolicy->MidpointRadius, Ref_MidpointImage->ImageSize * Ref_ZoomFactor, Ref_ZoomFactor),
					Ref_MidpointImage,
					Ref_ClippingRect,
					ESlateDrawEffect::None,
					middleEnd.Y > middleStart.Y ? 1.57 : 4.71,
					TOptional<FVector2D>(),
					FSlateDrawElement::RelativeToElement,
					Params.WireColor
				);
			}
			if (FMath::Abs(End.X - middleEnd.X) > Ref_MidpointImage->ImageSize.X * Ref_ZoomFactor)
			{
				FSlateDrawElement::MakeRotatedBox(
					Ref_DrawElementsList,
					LayerId,
					FPaintGeometry((End + middleEnd) / 2 - Ref_EffectingPolicy->MidpointRadius, Ref_MidpointImage->ImageSize * Ref_ZoomFactor, Ref_ZoomFactor),
					Ref_MidpointImage,
					Ref_ClippingRect,
					ESlateDrawEffect::None,
					End.X > middleEnd.X ? 0 : 3.14,
					TOptional<FVector2D>(),
					FSlateDrawElement::RelativeToElement,
					Params.WireColor
				);
			}
#endif
		}
	}
}

void FNGAGraphConnectionDrawingPolicyCommon::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params)
{
	Ref_EffectingPolicy->DetermineWiringStyle(OutputPin, InputPin, Params);
	return;
	//most material expression did not implement get type interface,so we can not get the correct color.
	/*if (GetDefault<UNodeGraphAssistantConfig>()->OverrideMaterialGraphPinColor)
	{
		uint32 i = static_cast<const UMaterialGraphSchema*>(OutputPin->GetOwningNode()->GetSchema())->GetMaterialValueType(OutputPin);
		FLinearColor newColor = FColor::White;
		switch (i)
		{
		case MCT_Float:
			newColor = GetDefault<UNodeGraphAssistantConfig>()->Float1PinWireColor;
			newColor.A = Params.WireColor.A;
			Params.WireColor = newColor;
			break;
		case MCT_Float2:
			newColor = GetDefault<UNodeGraphAssistantConfig>()->Float2PinWireColor;
			newColor.A = Params.WireColor.A;
			Params.WireColor = newColor;
			break;
		case MCT_Float3:
			newColor = GetDefault<UNodeGraphAssistantConfig>()->Float3PinWireColor;
			newColor.A = Params.WireColor.A;
			Params.WireColor = newColor;
			break;
		case MCT_Float4:
			newColor = GetDefault<UNodeGraphAssistantConfig>()->Float4PinWireColor;
			newColor.A = Params.WireColor.A;
			Params.WireColor = newColor;
			break;
		case MCT_StaticBool:
			newColor = GetDefault<UNodeGraphAssistantConfig>()->BoolPinWireColor;
			newColor.A = Params.WireColor.A;
			Params.WireColor = newColor;
			break;
		case MCT_Texture2D:
			newColor = GetDefault<UNodeGraphAssistantConfig>()->TexturePinWireColor;
			newColor.A = Params.WireColor.A;
			Params.WireColor = newColor;
			break;
		case MCT_TextureCube:
			newColor = GetDefault<UNodeGraphAssistantConfig>()->TexturePinWireColor;
			newColor.A = Params.WireColor.A;
			Params.WireColor = newColor;
			break;
		case MCT_MaterialAttributes:
			newColor = GetDefault<UNodeGraphAssistantConfig>()->MaterialAttributesPinWireColor;
			newColor.A = Params.WireColor.A;
			Params.WireColor = newColor;
			break;
		default:
			Params.WireColor = newColor;
		}
	}*/
}

//#pragma optimize("", on)