# AllInOneExample Demo

This is the buildable demo directory for `AllInOneExample`. It combines Meeting SDK authentication and meeting join with chat, participant events, raw audio/video receive, and external audio/video send paths.

## Configure and build

From this directory:

```bash
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_AllInOneDemo.sh
```

The build expects the extracted Linux Meeting SDK in the repository-level `../../sdk/` directory. Configure the raw-data flags in `config.json`; raw-data capture requires the required meeting permissions.
