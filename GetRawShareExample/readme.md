# GetRawShareExample

This sample joins a meeting and subscribes to raw frames from an active screen-sharing source.

## What it demonstrates

- Detecting when a participant starts or stops sharing.
- Subscribing with a raw-data renderer and `RAW_DATA_TYPE_SHARE`.
- Receiving shared-screen frames in I420 format.
- Enabling capture with `GetShareRawData` in `demo/config.json`.

Raw share capture requires host, co-host, or local-recording permission.

## Build and run

```bash
cd demo
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_GetRawShareDemo.sh
```

The sample uses the repository's shared `sdk/` directory. Update `demo/config.json` before running and see the [repository overview](../readme.md) for common setup.
