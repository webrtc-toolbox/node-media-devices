#include "native-media-recorder.h"
#include "native-media-stream.h"
#include "video-frame-delegate.h"
#include <AVFoundation/AVFoundation.h>
#include <napi.h>

NativeMediaRecorder::NativeMediaRecorder(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<NativeMediaRecorder>(info) {

  captureSession = [[AVCaptureSession alloc] init];
  [captureSession setSessionPreset:AVCaptureSessionPresetHigh];

  video_frame_delegate = [[VideoFrameDelegate alloc] initWithCPPInstance:this];

  Napi::Function cb{info[0].As<Napi::Function>()};
  worker = new MediaRecorderWorker(cb);
};

Napi::Object NativeMediaRecorder::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(
      env, "NativeMediaRecorder",
      {InstanceMethod("addInputDevice", &NativeMediaRecorder::addInputDevice),
       InstanceMethod("requestCapture", &NativeMediaRecorder::requestCapture)});

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  exports.Set("NativeMediaRecorder", func);
  return exports;
};

Napi::Value
NativeMediaRecorder::addInputDevice(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  NativeMediaStream *media_stream =
      Napi::ObjectWrap<NativeMediaStream>::Unwrap(info[0].As<Napi::Object>());

  if ([captureSession canAddInput:media_stream->videoInput]) {
    [captureSession addInput:media_stream->videoInput];
    return Napi::Number::New(env, 0);
  } else {
    return Napi::Number::New(env, 1);
  }
};

Napi::Value
NativeMediaRecorder::requestCapture(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  AVCaptureVideoDataOutput *videoOutput =
      [[AVCaptureVideoDataOutput alloc] init];
  videoOutput.videoSettings =
      @{(id)kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)};

  if ([captureSession canAddOutput:videoOutput]) {
    [captureSession addOutput:videoOutput];
  } else {
    return Napi::Number::New(env, 1);
  }

  dispatch_queue_t videoQueue = dispatch_queue_create("videoQueue", NULL);
  [videoOutput setSampleBufferDelegate:video_frame_delegate queue:videoQueue];

  [captureSession startRunning];

  [[NSRunLoop currentRunLoop] run];

  return Napi::Number::New(env, 0);
};

void NativeMediaRecorder::data(CVImageBufferRef imageBuffer) {
  worker->imageBuffers.push(imageBuffer);

  worker->Queue();

  // Create a N-API buffer that takes ownership of the data pointer
}

NativeMediaRecorder::MediaRecorderWorker::MediaRecorderWorker(
    Napi::Function &callback)
    : Napi::AsyncWorker(callback) {}

void NativeMediaRecorder::MediaRecorderWorker::Execute() {
  if (imageBuffers.size() == 0) {
    return;
  }

  CVImageBufferRef imageBuffer{imageBuffers.front()};
  imageBuffers.pop();

  CVPixelBufferLockBaseAddress(imageBuffer, 0);
  void *baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);

  size_t width = CVPixelBufferGetWidth(imageBuffer);
  size_t height = CVPixelBufferGetHeight(imageBuffer);
  size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);

  bufferLength = height * bytesPerRow;
  if (buffer == nullptr) {
    buffer = (char *)malloc(bufferLength);
  }

  // memcpy(buffer, baseAddress, bufferLength);

  std::cout << "width:" << width << "\n";
  std::cout << "height:" << height << "\n";

  CVPixelBufferUnlockBaseAddress(imageBuffer, 0);

  if (imageBuffers.size() > 0) {
    Queue();
  }
  // Need to simulate cpu heavy task
  // std::this_thread::sleep_for(std::chrono::seconds(1));
}

void NativeMediaRecorder::MediaRecorderWorker::OnOK() {
  Napi::HandleScope scope(Env());

  // Napi::Buffer<char> bf = Napi::Buffer<char>::New(
  //     Env(), buffer, bufferLength, [](Napi::Env env, void *data) {
  //       delete[] reinterpret_cast<char *>(data); // Free the allocated memory
  //     });

  // Callback().Call({Env().Null(), bf});
}
