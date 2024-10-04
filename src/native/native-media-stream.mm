#include "native-media-stream.h"
#include <AVFoundation/AVFoundation.h>
#include <napi.h>

NativeMediaStream::NativeMediaStream(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<NativeMediaStream>(info) {
  videoDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
  NSError *error = nil;
  videoInput = [AVCaptureDeviceInput deviceInputWithDevice:videoDevice
                                                     error:&error];
}

Napi::Object NativeMediaStream::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "NativeMediaStream", {});

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  exports.Set("NativeMediaStream", func);
  return exports;
}