#ifdef _WIN32
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <napi.h>
#include <windows.h>

Napi::Value enumerateDevices(const Napi::Env &env) {
  Napi::Array mediaDevices = Napi::Array::New(env);
  UINT32 count = 0;

  // Initialize COM and Media Foundation
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
  MFStartup(MF_VERSION);

  // Enumerate video devices
  IMFActivate **ppDevices = NULL;
  IMFAttributes *pVideoConfig = NULL;
  MFCreateAttributes(&pVideoConfig, 1);
  pVideoConfig->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
  HRESULT hr = MFEnumDeviceSources(pVideoConfig, &ppDevices, &count);

  if (SUCCEEDED(hr)) {
    for (UINT32 i = 0; i < count; i++) {
      WCHAR *friendlyName = NULL;
      UINT32 nameLength = 0;
      ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
                                       &friendlyName, &nameLength);

      Napi::Object deviceInfo = Napi::Object::New(env);
      deviceInfo.Set("label", Napi::String::New(
                                  env, std::wstring(friendlyName, nameLength)));
      deviceInfo.Set("deviceId", Napi::String::New(env, std::to_string(i)));
      deviceInfo.Set("kind", "videoinput");

      mediaDevices.Set(mediaDevices.Length(), deviceInfo);

      CoTaskMemFree(friendlyName);
      ppDevices[i]->Release();
    }
    CoTaskMemFree(ppDevices);
  } else {
    Napi::Error::New(env, "Failed to enumerate video devices")
        .ThrowAsJavaScriptException();

    return mediaDevices;
  }

  pVideoConfig->Release();

  // Enumerate audio devices
  ppDevices = NULL;
  IMFAttributes *pAudioConfig = NULL;
  MFCreateAttributes(&pAudioConfig, 1);
  pAudioConfig->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_AUDCAP_GUID);
  hr = MFEnumDeviceSources(pAudioConfig, &ppDevices, &count);

  if (SUCCEEDED(hr)) {
    for (UINT32 i = 0; i < count; i++) {
      WCHAR *friendlyName = NULL;
      UINT32 nameLength = 0;
      ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,
                                       &friendlyName, &nameLength);

      Napi::Object deviceInfo = Napi::Object::New(env);
      deviceInfo.Set("label", Napi::String::New(
                                  env, std::wstring(friendlyName, nameLength)));
      deviceInfo.Set("deviceId", Napi::String::New(env, std::to_string(i)));
      deviceInfo.Set("kind", "audioinput");

      mediaDevices.Set(mediaDevices.Length(), deviceInfo);

      CoTaskMemFree(friendlyName);
      ppDevices[i]->Release();
    }
    CoTaskMemFree(ppDevices);
  }

  pAudioConfig->Release();

  // Shutdown Media Foundation and uninitialize COM
  MFShutdown();
  CoUninitialize();

  return mediaDevices;
}

#elif defined(__APPLE__)
#include <AVFoundation/AVFoundation.h>
#include <Foundation/Foundation.h>
#include <iostream>
#include <napi.h>
#include <thread>

class EnumerateDevicesWorker : public Napi::AsyncWorker {
public:
  EnumerateDevicesWorker(Napi::Promise::Deferred deferred)
      : Napi::AsyncWorker(deferred.Env()), deferred_(deferred) {}

  void Execute() override {
    @autoreleasepool {
      NSArray *deviceTypes = @[
        AVCaptureDeviceTypeBuiltInWideAngleCamera,
        AVCaptureDeviceTypeBuiltInMicrophone
      ];

      AVCaptureDeviceDiscoverySession *discoverySession =
          [AVCaptureDeviceDiscoverySession
              discoverySessionWithDeviceTypes:deviceTypes
                                    mediaType:nil
                                     position:
                                         AVCaptureDevicePositionUnspecified];

      device_count_ =
          static_cast<unsigned long>(discoverySession.devices.count);

      // Get default video device
      AVCaptureDevice *defaultVideoDevice =
          [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
      defaultVideoLabel_ =
          convertNSStringToStdString(defaultVideoDevice.localizedName);

      // Get default audio device
      AVCaptureDevice *defaultAudioDevice =
          [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];
      defaultAudioLabel_ =
          convertNSStringToStdString(defaultAudioDevice.localizedName);

      // Enumerate all devices
      for (AVCaptureDevice *device in discoverySession.devices) {
        DeviceInfo info;
        info.label = convertNSStringToStdString(device.localizedName);
        info.deviceId = convertNSStringToStdString(device.uniqueID);
        info.kind = [device.deviceType containsString:@"Camera"] ? "videoinput"
                                                                 : "audioinput";
        devices_.push_back(info);
      }
    }
  }

  void OnOK() override {
    Napi::Env env = Env();
    Napi::Array mediaDevices = Napi::Array::New(env, device_count_ + 2);

    // Add default video device
    Napi::Object defaultVideoDeviceInfo = Napi::Object::New(env);
    defaultVideoDeviceInfo.Set("label", defaultVideoLabel_);
    defaultVideoDeviceInfo.Set("deviceId", "default");
    defaultVideoDeviceInfo.Set("kind", "videoinput");
    mediaDevices.Set((uint32_t)0, defaultVideoDeviceInfo);

    // Add default audio device
    Napi::Object defaultAudioDeviceInfo = Napi::Object::New(env);
    defaultAudioDeviceInfo.Set("label", defaultAudioLabel_);
    defaultAudioDeviceInfo.Set("deviceId", "default");
    defaultAudioDeviceInfo.Set("kind", "audioinput");
    mediaDevices.Set((uint32_t)1, defaultAudioDeviceInfo);

    // Add all other devices
    for (uint32_t i = 0; i < devices_.size(); i++) {
      Napi::Object mediaDeviceInfo = Napi::Object::New(env);
      mediaDeviceInfo.Set("label", devices_[i].label);
      mediaDeviceInfo.Set("deviceId", devices_[i].deviceId);
      mediaDeviceInfo.Set("kind", devices_[i].kind);
      mediaDevices.Set(i + 2, mediaDeviceInfo);
    }

    deferred_.Resolve(mediaDevices);
  }

private:
  std::string convertNSStringToStdString(NSString *nsString) {
    if (nsString == nil) {
      return std::string();
    }

    const char *cString = [nsString UTF8String];
    return std::string(cString);
  }

  struct DeviceInfo {
    std::string label;
    std::string deviceId;
    std::string kind;
  };

  Napi::Promise::Deferred deferred_;
  unsigned long device_count_;
  std::string defaultVideoLabel_;
  std::string defaultAudioLabel_;
  std::vector<DeviceInfo> devices_;
};

Napi::Value enumerateDevices(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(env);

  EnumerateDevicesWorker *worker = new EnumerateDevicesWorker(deferred);
  worker->Queue();

  return deferred.Promise();
}
#endif
