#ifndef AUDIO_CAPTURE_DELEGATE_H
#define AUDIO_CAPTURE_DELEGATE_H

#include <AVFoundation/AVFoundation.h>
#include <napi.h>

@interface AudioCaptureDelegate
    : NSObject <AVCaptureAudioDataOutputSampleBufferDelegate>
@property(nonatomic, assign) Napi::ThreadSafeFunction tsfn;
@property(atomic, assign) BOOL isTsfnReleased;

- (instancetype)initWithTSFN:(Napi::ThreadSafeFunction)tsfn;
- (void)captureOutput:(AVCaptureOutput *)output
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
           fromConnection:(AVCaptureConnection *)connection;
@end

#endif
