#include <napi.h>
#include "./enumerate-devices.mm"

#ifdef WIN32
#include "../win/native-video-stream-track.h"
#elif defined(__APPLE__)
#include "./native-video-stream-track.h"
#endif

// #include "./native-audio-stream-track.h"
// #include "./native-video-stream-track.h"
// #include "./native-media-stream.h"

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set(Napi::String::New(env, "enumerateDevices"),
              Napi::Function::New(env, enumerateDevices));

  NativeVideoStreamTrack::Init(env, exports);
  // NativeAudioStreamTrack::Init(env, exports);

  // NativeMediaStream::Init(env, exports);
  // NativeMediaRecorder::Init(env, exports);
  return exports;
}

NODE_API_MODULE(hello, Init)