import { EventEmitter } from "node:events";
const { NativeVideoStreamTrack, NativeAudioStreamTrack } = require("bindings")("media-devices-native");
import { VideoFrame } from "./types";

enum Kind {
  VIDEO = "video",
  AUDIO = "audio",
}

interface Options {
  kind: Kind;
}

export class MediaStreamTrack {
  public readonly kind: Kind;
  public _frameEmitter: EventEmitter;
  private _nativeTrack: any;

  public constructor(options: Options) {
    this.kind = options.kind;
    this._frameEmitter = new EventEmitter();
    if (this.kind === Kind.VIDEO) {
      this._nativeTrack = new NativeVideoStreamTrack();
    } else {
      this._nativeTrack = new NativeAudioStreamTrack();
    }
  }

  public startCapture() {
    this._nativeTrack.startCapture(
      (data: VideoFrame) => {
        this._frameEmitter.emit("frame", data);
      }
    );
  }

  public stopCapture() {
    this._nativeTrack.stopCapture();
  }
}
