import { EventEmitter } from "node:events";
const { NativeMediaStreamTrack } = require("bindings")("media-devices-native");
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
    this._nativeTrack = new NativeMediaStreamTrack();
  }

  public startCapture() {
    this._nativeTrack.startCapture(
      (data: VideoFrame) => {
        this._frameEmitter.emit("frame", data);
      },
      () => {
        console.log("111111111111111111111111111111111111111111");
        this._frameEmitter.emit("end");
      },
    );
  }

  public stopCapture() {
    this._nativeTrack.stopCapture();
  }
}
