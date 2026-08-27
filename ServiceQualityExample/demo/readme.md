# ServiceQualityExample Demo

This demo joins a meeting and reports service quality through connection-quality queries, periodic audio/video/share statistics, per-user network status callbacks, and meeting statistics warnings.

## Configure and build

From this directory:

```bash
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_ServiceQualityDemo.sh
```

Set the meeting number, JWT, and polling interval in `config.json`. Statistics may remain zero while a media channel is inactive. The build expects the extracted Linux Meeting SDK in `../../sdk/`.
