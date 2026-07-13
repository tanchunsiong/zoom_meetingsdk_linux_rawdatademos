# SendRawVideoAndAudioExample

This sample joins a meeting and injects prerecorded media through the Meeting SDK external video source and virtual microphone APIs.

## What it demonstrates

- Supplying decoded frames through an external video source.
- Supplying PCM audio through an external audio source and virtual microphone sender.
- Handling media-source initialization, start, and stop callbacks.
- Enabling injection with `SendVideoRawData` and `SendAudioRawData` in `demo/config.json`.

Configure external media after the sample is in the meeting. Input files must match the formats expected by the sample.

## Build and run

```bash
cd demo
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_SendRawVideoAndAudioDemo.sh
```

The sample uses the repository's shared `sdk/` directory. Update `demo/config.json` before running and see the [repository overview](../readme.md) for common setup.
