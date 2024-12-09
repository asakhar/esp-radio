#include "AudioGeneratorWavNonBlock.hpp"

AudioGeneratorWavNonBlock::AudioGeneratorWavNonBlock() {
  running = false;
  file = NULL;
  output = NULL;
}

bool AudioGeneratorWavNonBlock::stop() {
  if (!running)
    return true;
  running = false;
  buffer = {};
  output->stop();
  return file->close();
}

bool AudioGeneratorWavNonBlock::isRunning() { return running; }

// Handle buffered reading, reload each time we run out of data
bool AudioGeneratorWavNonBlock::GetBufferedData(size_t bytes, void *dest) {
  if (!running)
    return false; // Nothing to do here!
  if (bytes > buffer.bytesUsed()) {
    auto avail = buffer.bytesFree();
    auto read = file->readNonBlock(scratch, avail);
    // Serial.printf("Read in %u expected %u\n", read, avail);
    buffer.memcpyIn(scratch, read);
  }
  if (bytes <= buffer.bytesUsed()) {
    auto read = buffer.memcpyOut(dest, bytes);
    assert(read == bytes);
    return true;
  }
  return false;
}

bool AudioGeneratorWavNonBlock::loop() {
  bool filled = false;
  if (!running)
    goto done; // Nothing to do here!
  
  // Try and stuff the buffer one sample at a time
  do {
    if (bitsPerSample == 8) {
      if (channels == 1) {
        // TODO
      } else {
        // TODO
      }
    } else if (bitsPerSample == 16) {
      if (channels == 1) {
        if (!GetBufferedData(2, &lastSample[AudioOutput::LEFTCHANNEL]))
          break;
        lastSample[AudioOutput::RIGHTCHANNEL] = 0;
      } else {
        // TODO
      }
    }
    filled = !output->ConsumeSample(lastSample);
  } while (running && !filled);

done:
  file->loop();
  if (filled) {
    output->loop();
  }

  return running;
}

// bool AudioGeneratorWavNonBlock::loop() {
//   if (!running)
//     goto done; // Nothing to do here!

//   // First, try and push in the stored sample.  If we can't, then punt and
//   try
//   // later
//   if (!output->ConsumeSample(lastSample))
//     goto done; // Can't send, but no error detected

//   // Try and stuff the buffer one sample at a time
//   do {
//     if (bitsPerSample == 8) {
//       uint8_t l, r;
//       if (!GetBufferedData(1, &l))
//         stop();
//       if (channels == 2) {
//         if (!GetBufferedData(1, &r))
//           stop();
//       } else {
//         r = 0;
//       }
//       lastSample[AudioOutput::LEFTCHANNEL] = l;
//       lastSample[AudioOutput::RIGHTCHANNEL] = r;
//     } else if (bitsPerSample == 16) {
//       if (!GetBufferedData(2, &lastSample[AudioOutput::LEFTCHANNEL]))
//         stop();
//       if (channels == 2) {
//         if (!GetBufferedData(2, &lastSample[AudioOutput::RIGHTCHANNEL]))
//           stop();
//       } else {
//         lastSample[AudioOutput::RIGHTCHANNEL] = 0;
//       }
//     }
//   } while (running && output->ConsumeSample(lastSample));

// done:
//   file->loop();
//   output->loop();

//   return running;
// }

bool AudioGeneratorWavNonBlock::ReadWAVInfo() {
  uint32_t u32;
  uint16_t u16;
  int toSkip;

  // WAV specification document:
  // https://www.aelius.com/njh/wavemetatools/doc/riffmci.pdf

  // Header == "RIFF"
  if (!ReadU32(&u32)) {
    Serial.printf_P(PSTR(
        "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
    return false;
  };
  if (u32 != 0x46464952) {
    Serial.printf_P(PSTR("AudioGeneratorWavNonBlock::ReadWAVInfo: cannot read "
                         "WAV, invalid RIFF header\n"));
    return false;
  }

  // Skip ChunkSize
  if (!ReadU32(&u32)) {
    Serial.printf_P(PSTR(
        "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
    return false;
  };

  // Format == "WAVE"
  if (!ReadU32(&u32)) {
    Serial.printf_P(PSTR(
        "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
    return false;
  };
  if (u32 != 0x45564157) {
    Serial.printf_P(PSTR("AudioGeneratorWavNonBlock::ReadWAVInfo: cannot read "
                         "WAV, invalid WAVE header\n"));
    return false;
  }

  // there might be JUNK or PAD - ignore it by continuing reading until we get
  // to "fmt "
  while (1) {
    if (!ReadU32(&u32)) {
      Serial.printf_P(PSTR(
          "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
      return false;
    };
    if (u32 == 0x20746d66)
      break; // 'fmt '
  };

  // subchunk size
  if (!ReadU32(&u32)) {
    Serial.printf_P(PSTR(
        "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
    return false;
  };
  if (u32 == 16) {
    toSkip = 0;
  } else if (u32 == 18) {
    toSkip = 18 - 16;
  } else if (u32 == 40) {
    toSkip = 40 - 16;
  } else {
    Serial.printf_P(PSTR("AudioGeneratorWavNonBlock::ReadWAVInfo: cannot read "
                         "WAV, appears not to be standard PCM \n"));
    return false;
  } // we only do standard PCM

  // AudioFormat
  if (!ReadU16(&u16)) {
    Serial.printf_P(PSTR(
        "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
    return false;
  };
  if (u16 != 1) {
    Serial.printf_P(PSTR("AudioGeneratorWavNonBlock::ReadWAVInfo: cannot read "
                         "WAV, AudioFormat appears not to be standard PCM \n"));
    return false;
  } // we only do standard PCM

  // NumChannels
  if (!ReadU16(&channels)) {
    Serial.printf_P(PSTR(
        "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
    return false;
  };
  if ((channels < 1) || (channels > 2)) {
    Serial.printf_P(PSTR("AudioGeneratorWavNonBlock::ReadWAVInfo: cannot read "
                         "WAV, only mono and stereo are supported \n"));
    return false;
  } // Mono or stereo support only

  // SampleRate
  if (!ReadU32(&sampleRate)) {
    Serial.printf_P(PSTR(
        "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
    return false;
  };
  if (sampleRate < 1) {
    Serial.printf_P(PSTR("AudioGeneratorWavNonBlock::ReadWAVInfo: cannot read "
                         "WAV, unknown sample rate \n"));
    return false;
  } // Weird rate, punt.  Will need to check w/DAC to see if supported

  // Ignore byterate and blockalign
  if (!ReadU32(&u32)) {
    Serial.printf_P(PSTR(
        "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
    return false;
  };
  if (!ReadU16(&u16)) {
    Serial.printf_P(PSTR(
        "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
    return false;
  };

  // Bits per sample
  if (!ReadU16(&bitsPerSample)) {
    Serial.printf_P(PSTR(
        "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
    return false;
  };
  if ((bitsPerSample != 8) && (bitsPerSample != 16)) {
    Serial.printf_P(PSTR("AudioGeneratorWavNonBlock::ReadWAVInfo: cannot read "
                         "WAV, only 8 or 16 bits is supported \n"));
    return false;
  } // Only 8 or 16 bits

  // Skip any extra header
  while (toSkip) {
    uint8_t ign;
    if (!ReadU8(&ign)) {
      Serial.printf_P(PSTR(
          "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
      return false;
    };
    toSkip--;
  }

  // look for data subchunk
  do {
    // id == "data"
    if (!ReadU32(&u32)) {
      Serial.printf_P(PSTR(
          "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
      return false;
    };
    if (u32 == 0x61746164)
      break; // "data"
    // Skip size, read until end of chunk
    if (!ReadU32(&u32)) {
      Serial.printf_P(PSTR(
          "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
      return false;
    };
    if (!file->seek(u32, SEEK_CUR)) {
      Serial.printf_P(PSTR("AudioGeneratorWavNonBlock::ReadWAVInfo: failed to "
                           "read WAV data, seek failed\n"));
      return false;
    }
  } while (1);
  if (!file->isOpen()) {
    Serial.printf_P(PSTR("AudioGeneratorWavNonBlock::ReadWAVInfo: cannot read "
                         "WAV, file is not open\n"));
    return false;
  };

  // Skip size, read until end of file...
  if (!ReadU32(&u32)) {
    Serial.printf_P(PSTR(
        "AudioGeneratorWavNonBlock::ReadWAVInfo: failed to read WAV data\n"));
    return false;
  };
  availBytes = u32;
  return true;
}

bool AudioGeneratorWavNonBlock::begin(AudioFileSource *source,
                                      AudioOutput *output) {
  if (!source) {
    Serial.printf_P(
        PSTR("AudioGeneratorWavNonBlock::begin: failed: invalid source\n"));
    return false;
  }
  file = source;
  if (!output) {
    Serial.printf_P(PSTR("AudioGeneratorWavNonBlock::begin: invalid output\n"));
    return false;
  }
  this->output = output;
  if (!file->isOpen()) {
    Serial.printf_P(PSTR("AudioGeneratorWavNonBlock::begin: file not open\n"));
    return false;
  } // Error

  if (!ReadWAVInfo()) {
    Serial.printf_P(
        PSTR("AudioGeneratorWavNonBlock::begin: failed during ReadWAVInfo\n"));
    return false;
  }
  Serial.printf("sampleRate=%u, bitsPerSample=%u, channels=%u\n", sampleRate,
                bitsPerSample, channels);
  if (!output->SetRate(sampleRate)) {
    Serial.printf_P(PSTR(
        "AudioGeneratorWavNonBlock::begin: failed to SetRate in output\n"));
    return false;
  }
  if (!output->SetBitsPerSample(bitsPerSample)) {
    Serial.printf_P(PSTR("AudioGeneratorWavNonBlock::begin: failed to "
                         "SetBitsPerSample in output\n"));
    return false;
  }
  if (!output->SetChannels(channels)) {
    Serial.printf_P(PSTR(
        "AudioGeneratorWavNonBlock::begin: failed to SetChannels in output\n"));
    return false;
  }
  if (!output->begin()) {
    Serial.printf_P(PSTR("AudioGeneratorWavNonBlock::begin: output's begin did "
                         "not return true\n"));
    return false;
  }

  running = true;

  return true;
}
