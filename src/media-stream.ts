import { EventEmitter } from 'events';
import { Constraints } from './types';
const mediaDeviceNative = require('bindings')('media-devices-native');

interface NativeMediaStream {
    capture(): void;
}

export class MediaStream extends EventEmitter {
    public nativeMediaStream: NativeMediaStream;

    public constructor(constraints: Constraints) {
        super();
        this.nativeMediaStream = new mediaDeviceNative.NativeMediaStream();
    }

    public requestCapture() {
        this.nativeMediaStream.capture();
    }
}