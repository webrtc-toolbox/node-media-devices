const {
  MediaStreamTrack,
  MediaStreamTrackProcessor,
  enumerateDevices
} = require("../dist/index");

async function main() {
    setTimeout(()=>{},10000);
  const result = await enumerateDevices();
  console.log(result);
}

main();

// const videoTrack = new MediaStreamTrack({ kind: "video" });
// const audioTrack = new MediaStreamTrack({ kind: "audio" });

// const videoProcessor = new MediaStreamTrackProcessor({ track: videoTrack });
// const audioProcessor = new MediaStreamTrackProcessor({ track: audioTrack });

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

// setTimeout(() => {
//   videoTrack.stopCapture();
//   audioTrack.stopCapture();
//   console.log("stopped");
// }, 2000);
