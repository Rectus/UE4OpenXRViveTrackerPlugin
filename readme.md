Unreal Engine 4 OpenXR Vive Tracker Plugin
---

This plugin adds basic support for Vive trackers under OpenXR using the `XR_HTCX_vive_tracker_interaction` extension.

Supports UE4 version 4.27

Unlike Valve's official implementation, this does not require the engine to be patched.

Valve implementation:
https://github.com/ValveSoftware/openxr_engine_plugins/tree/main/unreal_engine/ue4/OpenXRViveTracker


### Usage ###

The plugin uses the existing motion controller component to provied the tracker pose.

The plugin adds new motion sources for each tracker role. Simply add a motion controller component and set it's Motion Source property to one of the below values to track the tracker with the assigned role.

Tracker_Camera
Tracker_Chest
Tracker_Elbow_Left
Tracker_Elbow_Right
Tracker_Foot_Left
Tracker_Foot_Right
Tracker_Knee_Left
Tracker_Knee_Right
Tracker_Shoulder_Left
Tracker_Shoulder_Right
Tracker_Keyboard
Tracker_Waist
Tracker_Handheld


### Todo ###

- Pogo pin input and output is unfinished, as input is currently broken on the SteamVR side.
- Tracker disconnection can't be detected, as tracker interaction profile querying is currently broken in SteamVR.
