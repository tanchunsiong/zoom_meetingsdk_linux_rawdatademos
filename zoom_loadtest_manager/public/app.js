const logEl = document.querySelector('#log');
const statusEl = document.querySelector('#env-status');
const containersEl = document.querySelector('#container-list');
const envFieldsEl = document.querySelector('#env-fields');
const userListEl = document.querySelector('#user-list');
const rtmsClientIdLabelEl = document.querySelector('#rtms-client-id-label');
const containerLastCheckEl = document.querySelector('#container-last-check');

const state = {
  users: [],
  containers: [],
  status: null
};

const secretKeys = /token|zak|password|secret|authorization/i;
const containerRefreshMs = 10_000;
let containerRefreshInFlight = false;

const envHelp = {
  MEETING_TOKEN_ENDPOINT: 'HTTPS endpoint the manager calls just-in-time to get a Zoom Meeting SDK JWT/signature for a meeting number and role. The returned token is passed into the container as JWT_TOKEN; the container should not fetch it itself.'
};

const envLabels = {
  ZOOM_ACCOUNT_ID: 'S2S Zoom Account ID',
  ZOOM_CLIENT_ID: 'S2S Zoom Client ID',
  ZOOM_CLIENT_SECRET: 'S2S Zoom Client Secret'
};

function escapeHtml(value) {
  return String(value ?? '')
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function maskSecrets(value) {
  if (Array.isArray(value)) return value.map(maskSecrets);
  if (!value || typeof value !== 'object') return value;

  return Object.fromEntries(Object.entries(value).map(([key, entry]) => {
    if (secretKeys.test(key) && typeof entry === 'string' && entry) {
      return [key, entry.length > 14 ? `${entry.slice(0, 6)}...${entry.slice(-4)}` : '[set]'];
    }
    return [key, maskSecrets(entry)];
  }));
}

function log(title, payload) {
  const time = new Date().toLocaleTimeString();
  logEl.textContent = `[${time}] ${title}\n${JSON.stringify(maskSecrets(payload), null, 2)}\n\n${logEl.textContent}`;
}

async function request(path, options = {}) {
  const response = await fetch(path, {
    headers: {
      'Content-Type': 'application/json'
    },
    ...options
  });
  const body = await response.json().catch(() => ({}));
  if (!response.ok) {
    throw Object.assign(new Error(body.error || `HTTP ${response.status}`), { body });
  }
  return body;
}

function formObject(form) {
  const data = {};
  for (const element of form.elements) {
    if (!element.name) continue;
    if (element.type === 'checkbox') {
      data[element.name] = element.checked;
    } else {
      data[element.name] = element.value;
    }
  }
  return data;
}

function envPill(label, ok, detail = '') {
  return `<div class="pill ${ok ? '' : 'warn'}"><span>${escapeHtml(label)}</span><strong>${ok ? 'ready' : 'missing'}</strong>${detail ? `<small>${escapeHtml(detail)}</small>` : ''}</div>`;
}

function dateLabel(value) {
  if (!value || String(value).startsWith('0001-')) return '';
  const date = new Date(value);
  return Number.isNaN(date.getTime()) ? '' : date.toLocaleTimeString();
}

function containerHealth(container) {
  if (container.running) {
    const health = container.healthStatus || container.health || 'alive';
    return {
      className: health === 'unhealthy' ? 'unhealthy' : 'alive',
      label: health === 'healthy' ? 'healthy' : 'alive',
      detail: container.startedAt ? `started ${dateLabel(container.startedAt)}` : 'running'
    };
  }

  const dead = container.health === 'dead' || container.status === 'dead';
  return {
    className: dead ? 'dead' : 'exited',
    label: dead ? 'dead' : 'exited',
    detail: `exit ${container.exitCode ?? 'unknown'}${container.finishedAt ? ` at ${dateLabel(container.finishedAt)}` : ''}`
  };
}

function statsMeta(container) {
  const stats = container.stats || {};
  if (!stats.cpuPercent && !stats.memoryUsage && !stats.blockIo) {
    return container.running ? '<span>stats pending</span>' : '<span>stats unavailable</span>';
  }

  return [
    stats.cpuPercent ? `<span>cpu ${escapeHtml(stats.cpuPercent)}</span>` : '',
    stats.memoryUsage ? `<span>mem ${escapeHtml(stats.memoryUsage)}</span>` : '',
    stats.memoryPercent ? `<span>mem% ${escapeHtml(stats.memoryPercent)}</span>` : '',
    stats.blockIo ? `<span>disk I/O ${escapeHtml(stats.blockIo)}</span>` : '',
    stats.pids ? `<span>pids ${escapeHtml(stats.pids)}</span>` : ''
  ].filter(Boolean).join('');
}

function renderEnvFields(values) {
  envFieldsEl.innerHTML = values.map(item => {
    const help = envHelp[item.key] || '';
    return `
    <label class="${help ? 'has-help' : ''}">
      <span class="label-row">
        <span>${escapeHtml(envLabels[item.key] || item.key)}</span>
        ${help ? `<span class="tooltip" title="${escapeHtml(help)}" aria-label="${escapeHtml(help)}">?</span>` : ''}
      </span>
      <input
        name="${escapeHtml(item.key)}"
        type="${item.isSecret ? 'password' : 'text'}"
        value="${escapeHtml(item.value)}"
        placeholder="${escapeHtml(item.placeholder || '')}"
        autocomplete="off"
      >
      ${help ? `<small class="field-help">${escapeHtml(help)}</small>` : ''}
    </label>
  `;
  }).join('');
}

function renderStatus(status) {
  statusEl.innerHTML = [
    envPill('Zoom Account ID', status.zoom.hasAccountId),
    envPill('Zoom Client ID', status.zoom.hasClientId),
    envPill('Zoom Client Secret', status.zoom.hasClientSecret),
    envPill('Meeting SDK JWT Service', Boolean(status.zoom.tokenEndpoint), status.zoom.tokenEndpoint),
    envPill('RTMS Client ID', Boolean(status.zoom.rtmsClientId), status.zoom.rtmsClientId),
    envPill('Docker Registry User', status.docker.hasRegistryUsername, status.docker.registryUrl),
    envPill('Docker Registry Password', status.docker.hasRegistryPassword),
    `<div class="pill"><span>Load-Test Image</span><strong>${escapeHtml(status.docker.image)}</strong></div>`,
    `<div class="pill"><span>Docker Project</span><strong>${escapeHtml(status.docker.project)}</strong></div>`
  ].join('');
  rtmsClientIdLabelEl.textContent = status.zoom.rtmsClientId || 'not configured';
}

function userName(user) {
  return [user.firstName, user.lastName].filter(Boolean).join(' ') || user.email || user.id;
}

function renderUsers(payload) {
  state.users = payload.users || [];

  if (!state.users.length) {
    userListEl.innerHTML = `
      <div class="pill warn">
        <span>No custCreate users found. Create one below, or refresh after adding API-created users.</span>
        <strong>none</strong>
      </div>
    `;
    return;
  }

  userListEl.innerHTML = state.users.map(user => {
    const meeting = user.meeting || {};
    const meetingNumber = meeting.number || user.pmi || '';
    const passcode = meeting.password || '';
    const warnings = meeting.warnings || [];
    return `
      <div class="user-row" data-user-id="${escapeHtml(user.id)}">
        <div class="user-main">
          <strong>${escapeHtml(userName(user))}</strong>
          <code>${escapeHtml(user.email || '')}</code>
          <div class="container-meta">
            <span>type ${escapeHtml(user.type || 'unknown')}</span>
            <span>${escapeHtml(user.selectableReason || user.source || 'custCreate candidate')}</span>
            <span>${meetingNumber ? `PMI ${escapeHtml(meetingNumber)}` : 'PMI not resolved'}</span>
            <span>${meeting.source ? `source ${escapeHtml(meeting.source)}` : 'source pending'}</span>
            <span>${passcode ? `passcode ${escapeHtml(passcode)}` : 'passcode blank/not exposed'}</span>
          </div>
          ${warnings.length ? `<div class="notice small-notice">${warnings.map(escapeHtml).join('<br>')}</div>` : ''}
        </div>
        <label class="compact-label">Instances
          <input data-user-count="${escapeHtml(user.id)}" type="number" min="1" value="1">
        </label>
        <div class="button-row inline-actions">
          <button data-action="resolve-user" data-user-id="${escapeHtml(user.id)}" class="secondary">Resolve</button>
          <button data-action="run-start-user" data-user-id="${escapeHtml(user.id)}">Start Meeting</button>
          <button data-action="run-join-user" data-user-id="${escapeHtml(user.id)}" class="secondary">Join Meeting</button>
          <button data-action="delete-user" data-user-id="${escapeHtml(user.id)}" class="danger">Delete</button>
        </div>
      </div>
    `;
  }).join('');

  if (payload.warnings?.length) {
    log('User list warnings', payload.warnings);
  }
}

function renderContainers(containers) {
  if (!Array.isArray(containers)) {
    containersEl.innerHTML = `<div class="pill warn"><span>Docker</span><strong>${escapeHtml(containers.error || 'unavailable')}</strong></div>`;
    return;
  }

  state.containers = containers;
  const sorted = [...containers].sort((a, b) => {
    const runningSort = Number(Boolean(b.running)) - Number(Boolean(a.running));
    if (runningSort) return runningSort;
    return new Date(b.startedAt || 0) - new Date(a.startedAt || 0);
  });

  if (!sorted.length) {
    containersEl.innerHTML = '<div class="pill"><span>No load-test containers</span><strong>idle</strong></div>';
    return;
  }

  containersEl.innerHTML = sorted.map(container => {
    const isHostContainer = container.mode === 'startmeeting';
    const canRtms = container.running && container.meetingNumber && isHostContainer;
    const health = containerHealth(container);
    const rtmsTitle = canRtms
      ? 'Start or stop RTMS as the host user stored on this start-mode container.'
      : 'RTMS start requires a running startmeeting container for the meeting host or alternative host.';
    return `
      <div class="container-item ${container.running ? 'running' : 'stopped'}">
        <div>
          <div class="health-line">
            <span class="health-badge ${escapeHtml(health.className)}">${escapeHtml(health.label)}</span>
            <small>${escapeHtml(health.detail)}</small>
          </div>
          <strong>${escapeHtml(container.name || container.id)}</strong>
          <code>${escapeHtml(container.image || '')}</code>
        </div>
        <div class="container-meta">
          ${statsMeta(container)}
          <span>${escapeHtml(container.status || '')}</span>
          <span>${escapeHtml(container.mode || '')}</span>
          <span>${container.meetingNumber ? `meeting ${escapeHtml(container.meetingNumber)}` : 'no meeting label'}</span>
          <span>${container.userId ? `rtms user ${escapeHtml(container.userId)}` : 'no rtms user label'}</span>
          <span>${escapeHtml(container.userEmail || '')}</span>
        </div>
        <div class="button-row inline-actions">
          <button data-action="rtms-start" data-container-id="${escapeHtml(container.id)}" title="${escapeHtml(rtmsTitle)}" ${canRtms ? '' : 'disabled'}>Start RTMS</button>
          <button data-action="rtms-stop" data-container-id="${escapeHtml(container.id)}" class="secondary" title="${escapeHtml(rtmsTitle)}" ${canRtms ? '' : 'disabled'}>Stop RTMS</button>
          <button class="danger" data-action="kill-container" data-container-id="${escapeHtml(container.id)}">Kill</button>
        </div>
      </div>
    `;
  }).join('');
}

async function loadEnv() {
  const result = await request('/api/env');
  renderEnvFields(result.values || []);
}

async function refreshStatus() {
  const status = await request('/api/status');
  state.status = status;
  renderStatus(status);
  renderContainers(status.containers);
  if (containerLastCheckEl) {
    containerLastCheckEl.textContent = new Date().toLocaleTimeString();
  }
}

async function refreshUsers() {
  renderUsers(await request('/api/managed-users'));
}

async function suggestUser() {
  const suggestion = await request('/api/managed-users/suggest');
  const form = document.querySelector('[data-form="create-user"]');
  form.elements.email.value = suggestion.email;
  form.elements.firstName.value = suggestion.firstName;
  form.elements.lastName.value = suggestion.lastName;
  form.elements.type.value = String(suggestion.type || 2);
}

async function refreshAll() {
  await Promise.all([
    loadEnv(),
    refreshStatus(),
    refreshUsers()
  ]);
  const createForm = document.querySelector('[data-form="create-user"]');
  if (!createForm.elements.email.value) {
    await suggestUser();
  }
}

function countForUser(userId) {
  return document.querySelector(`[data-user-count="${CSS.escape(userId)}"]`)?.value || '1';
}

function joinTarget() {
  const meetingNumber = document.querySelector('[data-join-target="meetingNumber"]')?.value.trim() || '';
  const meetingPassword = document.querySelector('[data-join-target="meetingPassword"]')?.value || '';
  return meetingNumber ? { meetingNumber, meetingPassword } : {};
}

async function handleForm(form) {
  const type = form.dataset.form;
  const data = formObject(form);

  if (type === 'env') {
    const result = await request('/api/env', {
      method: 'POST',
      body: JSON.stringify({ values: data })
    });
    renderEnvFields(result.values || []);
    renderStatus(result.status);
    log('Environment saved', { ok: true, note: 'Restart is required if HOST or PORT changed.' });
    return;
  }

  if (type === 'create-user') {
    const randomized = await request('/api/managed-users/suggest');
    form.elements.email.value = randomized.email;
    form.elements.firstName.value = randomized.firstName;
    form.elements.lastName.value = randomized.lastName;
    form.elements.type.value = String(randomized.type || 2);
    const result = await request('/api/managed-users', {
      method: 'POST',
      body: JSON.stringify({
        ...data,
        email: randomized.email,
        firstName: randomized.firstName,
        lastName: randomized.lastName,
        type: randomized.type || 2
      })
    });
    log('custCreate user created', result);
    await refreshUsers();
    await suggestUser();
    return;
  }

  if (type === 'adhoc-join') {
    const result = await request('/api/run/join', {
      method: 'POST',
      body: JSON.stringify(data)
    });
    log('Manual join containers launched', result);
    await refreshStatus();
  }
}

async function runUser(userId, mode) {
  const target = mode === 'join' ? joinTarget() : {};
  const result = await request('/api/run/selected', {
    method: 'POST',
    body: JSON.stringify({
      mode,
      userId,
      count: countForUser(userId),
      ...target
    })
  });
  log(mode === 'start' ? 'Start meeting containers launched' : 'Join meeting containers launched', result);
  await Promise.all([refreshStatus(), refreshUsers()]);
}

async function startAllUsers() {
  if (!state.users.length) {
    throw new Error('No custCreate users are available to start.');
  }

  const results = [];
  for (const user of state.users) {
    const result = await request('/api/run/selected', {
      method: 'POST',
      body: JSON.stringify({
        mode: 'start',
        userId: user.id,
        count: countForUser(user.id)
      })
    });
    results.push({
      email: user.email,
      meeting: result.meeting?.number || '',
      fallbackCreated: Boolean(result.fallbackCreated),
      started: result.started?.length || 0
    });
  }

  log('Start meeting for all completed', results);
  await Promise.all([refreshStatus(), refreshUsers()]);
}

async function startRtmsForAll() {
  const eligible = state.containers.filter(container => container.running && container.meetingNumber && container.mode === 'startmeeting');
  if (!eligible.length) {
    throw new Error('No running host start containers with meeting labels are available for RTMS.');
  }

  const results = [];
  for (const container of eligible) {
    const result = await request('/api/zoom/rtms/status', {
      method: 'POST',
      body: JSON.stringify({
        containerId: container.id,
        participantUserId: container.userId,
        action: 'start'
      })
    });
    results.push({
      container: container.name || container.id,
      meetingId: result.meetingId,
      ok: result.ok
    });
  }

  log('Start RTMS for all completed', results);
}

document.addEventListener('submit', async event => {
  const form = event.target.closest('form[data-form]');
  if (!form) return;

  event.preventDefault();
  const button = form.querySelector('button[type="submit"], button:not([type])');
  if (button) button.disabled = true;
  try {
    await handleForm(form);
  } catch (error) {
    log('Error', error.body || { error: error.message });
  } finally {
    if (button) button.disabled = false;
  }
});

document.addEventListener('click', async event => {
  const action = event.target.dataset.action;
  if (!action) return;

  event.preventDefault();
  event.target.disabled = true;
  try {
    if (action === 'refresh-all') {
      await refreshAll();
    } else if (action === 'refresh-users') {
      await refreshUsers();
    } else if (action === 'suggest-user') {
      await suggestUser();
    } else if (action === 'test-oauth') {
      log('Zoom OAuth test', await request('/api/zoom/oauth/test', { method: 'POST', body: '{}' }));
    } else if (action === 'docker-login') {
      log('Docker login', await request('/api/docker/login', { method: 'POST', body: '{}' }));
    } else if (action === 'start-all-users') {
      await startAllUsers();
    } else if (action === 'resolve-user') {
      const userId = event.target.dataset.userId;
      log('Instant meeting resolved', await request(`/api/managed-users/${encodeURIComponent(userId)}/resolve`, { method: 'POST', body: '{}' }));
      await refreshUsers();
    } else if (action === 'run-start-user') {
      await runUser(event.target.dataset.userId, 'start');
    } else if (action === 'run-join-user') {
      await runUser(event.target.dataset.userId, 'join');
    } else if (action === 'delete-user') {
      const userId = event.target.dataset.userId;
      const user = state.users.find(item => item.id === userId);
      if (!window.confirm(`Delete ${user?.email || userId} from Zoom and this manager?`)) return;
      log('custCreate user deleted', await request(`/api/managed-users/${encodeURIComponent(userId)}`, { method: 'DELETE' }));
      await refreshUsers();
    } else if (action === 'rtms-start' || action === 'rtms-stop') {
      const result = await request('/api/zoom/rtms/status', {
        method: 'POST',
        body: JSON.stringify({
          containerId: event.target.dataset.containerId,
          participantUserId: state.containers.find(container => container.id === event.target.dataset.containerId)?.userId || '',
          action: action === 'rtms-start' ? 'start' : 'stop'
        })
      });
      log(action === 'rtms-start' ? 'RTMS start requested' : 'RTMS stop requested', result);
    } else if (action === 'rtms-start-all') {
      await startRtmsForAll();
    } else if (action === 'kill-container') {
      log('Container killed', await request('/api/run/kill', {
        method: 'POST',
        body: JSON.stringify({ containerId: event.target.dataset.containerId })
      }));
      await refreshStatus();
    } else if (action === 'kill-all') {
      log('Killed all containers', await request('/api/run/kill', { method: 'POST', body: JSON.stringify({ target: 'all' }) }));
      await refreshStatus();
    }
  } catch (error) {
    log('Error', error.body || { error: error.message });
  } finally {
    event.target.disabled = false;
  }
});

async function refreshContainerHealth() {
  if (containerRefreshInFlight) return;
  containerRefreshInFlight = true;
  try {
    await refreshStatus();
  } catch (error) {
    if (containerLastCheckEl) {
      containerLastCheckEl.textContent = `failed ${new Date().toLocaleTimeString()}`;
    }
  } finally {
    containerRefreshInFlight = false;
  }
}

refreshAll().catch(error => log('Initial load failed', error.body || { error: error.message }));
window.setInterval(refreshContainerHealth, containerRefreshMs);
