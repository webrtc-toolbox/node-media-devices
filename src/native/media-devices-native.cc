#include "./enumerate-devices.mm"
// #include "./native-media-recorder.h"
#include "./native-audio-stream-track.h"
#include "./native-video-stream-track.h"
// #include "./native-media-stream.h"
#include <napi.h>

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "enumerateDevices"),
              Napi::Function::New(env, enumerateDevices));

  NativeVideoStreamTrack::Init(env, exports);
  NativeAudioStreamTrack::Init(env, exports);

  // NativeMediaStream::Init(env, exports);
  // NativeMediaRecorder::Init(env, exports);
  return exports;
}

NODE_API_MODULE(hello, Init)