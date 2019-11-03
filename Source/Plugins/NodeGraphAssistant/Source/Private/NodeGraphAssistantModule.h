// Copyright 2019 yangxiangyun
// All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "NGAInputProcessor.h"

class NodeGraphAssistantModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	TSharedPtr<NGAInputProcessor> MyInputProcessor;
};

