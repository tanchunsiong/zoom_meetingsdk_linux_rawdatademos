# SkeletonExample

This is the minimal baseline sample for authenticating with the Meeting SDK and joining a meeting without a Zoom user login.

See the [demo README](demo/readme.md) for the source-directory build details.

## What it demonstrates

- Initializing and cleaning up the Linux Meeting SDK.
- Authenticating with an SDK JWT.
- Joining with `SDK_UT_WITHOUT_LOGIN`.
- Handling basic meeting and participant lifecycle callbacks.

Use this project as a starting point for an integration that does not need the raw-media features in the other examples.

## Build and run

```bash
cd demo
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_SkeletonDemo.sh
```

The sample uses the repository's shared `sdk/` directory. Update `demo/config.json` before running. If the meeting requires a separate join token, set `join_token` in the ignored local config; it is blank in `config.json.example`. See the [repository overview](../readme.md) for common setup.
