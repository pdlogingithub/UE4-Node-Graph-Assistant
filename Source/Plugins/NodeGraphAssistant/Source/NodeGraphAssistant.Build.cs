// Copyright 2018 yangxiangyun
// All Rights Reserved

using System.IO;

namespace UnrealBuildTool.Rules
{
    public class NodeGraphAssistant : ModuleRules
    {
        public NodeGraphAssistant(ReadOnlyTargetRules Target) : base(Target)
        {
            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
            string enginePath = Path.GetFullPath(Target.RelativeEnginePath);

            PublicIncludePaths.AddRange(
                new string[] {
					// ... add public include paths required here ...
				}
                );

            PrivateIncludePaths.AddRange(
                new string[] {
                    enginePath+"Source/Runtime/Core/Public",
                    enginePath+"Source/Runtime/Projects/Public",
                    enginePath+"Source/Runtime/Projects/Public/Interfaces",
                    enginePath+"Source/Runtime/Engine/Classes",
                    enginePath+"Source/Runtime/Engine/Public",
                    enginePath+"Source/Runtime/SlateCore/Public/Styling",
                    enginePath+"Source/Runtime/Slate/Public/Framework/Application",
                    enginePath+"Source/Runtime/Slate/Public/Framework",
                    enginePath+"Source/Runtime/SlateCore/Public",
                    enginePath+"Source/Editor/UnrealEd/Public",
                    enginePath+"Source/Editor/UnrealEd/Classes",
                    enginePath+"Source/Editor/GraphEditor/Public",
                    enginePath+"Source/Editor/EditorStyle/Public",
                    enginePath+"Source/Editor/MaterialEditor/Public",
                    enginePath+"Source/Editor/AnimationBlueprintEditor/Public",
                    enginePath+"Source/Editor/Kismet/Public",
                    enginePath+"Source/Editor/BlueprintGraph/Classes",
                    enginePath+"Source/Editor/KismetWidgets/Public",

                    //enginePath+"Source/Editor/PropertyEditor/Public",

                }
                );
            PrivateIncludePaths.Add(enginePath + "Source/Editor/GraphEditor/Private");
            PrivateIncludePaths.Add(enginePath + "Source/Runtime/Launch/Resources");
            PrivateIncludePaths.Add(enginePath + "Source/Runtime/Engine/Classes/EdGraph");
            PrivateIncludePaths.Add(enginePath + "Source/Editor/AnimationBlueprintEditor/Private");
            PrivateIncludePaths.Add(enginePath + "Source/Editor/AudioEditor/Private");
            PrivateIncludePaths.Add(enginePath + "Source/Editor/UnrealEd/Private");

            //PrivateIncludePaths.Add(enginePath + "Plugins/FX/Niagara/Source/NiagaraEditor/Public");

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core","CoreUObject","Slate","SlateCore","UnrealEd","GraphEditor",
                    "InputCore","EditorStyle","Engine","AnimGraph","Projects",
                    "MaterialEditor","BlueprintGraph","AnimationBlueprintEditor","AudioEditor","KismetWidgets",
                    //"PropertyEditor","AppFramework","NiagaraEditor",
					// ... add private dependencies that you statically link with here ...
				}
                );

            //some classes is not exported from engine, so when build with release engine it can not find the implementation.
            if (File.Exists(enginePath + "Source/Editor/GraphEditor/Private/DragConnection.cpp"))
            {
                Definitions.Add("NGA_WITH_ENGINE_CPP");
            }
            else
            {
                Log.TraceInformation("one or more engine cpp file not found, is it release build of ue4?");
                Log.TraceInformation("source file added from plugin dir");
            }

            DynamicallyLoadedModuleNames.AddRange(
                new string[]
                {
					// ... add any modules that your module loads dynamically here ...
				}
                );
        }
    }
}
