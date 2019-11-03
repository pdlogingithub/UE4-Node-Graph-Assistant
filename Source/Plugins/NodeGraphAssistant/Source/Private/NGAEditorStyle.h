// Copyright 2019 yangxiangyun
// All Rights Reserved

#pragma once

#include "SlateStyle.h"
#include "SlateStyleRegistry.h"
#include "IPluginManager.h"


class FNGAEditorStyle
	: public FSlateStyleSet
{
public:
	FNGAEditorStyle()
		: FSlateStyleSet("NGAEditorStyle")
	{
		const FVector2D Icon16x16(16.0f, 16.0f);
		const FVector2D Icon40x40(40.0f, 40.0f);
		SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("NodeGraphAssistant"))->GetBaseDir()/ TEXT("Resources"));

		Set("NodeGraphAssistant.CycleWireDrawStyle", new FSlateImageBrush(RootToContentDir(TEXT("nga_icon_cycle_wire_style_40x.png")), Icon40x40));
		Set("NodeGraphAssistant.CycleWireDrawStyle.Small", new FSlateImageBrush(RootToContentDir(TEXT("nga_icon_cycle_wire_style_16x.png")), Icon16x16));
		Set("NodeGraphAssistant.ToggleAutoConnect", new FSlateImageBrush(RootToContentDir(TEXT("nga_icon_auto_connect_40x.png")), Icon40x40));
		Set("NodeGraphAssistant.ToggleAutoConnect.Small", new FSlateImageBrush(RootToContentDir(TEXT("nga_icon_auto_connect_16x.png")), Icon16x16));
		Set("NodeGraphAssistant.ToggleInsertNode", new FSlateImageBrush(RootToContentDir(TEXT("nga_icon_insert_node_40x.png")), Icon40x40));
		Set("NodeGraphAssistant.ToggleInsertNode.Small", new FSlateImageBrush(RootToContentDir(TEXT("nga_icon_insert_node_16x.png")), Icon16x16));

		Set("NodeGraphAssistant.ToggleMaterialGraphWireColor", new FSlateImageBrush(RootToContentDir(TEXT("nga_icon_wire_color.png")), Icon40x40));
		
		FSlateStyleRegistry::RegisterSlateStyle(*this);
	}

	static FNGAEditorStyle& Get()
	{
		static FNGAEditorStyle Inst;
		return Inst;
	}
	
	~FNGAEditorStyle()
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*this);
	}
};
