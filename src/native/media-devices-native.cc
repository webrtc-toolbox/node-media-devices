#include <napi.h>
#include "./enumerate-devices.mm"

#ifdef WIN32
#include "../win/native-video-stream-track.h"
#include "../win/native-audio-stream-track.h"
#elif defined(__APPLE__)
#include "../mac/native-video-stream-track.h"
#include "../mac/native-audio-stream-track.h"
#endif


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