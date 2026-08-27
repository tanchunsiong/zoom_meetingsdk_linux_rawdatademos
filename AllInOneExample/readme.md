# AllInOneExample

This sample combines Meeting SDK authentication, meeting join, chat, raw audio/video capture, and external audio/video injection in one application.

See the [demo README](demo/readme.md) for the source-directory build details.

## What it demonstrates

- Receiving participant video and meeting audio as raw data.
- Sending video through an external video source and audio through a virtual microphone.
- Handling meeting, participant, recording-permission, and chat events.
- Selecting workflows with `GetVideoRawData`, `GetAudioRawData`, `SendVideoRawData`, and `SendAudioRawData` in `demo/config.json`.

Raw-data capture requires host, co-host, or local-recording permission.

## Build and run

```bash
cd demo
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_AllInOneDemo.sh
```

The sample uses the repository's shared `sdk/` directory. Update `demo/config.json` before running and see the [repository overview](../readme.md) for common setup.
