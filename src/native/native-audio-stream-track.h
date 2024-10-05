#ifndef NATIVE_AUDIO_STREAM_TRACK_H
#define NATIVE_AUDIO_STREAM_TRACK_H

#include <napi.h>
#include <AVFoundation/AVFoundation.h>
#include "./audio-capture-delegate.h"

class NativeAudioStreamTrack : public Napi::ObjectWrap<NativeAudioStreamTrack> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  NativeAudioStreamTrack(const Napi::CallbackInfo &info);

  ~NativeAudioStreamTrack();

  Napi::Value startCapture(const Napi::CallbackInfo &info);

  Napi::Value stopCapture(const Napi::CallbackInfo &info);

private:
  AVCaptureSession *session;
  Napi::ThreadSafeFunction tsfn;
  AudioCaptureDelegate *delegate;
  std::atomic<bool> isTsfnReleased;
};

#endif
