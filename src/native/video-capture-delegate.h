#ifndef VIDEO_CAPTURE_DELEGATE_H
#define VIDEO_CAPTURE_DELEGATE_H

#include <AVFoundation/AVFoundation.h>
#include <napi.h>

@interface VideoCaptureDelegate
    : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate> {
  Napi::ThreadSafeFunction tsfn;
  std::atomic<bool> isReleased;
}

- (id)initWithTSFN:(Napi::ThreadSafeFunction)tsfn;
@end

#endif
