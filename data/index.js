/**
 * @type MediaRecorder
 */
var recorder;
/**
 * @type HTMLAudioElement
 */
var audio;
var isPaused = true;

/**
 * 
 * @param {BlobEvent} ev 
 */
async function onDataAvailable(ev) {
  console.log(ev.data);
  var data = await ev.data.arrayBuffer();
  for (let i = 0; i < Math.ceil(data.byteLength / 1536); ++i) {
    let tmp = data.slice(i * 1536, Math.min((i + 1) * 1536, data.byteLength));
    fetch(`http://${window.location.hostname}/audio`, {
      method: "POST",
      body: tmp
    });
  }
}

async function onPlayClick() {
  if (recorder) {
    if (isPaused) {
      recorder.resume();
    } else {
      recorder.pause();
    }
    return;
  }
  const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
  recorder = new MediaRecorder(stream, {
    mimeType: 'audio/mp4',
    audioBitsPerSecond: 32000,
    videoBitsPerSecond: 0,
    audioBitrateMode: 'constant'
  });
  recorder.ondataavailable = onDataAvailable;
  recorder.onpause = () => isPaused = true;
  recorder.onresume = () => isPaused = false;
  recorder.start(20);
  recorder.pause();
}