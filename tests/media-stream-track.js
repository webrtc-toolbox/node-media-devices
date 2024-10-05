const { read } = require("fs");
const {
  MediaStreamTrack,
  MediaStreamTrackProcessor,
} = require("../dist/index");

const track = new MediaStreamTrack({ kind: "video" });
const processor = new MediaStreamTrackProcessor({ track });

const reader = processor.readable.getReader();

function processFrame() {
    reader.read().then(({ done, value }) => {
        if (done) {
            console.log('Stream ended');
            return;
        }
        const { frameData, width, height } = value;
        // Process the frame data
        console.log(value);
        // Continue reading frames
        processFrame();
    });
}

processFrame();

setTimeout(() => {
  track.stopCapture();
  console.log("stopped");
}, 3000);
