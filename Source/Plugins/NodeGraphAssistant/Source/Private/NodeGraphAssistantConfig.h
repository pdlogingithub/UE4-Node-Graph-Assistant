// Copyright 2018 yangxiangyun
// All Rights Reserved

 
#pragma once
 
#include "NodeGraphAssistantConfig.generated.h"

UENUM()
enum class ECutOffMouseButton : uint8
{
	Middle	UMETA(DisplayName = "Middle"),
	Left	UMETA(DisplayName = "Left"),
	None	UMETA(DisplayName = "Disabled")
};

UENUM()
enum class EAutoConnectModifier : uint8
{
	Alt	    UMETA(DisplayName = "Alt"),
	None	UMETA(DisplayName = "None")
};
 

UCLASS(config = EditorSettings)
class UNodeGraphAssistantConfig : public UObject
{
	GENERATED_BODY()
 
public:
	/**  */
	UPROPERTY(EditAnywhere, config, Category = Features)
	bool EnableLazyConnect = true;

	/**  */
	UPROPERTY(EditAnywhere, config, Category = Features)
	bool EanbleCutoffWire = true;

	/** Select linked nodes */
	UPROPERTY(EditAnywhere, config, Category = Features)
	bool EanbleSelectStream = true;

	/** */
	UPROPERTY(EditAnywhere, config, Category = Features)
	bool EnableAutoConnect = true;

	/** When moving mouse with alt and this mouse button down will cut off wires under cursor, not affect alt+left click cut off wire feature. */
	UPROPERTY(config, EditAnywhere, Category = Default)
	ECutOffMouseButton DragCutOffWireMouseButton = ECutOffMouseButton::Middle;

	/** How much distance between two nodes next to each other after rearranging nodes. */
	UPROPERTY(EditAnywhere, config, Category = Default)
	FIntPoint NodesRearrangeSpacing = FIntPoint(20,20);

	/** How much distance between two nodes next to each other after rearranging nodes,for behavior tree graph and EQS graph. */
	UPROPERTY(EditAnywhere, config, Category = Default)
	FIntPoint NodesRearrangeSpacingAIGraph = FIntPoint(20, 100);

	//UPROPERTY(EditAnywhere, config, Category = Default)
	FIntPoint RearrangeNodesSpacingFar = FIntPoint(20, 20);

	/** Offset node a little if this node is connected to a lot wires so wires can be seen more clearly. */
	UPROPERTY(EditAnywhere, config, Category = Default)
	float NodesRearrangeSpacingRelaxFactor = 0.5;

	/** if checked,when wire under mouse is not selected(just been clicked) when add new node, new node wont get inserted into wire. */
	UPROPERTY(EditAnywhere, config, Category = Default)
	bool InsertNodeOnlyIfWireSelected = false;

	/** remove node even if can not fully bypass node's all pins. */
	UPROPERTY(EditAnywhere, config, Category = Default)
	bool BypassNodeAnyway = true;

	/** remove node even if can not fully bypass node's all pins. */
	UPROPERTY(EditAnywhere, config, Category = Default)
	float AutoConnectRadius = 80;

	/** when dragging a node and this button down will enable auto connect */
	UPROPERTY(config, EditAnywhere, Category = Default)
	EAutoConnectModifier AutoConnectModifier = EAutoConnectModifier::None;

	/** show button in toolbar,need to restart editor */
	UPROPERTY(config, EditAnywhere, Category = WireStyle)
	bool WireStyleToolBarButton = true;

	/** */
	UPROPERTY(config/*, EditAnywhere, Category = WireStyle*/)
	bool OverrideMaterialGraphPinColor = false;

	/** */
	UPROPERTY(config/*, EditAnywhere, Category = WireStyle*/)
	FLinearColor Float1PinWireColor = FLinearColor(0.22, 0.5, 0,1);

	/** */
	UPROPERTY(config/*, EditAnywhere, Category = WireStyle*/)
	FLinearColor Float2PinWireColor = FLinearColor(0.2, 0.2, 1, 1);

	/** */
	UPROPERTY(config/*, EditAnywhere, Category = WireStyle*/)
	FLinearColor Float3PinWireColor = FLinearColor(0.8, 0.65, 0, 1);

	/** */
	UPROPERTY(config/*, EditAnywhere, Category = WireStyle*/)
	FLinearColor Float4PinWireColor = FLinearColor(1, 0.25, 0.1, 1);

	/** */
	UPROPERTY(config/*, EditAnywhere, Category = WireStyle*/)
	FLinearColor BoolPinWireColor = FLinearColor(.9, 0.09, 0.09, 1);

	/** */
	UPROPERTY(config/*, EditAnywhere, Category = WireStyle*/)
	FLinearColor TexturePinWireColor = FLinearColor(0.09, 0.52, 0.9, 1);

	/** */
	UPROPERTY(config/*, EditAnywhere, Category = WireStyle*/)
	FLinearColor MaterialAttributesPinWireColor = FLinearColor(1, 1, 1, 1);

	UPROPERTY(config/*, EditAnywhere, Category = WireStyle*/)
	bool WireStyleStraight = false;

	/**should override auto connect preview wire color */
	UPROPERTY(config, EditAnywhere, Category = WireStyle)
	bool OverrideAutoConnectPreviewWireColor = true;

	UPROPERTY(config, EditAnywhere, Category = WireStyle)
	FLinearColor AutoConnectPreviewWireColor = FColor::Orange;
};