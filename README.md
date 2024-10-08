# node-media-devices
**node-media-devices** provides Node.js native bindings for camera and microphone capturing, and follows *(mostly)* the Web standard media capturing APIs such as `MediaStreamTrack`, `MediaStreamTrackProcessor`, etc.

## supported platforms
- [x] macOS
- [x] Windows
- [ ] Linux (WIP)

## usage

```javascript
const { MediaStreamTrack, MediaStreamTrackProcessor } = require('node-media-devices');

const track = new MediaStreamTrack({ kind: 'video' });
const processor = new MediaStreamTrackProcessor({ track });

for await (const videoFrame of processor.readable) {
    console.log(videoFrame.data);
    console.log(videoFrame.timestamp);
}
```