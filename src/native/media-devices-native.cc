#include "./enumerate-devices.mm"
#include "./native-media-recorder.h"
#include "./native-media-stream-track.h"
#include "./native-media-stream.h"
#include <napi.h>

Napi::Array enumerateDevices(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  return listAllMediaDevices(env);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "enumerateDevices"),
              Napi::Function::New(env, enumerateDevices));

  NativeMediaStream::Init(env, exports);
  NativeMediaRecorder::Init(env, exports);
  NativeMediaStreamTrack::Init(env, exports);

  return exports;
}

NODE_API_MODULE(hello, Init)