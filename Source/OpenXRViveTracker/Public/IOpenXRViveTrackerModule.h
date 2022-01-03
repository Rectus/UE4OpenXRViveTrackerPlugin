

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IInputDeviceModule.h"

//enum class EHandKeypoint : uint8;
//enum class EControllerHand : uint8;


class IOpenXRViveTrackerModule : public IInputDeviceModule
{
public:

	static inline IOpenXRViveTrackerModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IOpenXRViveTrackerModule>("OpenXRViveTracker");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("OpenXRViveTracker");
	}

	virtual TSharedPtr<IInputDevice> GetInputDevice() = 0;
};
