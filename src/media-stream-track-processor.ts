import { MediaStreamTrack } from "./media-stream-track";
import { VideoFrame } from "./types";
import { ReadableStream } from 'node:stream/web'

interface Options {
  track: MediaStreamTrack
}

export class MediaStreamTrackProcessor {
  private track: MediaStreamTrack;
  public readable: ReadableStream;

  public constructor(options: Options) {
    this.track = options.track;
    this.readable = this.createReadableStream();
  }

  private createReadableStream(): ReadableStream {
    const readable = new ReadableStream({
      start: (controller) => {
        this.track._frameEmitter.on('frame', (frame: VideoFrame) => {
          controller.enqueue(frame);
        });

        this.track._frameEmitter.on('end', () => {
          controller.close();
        });

        this.track.startCapture();
      },
      cancel: () => {
        
      },
    });


    return readable;
  }
}