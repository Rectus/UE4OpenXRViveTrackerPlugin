using UnrealBuildTool;
using System.IO;

namespace UnrealBuildTool.Rules
{
    public class OpenXRViveTracker: ModuleRules
    {
        public OpenXRViveTracker(ReadOnlyTargetRules Target) 
				: base(Target)
        {
			PublicDependencyModuleNames.AddRange(
			   new string[]
			   {
					"InputDevice"
			   }
		   );

			var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);
            PrivateIncludePaths.AddRange(
                new string[] {
                    //Path.Combine(EngineDir, "Source/ThirdParty/OpenXR/include"),
                    Path.Combine(ModuleDirectory, "include")
                }
                );

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
                    "CoreUObject",
                    "Projects",
                    "Engine",
                    "InputDevice",
                    "InputCore",
					"Slate",
					"HeadMountedDisplay",
					"SlateCore",
					"OpenXRHMD",
					"OpenXRInput"
				}
				);

            //AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenXR");

            if (Target.bBuildEditor == true)
            {
                PrivateDependencyModuleNames.Add("UnrealEd");
            }
        }
    }
}
