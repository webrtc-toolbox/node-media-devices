#include "./native-audio-stream-track.h"
#include "./audio-capture-delegate.h"
#include <iostream>
@implementation AudioCaptureDelegate

- (instancetype)initWithTSFN:(Napi::ThreadSafeFunction)tsfn {
  self = [super init];
  if (self) {
    _tsfn = tsfn;
    _isTsfnReleased = NO;
  }
  return self;
}

- (void)captureOutput:(AVCaptureOutput *)output
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
           fromConnection:(AVCaptureConnection *)connection {
  if (self.isTsfnReleased) {
    return;
  }

  CMBlockBufferRef blockBuffer = CMSampleBufferGetDataBuffer(sampleBuffer);
  size_t length = CMBlockBufferGetDataLength(blockBuffer);
  uint8_t *data = new uint8_t[length];
  CMBlockBufferCopyDataBytes(blockBuffer, 0, length, data);

  CMAudioFormatDescriptionRef formatDescription =
      CMSampleBufferGetFormatDescription(sampleBuffer);
  const AudioStreamBasicDescription *asbd =
      CMAudioFormatDescriptionGetStreamBasicDescription(formatDescription);

  _tsfn.BlockingCall([=](Napi::Env env, Napi::Function jsCallback) {
    Napi::Buffer<uint8_t> buffer = Napi::Buffer<uint8_t>::New(
        env, data, length,
        [](Napi::Env /*env*/, uint8_t *data) { delete[] data; });

    Napi::Object audioData = Napi::Object::New(env);
    audioData.Set("data", buffer);
    audioData.Set("sampleRate", Napi::Number::New(env, asbd->mSampleRate));
    audioData.Set("channelCount",
                  Napi::Number::New(env, asbd->mChannelsPerFrame));
    audioData.Set("bitsPerSample",
                  Napi::Number::New(env, asbd->mBitsPerChannel));

    jsCallback.Call({audioData});
  });
}

@end

Napi::Object NativeAudioStreamTrack::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(
      env, "NativeAudioStreamTrack",
      {InstanceMethod("startCapture", &NativeAudioStreamTrack::startCapture),
       InstanceMethod("stopCapture", &NativeAudioStreamTrack::stopCapture)});

  Napi::FunctionReference *constructor = new Napi::FunctionReference();
  *constructor = Napi::Persistent(func);
  env.SetInstanceData(constructor);

  exports.Set("NativeAudioStreamTrack", func);
  return exports;
}

NativeAudioStreamTrack::NativeAudioStreamTrack(const Napi::CallbackInfo &info)
    : Napi::ObjectWrap<NativeAudioStreamTrack>(info),
      session{[[AVCaptureSession alloc] init]}, delegate{nil} {}

Napi::Value
NativeAudioStreamTrack::startCapture(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1 || !info[0].IsFunction()) {
    Napi::TypeError::New(env, "Expected one callback function")
        .ThrowAsJavaScriptException();
    return env.Null();
  }

  Napi::Function jsCallback = info[0].As<Napi::Function>();

  // Create ThreadSafeFunction for the callback
  tsfn = Napi::ThreadSafeFunction::New(env, jsCallback, "AudioCaptureCallback",
                                       0, 1);

  dispatch_async(
      dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        @autoreleasepool {
          [session setSessionPreset:AVCaptureSessionPresetHigh];

          AVCaptureDevice *device =
              [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeAudio];
          NSError *error = nil;
          AVCaptureDeviceInput *input =
              [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];

          if (!input) {
            NSString *errorDescription =
                error ? [error localizedDescription] : @"Unknown error";
            std::string errorString = [errorDescription UTF8String];

            tsfn.BlockingCall(
                [errorString](Napi::Env env, Napi::Function jsCallback) {
                  jsCallback.Call({Napi::String::New(env, errorString)});
                });

            isTsfnReleased = true;
            tsfn.Release();

            return;
          }

          [session addInput:input];

          AVCaptureAudioDataOutput *output =
              [[AVCaptureAudioDataOutput alloc] init];

          delegate = [[AudioCaptureDelegate alloc] initWithTSFN:tsfn];
          dispatch_queue_t queue =
              dispatch_queue_create("AudioCaptureQueue", DISPATCH_QUEUE_SERIAL);
          [output setSampleBufferDelegate:delegate queue:queue];

          [session addOutput:output];

          [session startRunning];
        }
      });

  return env.Undefined();
}

Napi::Value
NativeAudioStreamTrack::stopCapture(const Napi::CallbackInfo &info) {
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

                     delegate.isTsfnReleased = YES;
                     isTsfnReleased = true;
                     tsfn.Release();
                   }
                 });

  return env.Undefined();
}

NativeAudioStreamTrack::~NativeAudioStreamTrack() {
  if (session) {
    [session stopRunning];
    [session release];
  }

  if (delegate) {
    [delegate release];
  }

  if (!isTsfnReleased) {
    tsfn.Release();
  }
}
