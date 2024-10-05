#include "./native-media-stream-track.h"
#include <AVFoundation/AVFoundation.h>
#include <iostream>
#include <napi.h>

@interface VideoCaptureDelegate
    : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate> {
  Napi::ThreadSafeFunction tsfn;
}

- (id)initWithTSFN:(Napi::ThreadSafeFunction)tsfn;
@end

@implementation VideoCaptureDelegate

- (id)initWithTSFN:(Napi::ThreadSafeFunction)tsfnParam {
  self = [super init];
  std::cout << "init" << std::endl;
  if (self) {
    self->tsfn = tsfnParam;
  }
  return self;
}

- (void)captureOutput:(AVCaptureOutput *)output
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
           fromConnection:(AVCaptureConnection *)connection {
  std::cout << "captureOutput" << std::endl;
  // Process the sampleBuffer and send data to JavaScript
  CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);
  if (!imageBuffer) {
    // Handle error
    return;
  }

  CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)imageBuffer;
  OSType pixelFormat = CVPixelBufferGetPixelFormatType(pixelBuffer);

  std::string format;
  if (pixelFormat == kCVPixelFormatType_32BGRA) {
    format = "BGRA";
  } else if (pixelFormat == kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange) {
    format = "NV12";
  } else {
    format = "unknown";
  }

  CVPixelBufferLockBaseAddress(imageBuffer, kCVPixelBufferLock_ReadOnly);

  size_t width = CVPixelBufferGetWidth(imageBuffer);
  size_t height = CVPixelBufferGetHeight(imageBuffer);
  size_t bytesPerRow = CVPixelBufferGetBytesPerRow(imageBuffer);
  void *baseAddress = CVPixelBufferGetBaseAddress(imageBuffer);
  size_t bufferSize = bytesPerRow * height;

  // Copy pixel data
  uint8_t *pixelData = new uint8_t[bufferSize];
  memcpy(pixelData, baseAddress, bufferSize);

  tsfn.BlockingCall([=](Napi::Env env, Napi::Function jsCallback) {
    // Convert sampleBuffer to a Node.js Buffer or any other format
    Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::New(
        env, pixelData, bufferSize,
        [](Napi::Env /*env*/, uint8_t *data) { delete[] data; });

    // Create an object to hold frame data
    Napi::Object frameData = Napi::Object::New(env);
    frameData.Set("data", buffer);
    frameData.Set("codedWitdh", Napi::Number::New(env, width));
    frameData.Set("codedHeight", Napi::Number::New(env, height));
    frameData.Set("format", Napi::String::New(env, format));

    jsCallback.Call({frameData});
  });
}

@end

Napi::Object NativeMediaStreamTrack::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(
      env, "NativeMediaStreamTrack",
      {InstanceMethod("startCapture", &NativeMediaStreamTrack::startCapture),
       InstanceMethod("stopCapture", &NativeMediaStreamTrack::stopCapture)});

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  exports.Set("NativeMediaStreamTrack", func);
  return exports;
}

NativeMediaStreamTrack::NativeMediaStreamTrack(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<NativeMediaStreamTrack>(info),
      session{[[AVCaptureSession alloc] init]} {}

Napi::Value
NativeMediaStreamTrack::startCapture(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (info.Length() < 2 || !info[0].IsFunction() || !info[1].IsFunction()) {
    Napi::TypeError::New(env, "Expected two callback functions")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Function jsCallback = info[0].As<Napi::Function>();
  Napi::Function stopCallback = info[1].As<Napi::Function>();

  // Create ThreadSafeFunctions for both callbacks
  Napi::ThreadSafeFunction tsfn =
      Napi::ThreadSafeFunction::New(env, jsCallback, "CaptureCallback", 0, 1);
  Napi::ThreadSafeFunction stopTsfn =
      Napi::ThreadSafeFunction::New(env, stopCallback, "StopCallback", 0, 1);

  dispatch_async(
      dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        @autoreleasepool {
          [session setSessionPreset:AVCaptureSessionPresetHigh];

          AVCaptureDevice *device =
              [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
          NSError *error = nil;
          AVCaptureDeviceInput *input =
              [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];

          if (!input) {
            NSString *errorDescription = error ? [error localizedDescription] : @"Unknown error";
            std::string errorString = [errorDescription UTF8String];

            tsfn.BlockingCall(
                [errorString](Napi::Env env, Napi::Function jsCallback) {
                  jsCallback.Call({Napi::String::New(env, errorString)});
                });
            tsfn.Release();
            return;
          }

          [session addInput:input];

          AVCaptureVideoDataOutput *output =
              [[AVCaptureVideoDataOutput alloc] init];
          output.videoSettings = @{
            (NSString *)
            kCVPixelBufferPixelFormatTypeKey : @(kCVPixelFormatType_32BGRA)
          };

          VideoCaptureDelegate *delegate =
              [[VideoCaptureDelegate alloc] initWithTSFN:tsfn];

          dispatch_queue_t videoQueue =
              dispatch_queue_create("videoQueue", NULL);
          [output setSampleBufferDelegate:delegate queue:videoQueue];

          

          [session addOutput:output];

          [[NSNotificationCenter defaultCenter]
              addObserverForName:AVCaptureSessionDidStopRunningNotification
                          object:session
                           queue:nil
                      usingBlock:^(NSNotification *note) {
                        std::cout << "AVCaptureSessionDidStopRunningNotification" << std::endl;
                        stopTsfn.BlockingCall();
                      }];

          [session startRunning];

          std::cout << "started" << std::endl;
        }
      });

  return env.Undefined();
}

Napi::Value
NativeMediaStreamTrack::stopCapture(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0),
                 ^{
                   @autoreleasepool {
                     if (session && [session isRunning]) {
                       [session stopRunning];

                       // Remove all inputs
                       for (AVCaptureInput *input in session.inputs) {
                         [session removeInput:input];
                       }

                       // Remove all outputs
                       for (AVCaptureOutput *output in session.outputs) {
                         [session removeOutput:output];
                       }

                       session = nil;
                     }
                   }
                 });

  return env.Undefined();
}
