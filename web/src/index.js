import { MediaRecorder, register } from 'extendable-media-recorder';
import { connect } from 'extendable-media-recorder-wav-encoder';
import RingBuffer from 'ringbufferjs';

const gateway = `ws://${window.location.hostname}/ws`;
/**
 * @type MediaRecorder
 */
let mediaRecorder = null;
/**
 * @type RingBuffer<number>
 */
let sendBuffer = new RingBuffer(100000);
let requested = 0;
/**
 * @type WebSocket
 */
let websocket = null;

// Register the extendable-media-recorder-wav-encoder
async function connectEncoder() {
  await register(await connect());
}

function sendToStream() {
  if (!websocket || websocket.readyState != websocket.OPEN) {
    return;
  }
  let tosend = Math.min(requested, sendBuffer.size());
  if (tosend <= 0) {
    return;
  }
  const data = new Uint8Array(sendBuffer.deqN(tosend));
  console.log('Sending: size=', data.length, data);
  websocket.send(data);
  requested -= tosend;
}

// Starts recording audio
async function startRecording() {
  const stream = await navigator.mediaDevices.getUserMedia({
    audio: {
      echoCancellation: true,
    },
    video: false
  });

  const sampleRate = 44100;

  const audioContext = new AudioContext({ sampleRate });
  const audioStreamSource = audioContext.createMediaStreamSource(stream);
  const mediaStreamAudioDestinationNode = audioContext.createMediaStreamDestination();
  mediaStreamAudioDestinationNode.channelCount = 1;
  audioStreamSource.connect(mediaStreamAudioDestinationNode);

  // Use the extended MediaRecorder library
  mediaRecorder = new MediaRecorder(mediaStreamAudioDestinationNode.stream, {
    mimeType: 'audio/wav',
    audioBitsPerSecond: sampleRate * 16,
  });

  // Add audio blobs while recording 
  mediaRecorder.addEventListener('dataavailable', async (event) => {
    // console.log(event.data);
    // audioBlobs.push(event.data);
    const newData = new Uint8Array(await event.data.arrayBuffer());
    for (let i of newData.values()) {
      sendBuffer.enq(i);
    }
    sendToStream();
  });

  websocket = new WebSocket(gateway);
  websocket.binaryType = 'arraybuffer';
  websocket.onopen = () => {
    mediaRecorder.start(10);
    // websocket.send(new ArrayBuffer([1, 2, 3, 4]));// TODO: remove
  };
  websocket.onclose = () => {
    console.log('websocket on close');
    // TODO: return
    // mediaRecorder.pause(); 
  };
  websocket.onmessage = (ev) => {
    console.log('Request received: ', ev.data);
    if (typeof ev.data === 'string') {
      let req = parseInt(ev.data, 10);
      requested += req || 0;
      console.log(`onrequest send req=${req}, total=${requested}`);
      sendToStream();
    }
  };
}

function stopRecording() {
  if (!mediaRecorder) {
    return;
  }
  mediaRecorder.addEventListener('stop', () => {
    console.log('Closing websocket');
    requested = 0;
    websocket.close();
  });
  mediaRecorder.stop();
}

// function playAudio(audioBlob) {
//   if (audioBlob) {
//     const audio = document.querySelector('#audio')
//     audio.src = URL.createObjectURL(audioBlob);
//   }
//   audio.play();
// }

window.onload = async () => {
  await connectEncoder();
  document.querySelector('#btn-record').addEventListener('click', startRecording);
  document.querySelector('#btn-stop').addEventListener('click', stopRecording);
  // document.querySelector('#btn-play').addEventListener('click', () => { playAudio(wavAudioBlob); });
}