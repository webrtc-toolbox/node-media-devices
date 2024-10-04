#ifndef VIDEO_FRAME_DELEGATE_H
#define VIDEO_FRAME_DELEGATE_H
#include <AVFoundation/AVFoundation.h>
#include <napi.h>

class NativeMediaRecorder;

@interface VideoFrameDelegate
    : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate> {
  NativeMediaRecorder *recorder;
}

- (id)initWithCPPInstance:(NativeMediaRecorder *)cppInstance;
@end

#endif
