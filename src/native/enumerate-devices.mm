#include <Foundation/Foundation.h>
#include <AVFoundation/AVFoundation.h>
#include <regex>
#include <napi.h>

std::string convertNSStringToStdString(NSString *nsString) {
  if (nsString == nil) {
    return std::string();
  }

  const char *cString = [nsString UTF8String];
  return std::string(cString);
}

Napi::Array listAllMediaDevices(const Napi::Env &env) {
  NSArray *deviceTypes = @[
    AVCaptureDeviceTypeBuiltInWideAngleCamera,
    AVCaptureDeviceTypeBuiltInMicrophone
  ];

  AVCaptureDeviceDiscoverySession *discoverySession =
      [AVCaptureDeviceDiscoverySession
          discoverySessionWithDeviceTypes:deviceTypes
                                mediaType:nil
                                 position:AVCaptureDevicePositionUnspecified];

  unsigned long device_cout{
      static_cast<unsigned long>(discoverySession.devices.count)};

  Napi::Array mediaDevices = Napi::Array::New(env, device_cout);

  // get default video device
  AVCaptureDevice *defaultVideoDevice =
      [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
  Napi::Object defaultVideoDeviceInfo = Napi::Object::New(env);
  defaultVideoDeviceInfo.Set(
      "label", convertNSStringToStdString(defaultVideoDevice.localizedName));
  defaultVideoDeviceInfo.Set("deviceId", "default");
  defaultVideoDeviceInfo.Set("kind", "videoinput");
  mediaDevices.Set((size_t)0, defaultVideoDeviceInfo);

  // get default audio device
  AVCaptureDevice *defaultAudioDevice =
      [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];
  Napi::Object defaultAudioDeviceInfo = Napi::Object::New(env);
  defaultAudioDeviceInfo.Set(
      "label", convertNSStringToStdString(defaultAudioDevice.localizedName));
  defaultAudioDeviceInfo.Set("deviceId", "default");
  defaultAudioDeviceInfo.Set("kind", "audioinput");
  mediaDevices.Set((size_t)1, defaultAudioDeviceInfo);

  for (size_t i = 0; i < device_cout; i++) {
    AVCaptureDevice *device = discoverySession.devices[i];

    // NSLog(@"Device name: %@", device.localizedName);
    // NSLog(@"Device type: %@", device.deviceType);
    // NSLog(@"Device position: %ld", (long)device.position);

    Napi::Object mediaDeviceInfo = Napi::Object::New(env);
    mediaDeviceInfo.Set("label",
                        convertNSStringToStdString(device.localizedName));
    mediaDeviceInfo.Set("deviceId",
                        convertNSStringToStdString(device.uniqueID));

    std::regex camera_regex{"Camera"};
    if (std::regex_search(convertNSStringToStdString(device.deviceType),
                          camera_regex)) {
      mediaDeviceInfo.Set("kind", "videoinput");
    } else {
      mediaDeviceInfo.Set("kind", "audioinput");
    }

    mediaDevices.Set(i + 2, mediaDeviceInfo);
  }

  return mediaDevices;
}