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

            PublicIncludePaths.AddRange(
                new string[] {
					// ... add public include paths required here ...
				}
                );

            PrivateIncludePaths.AddRange(
                new string[] {
                    "Runtime/Core/Public",
                    "Runtime/Slate/Public/Framework/Application",
                    "Runtime/SlateCore/Public",
                    "Editor/UnrealEd/Public",
                    "Editor/UnrealEd/Classes",
                    "Editor/GraphEditor/Public",
                    "Editor/EditorStyle/Public",
                    "Editor/AnimGraph/Classes",
                    "Editor/AIGraph/Classes",
                    "Runtime/Projects/Public",
                    "Editor/MaterialEditor/Public",
                    "Editor/Kismet/Public",
                    "Editor/BlueprintGraph/Classes",
                    "Editor/AudioEditor/Classes",
                    "Editor/KismetWidgets/Public",
                }
                );
            string enginePath = Path.GetFullPath(Target.RelativeEnginePath);

            PrivateIncludePaths.Add(enginePath + "Source/Editor/GraphEditor/Private");
            PrivateIncludePaths.Add(enginePath + "Source/Runtime/Launch/Resources");
            PrivateIncludePaths.Add(enginePath + "Source/Runtime/Engine/Classes/EdGraph");
            PrivateIncludePaths.Add(enginePath + "Source/Editor/AnimationBlueprintEditor/Private");
            PrivateIncludePaths.Add(enginePath + "Source/Editor/AudioEditor/Private");

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core","CoreUObject","Slate","SlateCore","UnrealEd","GraphEditor",
                    "InputCore","EditorStyle","Engine","AnimGraph","AIGraph","Projects",
                    "MaterialEditor","BlueprintGraph","AnimationBlueprintEditor","AudioEditor","KismetWidgets",
					// ... add private dependencies that you statically link with here ...
				}
                );

            //some classes is not exported from engine, so when build with release engine it can not find the implementation.
            if (File.Exists(enginePath + "Source/Editor/GraphEditor/Private/DragConnection.cpp")) 
            {
                Definitions.Add("NGA_With_DragConnection_API");
            }
            else
            {
                Log.TraceInformation("DragConnection.cpp not found, is it release build of ue4?");
                Log.TraceInformation("source file added from plugin dir");
            }
            if (File.Exists(enginePath + "Source/Editor/GraphEditor/Private/MaterialGraphConnectionDrawingPolicy.cpp"))
            {
                Definitions.Add("NGA_With_MatDrawPolicy_API");
            }
            else
            {
                Log.TraceInformation("MaterialGraphConnectionDrawingPolicy.cpp not found, is it release build of ue4?");
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
