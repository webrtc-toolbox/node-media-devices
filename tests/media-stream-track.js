const {
    MediaStreamTrack,
    MediaStreamTrackProcessor,
    enumerateDevices
} = require("../dist/index");

async function main() {
    const result = await enumerateDevices();
    console.log(result);

    const videoTrack = new MediaStreamTrack({ kind: "video", constraints: { deviceId: 'default' } });
    console.log('videoTrack.label:', videoTrack.label);
    console.log('videoTrack.id:', videoTrack.id);

    const audioTrack = new MediaStreamTrack({ kind: "audio", constraints: { deviceId: 'default' } });
    console.log('audioTrack.label:', audioTrack.label);
    console.log('audioTrack.id:', audioTrack.id);

    // const videoProcessor = new MediaStreamTrackProcessor({ track: videoTrack });
    // const reader = videoProcessor.readable.getReader();

    const audioProcessor = new MediaStreamTrackProcessor({ track: audioTrack });
    const audioReader = audioProcessor.readable.getReader();

    function processFrame() {
        reader.read().then(({ done, value }) => {
            if (done) {
                console.log('Stream ended');
                return;
            }
            const { frameData, width, height } = value;
            // Process the frame data
            console.log(value);
            processFrame();
        });
    }

    function processAudioFrame() {
        audioReader.read().then(({ done, value }) => {
            if (done) {
                console.log('Audio Stream ended');
                return;
            }
            const { frameData, width, height } = value;
            // Process the frame data
            console.log(value);
            processAudioFrame();
        });
    }

    // processFrame();
    processAudioFrame();


    setTimeout(() => {
        audioTrack.enabled = false;
        console.log("stopped");
    }, 2000);

    setTimeout(() => {
        audioTrack.enabled = true;
        console.log("started");
    }, 8000);






    // const audioTrack = new MediaStreamTrack({ kind: "audio" });
    // console.log('audioTrack.label:', audioTrack.label);
    // console.log('audioTrack.id:', audioTrack.id);

    // // const audioTrack = new MediaStreamTrack({ kind: "audio" });

    // const videoProcessor = new MediaStreamTrackProcessor({ track: videoTrack });

    // for await (const chunk of videoProcessor.readable) {
    //     console.log(chunk);
    // }

    // const audioProcessor = new MediaStreamTrackProcessor({ track: audioTrack });

    // setTimeout(() => {
    //     videoTrack.stopCapture();
    //     // audioTrack.stopCapture();
    //     console.log("stopped");
    // }, 2000);
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

