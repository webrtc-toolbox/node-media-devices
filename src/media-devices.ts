import { MediaStream } from "./media-stream";
import { Constraints } from "./types";

const mediaDeviceNative = require('bindings')('media-devices-native');

export function enumerateDevices() {
    return mediaDeviceNative.enumerateDevices();
}

export async function getUserMedia(constraints: Constraints) {
    return new MediaStream(constraints);
}