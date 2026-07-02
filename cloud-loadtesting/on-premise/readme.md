# Zoom Meeting SDK Load Test - On Premise

This is the local/on-premise load-testing copy. It keeps only the unified
Meeting SDK runner and the local manager UI/API.

## Local Architecture

```text
Operator browser
  |
  v
Node/Express manager
  |-- Zoom APIs: users, meetings, ZAK, RTMS
  |-- token endpoint for Meeting SDK JWT/signature
  `-- local Docker CLI
        |
        `-- one-off Meeting SDK runner containers
              |
              `-- outbound connection to Zoom meeting
```

The runner scales to zero by using disposable Docker containers. The manager
starts containers only when requested and removes them when killed.

## Folders

- `zoom_sendraw_loadtest-meeting`: unified start/join Meeting SDK runner image.
- `zoom_loadtest_manager`: local management API and static UI.

## Build

Configure this on-premise subset directly:

```bash
cmake -S . -B build
cmake --build build
```

From the repository root, the same runner is also included through the root
`CMakeLists.txt`.

By default, the on-premise wrapper looks for the Meeting SDK under the repository
root `sdk/` folder. If your SDK is elsewhere, pass it explicitly:

```bash
cmake -S . -B build -DZOOM_MEETING_SDK_DIR=/path/to/meeting-sdk
```

## Run The Manager

```bash
cd zoom_loadtest_manager
npm install
cp .env.example .env
npm start
```

Open:

```text
http://localhost:3090
```

The manager edits the ignored local `.env`, uses Zoom Server-to-Server OAuth for
control-plane actions, fetches Meeting SDK JWT/signature from the configured
token endpoint, and launches the unified Docker image through the local Docker
CLI.

## Sensitive Files

Do not commit local credentials, generated media, SDK binaries, dependency
folders, or local key material. The repo `.gitignore` excludes local env files,
media outputs, node modules, and key/cert files.
