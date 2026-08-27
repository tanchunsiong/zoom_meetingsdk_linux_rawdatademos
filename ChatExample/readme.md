# ChatExample

This sample authenticates with the Meeting SDK, joins a meeting, and demonstrates in-meeting chat messaging.

See the [demo README](demo/readme.md) for the source-directory build details.

## What it demonstrates

- Authentication and meeting join without a Zoom login.
- Meeting and participant lifecycle callbacks.
- Chat service initialization and message notifications.

## Build and run

```bash
cd demo
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_ChatDemo.sh
```

The sample uses the repository's shared `sdk/` directory. Update `demo/config.json` before running and see the [repository overview](../readme.md) for common setup.
