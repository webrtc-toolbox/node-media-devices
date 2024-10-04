#ifndef NATIVE_MEDIA_STREAM_H
#define NATIVE_MEDIA_STREAM_H
#include <AVFoundation/AVFoundation.h>
#include <napi.h>

class NativeMediaStream : public Napi::ObjectWrap<NativeMediaStream> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  NativeMediaStream(const Napi::CallbackInfo &info);

  AVCaptureDevice *videoDevice;
  AVCaptureDeviceInput *videoInput;
};

#endif