const {
    MediaStreamTrack,
    MediaStreamTrackProcessor,
    enumerateDevices
} = require("../dist/index");

async function main() {
    const result = await enumerateDevices();
    console.log(result);

    const videoTrack = new MediaStreamTrack({ kind: "video" });
    const audioTrack = new MediaStreamTrack({ kind: "audio" });

    const videoProcessor = new MediaStreamTrackProcessor({ track: videoTrack });
    const audioProcessor = new MediaStreamTrackProcessor({ track: audioTrack });

    setTimeout(() => {
        videoTrack.stopCapture();
        audioTrack.stopCapture();
        console.log("stopped");
    }, 2000);
}

main();


// // const reader = processor.readable.getReader();

// // function processFrame() {
// //     reader.read().then(({ done, value }) => {
// //         if (done) {
// //             console.log('Stream ended');
// //             return;
// //         }
// //         const { frameData, width, height } = value;
// //         // Process the frame data
// //         console.log(value);
// //         // Continue reading frames
// //         processFrame();
// //     });
// // }

// // processFrame();

