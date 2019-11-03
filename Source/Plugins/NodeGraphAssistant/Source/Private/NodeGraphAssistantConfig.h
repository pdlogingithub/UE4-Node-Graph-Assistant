// Copyright 2019 yangxiangyun
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
	UNodeGraphAssistantConfig()
	{
		InsertNodeShowDeletedWireAsRed = false;
	}

	/** Keep connection wire alive after making pin connection.*/
	UPROPERTY(EditAnywhere, config, Category = Features)
	bool EnableLeftClickMultiConnect = true;

	/** When dragging a wire over a node,wire automatically align to closest connectible pin*/
	UPROPERTY(EditAnywhere, config, Category = Features)
	bool EnableLazyConnect = true;

	/** Drag mouse to cut off wire along its way. */
	UPROPERTY(EditAnywhere, config, Category = Features)
	bool EnableCutoffWire = true;

	/** Select nodes that are connected to middle mouse double clicked node.*/
	UPROPERTY(EditAnywhere, config, Category = Features)
	bool EnableSelectStream = true;

	/** Right click on wire to insert new node*/
	UPROPERTY(EditAnywhere, config, Category = Features)
	bool EnableCreateNodeOnWire = true;

	/** When dragging a node, auto display surrounding connectible pin,release mouse to make connection*/
	UPROPERTY(EditAnywhere, config, Category = Features)
	bool EnableAutoConnect = true;

	/**  Quickly swing node 3 times to break node off wire.*/
	UPROPERTY(EditAnywhere, config, Category = Features)
	bool EnableShakeNodeOffWire = true;

	/** Drag and insert node on wire*/
	UPROPERTY(EditAnywhere, config, Category = Features)
	bool EnableInsertNodeOnWire = false;

	/** When moving mouse with alt and this mouse button down will cut off wires under cursor, not affect alt+left click cut off wire feature. */
	UPROPERTY(config, EditAnywhere, Category = Settings)
	ECutOffMouseButton DragCutOffWireMouseButton = ECutOffMouseButton::Middle;

	/** How much distance between two nodes next to each other after rearranging nodes. */
	UPROPERTY(EditAnywhere, config, Category = Settings)
	FIntPoint NodesRearrangeSpacing = FIntPoint(20,20);

	/** How much distance between two nodes next to each other after rearranging nodes,for behavior tree graph and EQS graph. */
	UPROPERTY(EditAnywhere, config, Category = Settings)
	FIntPoint NodesRearrangeSpacingAIGraph = FIntPoint(20, 100);

	//UPROPERTY(EditAnywhere, config, Category = Default)
	FIntPoint RearrangeNodesSpacingFar = FIntPoint(20, 20);

	/** Offset node a little if this node is connected to a lot wires so wires can be seen more clearly. */
	UPROPERTY(EditAnywhere, config, Category = Settings)
	float NodesRearrangeSpacingRelaxFactor = 0.5;

	/** if checked,when wire under mouse is not selected(just been clicked) when add new node, new node wont get inserted into wire. */
	UPROPERTY(EditAnywhere, config, Category = Settings)
	bool CreateNodeOnlyOnSelectedWire = false;

	/** remove node even if can not fully bypass node's all pins. */
	UPROPERTY(EditAnywhere, config, Category = Settings)
	bool BypassNodeAnyway = true;

	/** if you swing a node 3 times in this time period,will break off this node from wire.*/
	UPROPERTY(EditAnywhere, config, Category = Settings)
	float ShakeNodeOffWireTimeWindow = 0.3;

	/** remove node even if can not fully bypass node's all pins. */
	UPROPERTY(EditAnywhere, config, Category = Settings)
	float AutoConnectRadius = 80;

	/** when dragging a node and this button down will enable auto connect */
	UPROPERTY(config, EditAnywhere, Category = Settings)
	EAutoConnectModifier AutoConnectModifier = EAutoConnectModifier::None;

	/** copy nodes to clipboard after successful bypass. */
	UPROPERTY(EditAnywhere, config, Category = Settings)
	bool BypassAndCopyNodes = false;

	/** show button in toolbar,need to restart editor */
	UPROPERTY(config, EditAnywhere, Category = Settings)
	bool ToolBarButton = true;

	/**should override auto connect preview wire color */
	UPROPERTY(config, EditAnywhere, Category = Settings)
	bool OverrideAutoConnectPreviewWireColor = true;

	UPROPERTY(config, EditAnywhere, Category = Settings)
	FLinearColor AutoConnectPreviewWireColor = FColor::Orange;

	/** When dragging node can be inserted into hovered wire,mark hovered wire as red */
	UPROPERTY(config, EditAnywhere, Category = Settings)
	bool InsertNodeShowDeletedWireAsRed = false;

	/** */
	UPROPERTY(EditAnywhere, config, Category = Other)
		bool HideToolTipWhenDraggingNode = true;


	/** */
	UPROPERTY(config/*, EditAnywhere, Category = WireStyle*/)
		bool OverrideMaterialGraphPinColor = false;

	/** */
	UPROPERTY(config/*, EditAnywhere, Category = WireStyle*/)
		FLinearColor Float1PinWireColor = FLinearColor(0.22, 0.5, 0, 1);

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
};