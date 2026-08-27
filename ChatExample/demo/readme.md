# ChatExample Demo

This is the buildable demo directory for `ChatExample`. It authenticates with the Meeting SDK, joins a meeting without a Zoom user login, and demonstrates meeting lifecycle callbacks and in-meeting chat notifications.

## Configure and build

From this directory:

```bash
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_ChatDemo.sh
```

The build expects the extracted Linux Meeting SDK in the repository-level `../../sdk/` directory.
