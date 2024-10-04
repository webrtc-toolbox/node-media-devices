const { enumerateDevices, getUserMedia, MediaRecorder } = require('../dist/index.js');

// console.log(addon.hello()); // 'world'
async function main() {
    const stream = await getUserMedia();
    const recorder = new MediaRecorder(stream);
    recorder.start();
}

main();


