# GetRawShareExample Demo

This demo joins a meeting and subscribes to raw frames from an active screen-sharing source through the `RAW_DATA_TYPE_SHARE` renderer path.

## Configure and build

From this directory:

```bash
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_GetRawShareDemo.sh
```

Set `GetShareRawData` in `config.json` to enable the share capture path. Raw share capture requires the required meeting permissions. The build expects the extracted Linux Meeting SDK in `../../sdk/`.
