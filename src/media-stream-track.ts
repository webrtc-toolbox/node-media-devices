/// <reference lib="dom" />
import { EventEmitter } from "node:events";
const { NativeVideoStreamTrack, NativeAudioStreamTrack } = require("bindings")("media-devices-native");
import { VideoFrame } from "./types";

enum Kind {
  VIDEO = "video",
  AUDIO = "audio",
}

interface Options {
  kind: Kind;
  constraints?: MediaTrackConstraints;
}

export class MediaStreamTrack {
  public readonly kind: Kind;
  public _frameEmitter: EventEmitter;
  private _nativeTrack: any;
  private _enabled: boolean = true;

  public get label() {
    return this._nativeTrack.getLabel();
  }

  public get id() {
    return this._nativeTrack.getId();
  }

  public get enabled() {
    return this._enabled;
  }

  public set enabled(value: boolean) {
    if (this._enabled === value) {
      return;
    }

    if (value) {
      this.startCapture();
    } else {
      this.stopCapture();
    }

    this._enabled = value;
  }

  public constructor(options: Options) {
    this.kind = options.kind;
    this._frameEmitter = new EventEmitter();

    const constraints = {
      deviceId: options.constraints?.deviceId || "default"
    };

    if (this.kind === Kind.VIDEO) {
      this._nativeTrack = new NativeVideoStreamTrack(constraints);
    } else {
      this._nativeTrack = new NativeAudioStreamTrack(constraints);
    }
  }

  public startCapture() {
    this._nativeTrack.startCapture(this.onFrame);
  }

  public stopCapture() {
    this._nativeTrack.stopCapture();
  }

  private onFrame = (data: VideoFrame) => {
    this._frameEmitter.emit("frame", data);
  }
}
