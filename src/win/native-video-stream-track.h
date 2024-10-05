#ifndef NATIVE_VIDEO_STREAM_TRACK_H
#define NATIVE_VIDEO_STREAM_TRACK_H

#include <atomic>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <napi.h>

class NativeVideoStreamTrack : public Napi::ObjectWrap<NativeVideoStreamTrack> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  NativeVideoStreamTrack(const Napi::CallbackInfo &info);
  ~NativeVideoStreamTrack();

  Napi::Value startCapture(const Napi::CallbackInfo &info);
  Napi::Value stopCapture(const Napi::CallbackInfo &info);

private:
  IMFSourceReader *sourceReader;
  Napi::ThreadSafeFunction tsfn;
  std::atomic<bool> isTsfnReleased;

  void InitializeMF();
  void UninitializeMF();
  void ReadFrame();
};

#endif // NATIVE_VIDEO_STREAM_TRACK_H
    