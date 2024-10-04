import { MediaStream } from "./media-stream";
import { EventEmitter } from 'events';
const mediaDeviceNative = require('bindings')('media-devices-native');

export enum MediaRecorderState {
    inactive,
    recording,
    paused
};

export class MediaRecorder extends EventEmitter {
    private stream: MediaStream;
    private state: MediaRecorderState = MediaRecorderState.inactive;
    private nativeMediaRecorder;

    public constructor(stream: MediaStream) {
        super();
        this.nativeMediaRecorder = new mediaDeviceNative.NativeMediaRecorder(this.onData);
        this.stream = stream;

        const result = this.nativeMediaRecorder.addInputDevice(stream.nativeMediaStream);
        console.log('result: ', result);
    }

    public start() {
        if (this.state === MediaRecorderState.recording) {
            return;
        }

        this.stream.on('data', (data: Buffer) => {
            this.emit('dataavailable', data);
        });

        console.log(this.nativeMediaRecorder.requestCapture());
    }

    private onData = (buffer: any) => {
        console.log(buffer);
    }
}