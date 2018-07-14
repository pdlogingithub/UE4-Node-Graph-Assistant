// Copyright 2018 yangxiangyun
// All Rights Reserved

#include "NodeGraphAssistant.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "SlateApplication.h"
#include "GraphEditorSettings.h"

#include "Version.h"


IMPLEMENT_MODULE(NodeGraphAssistant, NodeGraphAssistant)

void NodeGraphAssistant::StartupModule()
{
	if (FSlateApplication::IsInitialized())
	{
		MyInputProcessor = MakeShareable(new FOverrideSlateInputProcessor());
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
		}
	}
}


void NodeGraphAssistant::ShutdownModule()
{
	if (MyInputProcessor.IsValid() && FSlateApplication::IsInitialized())
	{

#if ENGINE_MINOR_VERSION > 16
		FSlateApplication::Get().UnregisterInputPreProcessor(MyInputProcessor);
#else
		FSlateApplication::Get().SetInputPreProcessor(false, TSharedPtr<class IInputProcessor>());
#endif

		MyInputProcessor.Reset();
	}
}