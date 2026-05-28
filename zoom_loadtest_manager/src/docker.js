import { spawn } from 'node:child_process';
import { config } from './config.js';
import { HttpError } from './errors.js';

function runCommand(command, args, options = {}) {
  return new Promise((resolve, reject) => {
    const child = spawn(command, args, {
      stdio: ['pipe', 'pipe', 'pipe'],
      env: process.env
    });

    let stdout = '';
    let stderr = '';
    const timeout = setTimeout(() => {
      child.kill('SIGTERM');
      reject(new HttpError(504, `${command} timed out`));
    }, options.timeoutMs ?? 120_000);

    child.stdout.on('data', chunk => {
      stdout += chunk.toString();
    });
    child.stderr.on('data', chunk => {
      stderr += chunk.toString();
    });
    child.on('error', error => {
      clearTimeout(timeout);
      reject(new HttpError(500, `${command} failed to start`, { message: error.message }));
    });
    child.on('close', code => {
      clearTimeout(timeout);
      if (code === 0) {
        resolve({ stdout, stderr });
      } else {
        if (command === 'docker' && /permission denied.*docker\.sock|permission denied while trying to connect to the docker API/i.test(stderr)) {
          reject(new HttpError(500, 'Docker socket permission denied', {
            stdout,
            stderr,
            fix: 'Add the service user to the docker group, then log out/in or restart the manager: sudo usermod -aG docker dreamtcs'
          }));
          return;
        }
        reject(new HttpError(500, `${command} exited with code ${code}`, { stdout, stderr }));
      }
    });

    if (options.input) {
      child.stdin.write(options.input);
    }
    child.stdin.end();
  });
}

export async function dockerLogin() {
  if (!config.docker.registryUsername || !config.docker.registryPassword) {
    throw new HttpError(400, 'DOCKER_REGISTRY_USERNAME and DOCKER_REGISTRY_PASSWORD are required for docker login');
  }

  await runCommand(
    'docker',
    ['login', config.docker.registryUrl, '--username', config.docker.registryUsername, '--password-stdin'],
    { input: `${config.docker.registryPassword}\n`, timeoutMs: 60_000 }
  );
  return { registry: config.docker.registryUrl, username: config.docker.registryUsername };
}

function addOptionalEnv(args, key, value) {
  if (value !== undefined && value !== null && String(value) !== '') {
    args.push('-e', `${key}=${value}`);
  }
}

function addOptionalLabel(args, key, value) {
  if (value !== undefined && value !== null && String(value) !== '') {
    args.push('--label', `${key}=${String(value)}`);
  }
}

function parseDockerStats(stdout) {
  const statsById = new Map();
  for (const line of stdout.split('\n').map(entry => entry.trim()).filter(Boolean)) {
    try {
      const raw = JSON.parse(line);
      const id = String(raw.ID || raw.Container || '');
      const name = String(raw.Name || '');
      const stats = {
        cpuPercent: raw.CPUPerc || '',
        memoryUsage: raw.MemUsage || '',
        memoryPercent: raw.MemPerc || '',
        networkIo: raw.NetIO || '',
        blockIo: raw.BlockIO || '',
        pids: raw.PIDs || ''
      };
      for (const key of [id, id.slice(0, 12), name].filter(Boolean)) {
        statsById.set(key, stats);
      }
    } catch {
      // Ignore malformed docker stats lines instead of breaking container health.
    }
  }
  return statsById;
}

async function statsForRunningContainers(containers) {
  const runningIds = containers
    .filter(container => container.State?.Running)
    .map(container => container.Id)
    .filter(Boolean);
  if (!runningIds.length) return new Map();

  try {
    const result = await runCommand('docker', [
      'stats',
      '--no-stream',
      '--format',
      '{{json .}}',
      ...runningIds
    ], { timeoutMs: 20_000 });
    return parseDockerStats(result.stdout);
  } catch {
    return new Map();
  }
}

export async function startContainers(input) {
  const mode = input.mode === 'start' ? 'start' : 'join';
  const project = input.project || config.docker.project;
  const image = input.image || config.docker.image;
  const runId = input.runId || new Date().toISOString().replace(/[-:TZ.]/g, '').slice(0, 14);
  const count = Number(input.count || 1);
  const started = [];

  if (config.features.dockerLoginBeforeRun) {
    await dockerLogin();
  }

  for (let i = 1; i <= count; i += 1) {
    const name = `${project}-${mode}-${runId}-${i}`;
    await runCommand('docker', ['rm', '-f', name], { timeoutMs: 30_000 }).catch(() => null);

    const args = [
      'run', '-d',
      '--name', name,
      '--restart', input.restartPolicy || config.docker.restartPolicy,
      '--shm-size', input.shmSize || config.docker.shmSize,
      '--label', 'zoom-loadtest=true',
      '--label', `zoom-loadtest.project=${project}`,
      '--label', `zoom-loadtest.mode=${mode === 'start' ? 'startmeeting' : 'joinmeeting'}`,
      '--label', `zoom-loadtest.run-id=${runId}`,
      '--label', `zoom-loadtest.started-at=${new Date().toISOString()}`
    ];
    addOptionalLabel(args, 'zoom-loadtest.meeting-number', input.meetingNumber);
    addOptionalLabel(args, 'zoom-loadtest.user-id', input.userId);
    addOptionalLabel(args, 'zoom-loadtest.user-email', input.userEmail);
    addOptionalLabel(args, 'zoom-loadtest.user-type', input.userType);

    addOptionalEnv(args, 'MEETING_NUMBER', input.meetingNumber);
    addOptionalEnv(args, 'MEETING_PASSWORD', input.meetingPassword);
    addOptionalEnv(args, 'JWT_TOKEN', input.jwtToken);
    addOptionalEnv(args, 'USER_ZAK', input.userZak);
    addOptionalEnv(args, 'MEETING_MODE', mode);
    addOptionalEnv(args, 'JWT_ROLE', mode === 'start' ? '1' : '0');
    addOptionalEnv(args, 'ZOOM_USERNAME', `${input.usernamePrefix || (mode === 'start' ? 'LoadHost' : 'LoadBot')}-${i}`);
    addOptionalEnv(args, 'ZOOM_INSTANCE_ID', String(i));
    addOptionalEnv(args, 'USE_JWT_TOKEN_FROM_WEB_SERVICE', 'false');
    addOptionalEnv(args, 'SEND_VIDEO_RAW_DATA', input.sendVideoRawData ?? 'true');
    addOptionalEnv(args, 'SEND_AUDIO_RAW_DATA', input.sendAudioRawData ?? 'true');
    addOptionalEnv(args, 'CHAT_DEMO', input.chatDemo ?? 'true');
    addOptionalEnv(args, 'EXIT_ON_MEETING_END', input.exitOnMeetingEnd ?? 'true');
    addOptionalEnv(args, 'MEDIA_INDEX', input.mediaIndex);
    addOptionalEnv(args, 'VIDEO_WIDTH', input.videoWidth);
    addOptionalEnv(args, 'VIDEO_HEIGHT', input.videoHeight);
    addOptionalEnv(args, 'VIDEO_FPS', input.videoFps);
    addOptionalEnv(args, 'AUDIO_SAMPLE_RATE', input.audioSampleRate);
    addOptionalEnv(args, 'AUDIO_CHANNELS', input.audioChannels);

    if (input.cpus || config.docker.cpus) args.push('--cpus', input.cpus || config.docker.cpus);
    if (input.memory || config.docker.memory) args.push('--memory', input.memory || config.docker.memory);
    if (input.network || config.docker.network) args.push('--network', input.network || config.docker.network);

    args.push(image);
    const result = await runCommand('docker', args, { timeoutMs: 120_000 });
    started.push({
      name,
      containerId: result.stdout.trim(),
      image,
      project,
      runId,
      mode,
      meetingNumber: input.meetingNumber,
      userEmail: input.userEmail || ''
    });
  }

  return started;
}

export async function listContainers() {
  const idsResult = await runCommand('docker', [
    'ps', '-a',
    '--filter', 'label=zoom-loadtest=true',
    '-q'
  ], { timeoutMs: 30_000 });

  const ids = idsResult.stdout
    .split('\n')
    .map(line => line.trim())
    .filter(Boolean);

  if (!ids.length) return [];

  const inspectResult = await runCommand('docker', ['inspect', ...ids], { timeoutMs: 30_000 });
  const inspected = JSON.parse(inspectResult.stdout);
  const statsById = await statsForRunningContainers(inspected);

  return inspected.map(container => {
    const labels = container.Config?.Labels ?? {};
    const dockerState = container.State ?? {};
    const running = Boolean(dockerState.Running);
    const status = dockerState.Status || '';
    const healthStatus = dockerState.Health?.Status || '';
    const health = running ? (healthStatus || 'alive') : (status === 'dead' ? 'dead' : 'exited');
    const id = String(container.Id || '');
    const name = String(container.Name || '').replace(/^\//, '');
    return {
      id: id.slice(0, 12),
      fullId: id,
      name,
      image: container.Config?.Image || '',
      status,
      health,
      healthStatus,
      running,
      exitCode: dockerState.ExitCode,
      startedAt: dockerState.StartedAt,
      finishedAt: dockerState.FinishedAt,
      project: labels['zoom-loadtest.project'] || '',
      mode: labels['zoom-loadtest.mode'] || '',
      runId: labels['zoom-loadtest.run-id'] || '',
      meetingNumber: labels['zoom-loadtest.meeting-number'] || '',
      userId: labels['zoom-loadtest.user-id'] || '',
      userEmail: labels['zoom-loadtest.user-email'] || '',
      userType: labels['zoom-loadtest.user-type'] || '',
      stats: statsById.get(id) || statsById.get(id.slice(0, 12)) || statsById.get(name) || null,
      labels
    };
  });
}

export async function killContainers({ target = 'all', project = '' } = {}) {
  if (target && target !== 'all' && target !== 'join' && target !== 'start') {
    await runCommand('docker', ['rm', '-f', target], { timeoutMs: 60_000 });
    return [{ project: 'single-container', count: 1, containerIds: [target] }];
  }

  const filters = [
    'ps', '-aq',
    '--filter', 'label=zoom-loadtest=true',
    '--filter', `label=zoom-loadtest.project=${project || config.docker.project}`
  ];
  if (target === 'join' || target === 'start') {
    filters.push('--filter', `label=zoom-loadtest.mode=${target === 'start' ? 'startmeeting' : 'joinmeeting'}`);
  }

  const ids = await runCommand('docker', filters, { timeoutMs: 30_000 });
  const containerIds = ids.stdout.split('\n').map(line => line.trim()).filter(Boolean);
  if (!containerIds.length) {
    return [{ project: project || config.docker.project, target, count: 0, containerIds: [] }];
  }

  await runCommand('docker', ['rm', '-f', ...containerIds], { timeoutMs: 60_000 });
  return [{ project: project || config.docker.project, target, count: containerIds.length, containerIds }];
}
