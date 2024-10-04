#include "video-frame-delegate.h"
#include "native-media-recorder.h"

@implementation VideoFrameDelegate

- (void)captureOutput:(AVCaptureOutput *)output
    didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
           fromConnection:(AVCaptureConnection *)connection {
  // Here you can process the sampleBuffer
  CVImageBufferRef imageBuffer = CMSampleBufferGetImageBuffer(sampleBuffer);

  recorder->data(imageBuffer);

  // Process the image buffer (e.g., convert to UIImage, etc.)
  // NSLog(@"Frame captured");
}

- (id)initWithCPPInstance:(NativeMediaRecorder *)cppInstance {
  if (self = [super init]) {
    recorder = cppInstance; // Assign the C++ instance
  }
  return self;
}

@end