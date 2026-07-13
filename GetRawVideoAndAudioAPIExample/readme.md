# GetRawVideoAndAudioAPIExample

This sample receives meeting media and connects the audio workflow to a Python service for transcription and generated responses.

## What it demonstrates

- Receiving participant video and meeting audio through raw-data interfaces.
- Exchanging data with `demo/pythonserver.py` over local UDP ports.
- Using Deepgram for transcription, Cerebras for response generation, and Cartesia for text-to-speech.
- Enabling capture with `GetVideoRawData` and `GetAudioRawData` in `demo/config.json`.

Set `DEEPGRAM_API_KEY`, `CEREBRAS_API_KEY`, and `CARTESIA_API_KEY` in the Python service's environment. Do not store credentials in source control. Raw capture requires host, co-host, or local-recording permission.

## Build and run

```bash
cd demo
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_GetRawVideoAndAudioAPIDemo.sh
```

The sample uses the repository's shared `sdk/` directory. Install the dependencies imported by `demo/pythonserver.py`, update `demo/config.json`, and see the [repository overview](../readme.md) for common setup.
