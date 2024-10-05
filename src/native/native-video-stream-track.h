#ifndef NATIVE_VIDEO_STREAM_TRACK_H
#define NATIVE_VIDEO_STREAM_TRACK_H

#include "./video-capture-delegate.h"
#include <AVFoundation/AVFoundation.h>
#include <napi.h>

class NativeVideoStreamTrack : public Napi::ObjectWrap<NativeVideoStreamTrack> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  NativeVideoStreamTrack(const Napi::CallbackInfo &info);

  ~NativeVideoStreamTrack();

  Napi::Value startCapture(const Napi::CallbackInfo &info);

  Napi::Value stopCapture(const Napi::CallbackInfo &info);

private:
  AVCaptureSession *session;
  Napi::ThreadSafeFunction tsfn;
  VideoCaptureDelegate *delegate;
  std::atomic<bool> isTsfnReleased;
};

#endif
