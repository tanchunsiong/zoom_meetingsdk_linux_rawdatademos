# GetRawVideoAndAudioExample Demo

This demo joins a meeting and receives participant video and meeting audio through the Meeting SDK raw-data interfaces. It includes the listeners and renderers used by the parent example.

## Configure and build

From this directory:

```bash
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_GetRawVideoAndAudioDemo.sh
```

Enable `GetVideoRawData` and `GetAudioRawData` in `config.json`. Raw media capture requires the required meeting permissions. The build expects the extracted Linux Meeting SDK in `../../sdk/`.
