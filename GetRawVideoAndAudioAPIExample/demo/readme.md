# GetRawVideoAndAudioAPIExample Demo

This demo receives raw participant video and meeting audio, then exchanges audio data with the included `pythonserver.py` for transcription, response generation, and text-to-speech workflows.

## Configure and build

From this directory:

```bash
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_GetRawVideoAndAudioAPIDemo.sh
```

Set `DEEPGRAM_API_KEY`, `CEREBRAS_API_KEY`, and `CARTESIA_API_KEY` in the Python service environment, never in source control. Enable the required raw-data flags in `config.json`. The build expects the extracted Linux Meeting SDK in `../../sdk/`.
