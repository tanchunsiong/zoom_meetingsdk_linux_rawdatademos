# Zoom Load Test Manager

Node/Express management website for the unified Docker load-test image:

- `dcr.asdc.cc/zoom-sendraw-loadtest-meeting:latest`

It uses Zoom Server-to-Server OAuth for REST API control-plane actions and the
local Docker CLI to start or kill Meeting SDK Linux containers. The browser UI
is optimized for load testing: each custCreate user row has inline resolve,
start, join, delete, and instance-count controls.

## What It Does

- Edit the ignored local `.env` from the browser page.
- Auto-fill fake email, first name, last name, and a Licensed user type for `custCreate` user creation.
- List selectable custCreate candidates from manager-created users, fake-domain users, and Zoom API/login type data when available.
- Resolve the selected user's PMI and PMI passcode from `GET /users/{userId}` and `GET /users/{userId}/settings`.
- Schedule a fallback meeting just before launch if Zoom does not expose an instant/PMI meeting ID for that user.
- Delete a custCreate user from Zoom with `DELETE /users/{userId}?action=delete` and remove it from the local manager list.
- Get a user's ZAK just in time when starting a meeting as host.
- Start or stop RTMS inline for running Docker containers with `PATCH /live_meetings/{meetingId}/rtms_app/status`.
- Fetch Meeting SDK JWT/signature from `https://nodejs.asdc.cc/meeting`.
- Start join containers from the selected user's instant meeting ID/passcode and runtime JWT with `MEETING_MODE=join`.
- Start host containers from the selected user's instant meeting ID, runtime JWT, and just-in-time ZAK with `MEETING_MODE=start`.
- Show ongoing Docker containers with meeting/user labels, then kill one container, join containers, start containers, or all load-test containers.

The RTMS start request body is:

```json
{
  "action": "start",
  "settings": {
    "client_id": "bnLICtNSlytlF35PKrpQ",
    "participant_user_id": "ZOOM_HOST_OR_ALTERNATIVE_HOST_USER_ID"
  }
}
```

The manager fills `participant_user_id` from the selected custCreate user or the
container label `zoom-loadtest.user-id`. If this field is omitted, Zoom falls
back to the OAuth token user, which can return error `2308` when that user is
not the meeting host or an alternative host.

## Setup

```bash
cd zoom_loadtest_manager
npm install
cp .env.example .env
```

Edit `.env`, or start the app and edit these values from the Environment card:

```bash
ZOOM_ACCOUNT_ID=
ZOOM_CLIENT_ID=
ZOOM_CLIENT_SECRET=

DOCKER_REGISTRY_URL=dcr.asdc.cc
DOCKER_REGISTRY_USERNAME=
DOCKER_REGISTRY_PASSWORD=
```

Do not commit `.env`. It is intentionally ignored.

## Run

```bash
npm start
```

Open:

```text
http://localhost:3090
```

## Required Zoom Scopes

Exact scope names depend on the app's scope model, but the manager uses these
operation families:

- Users: create/read users and read user token/ZAK.
- Meetings: read user PMI/settings and optionally create/read meetings through the retained API endpoints.
- RTMS: update participant RTMS app status.

From the installed Zoom skills, the relevant granular scopes include:

- `user:read:token` or `user:read:zak`
- `user:write:user` or corresponding admin user-create scope
- `meeting:write:meeting` / `meeting:read:meeting`
- `meeting:update:participant_rtms_app_status`

Verify exact scopes in the Zoom Marketplace/API Hub for the app type you use.

## Runtime Notes

The Docker images themselves do not fetch Meeting SDK JWTs. This manager fetches
the JWT/signature from `MEETING_TOKEN_ENDPOINT` and passes it to the containers
as `JWT_TOKEN`.

The inline launcher does not ask for meeting number, passcode, JWT, or ZAK. It
uses the custCreate user from that row:

- Join mode: resolves PMI/passcode or creates a scheduled fallback meeting, fetches Meeting SDK JWT with role `0`, and starts the unified image with `MEETING_MODE=join`.
- Start mode: resolves PMI or creates a scheduled fallback meeting, fetches Meeting SDK JWT with role `1`, fetches the selected user's ZAK, and starts the unified image with `MEETING_MODE=start`.

Zoom exposes the user's `type` as the plan type. The manager still shows it, but
it does not rely on plan type alone to identify custCreate users. It treats
manager-created users, users matching `CUSTCREATE_EMAIL_DOMAIN`, and API/login
type matches from Zoom as selectable custCreate candidates.

For join containers:

```bash
MEETING_MODE=join
MEETING_NUMBER=1234567890
MEETING_PASSWORD=passcode-if-required
JWT_TOKEN=meeting-sdk-jwt-or-signature
```

For start-meeting containers:

```bash
MEETING_MODE=start
MEETING_NUMBER=1234567890
JWT_TOKEN=meeting-sdk-jwt-or-signature
USER_ZAK=host-zak-token
```

The app invokes Docker directly on the host where it runs. The account running
Node must be allowed to run `docker`.
