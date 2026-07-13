# GetRawVideoAndAudioExample

This sample joins a meeting and receives participant video and meeting audio through the Meeting SDK raw-data interfaces.

## What it demonstrates

- Subscribing to participant video with a raw-data renderer.
- Receiving meeting audio through the audio raw-data helper.
- Processing raw media for file or FFmpeg-based output.
- Enabling capture with `GetVideoRawData` and `GetAudioRawData` in `demo/config.json`.

Raw media capture requires host, co-host, or local-recording permission.

## Build and run

```bash
cd demo
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_GetRawVideoAndAudioDemo.sh
```

The sample uses the repository's shared `sdk/` directory. Update `demo/config.json` before running and see the [repository overview](../readme.md) for common setup.
