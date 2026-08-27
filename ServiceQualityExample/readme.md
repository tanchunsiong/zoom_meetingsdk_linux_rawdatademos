# Service Quality Example

This sample joins a Zoom meeting and demonstrates the Linux Meeting SDK service-quality APIs. It:

See the [demo README](demo/readme.md) for the source-directory build details.

- receives per-user network quality changes from `onUserNetworkStatusChanged`
- receives SDK quality warnings from `onMeetingStatisticsWarningNotification`
- polls the current audio, video, and share connection quality
- polls audio, video, and share bandwidth, latency, jitter, packet-loss, frame-rate, and resolution statistics

Statistics are printed after the sample reaches `MEETING_STATUS_INMEETING`. Some values remain zero when the corresponding media channel is inactive.

## Configure

```bash
cd ServiceQualityExample/demo
cp config.json.example config.json
```

Set `meeting_number`, `token`, and any required `meeting_password` in `config.json`. The token must be a valid Meeting SDK JWT.

## Build and run

Place the Linux Meeting SDK in the repository-level `sdk/` directory, then run:

```bash
cmake -S . -B build
cmake --build build
./bin/run_ServiceQualityDemo.sh
```

See the [Zoom Linux Meeting SDK service-quality documentation](https://developers.zoom.us/docs/meeting-sdk/linux/service-quality/) for the API concepts represented by this sample.
