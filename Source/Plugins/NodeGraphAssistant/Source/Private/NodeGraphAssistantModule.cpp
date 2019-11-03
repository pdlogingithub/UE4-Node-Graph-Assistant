// Copyright 2019 yangxiangyun
// All Rights Reserved

#include "NodeGraphAssistantModule.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "ISettingsModule.h"
#include "SlateApplication.h"
#include "GraphEditorSettings.h"
#include "NodeGraphAssistantConfig.h"

#include "PropertyEditorModule.h"


#include "Version.h"


IMPLEMENT_MODULE(NodeGraphAssistantModule, NodeGraphAssistant)

void NodeGraphAssistantModule::StartupModule()
{
	if (FSlateApplication::IsInitialized())
	{
		MyInputProcessor = MakeShareable(new NGAInputProcessor());
		bool ret = false;

#if ENGINE_MINOR_VERSION > 16
		ret = FSlateApplication::Get().RegisterInputPreProcessor(MyInputProcessor);
#else
		FSlateApplication::Get().SetInputPreProcessor(true, MyInputProcessor);
		ret = true;
#endif

		if (ret)
		{
			if (GetDefault<UGraphEditorSettings>()->bTreatSplinesLikePins == false)
			{
				const_cast<UGraphEditorSettings*>(GetDefault<UGraphEditorSettings>())->bTreatSplinesLikePins = true;
			}
			if (GetDefault<UGraphEditorSettings>()->SplineHoverTolerance < 15)
			{
				const_cast<UGraphEditorSettings*>(GetDefault<UGraphEditorSettings>())->SplineHoverTolerance = 15;
			}

#if ENGINE_MINOR_VERSION > 16
			if (GetDefault<UGraphEditorSettings>()->PanningMouseButton == EGraphPanningMouseButton::Middle)
			{
				const_cast<UNodeGraphAssistantConfig*>(GetDefault<UNodeGraphAssistantConfig>())->DragCutOffWireMouseButton = ECutOffMouseButton::Left;
			}
#endif

			if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
			{
				SettingsModule->RegisterSettings("Editor", "Plugins", "NodeGraphAssistant",
					NSLOCTEXT("NodeGraphAssistant","SettingsName", "Node Graph Assistant"),
					NSLOCTEXT("NodeGraphAssistant","SettingsDescription", "Settings for plugin Node Graph Assistant"),
					GetMutableDefault<UNodeGraphAssistantConfig>());
			}
		}
	}

	
}


void NodeGraphAssistantModule::ShutdownModule()
{
	if (MyInputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{

#if ENGINE_MINOR_VERSION > 16
		FSlateApplication::Get().UnregisterInputPreProcessor(MyInputProcessor);
#else
		FSlateApplication::Get().SetInputPreProcessor(false, TSharedPtr<class IInputProcessor>());
#endif

		MyInputProcessor.Reset();

		if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
		{
			SettingsModule->UnregisterSettings("Editor", "Plugins", "NodeGraphAssistant");
		}
	}
}