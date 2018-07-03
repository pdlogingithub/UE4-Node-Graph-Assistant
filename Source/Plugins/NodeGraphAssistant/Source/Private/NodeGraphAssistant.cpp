// Copyright 2018 yangxiangyun
// All Rights Reserved

#include "NodeGraphAssistant.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "SlateApplication.h"
#include "GraphEditorSettings.h"


IMPLEMENT_MODULE(NodeGraphAssistant, NodeGraphAssistant)

void NodeGraphAssistant::StartupModule()
{
	if (FSlateApplication::IsInitialized())
	{
		MyInputProcessor = MakeShareable(new FOverrideSlateInputProcessor());
		bool ret = FSlateApplication::Get().RegisterInputPreProcessor(MyInputProcessor);

		if (ret)
		{
			if (GetDefault<UGraphEditorSettings>()->bTreatSplinesLikePins == false)
			{
				const_cast<UGraphEditorSettings*>(GetDefault<UGraphEditorSettings>())->bTreatSplinesLikePins = true;
			}
			if (GetDefault<UGraphEditorSettings>()->SplineHoverTolerance != 15)
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
		FSlateApplication::Get().UnregisterInputPreProcessor(MyInputProcessor);
		MyInputProcessor.Reset();
	}
}