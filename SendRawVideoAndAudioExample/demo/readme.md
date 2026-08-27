# SendRawVideoAndAudioExample Demo

This demo injects prerecorded video through an external video source and PCM audio through the virtual microphone API after joining a meeting.

## Configure and build

From this directory:

```bash
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_SendRawVideoAndAudioDemo.sh
```

Enable `SendVideoRawData` and `SendAudioRawData` in `config.json`. The input media must match the formats expected by the source implementations. The build expects the extracted Linux Meeting SDK in `../../sdk/`.
