import express from 'express';
import crypto from 'node:crypto';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { config, envSnapshot, publicStatus, saveEnv } from './config.js';
import { HttpError, asPositiveInteger, requireField } from './errors.js';
import {
  createCustCreateUser,
  deleteUser,
  getMeeting,
  getS2SAccessToken,
  getUser,
  getZakToken,
  listUsers,
  resolveUserInstantMeeting,
  scheduleMeeting,
  updateRtmsStatus
} from './zoom.js';
import { dockerLogin, killContainers, listContainers, startContainers } from './docker.js';
import { getMeetingSdkToken } from './token-service.js';
import { addManagedUser, findManagedUser, listManagedUsers, removeManagedUser, suggestManagedUser, updateManagedUser } from './store.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const app = express();

app.use(express.json({ limit: '1mb' }));
app.use(express.static(path.join(__dirname, '..', 'public')));

function secureEqual(actual, expected) {
  const actualBuffer = Buffer.from(String(actual));
  const expectedBuffer = Buffer.from(String(expected));
  return actualBuffer.length === expectedBuffer.length && crypto.timingSafeEqual(actualBuffer, expectedBuffer);
}

function basicCredentials(header = '') {
  if (!header.startsWith('Basic ')) return null;
  try {
    const decoded = Buffer.from(header.slice(6), 'base64').toString('utf8');
    const separator = decoded.indexOf(':');
    if (separator === -1) return null;
    return {
      username: decoded.slice(0, separator),
      password: decoded.slice(separator + 1)
    };
  } catch {
    return null;
  }
}

app.use('/api', (req, res, next) => {
  if (req.path === '/zoom/rtms/webhook') {
    next();
    return;
  }

  const expectedUsername = process.env.MANAGER_AUTH_USERNAME || 'admin';
  const expectedPassword = process.env.MANAGER_AUTH_PASSWORD || 'admin';
  const credentials = basicCredentials(req.get('authorization'));
  if (
    credentials &&
    secureEqual(credentials.username, expectedUsername) &&
    secureEqual(credentials.password, expectedPassword)
  ) {
    next();
    return;
  }

  res.set('WWW-Authenticate', 'Basic realm="Zoom Load Test Manager", charset="UTF-8"');
  res.status(401).json({ error: 'Authentication required' });
});

function asyncRoute(handler) {
  return async (req, res, next) => {
    try {
      await handler(req, res);
    } catch (error) {
      next(error);
    }
  };
}

function tokenPreview(value) {
  if (!value) return '';
  if (value.length <= 16) return '[set]';
  return `${value.slice(0, 8)}...${value.slice(-6)}`;
}

const rtmsConfirmDelayMs = 60_000;
const rtmsByContainer = new Map();
const rtmsByMeetingUser = new Map();

function normalizedId(value) {
  return String(value || '').trim();
}

function meetingUserKey(meetingId, participantUserId = '') {
  return `${normalizedId(meetingId)}::${normalizedId(participantUserId)}`;
}

function isSameMeetingUser(record, meetingId, participantUserId = '') {
  if (normalizedId(record.meetingId) !== normalizedId(meetingId)) return false;
  return !participantUserId || !record.participantUserId || normalizedId(record.participantUserId) === normalizedId(participantUserId);
}

function refreshRtmsRecord(record) {
  if (!record) return null;
  const deadline = record.confirmationDeadlineAt ? Date.parse(record.confirmationDeadlineAt) : 0;
  if (record.status === 'start_requested' && deadline && deadline <= Date.now()) {
    record.status = 'not_confirmed';
    record.lastUpdatedAt = new Date().toISOString();
    record.message = 'No meeting.rtms_started webhook was observed within 60 seconds of the start request.';
  }
  return record;
}

function rememberRtmsRecord(patch) {
  const containerId = normalizedId(patch.containerId);
  const meetingId = normalizedId(patch.meetingId);
  const participantUserId = normalizedId(patch.participantUserId);
  const existing = containerId
    ? rtmsByContainer.get(containerId)
    : rtmsByMeetingUser.get(meetingUserKey(meetingId, participantUserId));
  const record = {
    ...(existing || {}),
    ...patch,
    containerId,
    meetingId,
    participantUserId,
    lastUpdatedAt: new Date().toISOString()
  };

  if (containerId) {
    rtmsByContainer.set(containerId, record);
  }
  if (meetingId) {
    rtmsByMeetingUser.set(meetingUserKey(meetingId, participantUserId), record);
  }
  return refreshRtmsRecord(record);
}

function rtmsRecordFor(container) {
  if (!container) return null;
  return refreshRtmsRecord(
    rtmsByContainer.get(container.id) ||
    rtmsByContainer.get(container.fullId) ||
    rtmsByMeetingUser.get(meetingUserKey(container.meetingNumber, container.userId)) ||
    rtmsByMeetingUser.get(meetingUserKey(container.meetingNumber, '')) ||
    null
  );
}

function publicRtmsRecord(record) {
  const refreshed = refreshRtmsRecord(record);
  if (!refreshed) {
    return {
      status: 'unknown',
      message: 'No RTMS command or webhook has been tracked for this container.'
    };
  }
  return {
    status: refreshed.status,
    action: refreshed.action || '',
    meetingId: refreshed.meetingId || '',
    participantUserId: refreshed.participantUserId || '',
    streamId: refreshed.streamId || '',
    event: refreshed.event || '',
    message: refreshed.message || '',
    lastCommandAt: refreshed.lastCommandAt || '',
    confirmationDeadlineAt: refreshed.confirmationDeadlineAt || '',
    confirmedAt: refreshed.confirmedAt || '',
    webhookReceivedAt: refreshed.webhookReceivedAt || '',
    lastUpdatedAt: refreshed.lastUpdatedAt || ''
  };
}

async function listContainersWithRtms() {
  const containers = await listContainers();
  return containers.map(container => ({
    ...container,
    rtms: publicRtmsRecord(rtmsRecordFor(container))
  }));
}

function rememberRtmsCommand({ action, target, participantUserId, result }) {
  const now = new Date();
  const container = target.container || {};
  const status = action === 'start'
    ? 'start_requested'
    : 'stopped';

  return rememberRtmsRecord({
    status,
    action,
    meetingId: target.meetingId,
    participantUserId,
    containerId: container.id || '',
    containerName: container.name || '',
    lastCommandAt: now.toISOString(),
    confirmationDeadlineAt: action === 'start'
      ? new Date(now.getTime() + rtmsConfirmDelayMs).toISOString()
      : '',
    result,
    message: action === 'start'
      ? 'RTMS start command accepted. Waiting for meeting.rtms_started webhook confirmation.'
      : 'RTMS stop command accepted.'
  });
}

function rtmsEventPayload(body) {
  const payload = body?.payload || {};
  const object = payload.object || payload;
  return {
    event: body?.event || '',
    meetingId: normalizedId(object.meeting_id || object.meetingId || payload.meeting_id || payload.meetingId || object.id || payload.id),
    meetingUuid: normalizedId(object.meeting_uuid || object.meetingUuid || payload.meeting_uuid || payload.meetingUuid || object.uuid || payload.uuid),
    participantUserId: normalizedId(
      object.operator_id ||
      object.participant_user_id ||
      object.participantUserId ||
      object.host_id ||
      payload.operator_id ||
      payload.participant_user_id ||
      payload.participantUserId ||
      payload.host_id
    ),
    streamId: normalizedId(object.rtms_stream_id || object.rtmsStreamId || payload.rtms_stream_id || payload.rtmsStreamId),
    serverUrls: object.server_urls || object.serverUrls || payload.server_urls || payload.serverUrls || ''
  };
}

function rtmsStatusFromEvent(event) {
  if (/\.rtms_started$/.test(event)) return 'started';
  if (/\.rtms_stopped$/.test(event)) return 'stopped';
  if (/\.rtms_interrupted$/.test(event)) return 'interrupted';
  return '';
}

function applyRtmsWebhook(body) {
  const event = body?.event || '';
  const status = rtmsStatusFromEvent(event);
  if (!status) return null;

  const eventData = rtmsEventPayload(body);
  if (!eventData.meetingId) return null;

  const records = new Set();
  const direct = rtmsByMeetingUser.get(meetingUserKey(eventData.meetingId, eventData.participantUserId));
  if (direct) records.add(direct);
  for (const record of rtmsByContainer.values()) {
    if (isSameMeetingUser(record, eventData.meetingId, eventData.participantUserId)) {
      records.add(record);
    }
  }

  const now = new Date().toISOString();
  const patch = {
    status,
    event,
    meetingId: eventData.meetingId,
    meetingUuid: eventData.meetingUuid,
    participantUserId: eventData.participantUserId,
    streamId: eventData.streamId,
    serverUrls: eventData.serverUrls,
    webhookReceivedAt: now,
    confirmedAt: status === 'started' ? now : '',
    message: `Confirmed by Zoom webhook ${event}.`
  };

  if (!records.size) {
    return rememberRtmsRecord(patch);
  }

  let lastRecord = null;
  for (const record of records) {
    lastRecord = rememberRtmsRecord({
      ...record,
      ...patch,
      containerId: record.containerId || ''
    });
  }
  return lastRecord;
}

function rtmsUrlValidationResponse(body) {
  if (body?.event !== 'endpoint.url_validation') return null;
  if (!config.zoom.webhookSecretToken) {
    throw new HttpError(500, 'ZOOM_WEBHOOK_SECRET_TOKEN is required for Zoom webhook URL validation');
  }
  const plainToken = body.payload?.plainToken || '';
  return {
    plainToken,
    encryptedToken: crypto
      .createHmac('sha256', config.zoom.webhookSecretToken)
      .update(plainToken)
      .digest('hex')
  };
}

function cleanUserNamePart(value) {
  return String(value || '')
    .replace(/@.*/, '')
    .replace(/[^A-Za-z0-9_-]+/g, '')
    .slice(0, 30);
}

function normalizeZoomUser(user, local = {}) {
  return {
    id: local.id || user.id || user.email,
    zoomUserId: local.zoomUserId || user.id || user.email,
    email: local.email || user.email || '',
    firstName: local.firstName || user.first_name || user.firstName || '',
    lastName: local.lastName || user.last_name || user.lastName || '',
    type: user.type ?? local.type ?? '',
    loginType: user.login_type ?? local.loginType ?? '',
    status: user.status || local.status || '',
    pmi: user.pmi || local.pmi || '',
    source: local.source || 'zoom',
    createdAt: local.createdAt || user.created_at || '',
    updatedAt: local.updatedAt || '',
    meeting: local.meeting || null
  };
}

function userMatchesDomain(user) {
  const email = String(user.email || '').toLowerCase();
  const domain = String(config.server.custCreateEmailDomain || '').toLowerCase().replace(/^@/, '');
  return Boolean(domain && email.endsWith(`@${domain}`));
}

function isCustCreateCandidate(user, localById, localByEmail) {
  const id = String(user.id || user.zoomUserId || '');
  const email = String(user.email || '').toLowerCase();
  const loginType = String(user.login_type ?? user.loginType ?? '').toLowerCase();
  const planType = String(user.type ?? '').toLowerCase();
  const creationType = String(user.creation_type ?? user.create_action ?? user.action ?? user.user_type ?? '').toLowerCase();

  return Boolean(
    localById.has(id) ||
    localByEmail.has(email) ||
    userMatchesDomain(user) ||
    loginType === '99' ||
    loginType === 'api' ||
    planType === '99' ||
    planType === 'api' ||
    creationType.includes('cust')
  );
}

async function selectableCustCreateUsers() {
  const localUsers = await listManagedUsers();
  const localById = new Map(localUsers.flatMap(user => [
    [String(user.id || ''), user],
    [String(user.zoomUserId || ''), user]
  ]).filter(([key]) => key));
  const localByEmail = new Map(localUsers
    .filter(user => user.email)
    .map(user => [String(user.email).toLowerCase(), user]));
  const byId = new Map();
  const warnings = [];
  let zoomUserCount = 0;

  for (const local of localUsers) {
    byId.set(local.id || local.zoomUserId || local.email, {
      ...local,
      selectableReason: 'created or saved by this manager'
    });
  }

  try {
    const result = await listUsers();
    zoomUserCount = result.users.length;
    for (const zoomUser of result.users) {
      if (!isCustCreateCandidate(zoomUser, localById, localByEmail)) continue;
      const local = localById.get(String(zoomUser.id || '')) || localByEmail.get(String(zoomUser.email || '').toLowerCase()) || {};
      const normalized = normalizeZoomUser(zoomUser, local);
      normalized.selectableReason = local.source
        ? 'created or saved by this manager'
        : userMatchesDomain(zoomUser)
          ? `email matches ${config.server.custCreateEmailDomain}`
          : 'Zoom API/login type indicates API-created user';
      byId.set(normalized.id, normalized);
    }
  } catch (error) {
    warnings.push(`Zoom user list failed; showing locally saved users only: ${error.message}`);
  }

  return {
    users: [...byId.values()].sort((a, b) => String(a.email).localeCompare(String(b.email))),
    warnings,
    localUserCount: localUsers.length,
    zoomUserCount
  };
}

async function selectedUser(userId) {
  const local = await findManagedUser(userId);
  if (local) return local;

  const zoomUser = await getUser(userId);
  if (!isCustCreateCandidate(zoomUser, new Map(), new Map())) {
    throw new HttpError(400, 'Selected user is not a custCreate candidate. Use the fake-domain custCreate flow first.');
  }
  return addManagedUser(normalizeZoomUser(zoomUser));
}

async function resolveAndCacheInstantMeeting(user) {
  const lookup = user.zoomUserId || user.email || user.id;
  const resolved = await resolveUserInstantMeeting(lookup);
  const meeting = {
    number: resolved.meetingNumber,
    password: resolved.meetingPassword,
    source: resolved.meetingSource,
    usePmiForInstant: resolved.usePmiForInstant,
    personalMeetingUrl: resolved.personalMeetingUrl,
    warnings: resolved.warnings,
    resolvedAt: new Date().toISOString()
  };

  const updated = await updateManagedUser(user.id, {
    zoomUserId: resolved.zoomUserId || user.zoomUserId,
    email: resolved.email || user.email,
    firstName: resolved.firstName || user.firstName,
    lastName: resolved.lastName || user.lastName,
    type: resolved.type || user.type,
    loginType: resolved.loginType ?? user.loginType,
    meeting
  });

  return {
    user: updated || { ...user, meeting },
    meeting,
    resolved
  };
}

function runUsernamePrefix(user, mode) {
  const base = cleanUserNamePart(`${user.firstName || ''}${user.lastName || ''}`) || cleanUserNamePart(user.email) || 'Bot';
  return `${mode === 'start' ? 'Host' : 'Join'}-${base}`;
}

function randomMeetingPasscode() {
  return Math.random().toString(36).replace(/[^a-z0-9]/g, '').slice(2, 10);
}

function meetingFromScheduledMeeting(meeting, password, previousWarnings = []) {
  return {
    number: String(meeting.id || meeting.meetingNumber || ''),
    password: meeting.password || password || '',
    source: 'scheduled-fallback',
    topic: meeting.topic || '',
    joinUrl: meeting.join_url || '',
    startUrl: meeting.start_url || '',
    warnings: [
      ...previousWarnings,
      'No instant/PMI meeting ID was exposed for this user, so a scheduled fallback meeting was created just before launch.'
    ],
    resolvedAt: new Date().toISOString()
  };
}

async function meetingForRun(user, mode) {
  const resolved = await resolveAndCacheInstantMeeting(user);
  if (resolved.meeting.number) {
    return {
      ...resolved,
      fallbackCreated: false
    };
  }

  const lookup = user.zoomUserId || user.email || user.id;
  const password = randomMeetingPasscode();
  const scheduled = await scheduleMeeting(lookup, {
    topic: `Zoom Load Test ${mode === 'start' ? 'Host' : 'Join'} ${new Date().toISOString()}`,
    type: 2,
    duration: 30,
    password,
    hostVideo: true,
    participantVideo: true,
    joinBeforeHost: true,
    waitingRoom: false,
    muteUponEntry: false,
    startTime: new Date(Date.now() + 60 * 1000).toISOString()
  });
  const meeting = meetingFromScheduledMeeting(scheduled, password, resolved.meeting.warnings);
  const updated = await updateManagedUser(user.id, {
    meeting
  });

  return {
    user: updated || { ...user, meeting },
    meeting,
    resolved: {
      ...resolved.resolved,
      scheduledMeeting: scheduled
    },
    fallbackCreated: true
  };
}

async function jwtForRun(body, role) {
  if (body.jwtToken) return body.jwtToken;
  const result = await getMeetingSdkToken({ meetingNumber: body.meetingNumber, role });
  return result.token;
}

async function meetingIdFromRequest(body) {
  if (body.containerId) {
    const containers = await listContainers();
    const container = containers.find(item => (
      item.id === body.containerId ||
      item.fullId === body.containerId ||
      item.name === body.containerId
    ));
    if (!container) throw new HttpError(404, 'Container not found');
    if (!container.meetingNumber) throw new HttpError(400, 'Container does not have a meeting-number label');
    return {
      meetingId: container.meetingNumber,
      container
    };
  }

  if (body.userId) {
    const user = await selectedUser(body.userId);
    const { meeting } = await resolveAndCacheInstantMeeting(user);
    if (!meeting.number) throw new HttpError(400, 'Selected user has no resolvable instant meeting ID');
    return {
      meetingId: meeting.number,
      user
    };
  }

  return {
    meetingId: requireField(body.meetingId, 'meetingId')
  };
}

app.get('/api/status', asyncRoute(async (_req, res) => {
  const containers = await listContainersWithRtms().catch(error => ({
    error: error.message,
    details: error.details
  }));
  res.json({
    ...publicStatus(),
    containers
  });
}));

app.post('/api/zoom/rtms/webhook', (req, res, next) => {
  try {
    const validation = rtmsUrlValidationResponse(req.body);
    if (validation) {
      res.json(validation);
      return;
    }
    res.sendStatus(200);
    queueMicrotask(() => {
      try {
        applyRtmsWebhook(req.body);
      } catch (error) {
        console.error('Failed to process RTMS webhook', error);
      }
    });
  } catch (error) {
    next(error);
  }
});

app.get('/api/env', asyncRoute(async (_req, res) => {
  res.json({ values: envSnapshot() });
}));

app.post('/api/env', asyncRoute(async (req, res) => {
  await saveEnv(req.body.values || req.body);
  res.json({
    ok: true,
    values: envSnapshot(),
    status: publicStatus()
  });
}));

app.post('/api/zoom/oauth/test', asyncRoute(async (_req, res) => {
  const token = await getS2SAccessToken();
  res.json({
    ok: true,
    apiBaseUrl: token.apiBaseUrl,
    scope: token.scope,
    expiresAt: new Date(token.expiresAt).toISOString()
  });
}));

app.get('/api/managed-users/suggest', asyncRoute(async (_req, res) => {
  res.json(suggestManagedUser(config.server.custCreateEmailDomain));
}));

app.get('/api/managed-users', asyncRoute(async (_req, res) => {
  res.json(await selectableCustCreateUsers());
}));

app.post('/api/managed-users/resolve-all', asyncRoute(async (_req, res) => {
  const selectable = await selectableCustCreateUsers();
  const results = [];

  for (const candidate of selectable.users) {
    try {
      const user = await selectedUser(candidate.id || candidate.zoomUserId || candidate.email);
      const resolved = await resolveAndCacheInstantMeeting(user);
      results.push({
        ok: true,
        userId: user.id,
        zoomUserId: resolved.user?.zoomUserId || user.zoomUserId || '',
        email: resolved.user?.email || user.email || '',
        meetingNumber: resolved.meeting?.number || '',
        meetingPassword: resolved.meeting?.password || '',
        meetingSource: resolved.meeting?.source || '',
        warnings: resolved.meeting?.warnings || []
      });
    } catch (error) {
      results.push({
        ok: false,
        userId: candidate.id || candidate.zoomUserId || candidate.email || '',
        email: candidate.email || '',
        error: error.message,
        details: error.details
      });
    }
  }

  res.json({
    ok: results.every(result => result.ok),
    resolvedCount: results.filter(result => result.ok && result.meetingNumber).length,
    failedCount: results.filter(result => !result.ok).length,
    totalCount: results.length,
    warnings: selectable.warnings || [],
    results
  });
}));

app.post('/api/managed-users', asyncRoute(async (req, res) => {
  const suggested = suggestManagedUser(config.server.custCreateEmailDomain);
  const user = await createCustCreateUser({
    email: req.body.email || suggested.email,
    firstName: req.body.firstName || suggested.firstName,
    lastName: req.body.lastName || suggested.lastName,
    type: req.body.type || suggested.type || 2
  });
  const record = await addManagedUser({
    id: user.id,
    zoomUserId: user.id,
    email: user.email,
    firstName: user.first_name,
    lastName: user.last_name,
    type: user.type,
    createdAt: user.created_at
  });
  res.json({ ok: true, user: record, raw: user });
}));

app.delete('/api/managed-users/:userId', asyncRoute(async (req, res) => {
  const user = await selectedUser(req.params.userId);
  const zoomUserId = user.zoomUserId || user.email || user.id;
  let zoomDelete = null;
  let warning = '';

  if (req.query.localOnly === 'true') {
    warning = 'Removed from local manager store only.';
  } else {
    try {
      zoomDelete = await deleteUser(zoomUserId, 'delete');
    } catch (error) {
      warning = `Zoom delete failed; local record was still removed: ${error.message}`;
    }
  }

  const localDelete = await removeManagedUser(user.id);
  res.json({
    ok: true,
    user: {
      id: user.id,
      zoomUserId,
      email: user.email
    },
    zoomDelete,
    localDelete,
    warning
  });
}));

app.post('/api/managed-users/:userId/resolve', asyncRoute(async (req, res) => {
  const user = await selectedUser(req.params.userId);
  res.json({
    ok: true,
    ...(await resolveAndCacheInstantMeeting(user))
  });
}));

app.post('/api/zoom/users', asyncRoute(async (req, res) => {
  const user = await createCustCreateUser({
    email: requireField(req.body.email, 'email'),
    firstName: req.body.firstName,
    lastName: req.body.lastName,
    type: req.body.type,
    password: req.body.password
  });
  await addManagedUser({
    id: user.id,
    zoomUserId: user.id,
    email: user.email,
    firstName: user.first_name,
    lastName: user.last_name,
    type: user.type,
    createdAt: user.created_at
  });
  res.json(user);
}));

app.get('/api/zoom/users/:userId', asyncRoute(async (req, res) => {
  res.json(await getUser(req.params.userId));
}));

app.post('/api/zoom/users/zak', asyncRoute(async (req, res) => {
  const userId = requireField(req.body.userId, 'userId');
  const ttl = asPositiveInteger(req.body.ttl, 'ttl', 7200);
  const result = await getZakToken(userId, ttl);
  res.json({
    token: result.token,
    tokenPreview: tokenPreview(result.token),
    raw: result.raw
  });
}));

app.post('/api/zoom/meetings', asyncRoute(async (req, res) => {
  const userId = requireField(req.body.userId, 'userId');
  const meeting = await scheduleMeeting(userId, req.body);
  res.json(meeting);
}));

app.get('/api/zoom/meetings/:meetingId', asyncRoute(async (req, res) => {
  res.json(await getMeeting(req.params.meetingId));
}));

app.get('/api/zoom/rtms/status', asyncRoute(async (req, res) => {
  const target = await meetingIdFromRequest(req.query);
  const participantUserId = req.query.participantUserId ||
    target.container?.userId ||
    target.user?.zoomUserId ||
    target.user?.id ||
    '';
  const record = target.container
    ? rtmsRecordFor(target.container)
    : rtmsByMeetingUser.get(meetingUserKey(target.meetingId, participantUserId));

  res.json({
    ok: true,
    meetingId: target.meetingId,
    participantUserId,
    containerId: target.container?.id || req.query.containerId || '',
    rtms: publicRtmsRecord(record),
    note: 'Zoom confirms RTMS readiness through meeting.rtms_started/meeting.rtms_stopped webhooks. The REST endpoint updates status but does not expose a non-mutating GET status API.'
  });
}));

app.post('/api/zoom/rtms/status', asyncRoute(async (req, res) => {
  const target = await meetingIdFromRequest(req.body);
  const action = req.body.action === 'stop' ? 'stop' : 'start';
  const clientId = req.body.clientId || config.zoom.rtmsClientId;
  const participantUserId = req.body.participantUserId ||
    target.container?.userId ||
    target.user?.zoomUserId ||
    target.user?.id ||
    '';
  if (!participantUserId) {
    throw new HttpError(400, 'RTMS participantUserId is required so Zoom does not fall back to the OAuth token user', {
      fix: 'Start RTMS from a start-mode host container, select the host custCreate user, or pass participantUserId explicitly.'
    });
  }
  if (target.container && action === 'start' && target.container.mode !== 'startmeeting' && !req.body.participantUserId) {
    throw new HttpError(400, 'RTMS start requires the meeting host or alternative host user id', {
      containerMode: target.container.mode,
      participantUserId,
      fix: 'Use a startmeeting container, or pass participantUserId for the meeting host/alternative host.'
    });
  }
  let result;
  try {
    result = await updateRtmsStatus(target.meetingId, action, clientId, participantUserId);
  } catch (error) {
    if (action === 'stop' && error instanceof HttpError && Number(error.details?.code) === 13277) {
      result = {
        alreadyStopped: true,
        zoomCode: error.details.code,
        message: error.details.message
      };
    } else {
      if (error instanceof HttpError && Number(error.details?.code) === 2308) {
        error.details = {
          ...error.details,
          participantUserId,
          fix: 'RTMS can only be started by the meeting host or an alternative host. Use a start-mode host container, or pass participantUserId for the host/alternative host.'
        };
      }
      throw error;
    }
  }
  const rtms = rememberRtmsCommand({ action, target, participantUserId, result });
  res.json({
    ok: true,
    action,
    meetingId: target.meetingId,
    participantUserId,
    target,
    result,
    rtms: publicRtmsRecord(rtms)
  });
}));

app.post('/api/meeting-sdk-token', asyncRoute(async (req, res) => {
  const meetingNumber = requireField(req.body.meetingNumber, 'meetingNumber');
  const role = Number(req.body.role ?? 0);
  const result = await getMeetingSdkToken({ meetingNumber, role });
  res.json({
    token: result.token,
    tokenPreview: tokenPreview(result.token),
    meetingNumber: result.meetingNumber,
    meetingPassword: result.meetingPassword,
    raw: result.raw
  });
}));

app.post('/api/docker/login', asyncRoute(async (_req, res) => {
  res.json({
    ok: true,
    login: await dockerLogin()
  });
}));

app.get('/api/docker/containers', asyncRoute(async (_req, res) => {
  res.json(await listContainersWithRtms());
}));

app.post('/api/run/selected', asyncRoute(async (req, res) => {
  const mode = req.body.mode === 'start' ? 'start' : 'join';
  const user = await selectedUser(requireField(req.body.userId, 'userId'));
  const manualJoinMeetingNumber = mode === 'join' ? String(req.body.meetingNumber || '').trim() : '';
  const runMeeting = manualJoinMeetingNumber
    ? {
        meeting: {
          number: manualJoinMeetingNumber,
          password: req.body.meetingPassword ?? '',
          source: 'manual join target',
          warnings: []
        },
        fallbackCreated: false
      }
    : await meetingForRun(user, mode);
  const { meeting } = runMeeting;

  if (!meeting.number) {
    throw new HttpError(400, 'Selected user has no instant or scheduled fallback meeting ID', { warnings: meeting.warnings });
  }

  const jwtToken = await jwtForRun({ meetingNumber: meeting.number }, mode === 'start' ? 1 : 0);
  let userZak = '';
  if (mode === 'start') {
    const zak = await getZakToken(user.zoomUserId || user.email, asPositiveInteger(req.body.zakTtl, 'zakTtl', 7200));
    userZak = zak.token;
  }

  const started = await startContainers({
    mode,
    count: asPositiveInteger(req.body.count, 'count', 1),
    meetingNumber: meeting.number,
    meetingPassword: meeting.password || '',
    jwtToken,
    userZak,
    userId: user.zoomUserId || user.id,
    userEmail: user.email,
    userType: 'custCreate',
    usernamePrefix: req.body.usernamePrefix || runUsernamePrefix(user, mode),
    sendVideoRawData: 'true',
    sendAudioRawData: 'true',
    chatDemo: 'true',
    exitOnMeetingEnd: 'true',
    mediaIndex: req.body.mediaIndex ?? '-1',
    videoWidth: req.body.videoWidth ?? '1280',
    videoHeight: req.body.videoHeight ?? '720',
    videoFps: req.body.videoFps ?? '30',
    audioSampleRate: req.body.audioSampleRate ?? '48000',
    audioChannels: req.body.audioChannels ?? '1'
  });

  res.json({
    ok: true,
    mode,
    user: {
      id: user.id,
      zoomUserId: user.zoomUserId,
      email: user.email,
      type: user.type
    },
    meeting,
    fallbackCreated: runMeeting.fallbackCreated,
    jwtTokenPreview: tokenPreview(jwtToken),
    userZakPreview: tokenPreview(userZak),
    started
  });
}));

app.post('/api/run/join', asyncRoute(async (req, res) => {
  const meetingNumber = requireField(req.body.meetingNumber, 'meetingNumber');
  const jwtToken = await jwtForRun({ ...req.body, meetingNumber }, 0);
  const started = await startContainers({
    mode: 'join',
    count: asPositiveInteger(req.body.count, 'count', 1),
    meetingNumber,
    meetingPassword: req.body.meetingPassword ?? '',
    jwtToken,
    usernamePrefix: req.body.usernamePrefix || 'LoadBot',
    sendVideoRawData: req.body.sendVideoRawData,
    sendAudioRawData: req.body.sendAudioRawData,
    chatDemo: req.body.chatDemo,
    exitOnMeetingEnd: req.body.exitOnMeetingEnd,
    mediaIndex: req.body.mediaIndex
  });
  res.json({ ok: true, jwtTokenPreview: tokenPreview(jwtToken), started });
}));

app.post('/api/run/start', asyncRoute(async (req, res) => {
  const meetingNumber = requireField(req.body.meetingNumber, 'meetingNumber');
  let userZak = req.body.userZak || '';
  if (!userZak && req.body.userId) {
    const zak = await getZakToken(req.body.userId, asPositiveInteger(req.body.zakTtl, 'zakTtl', 7200));
    userZak = zak.token;
  }
  if (!userZak) {
    throw new HttpError(400, 'userZak or userId is required to start as host');
  }

  const jwtToken = await jwtForRun({ ...req.body, meetingNumber }, 1);
  const started = await startContainers({
    mode: 'start',
    count: asPositiveInteger(req.body.count, 'count', 1),
    meetingNumber,
    jwtToken,
    userZak,
    usernamePrefix: req.body.usernamePrefix || 'LoadHost',
    sendVideoRawData: req.body.sendVideoRawData,
    sendAudioRawData: req.body.sendAudioRawData,
    chatDemo: req.body.chatDemo,
    exitOnMeetingEnd: req.body.exitOnMeetingEnd,
    mediaIndex: req.body.mediaIndex
  });
  res.json({ ok: true, jwtTokenPreview: tokenPreview(jwtToken), userZakPreview: tokenPreview(userZak), started });
}));

app.post('/api/run/kill', asyncRoute(async (req, res) => {
  res.json({
    ok: true,
    stopped: await killContainers({
      target: req.body.target || req.body.containerId || 'all',
      project: req.body.project || ''
    })
  });
}));

app.use((error, _req, res, _next) => {
  const status = error instanceof HttpError ? error.status : 500;
  res.status(status).json({
    error: error.message || 'Unexpected error',
    details: error.details
  });
});

app.listen(config.server.port, config.server.host, () => {
  console.log(`Zoom load-test manager listening on http://${config.server.host}:${config.server.port}`);
});
