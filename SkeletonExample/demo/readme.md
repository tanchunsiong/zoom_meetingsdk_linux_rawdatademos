# SkeletonExample Demo

This is the minimal buildable Meeting SDK sample. It initializes the SDK, authenticates with a Meeting SDK JWT, joins without a Zoom user login, and handles basic meeting and participant lifecycle events.

## Configure and build

From this directory:

```bash
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_SkeletonDemo.sh
```

The build expects the extracted Linux Meeting SDK in the repository-level `../../sdk/` directory. If a separate join token is required, set `join_token` in the ignored local `config.json`. Use this demo as the starting point when raw-media features are not required.
