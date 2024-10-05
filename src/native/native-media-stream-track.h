#ifndef NATIVE_MEDIA_STREAM_TRACK_H
#define NATIVE_MEDIA_STREAM_TRACK_H

#include <AVFoundation/AVFoundation.h>
#include <napi.h>

class NativeMediaStreamTrack : public Napi::ObjectWrap<NativeMediaStreamTrack> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  NativeMediaStreamTrack(const Napi::CallbackInfo &info);

  Napi::Value startCapture(const Napi::CallbackInfo &info);

  Napi::Value stopCapture(const Napi::CallbackInfo &info);

private:
  AVCaptureSession *session;
};

#endif
