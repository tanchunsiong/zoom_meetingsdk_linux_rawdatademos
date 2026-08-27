# Breakout Room Example

This sample demonstrates the Linux Meeting SDK breakout-room controller and its role-specific helpers. It reports breakout-room status and role changes, lists rooms and unassigned users, and can optionally:

See the [demo README](demo/readme.md) for the source-directory build details.

- create one breakout room
- assign the first available participant to the created room
- start all configured breakout rooms
- stop breakout rooms after a configured delay

All management actions are disabled by default. The joined user must receive the required creator/admin privileges, normally by being the meeting host or an eligible co-host.

## Configure

```bash
cd BreakoutRoomExample/demo
cp config.json.example config.json
```

Set `meeting_number`, `token`, and any required `meeting_password`. Then enable only the operations you want to test:

- `create_room_on_join` creates `breakout_room_name`.
- `assign_first_unassigned_user` assigns the first non-self unassigned participant to the newly created room.
- `start_breakout_rooms` starts the configured rooms after creation and assignment complete.
- `stop_breakout_rooms_after_seconds` stops active rooms after that delay; `0` disables automatic stopping.

The token must be a valid Meeting SDK JWT. Breakout rooms must also be enabled for the Zoom account and meeting.

## Build and run

Place the Linux Meeting SDK in the repository-level `sdk/` directory, then run:

```bash
cmake -S . -B build
cmake --build build
./bin/run_BreakoutRoomDemo.sh
```

See the [Zoom Linux Meeting SDK breakout-room documentation](https://developers.zoom.us/docs/meeting-sdk/linux/default-ui/advanced-features/breakout-rooms/) for the role and lifecycle concepts represented by this sample.
