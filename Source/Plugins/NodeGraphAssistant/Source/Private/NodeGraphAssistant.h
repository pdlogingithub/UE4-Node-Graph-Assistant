// Copyright 2018 yangxiangyun
// All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "OverrideSlateInputProcessor.h"

class NodeGraphAssistant : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	TSharedPtr<FOverrideSlateInputProcessor> MyInputProcessor;
};

