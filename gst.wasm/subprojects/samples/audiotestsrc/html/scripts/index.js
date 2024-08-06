/* Copyright (C) Fluendo S.A. <support@fluendo.com> */

var AudioData;
var AudioPlayer;

AudioPlayer = class {
  constructor(options) {
    // FIXME: EMBIND forbids extending Module.AudioPlayer.
    const pipeline_desc = document.getElementById("pipeline").value;
    this._audioDecoder = new Module.AudioPlayer(this, pipeline_desc);
    this.options = options;
  }

  onNewSample(audioData, nSamples) {
    if (!this.options.onNewSample) return;
    this.options.onNewSample(audioData, nSamples);
  }

  onError(msg, name) {
    if (!this.options.onError) return;

    const error = new DOMException(msg, name);
    this.options.onError(error);
  }

  close() {
    this._audioDecoder.close();
    // TODO?: this._audioDecoder.delete();
  }

  play() {
    this._audioDecoder.play();
  }
};

gst_init = () => {
  if (gst_initialized) return;
  const _init = Module.cwrap("init", "boolean");
  var gst_initialized = _init();
  return gst_initialized;
};

// FIXME: For non 8-bits width audios, a gap is heard between samples.
normalize = (val, format) => {
  // Documentation says values must be between -1.0 and 1.0
  if (format.startsWith("u8")) return (val - 128.0) / 256.0;
  else if (format.startsWith("s16")) return parseFloat(val) / 32768.0;
  else if (format.startsWith("s32")) return parseFloat(val) / 2147483648.0;
  else if (format.startsWith("f32")) return val;
  else return 0;
};

Module.onRuntimeInitialized = function () {
  if (!window.AudioData) {
    console.warn("AudioData not supported, defining dummy AudioData class.");
    AudioData = class {
      constructor(init) {
        this.format = init.format;
        this.sampleRate = init.sampleRate;
        this.numberOfFrames = init.numberOfFrames;
        this.numberOfChannels = init.numberOfChannels;
        this.timestamp = init.timestamp;
      }
    };
  }

  AudioData = class extends AudioData {
    constructor(init) {
      super(init);
      // FIXME: How to access native AudioData's data otherwise? this.copyTo?
      this.data = init.data;
    }
  };

  if (!gst_init()) throw new DOMException("", "NotSupportedError");

  const playButton = document.getElementById("play");
  playButton.onclick = () => {
    const audioContext = new AudioContext({ latencyHint: "playback" });
    let currentTime = 0;

    const player = new AudioPlayer({
      onNewSample: (audioData, nSamples) => {
        if (currentTime === 0)
          currentTime =
            audioContext.currentTime +
            audioContext.outputLatency +
            audioContext.baseLatency;

        let data = new Uint8Array(audioData.data);
        if (audioData.format.startsWith("u8"))
          data = new Uint8Array(data.buffer);
        else if (audioData.format.startsWith("s16"))
          data = new Int16Array(data.buffer);
        else if (audioData.format.startsWith("s32"))
          data = new Int32Array(data.buffer);
        else if (audioData.format.startsWith("f32"))
          data = new Float32Array(data.buffer);

        const buf = audioContext.createBuffer(
          audioData.numberOfChannels,
          nSamples,
          audioData.sampleRate,
        );

        if (audioData.format.includes("planar")) {
          let i = 0;
          for (let c = 0; c < audioData.numberOfChannels; c++) {
            channelData = buf.getChannelData(c);
            for (; i < nSamples * (c + 1); i++) {
              channelData[i] = normalize(data[i], audioData.format);
            }
          }
        } else {
          let channels = [];
          for (let c = 0; c < audioData.numberOfChannels; c++) {
            channelData = buf.getChannelData(c);
            channels.push(channelData);
          }

          for (let s = 0; s < nSamples; s++) {
            for (let c = 0; c < audioData.numberOfChannels; c++) {
              channels[c][s] = normalize(
                data[s * audioData.numberOfChannels + c],
                audioData.format,
              );
            }
          }
          console.log(channels);
        }

        const sourceNode = audioContext.createBufferSource();
        sourceNode.buffer = buf;

        sourceNode.connect(audioContext.destination);
        sourceNode.start(currentTime + audioData.timestamp / 1000000.0);
      },
    });

    player.play();
  };
};
