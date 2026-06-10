import { DefaultAzureCredential } from '@azure/identity';

import { config } from './config.js';
import { HttpError } from './errors.js';

const credential = new DefaultAzureCredential();
const activeStatuses = new Set(['Processing', 'Running']);

function requireAzureConfig() {
  const missing = [];
  if (!config.azure.subscriptionId) missing.push('AZURE_SUBSCRIPTION_ID');
  if (!config.azure.resourceGroup) missing.push('AZURE_RESOURCE_GROUP');
  if (!config.azure.jobName) missing.push('AZURE_CONTAINER_APP_JOB_NAME');
  if (!config.azure.containerName) missing.push('AZURE_CONTAINER_NAME');
  if (!config.azure.runnerImage) missing.push('AZURE_RUNNER_IMAGE');
  if (missing.length) {
    throw new HttpError(500, `Missing Azure Container Apps config: ${missing.join(', ')}`);
  }
}

function addOptionalEnv(environment, name, value) {
  if (value !== undefined && value !== null && String(value) !== '') {
    environment.push({ name, value: String(value) });
  }
}

function environmentForExecution(input, mode, index, metadata) {
  const environment = [];
  addOptionalEnv(environment, 'MEETING_NUMBER', input.meetingNumber);
  addOptionalEnv(environment, 'MEETING_PASSWORD', input.meetingPassword);
  addOptionalEnv(environment, 'JWT_TOKEN', input.jwtToken);
  addOptionalEnv(environment, 'USER_ZAK', input.userZak);
  addOptionalEnv(environment, 'MEETING_MODE', mode);
  addOptionalEnv(environment, 'JWT_ROLE', mode === 'start' ? '1' : '0');
  addOptionalEnv(environment, 'ZOOM_USERNAME', `${input.usernamePrefix || (mode === 'start' ? 'LoadHost' : 'LoadBot')}-${index}`);
  addOptionalEnv(environment, 'ZOOM_INSTANCE_ID', String(index));
  addOptionalEnv(environment, 'USE_JWT_TOKEN_FROM_WEB_SERVICE', 'false');
  addOptionalEnv(environment, 'SEND_VIDEO_RAW_DATA', input.sendVideoRawData ?? 'true');
  addOptionalEnv(environment, 'SEND_AUDIO_RAW_DATA', input.sendAudioRawData ?? 'true');
  addOptionalEnv(environment, 'CHAT_DEMO', input.chatDemo ?? 'true');
  addOptionalEnv(environment, 'EXIT_ON_MEETING_END', input.exitOnMeetingEnd ?? 'true');
  addOptionalEnv(environment, 'MEDIA_INDEX', input.mediaIndex);
  addOptionalEnv(environment, 'VIDEO_WIDTH', input.videoWidth);
  addOptionalEnv(environment, 'VIDEO_HEIGHT', input.videoHeight);
  addOptionalEnv(environment, 'VIDEO_FPS', input.videoFps);
  addOptionalEnv(environment, 'AUDIO_SAMPLE_RATE', input.audioSampleRate);
  addOptionalEnv(environment, 'AUDIO_CHANNELS', input.audioChannels);
  for (const [name, value] of Object.entries(metadata)) addOptionalEnv(environment, name, value);
  return environment;
}

function metadataFromEnv(environment = []) {
  return Object.fromEntries(
    environment
      .filter(item => item.name?.startsWith('ZOOM_LOADTEST_'))
      .map(item => [item.name, item.value || ''])
  );
}

function executionToContainer(execution) {
  const container = execution.properties?.template?.containers?.[0] || {};
  const metadata = metadataFromEnv(container.env);
  const status = execution.properties?.status || 'Unknown';
  const running = activeStatuses.has(status);
  const pending = status === 'Processing';
  const active = running;
  return {
    id: execution.name,
    fullId: execution.id,
    name: metadata.ZOOM_LOADTEST_NAME || execution.name,
    image: container.image || '',
    status,
    desiredStatus: running ? 'RUNNING' : status.toUpperCase(),
    lifecycle: active ? 'active' : 'disposed',
    health: running ? 'alive' : 'exited',
    healthStatus: status,
    active,
    running,
    pending,
    stopped: !active,
    disposed: !active,
    exitCode: '',
    startedAt: execution.properties?.startTime || '',
    finishedAt: execution.properties?.endTime || '',
    stoppedReason: '',
    project: metadata.ZOOM_LOADTEST_PROJECT || '',
    mode: metadata.ZOOM_LOADTEST_MODE || '',
    runId: metadata.ZOOM_LOADTEST_RUN_ID || '',
    meetingNumber: metadata.ZOOM_LOADTEST_MEETING_NUMBER || '',
    userId: metadata.ZOOM_LOADTEST_USER_ID || '',
    userEmail: metadata.ZOOM_LOADTEST_USER_EMAIL || '',
    userType: metadata.ZOOM_LOADTEST_USER_TYPE || '',
    stats: null,
    labels: metadata,
    taskArn: execution.id,
    launchType: 'Azure Container Apps Job',
    platformVersion: config.azure.apiVersion
  };
}

function jobUrl(suffix = '') {
  const base = `https://management.azure.com/subscriptions/${encodeURIComponent(config.azure.subscriptionId)}/resourceGroups/${encodeURIComponent(config.azure.resourceGroup)}/providers/Microsoft.App/jobs/${encodeURIComponent(config.azure.jobName)}`;
  return `${base}${suffix}?api-version=${encodeURIComponent(config.azure.apiVersion)}`;
}

async function armRequest(method, suffix, body) {
  const accessToken = await credential.getToken('https://management.azure.com/.default');
  const response = await fetch(jobUrl(suffix), {
    method,
    headers: {
      Authorization: `Bearer ${accessToken.token}`,
      'Content-Type': 'application/json'
    },
    body: body === undefined ? undefined : JSON.stringify(body)
  });
  const text = await response.text();
  let payload = {};
  try {
    payload = text ? JSON.parse(text) : {};
  } catch {
    payload = { raw: text };
  }
  if (!response.ok) {
    throw new HttpError(response.status, `Azure ARM request failed: ${method} ${suffix || '/'}`, payload);
  }
  return payload;
}

export async function dockerLogin() {
  return {
    ok: true,
    note: 'Container Apps Jobs pull from ACR using managed identity; docker login is not used at runtime.'
  };
}

export async function listContainers() {
  requireAzureConfig();
  const result = await armRequest('GET', '/executions');
  return (result.value || [])
    .map(executionToContainer)
    .filter(execution => (
      execution.active &&
      (execution.project === config.azure.project || execution.labels.ZOOM_LOADTEST_MANAGED === 'true')
    ));
}

export async function startContainers(input) {
  requireAzureConfig();
  const mode = input.mode === 'start' ? 'start' : 'join';
  const project = input.project || config.azure.project;
  const runId = input.runId || new Date().toISOString().replace(/[-:TZ.]/g, '').slice(0, 14);
  const count = Number(input.count || 1);
  if (!Number.isInteger(count) || count < 1 || count > config.azure.maxExecutions) {
    throw new HttpError(400, `count must be an integer between 1 and AZURE_MAX_EXECUTIONS (${config.azure.maxExecutions})`);
  }

  const activeCount = (await listContainers()).filter(item => item.running && item.project === project).length;
  if (activeCount + count > config.azure.maxExecutions) {
    throw new HttpError(409, `Launch would exceed AZURE_MAX_EXECUTIONS (${config.azure.maxExecutions})`, {
      activeExecutions: activeCount,
      requestedExecutions: count,
      availableExecutions: Math.max(0, config.azure.maxExecutions - activeCount)
    });
  }

  const started = [];
  for (let index = 1; index <= count; index += 1) {
    const name = `${project}-${mode}-${runId}-${index}`;
    const metadata = {
      ZOOM_LOADTEST_MANAGED: 'true',
      ZOOM_LOADTEST_NAME: name,
      ZOOM_LOADTEST_PROJECT: project,
      ZOOM_LOADTEST_MODE: mode === 'start' ? 'startmeeting' : 'joinmeeting',
      ZOOM_LOADTEST_RUN_ID: runId,
      ZOOM_LOADTEST_MEETING_NUMBER: input.meetingNumber,
      ZOOM_LOADTEST_USER_ID: input.userId,
      ZOOM_LOADTEST_USER_EMAIL: input.userEmail,
      ZOOM_LOADTEST_USER_TYPE: input.userType
    };
    const result = await armRequest('POST', '/start', {
      containers: [{
        name: config.azure.containerName,
        image: config.azure.runnerImage,
        resources: {
          cpu: config.azure.runnerCpu,
          memory: config.azure.runnerMemory
        },
        env: environmentForExecution(input, mode, index, metadata)
      }]
    });
    started.push({
      name,
      containerId: result.name || '',
      taskArn: result.id || '',
      image: config.azure.runnerImage,
      project,
      runId,
      mode,
      meetingNumber: input.meetingNumber,
      userEmail: input.userEmail || ''
    });
  }
  return started;
}

export async function killContainers({ target = 'all', project = '' } = {}) {
  requireAzureConfig();
  const selectedProject = project || config.azure.project;
  const selected = (await listContainers()).filter(container => {
    if (target && !['all', 'join', 'start'].includes(target)) {
      return [container.id, container.fullId, container.name].includes(target);
    }
    if (container.project !== selectedProject) return false;
    if (target === 'join') return container.mode === 'joinmeeting';
    if (target === 'start') return container.mode === 'startmeeting';
    return true;
  });

  const stopped = [];
  for (const container of selected.filter(item => item.running)) {
    await armRequest('POST', `/executions/${encodeURIComponent(container.id)}/stop`);
    stopped.push(container.id);
  }

  const activeRemaining = (await listContainers()).filter(container => {
    if (!container.running || container.project !== selectedProject) return false;
    if (target === 'join') return container.mode === 'joinmeeting';
    if (target === 'start') return container.mode === 'startmeeting';
    if (target && target !== 'all') return [container.id, container.fullId, container.name].includes(target);
    return true;
  });

  return [{
    project: selectedProject,
    target,
    count: stopped.length,
    stoppedCount: stopped.length,
    activeRemainingCount: activeRemaining.length,
    disposed: activeRemaining.length === 0,
    containerIds: stopped
  }];
}
