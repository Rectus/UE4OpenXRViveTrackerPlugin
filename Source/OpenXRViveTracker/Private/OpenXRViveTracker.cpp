
// Contains snippets from the Unreal Engine 4 source code.
// Copyright Epic Games, Inc. All Rights Reserved.

#include "OpenXRViveTracker.h"
#include "IOpenXRViveTrackerModule.h"

#include "IOpenXRHMDPlugin.h"
//#include "GameFramework/InputSettings.h"


#define LOCTEXT_NAMESPACE "OpenXRViveTracker"

#define BASE_ROLE_PATH TEXT("/user/vive_tracker_htcx/role/")

//const FKey ViveTracker_System_Click("ViveTracker_System_Click");
//const FKey ViveTracker_Menu_Click("ViveTracker_Menu_Click");
//const FKey ViveTracker_Trigger_Click("ViveTracker_Trigger_Click");
//const FKey ViveTracker_Squeeze_Click("ViveTracker_Squeeze_Click");
//const FKey ViveTracker_Trigger_Axis("ViveTracker_Trigger_Axis");
//const FKey ViveTracker_Trackpad_X("ViveTracker_Trackpad_X");
//const FKey ViveTracker_Trackpad_Y("ViveTracker_Trackpad_Y");
//const FKey ViveTracker_Trackpad_Click("ViveTracker_Trackpad_Click");
//const FKey ViveTracker_Trackpad_Touch("ViveTracker_Trackpad_Touch");


namespace ViveTrackerSourceNames
{
	static const FName Tracker_Camera("Tracker_Camera");
	static const FName Tracker_Chest("Tracker_Chest");
	static const FName Tracker_Elbow_Left("Tracker_Elbow_Left");
	static const FName Tracker_Elbow_Right("Tracker_Elbow_Right");
	static const FName Tracker_Foot_Left("Tracker_Foot_Left");
	static const FName Tracker_Foot_Right("Tracker_Foot_Right");
	static const FName Tracker_Knee_Left("Tracker_Knee_Left");
	static const FName Tracker_Knee_Right("Tracker_Knee_Right");
	static const FName Tracker_Shoulder_Left("Tracker_Shoulder_Left");
	static const FName Tracker_Shoulder_Right("Tracker_Shoulder_Right");
	static const FName Tracker_Keyboard("Tracker_Keyboard");
	static const FName Tracker_Waist("Tracker_Waist");
	static const FName Tracker_Handheld("Tracker_Handheld");

	// These don't seem to be supported by OpenXR.
	/*static const FName Tracker_Handheld_RawPose_Left("Tracker_Handheld_RawPose_Left");
	static const FName Tracker_Handheld_RawPose_Right("Tracker_Handheld_RawPose_Right");
	static const FName Tracker_Handheld_Back_Left("Tracker_Handheld_Back_Left");
	static const FName Tracker_Handheld_Back_Right("Tracker_Handheld_Back_Right");
	static const FName Tracker_Handheld_Front_Left("Tracker_Handheld_Front_Left");
	static const FName Tracker_Handheld_Front_Right("Tracker_Handheld_Front_Right");
	static const FName Tracker_Handheld_FrontRolled_Left("Tracker_Handheld_FrontRolled_Left");
	static const FName Tracker_Handheld_FrontRolled_Right("Tracker_Handheld_FrontRolled_Right");
	static const FName Tracker_Handheld_PistolGrip_Left("Tracker_Handheld_PistolGrip_Left");
	static const FName Tracker_Handheld_PistolGrip_Right("Tracker_Handheld_PistolGrip_Right");*/
}

static const FName ViveTrackerGripAction("Vive Tracker Grip");
static const FName ViveTrackerVibrationAction("Vive Tracker Vibration");


FORCEINLINE XrPath GetPath(XrInstance Instance, const char* PathString)
{
	XrPath Path = XR_NULL_PATH;
	XrResult Result = xrStringToPath(Instance, PathString, &Path);
	check(XR_SUCCEEDED(Result));
	return Path;
}

FORCEINLINE XrPath GetPath(XrInstance Instance, const FString& PathString)
{
	return GetPath(Instance, (ANSICHAR*)StringCast<ANSICHAR>(*PathString).Get());
}

FORCEINLINE void AddPathRole(XrInstance Instance, TMap<XrPath, FName>* PathRoles, const FString& PathString, const FName Role)
{
	PathRoles->Add(GetPath(Instance, BASE_ROLE_PATH + PathString), Role);
}

FORCEINLINE FString PathToString(XrInstance Instance, const XrPath Path)
{
	uint32_t nCount;
	char PathStr[XR_MAX_PATH_LENGTH];
	XR_ENSURE(xrPathToString(Instance, Path, sizeof(PathStr), &nCount, PathStr));
	return FString(ANSI_TO_TCHAR(PathStr));
}

FORCEINLINE void FilterActionName(const char* InActionName, char* OutActionName)
{
	size_t i;
	for (i = 0; i < XR_MAX_ACTION_NAME_SIZE - 1 && InActionName[i] != '\0'; i++)
	{
		unsigned char c = InActionName[i];
		OutActionName[i] = (c == ' ') ? '-' : isalnum(c) ? tolower(c) : '_';
	}
	OutActionName[i] = '\0';
}

class FOpenXRViveTrackerModule :
	public IOpenXRViveTrackerModule,
	public IOpenXRExtensionPlugin
{
public:
	FOpenXRViveTrackerModule()
		: ViveTrackerInstance(nullptr)
	{}

	virtual void StartupModule() override
	{
		IOpenXRViveTrackerModule::StartupModule();
		RegisterOpenXRExtensionModularFeature();
		CreateInputDevice(TSharedRef<FGenericApplicationMessageHandler>(new FGenericApplicationMessageHandler()));
	}

	virtual void ShutdownModule() override
	{
		IOpenXRViveTrackerModule::ShutdownModule();
	}

	virtual TSharedPtr<class IInputDevice> CreateInputDevice(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) override
	{
		if (!ViveTrackerInstance.IsValid())
		{
			ViveTrackerInstance = TSharedPtr<FOpenXRViveTrackerPlugin>(new FOpenXRViveTrackerPlugin(InMessageHandler));
			return ViveTrackerInstance;
		}
		else
		{
			ViveTrackerInstance.Get()->SetMessageHandler(InMessageHandler);
			return ViveTrackerInstance;
		}
		return nullptr;
	}

	virtual TSharedPtr<IInputDevice> GetInputDevice() override
	{
		if (!ViveTrackerInstance.IsValid())
		{
			CreateInputDevice(FSlateApplication::Get().GetPlatformApplication()->GetMessageHandler());
		}
		return ViveTrackerInstance;
	}

private:
	TSharedPtr<FOpenXRViveTrackerPlugin> ViveTrackerInstance;
};

IMPLEMENT_MODULE(FOpenXRViveTrackerModule, FOpenXRViveTrackerPlugin);


///////////////////////////////////////////////////////////////////////////////
// FViveTracker
///////////////////////////////////////////////////////////////////////////////

FViveTracker::FViveTracker(XrPath InUserPath, XrPath InPersistentPath, const char* InName)
	: PersistentPath(InPersistentPath)
	, RolePath(InUserPath)
	, GripAction(XR_NULL_HANDLE)
	, VibrationAction(XR_NULL_HANDLE)
	, GripDeviceId(-1)
	, GripActionSpace(XR_NULL_HANDLE)
{
	bIsActive = (PersistentPath != XR_NULL_PATH);
}

void FViveTracker::AddActionDevices(FOpenXRHMD* HMD)
{
	if (HMD && GripDeviceId == -1)
	{
		GripDeviceId = HMD->AddActionDevice(GripAction, RolePath);
	}
}


///////////////////////////////////////////////////////////////////////////////
// FOpenXRViveTrackerPlugin
///////////////////////////////////////////////////////////////////////////////

FOpenXRViveTrackerPlugin::FOpenXRViveTrackerPlugin(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
	: MessageHandler(InMessageHandler)
{
	IModularFeatures::Get().RegisterModularFeature(IMotionController::GetModularFeatureName(), static_cast<IMotionController*>(this));
	IModularFeatures::Get().RegisterModularFeature(IOpenXRExtensionPlugin::GetModularFeatureName(), static_cast<IOpenXRExtensionPlugin*>(this));

	if (!IOpenXRHMDPlugin::Get().IsAvailable())
	{
		UE_LOG(LogOpenXRViveTracker, Error, TEXT("Error - OpenXRHMDPlugin isn't available"));
	}


	//ActionKeys.Add(ViveTracker_System_Click, TEXT("/input/system/click"));
	//ActionKeys.Add(ViveTracker_Menu_Click, TEXT("/input/menu/click"));
	//ActionKeys.Add(ViveTracker_Trigger_Click, TEXT("/input/trigger/click"));
	//ActionKeys.Add(ViveTracker_Trackpad_Click, TEXT("/input/trackpad/click"));
	//ActionKeys.Add(ViveTracker_Trackpad_Touch, TEXT("/input/trackpad/click"));

	//AxisKeys.Add(ViveTracker_Trigger_Axis, TEXT("/input/trigger/value"));
	//AxisKeys.Add(ViveTracker_Trackpad_X, TEXT("/input/trackpad/x"));
	//AxisKeys.Add(ViveTracker_Trackpad_Y, TEXT("/input/trackpad/y"));


	//EKeys::AddMenuCategoryDisplayInfo("ViveTracker",
	//	LOCTEXT("ViveTrackerSubCategory", "Vive Tracker"), TEXT("GraphEditor.PadEvent_16x"));

	//EKeys::AddKey(FKeyDetails(ViveTracker_System_Click,
	//	LOCTEXT("ViveTracker_System_Click", "ViveTracker System Press"),
	//	FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "ViveTracker"));

	//EKeys::AddKey(FKeyDetails(ViveTracker_Menu_Click,
	//	LOCTEXT("ViveTracker_Menu_Click", "ViveTracker Menu Press"),
	//	FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "ViveTracker"));

	//EKeys::AddKey(FKeyDetails(ViveTracker_Trigger_Click,
	//	LOCTEXT("ViveTracker_Trigger_Click", "ViveTracker Trigger Press"),
	//	FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "ViveTracker"));

	//EKeys::AddKey(FKeyDetails(ViveTracker_Trigger_Axis,
	//	LOCTEXT("ViveTracker_Trigger_Axis", "ViveTracker Trigger Axis"),
	//	FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "ViveTracker"));

	//EKeys::AddKey(FKeyDetails(ViveTracker_Trackpad_X,
	//	LOCTEXT("ViveTracker_Trackpad_X", "ViveTracker Trackpad X"),
	//	FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "ViveTracker"));

	//EKeys::AddKey(FKeyDetails(ViveTracker_Trackpad_Y,
	//	LOCTEXT("ViveTracker_Trackpad_Y", "ViveTracker Trackpad Y"),
	//	FKeyDetails::GamepadKey | FKeyDetails::Axis1D | FKeyDetails::NotBlueprintBindableKey, "ViveTracker"));

	//EKeys::AddKey(FKeyDetails(ViveTracker_Trackpad_Click,
	//	LOCTEXT("ViveTracker_Trackpad_Click", "ViveTracker Trackpad Press"),
	//	FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "ViveTracker"));

	//EKeys::AddKey(FKeyDetails(ViveTracker_Trackpad_Touch,
	//	LOCTEXT("ViveTracker_Trackpad_Touch", "ViveTracker Trackpad Touch"),
	//	FKeyDetails::GamepadKey | FKeyDetails::NotBlueprintBindableKey, "ViveTracker"));


}

FOpenXRViveTrackerPlugin::~FOpenXRViveTrackerPlugin()
{
	IModularFeatures::Get().UnregisterModularFeature(IMotionController::GetModularFeatureName(), static_cast<IMotionController*>(this));
	IModularFeatures::Get().UnregisterModularFeature(IOpenXRExtensionPlugin::GetModularFeatureName(), static_cast<IOpenXRExtensionPlugin*>(this));
}

bool FOpenXRViveTrackerPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	OutExtensions.Add("XR_HTCX_vive_tracker_interaction");
	return true;
}

const void* FOpenXRViveTrackerPlugin::OnGetSystem(XrInstance InInstance, const void* InNext)
{
	XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrEnumerateViveTrackerPathsHTCX", (PFN_xrVoidFunction*)&xrEnumerateViveTrackerPathsHTCX));


	return InNext;
}

void FOpenXRViveTrackerPlugin::PostCreateInstance(XrInstance InInstance)
{
	AddPathRole(InInstance, &PathRoles, TEXT("camera"), ViveTrackerSourceNames::Tracker_Camera);
	AddPathRole(InInstance, &PathRoles, TEXT("chest"), ViveTrackerSourceNames::Tracker_Chest);
	AddPathRole(InInstance, &PathRoles, TEXT("left_elbow"), ViveTrackerSourceNames::Tracker_Elbow_Left);
	AddPathRole(InInstance, &PathRoles, TEXT("right_elbow"), ViveTrackerSourceNames::Tracker_Elbow_Right);
	AddPathRole(InInstance, &PathRoles, TEXT("left_foot"), ViveTrackerSourceNames::Tracker_Foot_Left);
	AddPathRole(InInstance, &PathRoles, TEXT("right_foot"), ViveTrackerSourceNames::Tracker_Foot_Right);
	AddPathRole(InInstance, &PathRoles, TEXT("left_knee"), ViveTrackerSourceNames::Tracker_Knee_Left);
	AddPathRole(InInstance, &PathRoles, TEXT("right_knee"), ViveTrackerSourceNames::Tracker_Knee_Right);
	AddPathRole(InInstance, &PathRoles, TEXT("left_shoulder"), ViveTrackerSourceNames::Tracker_Shoulder_Left);
	AddPathRole(InInstance, &PathRoles, TEXT("right_shoulder"), ViveTrackerSourceNames::Tracker_Shoulder_Right);
	AddPathRole(InInstance, &PathRoles, TEXT("keyboard"), ViveTrackerSourceNames::Tracker_Keyboard);
	AddPathRole(InInstance, &PathRoles, TEXT("waist"), ViveTrackerSourceNames::Tracker_Waist);
	AddPathRole(InInstance, &PathRoles, TEXT("handheld_object"), ViveTrackerSourceNames::Tracker_Handheld);
}

void FOpenXRViveTrackerPlugin::OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader)
{
	FOpenXRHMD* OpenXRHMD = GetXRSystem();

	switch (InHeader->type)
	{
		case XR_TYPE_EVENT_DATA_VIVE_TRACKER_CONNECTED_HTCX:
		{
			const XrEventDataViveTrackerConnectedHTCX* TrackerConnected = reinterpret_cast<const XrEventDataViveTrackerConnectedHTCX*>(InHeader);

			if (PathRoles.Contains(TrackerConnected->paths->rolePath))
			{
				FViveTracker* Tracker = Trackers.Find(*PathRoles.Find(TrackerConnected->paths->rolePath));
				Tracker->PersistentPath = TrackerConnected->paths->persistentPath;
				Tracker->bIsActive = true;
			}

			FString RolePath = PathToString(OpenXRHMD->GetInstance(), TrackerConnected->paths->rolePath);
			FString PersistentPath = PathToString(OpenXRHMD->GetInstance(), TrackerConnected->paths->persistentPath);
			UE_LOG(LogOpenXRViveTracker, Log, TEXT("Tracker connected: %s, role: %s"), *PersistentPath, *RolePath);

			break;
		}

		case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
		{
			for (TPair<XrPath, FName> Pair : PathRoles)
			{
				XrInteractionProfileState ProfileState;
				ProfileState.type = XR_TYPE_INTERACTION_PROFILE_STATE;
				XrResult Result = xrGetCurrentInteractionProfile(InSession, Pair.Key, &ProfileState);

				if (Result == XR_ERROR_PATH_UNSUPPORTED)
				{
					FString Path = PathToString(OpenXRHMD->GetInstance(), Pair.Key);
					UE_LOG(LogOpenXRViveTracker, Log, TEXT("Path unsupported: %s"), *Path);

				}
				else if (XR_SUCCEEDED(Result) && ProfileState.interactionProfile != InteractionProfile && Trackers.Contains(Pair.Value))
				{
					FViveTracker* Tracker = Trackers.Find(Pair.Value);

					if (Tracker->PersistentPath != XR_NULL_PATH)
					{
						FString PersistentPath = PathToString(OpenXRHMD->GetInstance(), Tracker->PersistentPath);
						UE_LOG(LogOpenXRViveTracker, Log, TEXT("Tracker disconnected: %s"), *PersistentPath);
					}
					Tracker->PersistentPath = XR_NULL_PATH;
					Tracker->bIsActive = false;
				}
			}
			break;
		}
	}
}

void FOpenXRViveTrackerPlugin::AddActions(XrInstance InInstance, TFunction<XrAction(XrActionType InActionType, const FName& InName, const TArray<XrPath>& InSubactionPaths)> AddAction)
{
	FOpenXRHMD* OpenXRHMD = GetXRSystem();

	Trackers.Reset();

	TArray<XrActionSuggestedBinding> Bindings;

	/*TArray<XrPath> SubactionPaths;

	for (TPair<XrPath, FName>& Pair : PathRoles)
	{
		SubactionPaths.Add(Pair.Key);
	}

	auto InputSettings = GetMutableDefault<UInputSettings>();
	if (InputSettings != nullptr)
	{
		TArray<FName> ActionNames;
		InputSettings->GetActionNames(ActionNames);
		for (const FName& ActionName : ActionNames)
		{
			{
				TArray<FInputActionKeyMapping> Mappings;
				InputSettings->GetActionMappingByName(ActionName, Mappings);

				bool bHasBindings = false;

				XrAction ActionHandle = XR_NULL_HANDLE;

				for (auto Mapping : Mappings)
				{
					if (ActionKeys.Contains(Mapping.Key))
					{
						if (!bHasBindings)
						{
							bHasBindings = true;
							ActionHandle = AddAction(XR_ACTION_TYPE_BOOLEAN_INPUT, ActionName, SubactionPaths);
						}

						const FString InputPath = ActionKeys.Find(Mapping.Key)->ToString();
						for (TPair<XrPath, FName>& Pair : PathRoles)
						{
							FString Role = PathToString(InInstance, Pair.Key);
							Bindings.Add(XrActionSuggestedBinding{ ActionHandle, GetPath(InInstance, Role + InputPath) });
						}
					}
				}
			}

			{
				TArray<FInputAxisKeyMapping> Mappings;
				InputSettings->GetAxisMappingByName(ActionName, Mappings);

				bool bHasBindings = false;

				XrAction ActionHandle = XR_NULL_HANDLE;

				for (auto Mapping : Mappings)
				{
					if (ActionKeys.Contains(Mapping.Key))
					{
						if (!bHasBindings)
						{
							bHasBindings = true;
							ActionHandle = AddAction(XR_ACTION_TYPE_FLOAT_INPUT, ActionName, SubactionPaths);
						}

						const FString InputPath = ActionKeys.Find(Mapping.Key)->ToString();
						for (TPair<XrPath, FName>& Pair : PathRoles)
						{
							FString Role = PathToString(InInstance, Pair.Key);
							Bindings.Add(XrActionSuggestedBinding{ ActionHandle, GetPath(InInstance, Role + InputPath) });
						}
					}
				}
			}	
		}

		InputSettings->ForceRebuildKeymaps();
	}*/


	for (TPair<XrPath, FName>& Pair : PathRoles)
	{
		FString Name = Pair.Value.ToString();
		FViveTracker Tracker = FViveTracker(Pair.Key, XR_NULL_PATH, TCHAR_TO_ANSI(*Name));

		TArray<XrPath> EmptySubactionPaths;

		Tracker.GripAction = AddAction(XR_ACTION_TYPE_POSE_INPUT, FName(TEXT("Vive Tracker Grip ") + Name), EmptySubactionPaths);
		Tracker.VibrationAction = AddAction(XR_ACTION_TYPE_VIBRATION_OUTPUT, FName("Vive Tracker Vibration " + Name), EmptySubactionPaths);

		FString Role = PathToString(InInstance, Tracker.RolePath);


		Bindings.Add(XrActionSuggestedBinding{ Tracker.GripAction, GetPath(InInstance, Role + "/input/grip/pose") });
		Bindings.Add(XrActionSuggestedBinding{ Tracker.VibrationAction, GetPath(InInstance, Role + "/output/haptic") });

		Trackers.Add(Pair.Value, Tracker);
	}


	check(XR_SUCCEEDED(xrStringToPath(InInstance, "/interaction_profiles/htc/vive_tracker_htcx", &InteractionProfile)));


	XrInteractionProfileSuggestedBinding ProfileBinding;
	ProfileBinding.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
	ProfileBinding.next = nullptr;
	ProfileBinding.interactionProfile = InteractionProfile;
	ProfileBinding.countSuggestedBindings = Bindings.Num();
	ProfileBinding.suggestedBindings = Bindings.GetData();
	XR_ENSURE(xrSuggestInteractionProfileBindings(InInstance, &ProfileBinding));


	// Add a callback add the action devices as soon as possible (after the OpenXR Input module has finished loading).
	// The OpenXR plugin assumes the regular motion controllers occupy the first device slots, 
	// so adding trackers here causes the controller tracking to fail.
	IPluginManager& PluginManager = IPluginManager::Get();
	PluginManager.OnLoadingPhaseComplete().AddRaw(this, &FOpenXRViveTrackerPlugin::OnLoadingPhaseComplete);
}

void FOpenXRViveTrackerPlugin::OnLoadingPhaseComplete(ELoadingPhase::Type LoadingPhase, bool bPhaseSuccessful)
{
	// Delayed load of tracker pose action devices.
	if (bPhaseSuccessful && LoadingPhase == ELoadingPhase::Type::PostEngineInit)
	{
		FOpenXRHMD* OpenXRHMD = GetXRSystem();

		for (TPair<FName, FViveTracker>& Pair : Trackers)
		{
			Pair.Value.AddActionDevices(OpenXRHMD);
		}
	}
}

void FOpenXRViveTrackerPlugin::PostSyncActions(XrSession InSession)
{

}

bool FOpenXRViveTrackerPlugin::GetControllerOrientationAndPosition(const int32 ControllerIndex, const FName MotionSource, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const
{
	FOpenXRHMD* OpenXRHMD = GetXRSystem();

	if (ControllerIndex == 0 && Trackers.Contains(MotionSource))
	{
		XrSession Session = OpenXRHMD->GetSession();

		if (Session == XR_NULL_HANDLE)
		{
			return false;
		}

		const FViveTracker* Tracker = Trackers.Find(MotionSource);

		XrActionStateGetInfo GetInfo;
		GetInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
		GetInfo.next = nullptr;
		GetInfo.subactionPath = XR_NULL_PATH;
		GetInfo.action = Tracker->GripAction;

		XrActionStatePose State;
		State.type = XR_TYPE_ACTION_STATE_POSE;
		State.next = nullptr;
		XrResult Result = xrGetActionStatePose(Session, &GetInfo, &State);
		if (XR_SUCCEEDED(Result) && State.isActive)
		{
			FQuat Orientation;
			OpenXRHMD->GetCurrentPose(Tracker->GripDeviceId, Orientation, OutPosition);
			OutOrientation = FRotator(Orientation);
			return true;
		}
	}

	return false;
}

ETrackingStatus FOpenXRViveTrackerPlugin::GetControllerTrackingStatus(const int32 ControllerIndex, const FName MotionSource) const
{
	FOpenXRHMD* OpenXRHMD = GetXRSystem();

	if (ControllerIndex == 0 && Trackers.Contains(MotionSource))
	{
		const FViveTracker* Tracker = Trackers.Find(MotionSource);

		XrSession Session = OpenXRHMD->GetSession();

		if (Session == XR_NULL_HANDLE)
		{
			return ETrackingStatus::NotTracked;
		}

		XrActionStateGetInfo GetInfo;
		GetInfo.type = XR_TYPE_ACTION_STATE_GET_INFO;
		GetInfo.next = nullptr;
		GetInfo.subactionPath = XR_NULL_PATH;
		GetInfo.action = Tracker->GripAction;

		XrActionStatePose State;
		State.type = XR_TYPE_ACTION_STATE_POSE;
		State.next = nullptr;
		XrResult Result = xrGetActionStatePose(Session, &GetInfo, &State);
		if (XR_SUCCEEDED(Result) && State.isActive)
		{
			FQuat Orientation;
			bool bIsTracked = OpenXRHMD->GetIsTracked(Tracker->GripDeviceId);
			return bIsTracked ? ETrackingStatus::Tracked : ETrackingStatus::NotTracked;
		}
	}

	return ETrackingStatus::NotTracked;
}

FName FOpenXRViveTrackerPlugin::GetMotionControllerDeviceTypeName() const
{
	const static FName DefaultName(TEXT("OpenXRViveTracker"));
	return DefaultName;
}

void FOpenXRViveTrackerPlugin::EnumerateSources(TArray<FMotionControllerSource>& SourcesOut) const
{
	for (TPair<XrPath, FName> Pair : PathRoles)
	{
		SourcesOut.Add(Pair.Value);
	}
}

void FOpenXRViveTrackerPlugin::Tick(float DeltaTime)
{

}

void FOpenXRViveTrackerPlugin::SendControllerEvents()
{

}

void FOpenXRViveTrackerPlugin::SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler)
{
	MessageHandler = InMessageHandler;
}

bool FOpenXRViveTrackerPlugin::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	return false;
}

FOpenXRHMD* FOpenXRViveTrackerPlugin::GetXRSystem() const
{
	static FName SystemName(TEXT("OpenXR"));
	if (GEngine != nullptr && GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == SystemName))
	{
		return static_cast<FOpenXRHMD*>(GEngine->XRSystem.Get());
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE

