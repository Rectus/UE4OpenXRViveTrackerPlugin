

#pragma once

#include "CoreMinimal.h"

#include "XRMotionControllerBase.h"
#include "IOpenXRExtensionPlugin.h"
#include "Modules/ModuleInterface.h"
#include "OpenXRCore.h"
#include "IInputDevice.h"
#include "Interfaces/IPluginManager.h"
#include <OpenXRHMD/Private/OpenXRHMD.h>

DEFINE_LOG_CATEGORY_STATIC(LogOpenXRViveTracker, Log, All);

class FOpenXRHMD;

struct FViveTracker
{
	XrPath			PersistentPath;
	XrPath			RolePath;
	XrAction		GripAction;
	XrAction		VibrationAction;
	int32			GripDeviceId;
	XrSpace			GripActionSpace;
	bool			bIsActive;

	FViveTracker(XrPath InUserPath, XrPath InPersistentPath, const char* InName);
	void AddActionDevices(FOpenXRHMD* HMD);
};


class FOpenXRViveTrackerPlugin :
	public IOpenXRExtensionPlugin,
	public FXRMotionControllerBase,
	public IInputDevice
{
public:

	FOpenXRViveTrackerPlugin(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler);
	virtual ~FOpenXRViveTrackerPlugin();

	/** IOpenXRExtensionPlugin */
	virtual FString GetDisplayName() override
	{
		return FString(TEXT("OpenXR Vive Tracker"));
	}
	virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
	virtual const void* OnGetSystem(XrInstance InInstance, const void* InNext) override;
	virtual void PostCreateInstance(XrInstance InInstance) override;
	virtual void OnEvent(XrSession InSession, const XrEventDataBaseHeader* InHeader) override;
	virtual void AddActions(XrInstance Instance, TFunction<XrAction(XrActionType InActionType, const FName& InName, const TArray<XrPath>& InSubactionPaths)> AddAction) override;
	virtual void PostSyncActions(XrSession InSession) override;


	/** IMotionController interface */
	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const FName MotionSource, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const override;
	virtual ETrackingStatus GetControllerTrackingStatus(const int32 ControllerIndex, const FName MotionSource) const override;
	virtual FName GetMotionControllerDeviceTypeName() const override;
	virtual void EnumerateSources(TArray<FMotionControllerSource>& SourcesOut) const override;
	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const EControllerHand DeviceHand, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const override { return false; }
	virtual ETrackingStatus GetControllerTrackingStatus(const int32 ControllerIndex, const EControllerHand DeviceHand) const override { return ETrackingStatus::NotTracked; }


	/** IInputDevice interface */
	virtual void Tick(float DeltaTime) override;
	virtual void SendControllerEvents() override;
	virtual void SetMessageHandler(const TSharedRef<FGenericApplicationMessageHandler>& InMessageHandler) override;
	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	virtual void SetChannelValue(int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value) override {};
	virtual void SetChannelValues(int32 ControllerId, const FForceFeedbackValues& values) override {};


	void OnLoadingPhaseComplete(ELoadingPhase::Type LoadingPhase, bool bPhaseSuccessful);

private:

	FOpenXRHMD* GetXRSystem() const;

	PFN_xrEnumerateViveTrackerPathsHTCX xrEnumerateViveTrackerPathsHTCX = nullptr;

	TSharedPtr<FGenericApplicationMessageHandler> MessageHandler;

	XrPath InteractionProfile;

	TMap<FName, FViveTracker> Trackers;

	TMap<XrPath, FName> PathRoles;

	//TMap<const FKey, const FName> ActionKeys;
	//TMap<const FKey, const FName> AxisKeys;
};


