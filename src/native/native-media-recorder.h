#ifndef NATIVE_MEDIA_RECORDER_H
#define NATIVE_MEDIA_RECORDER_H
#include "native-media-stream.h"
#include "video-frame-delegate.h"
#include <AVFoundation/AVFoundation.h>
#include <iostream>
#include <napi.h>
#include <queue>

class NativeMediaRecorder : public Napi::ObjectWrap<NativeMediaRecorder> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);

  NativeMediaRecorder(const Napi::CallbackInfo &info);

  Napi::Value addInputDevice(const Napi::CallbackInfo &info);

  Napi::Value requestCapture(const Napi::CallbackInfo &info);

  void data(CVImageBufferRef imageBuffer);

  class MediaRecorderWorker : public Napi::AsyncWorker {
  public:
    MediaRecorderWorker(Napi::Function &callback);

    void Execute() override;

    void OnOK() override;

    std::queue<CVImageBufferRef> imageBuffers;

  private:
    char *buffer{nullptr};
    size_t bufferLength{0};
  };

private:
  AVCaptureSession *captureSession;
  VideoFrameDelegate *video_frame_delegate;
  MediaRecorderWorker *worker;
};

#endif