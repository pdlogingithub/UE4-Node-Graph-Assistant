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
					// ... add other private include paths required here ...
				}
				);

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					// ... add other public dependencies that you statically link with here ...
				}
				);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
                    "Core","CoreUObject","Slate","SlateCore","UnrealEd","GraphEditor","InputCore","EditorStyle",
					// ... add private dependencies that you statically link with here ...
				}
				);

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);


            //include one header file that is inside private folder.
            string engine_path = Path.GetFullPath(BuildConfiguration.RelativeEnginePath);
            string srcrt_path1 = engine_path + "Source/Editor/GraphEditor/Private";
            PrivateIncludePaths.Add(srcrt_path1);
        }
	}
}
