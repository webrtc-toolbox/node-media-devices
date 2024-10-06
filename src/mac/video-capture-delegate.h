#ifndef VIDEO_CAPTURE_DELEGATE_H
#define VIDEO_CAPTURE_DELEGATE_H

#include <AVFoundation/AVFoundation.h>
#include <napi.h>

@interface VideoCaptureDelegate
    : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
@property (nonatomic, assign) Napi::ThreadSafeFunction tsfn;
@property (atomic, assign) BOOL isReleased;

- (instancetype)initWithTSFN:(Napi::ThreadSafeFunction)tsfn;
@end

#endif
