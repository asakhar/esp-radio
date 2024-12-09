import { MediaRecorder, register } from 'extendable-media-recorder';
import { connect } from 'extendable-media-recorder-wav-encoder';
import RingBuffer from 'ringbufferjs';

const gateway = `ws://${window.location.hostname}/ws`;
/**
 * @type MediaRecorder
 */
let mediaRecorder = null;
let audioBlobs = [];
/**
 * @type RingBuffer<number>
 */
let sendBuffer = new RingBuffer(100000);
let capturedStream = null;
let wavAudioBlob = null;
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
    }
  })
  const audioContext = new AudioContext({ sampleRate: 8000 });
  const mediaStreamAudioSourceNode = new MediaStreamAudioSourceNode(audioContext, { mediaStream: stream });
  const mediaStreamAudioDestinationNode = new MediaStreamAudioDestinationNode(audioContext, { channelCount: 1 });

  mediaStreamAudioSourceNode.connect(mediaStreamAudioDestinationNode);

  audioBlobs = [];
  capturedStream = stream;

  // Use the extended MediaRecorder library
  mediaRecorder = new MediaRecorder(mediaStreamAudioDestinationNode.stream, {
    mimeType: 'audio/wav'
  });

  // Add audio blobs while recording 
  mediaRecorder.addEventListener('dataavailable', async (event) => {
    console.log(event.data);
    audioBlobs.push(event.data);
    const newData = new Uint8Array(await event.data.arrayBuffer());
    for (let i of newData.values()) {
      sendBuffer.enq(i);
    }
    sendToStream();
  });

  websocket = new WebSocket(gateway);
  websocket.binaryType = 'arraybuffer';
  websocket.onopen = () => {
    mediaRecorder.start(1000);
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

async function stopRecording() {
  new Promise(resolve => {
    if (!mediaRecorder) {
      resolve(null);
      return;
    }

    mediaRecorder.addEventListener('stop', () => {
      const mimeType = mediaRecorder.mimeType;
      const audioBlob = new Blob(audioBlobs, { type: mimeType });

      if (capturedStream) {
        capturedStream.getTracks().forEach(track => track.stop());
      }

      resolve(audioBlob);
      console.log('Closing websocket');
      websocket.close();
      requested = 0;
    });

    mediaRecorder.stop();
  });
}

function playAudio(audioBlob) {
  if (audioBlob) {
    const audio = document.querySelector('#audio')
    audio.src = URL.createObjectURL(audioBlob);
    audio.play();
  }
}
window.onload = async () => {
  await connectEncoder();
  document.querySelector('#btn-record').addEventListener('click', startRecording);
  document.querySelector('#btn-stop').addEventListener('click', async () => { wavAudioBlob = await stopRecording(); });
  document.querySelector('#btn-play').addEventListener('click', () => { playAudio(wavAudioBlob); });
}