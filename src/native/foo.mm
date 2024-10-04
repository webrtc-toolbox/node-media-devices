#import <AVFoundation/AVFoundation.h>
#include <napi.h>

using namespace Napi;

@interface VideoCaptureDelegate
    : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate> {
  Napi::ThreadSafeFunction tsfn;
}

- (id)initWithTSFN:(Napi::ThreadSafeFunction)tsfn;
@end

@implementation VideoCaptureDelegate

- (id)initWithTSFN:(Napi::ThreadSafeFunction)tsfn {
  self = [super init];
  if (self) {
    self->tsfn = tsfn;
  }
  return self;
}

- (void)captureOutput:(AVCaptureOutput *)output
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
           fromConnection:(AVCaptureConnection *)connection {
  // Process the sampleBuffer and send data to JavaScript
  tsfn.BlockingCall([=](Napi::Env env, Napi::Function jsCallback) {
    // Convert sampleBuffer to a Node.js Buffer or any other format
    jsCallback.Call({/* Arguments to JavaScript callback */});
  });
}

@end

Napi::Value StartCapture(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (!info[0].IsFunction()) {
    Napi::TypeError::New(env, "Expected a callback function")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Function jsCallback = info[0].As<Napi::Function>();

  // Create a ThreadSafeFunction
  Napi::ThreadSafeFunction tsfn =
      Napi::ThreadSafeFunction::New(env, jsCallback, "CaptureCallback", 0, 1);

  dispatch_async(
      dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        @autoreleasepool {
          AVCaptureSession *session = [[AVCaptureSession alloc] init];
          [session setSessionPreset:AVCaptureSessionPresetHigh];

          AVCaptureDevice *device =
              [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
          NSError *error = nil;
          AVCaptureDeviceInput *input =
              [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];

          if (!input) {
            tsfn.BlockingCall(
                [error](Napi::Env env, Napi::Function jsCallback) {
                  jsCallback.Call({Napi::String::New(env, "shit")});
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
          [output setSampleBufferDelegate:delegate
                                    queue:dispatch_get_main_queue()];

          [session addOutput:output];
          [session startRunning];
        }
      });

  return env.Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("startCapture", Napi::Function::New(env, StartCapture));
  return exports;
}

NODE_API_MODULE(camera_capture, Init)
