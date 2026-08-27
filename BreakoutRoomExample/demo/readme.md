# BreakoutRoomExample Demo

This demo joins a meeting and exercises the Linux Meeting SDK breakout-room controller. It reports role, room, assignment, and lifecycle events and can optionally create a room, assign a participant, start rooms, and stop them after a delay.

## Configure and build

From this directory:

```bash
cp config.json.example config.json
cmake -B build
cmake --build build -j
./bin/run_BreakoutRoomDemo.sh
```

Management actions are disabled by default. Enable them in `config.json` only when the joined user has the required creator/admin privileges and breakout rooms are enabled for the meeting. The build expects the extracted SDK in `../../sdk/`.
