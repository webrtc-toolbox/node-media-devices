#pragma once

#include <napi.h>
#include <mfidl.h>
#include <mfapi.h>
#include <mfreadwrite.h>

class NativeAudioStreamTrack : public Napi::ObjectWrap<NativeAudioStreamTrack> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  NativeAudioStreamTrack(const Napi::CallbackInfo &info);
  ~NativeAudioStreamTrack();

  Napi::Value startCapture(const Napi::CallbackInfo &info);
  Napi::Value stopCapture(const Napi::CallbackInfo &info);

private:
  IMFSourceReader *sourceReader;
  Napi::ThreadSafeFunction tsfn;
  std::atomic<bool> isTsfnReleased;

  void InitializeMF();
  void UninitializeMF();
  void ReadFrame();
  void ProcessSample(IMFSample *pSample);
  void GetAudioFormat(WAVEFORMATEX *pWaveFormat);
  std::string GetAudioEncoding();
};
