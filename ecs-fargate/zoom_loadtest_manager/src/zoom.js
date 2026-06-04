import { config } from './config.js';
import { HttpError } from './errors.js';

let cachedToken = null;

function requiredZoomConfig() {
  const missing = [];
  if (!config.zoom.accountId) missing.push('ZOOM_ACCOUNT_ID');
  if (!config.zoom.clientId) missing.push('ZOOM_CLIENT_ID');
  if (!config.zoom.clientSecret) missing.push('ZOOM_CLIENT_SECRET');
  if (missing.length) {
    throw new HttpError(500, `Missing Zoom S2S OAuth config: ${missing.join(', ')}`);
  }
}

function apiBaseFromTokenResponse(tokenResponse) {
  if (process.env.ZOOM_API_BASE_URL) return config.zoom.apiBaseUrl;
  if (tokenResponse.api_url) return `${String(tokenResponse.api_url).replace(/\/$/, '')}/v2`;
  return config.zoom.apiBaseUrl;
}

export async function getS2SAccessToken() {
  requiredZoomConfig();

  const now = Date.now();
  if (cachedToken && cachedToken.expiresAt > now + 60_000) {
    return cachedToken;
  }

  const url = new URL('https://zoom.us/oauth/token');
  url.searchParams.set('grant_type', 'account_credentials');
  url.searchParams.set('account_id', config.zoom.accountId);

  const basic = Buffer.from(`${config.zoom.clientId}:${config.zoom.clientSecret}`).toString('base64');
  const response = await fetch(url, {
    method: 'POST',
    headers: {
      Authorization: `Basic ${basic}`,
      'Content-Type': 'application/x-www-form-urlencoded'
    }
  });

  const body = await parseJson(response);
  if (!response.ok) {
    throw new HttpError(response.status, 'Zoom OAuth token request failed', body);
  }

  cachedToken = {
    accessToken: body.access_token,
    expiresAt: now + Math.max(0, Number(body.expires_in ?? 3600) - 60) * 1000,
    apiBaseUrl: apiBaseFromTokenResponse(body),
    scope: body.scope ?? ''
  };
  return cachedToken;
}

async function parseJson(response) {
  const text = await response.text();
  if (!text) return {};
  try {
    return JSON.parse(text);
  } catch {
    return { raw: text };
  }
}

async function zoomRequest(path, options = {}) {
  const token = await getS2SAccessToken();
  const response = await fetch(`${token.apiBaseUrl}${path}`, {
    ...options,
    headers: {
      Authorization: `Bearer ${token.accessToken}`,
      'Content-Type': 'application/json',
      ...(options.headers ?? {})
    }
  });

  const body = await parseJson(response);
  if (!response.ok) {
    throw new HttpError(response.status, `Zoom API request failed: ${options.method ?? 'GET'} ${path}`, body);
  }
  return body;
}

export async function createCustCreateUser(input) {
  const userInfo = {
    email: input.email,
    type: Number(input.type || 2),
    first_name: input.firstName || undefined,
    last_name: input.lastName || undefined
  };
  if (input.password) {
    userInfo.password = input.password;
  }

  return zoomRequest('/users', {
    method: 'POST',
    body: JSON.stringify({
      action: 'custCreate',
      user_info: userInfo
    })
  });
}

export async function deleteUser(userId, action = 'delete') {
  const params = new URLSearchParams({
    action
  });
  return zoomRequest(`/users/${encodeURIComponent(userId)}?${params.toString()}`, {
    method: 'DELETE'
  });
}

export async function listUsers({ status = 'active', pageSize = 300, maxPages = 20 } = {}) {
  const users = [];
  let nextPageToken = '';
  let pages = 0;

  do {
    const params = new URLSearchParams({
      status,
      page_size: String(pageSize)
    });
    if (nextPageToken) {
      params.set('next_page_token', nextPageToken);
    }

    const body = await zoomRequest(`/users?${params.toString()}`);
    if (Array.isArray(body.users)) {
      users.push(...body.users);
    }

    nextPageToken = body.next_page_token || '';
    pages += 1;
  } while (nextPageToken && pages < maxPages);

  return {
    users,
    pages,
    nextPageToken
  };
}

export async function getUser(userId) {
  return zoomRequest(`/users/${encodeURIComponent(userId)}`);
}

export async function getUserSettings(userId) {
  return zoomRequest(`/users/${encodeURIComponent(userId)}/settings`);
}

export async function getZakToken(userId, ttl = 7200) {
  const body = await zoomRequest(`/users/${encodeURIComponent(userId)}/token?type=zak&ttl=${encodeURIComponent(ttl)}`);
  return {
    token: body.token ?? body.zak ?? '',
    raw: body
  };
}

function firstString(...values) {
  for (const value of values) {
    if (typeof value === 'string' && value.trim()) return value.trim();
    if (typeof value === 'number' && Number.isFinite(value)) return String(value);
  }
  return '';
}

function extractMeetingNumberFromUrl(url) {
  const match = String(url || '').match(/\/j\/(\d+)/);
  return match ? match[1] : '';
}

function extractPasswordFromUrl(url) {
  try {
    const parsed = new URL(String(url || ''));
    return firstString(parsed.searchParams.get('pwd'), parsed.searchParams.get('password'));
  } catch {
    return '';
  }
}

function findPasswordCandidate(settings) {
  const schedule = settings?.schedule_meeting ?? {};
  const security = settings?.meeting_security ?? {};
  return firstString(
    schedule.pmi_password,
    schedule.pmi_passcode,
    schedule.personal_meeting_password,
    schedule.personal_meeting_passcode,
    schedule.personal_meeting_pwd,
    schedule.meeting_password,
    schedule.default_password,
    schedule.default_passcode,
    security.pmi_password,
    security.pmi_passcode,
    security.personal_meeting_password,
    security.personal_meeting_passcode,
    security.passcode,
    security.password
  );
}

export async function resolveUserInstantMeeting(userId) {
  const [user, settingsResult] = await Promise.allSettled([
    getUser(userId),
    getUserSettings(userId)
  ]);

  if (user.status === 'rejected') {
    throw user.reason;
  }

  const profile = user.value;
  const settings = settingsResult.status === 'fulfilled' ? settingsResult.value : {};
  const meetingNumber = firstString(
    profile.pmi,
    profile.personal_meeting_id,
    profile.personal_meeting_number,
    settings?.schedule_meeting?.pmi,
    settings?.schedule_meeting?.personal_meeting_id,
    settings?.schedule_meeting?.personal_meeting_number,
    extractMeetingNumberFromUrl(profile.personal_meeting_url)
  );
  const meetingPassword = firstString(
    profile.pmi_password,
    profile.pmi_passcode,
    profile.personal_meeting_password,
    profile.personal_meeting_passcode,
    extractPasswordFromUrl(profile.personal_meeting_url),
    findPasswordCandidate(settings)
  );
  const scheduleSettings = settings?.schedule_meeting ?? {};
  const usePmiForInstant = profile.use_pmi ?? scheduleSettings.use_pmi ?? scheduleSettings.personal_meeting ?? null;
  const warnings = [];

  if (!meetingNumber) {
    warnings.push('Could not find a personal/instant meeting id from GET /users/{userId} or /settings.');
  }
  if (!meetingPassword) {
    warnings.push('Could not find a personal meeting passcode in user settings. The meeting may not require one, or Zoom may not expose it for this account.');
  }
  if (settingsResult.status === 'rejected') {
    warnings.push(`User settings lookup failed: ${settingsResult.reason.message}`);
  }
  if (usePmiForInstant === false) {
    warnings.push('This user is not configured to use PMI for instant meetings. The manager will still use the PMI because random instant meeting IDs are not knowable before start.');
  }

  return {
    userId,
    zoomUserId: profile.id,
    email: profile.email,
    firstName: profile.first_name,
    lastName: profile.last_name,
    type: profile.type,
    loginType: profile.login_type,
    meetingNumber,
    meetingPassword,
    usePmiForInstant,
    meetingSource: 'pmi',
    personalMeetingUrl: profile.personal_meeting_url || '',
    warnings,
    raw: {
      user: profile,
      settings
    }
  };
}

export async function scheduleMeeting(userId, input) {
  const type = Number(input.type || 2);
  const meeting = {
    topic: input.topic || 'Zoom Load Test',
    type,
    duration: Number(input.duration || 30),
    timezone: input.timezone || 'UTC',
    agenda: input.agenda || undefined,
    password: input.password || undefined,
    settings: {
      host_video: Boolean(input.hostVideo),
      participant_video: Boolean(input.participantVideo),
      join_before_host: Boolean(input.joinBeforeHost),
      waiting_room: Boolean(input.waitingRoom),
      mute_upon_entry: Boolean(input.muteUponEntry),
      approval_type: 2
    }
  };
  if (input.startTime) {
    meeting.start_time = input.startTime;
  } else if (type === 2) {
    meeting.start_time = new Date(Date.now() + 10 * 60 * 1000).toISOString();
  }

  return zoomRequest(`/users/${encodeURIComponent(userId)}/meetings`, {
    method: 'POST',
    body: JSON.stringify(meeting)
  });
}

export async function getMeeting(meetingId) {
  return zoomRequest(`/meetings/${encodeURIComponent(meetingId)}`);
}

export async function updateRtmsStatus(meetingId, action = 'start', clientId = config.zoom.rtmsClientId, participantUserId = '') {
  const settings = {
    client_id: clientId
  };
  if (participantUserId) {
    settings.participant_user_id = participantUserId;
  }

  return zoomRequest(`/live_meetings/${encodeURIComponent(meetingId)}/rtms_app/status`, {
    method: 'PATCH',
    body: JSON.stringify({
      action,
      settings
    })
  });
}
