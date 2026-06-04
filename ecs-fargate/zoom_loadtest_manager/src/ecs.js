import {
  DescribeTasksCommand,
  ECSClient,
  ListTasksCommand,
  RunTaskCommand,
  StopTaskCommand
} from '@aws-sdk/client-ecs';

import { config } from './config.js';
import { HttpError } from './errors.js';

const runTaskConcurrency = 10;

function ecsClient() {
  return new ECSClient({ region: config.ecs.region });
}

function requireEcsConfig() {
  const missing = [];
  if (!config.ecs.cluster) missing.push('ECS_CLUSTER');
  if (!config.ecs.taskDefinition) missing.push('ECS_TASK_DEFINITION');
  if (!config.ecs.containerName) missing.push('ECS_CONTAINER_NAME');
  if (!config.ecs.subnets.length) missing.push('ECS_SUBNETS');
  if (missing.length) {
    throw new HttpError(500, `Missing ECS Fargate config: ${missing.join(', ')}`);
  }
}

function chunked(values, size) {
  const chunks = [];
  for (let index = 0; index < values.length; index += size) {
    chunks.push(values.slice(index, index + size));
  }
  return chunks;
}

function addOptionalEnv(environment, key, value) {
  if (value !== undefined && value !== null && String(value) !== '') {
    environment.push({ name: key, value: String(value) });
  }
}

function tagMap(tags = []) {
  return Object.fromEntries(tags.map(tag => [tag.key, tag.value]));
}

function taskId(taskArn = '') {
  return String(taskArn).split('/').pop() || taskArn;
}

function shortTaskId(taskArn = '') {
  return taskId(taskArn).slice(0, 12);
}

function tagList(values) {
  return Object.entries(values)
    .filter(([, value]) => value !== undefined && value !== null && String(value) !== '')
    .map(([key, value]) => ({ key, value: String(value) }));
}

function networkConfiguration() {
  const awsvpcConfiguration = {
    subnets: config.ecs.subnets,
    assignPublicIp: config.ecs.assignPublicIp ? 'ENABLED' : 'DISABLED'
  };
  if (config.ecs.securityGroups.length) {
    awsvpcConfiguration.securityGroups = config.ecs.securityGroups;
  }
  return { awsvpcConfiguration };
}

function environmentForTask(input, mode, index) {
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
  return environment;
}

function taskToContainer(task) {
  const tags = tagMap(task.tags);
  const taskArn = task.taskArn || '';
  const container = task.containers?.[0] || {};
  const lastStatus = task.lastStatus || '';
  const desiredStatus = task.desiredStatus || '';
  const running = lastStatus === 'RUNNING';
  const stopped = lastStatus === 'STOPPED';

  return {
    id: shortTaskId(taskArn),
    fullId: taskArn,
    name: tags['zoom-loadtest.name'] || taskId(taskArn),
    image: container.image || '',
    status: lastStatus,
    desiredStatus,
    health: running ? 'alive' : 'exited',
    healthStatus: '',
    running,
    exitCode: container.exitCode ?? '',
    startedAt: task.startedAt ? task.startedAt.toISOString() : '',
    finishedAt: task.stoppedAt ? task.stoppedAt.toISOString() : '',
    stoppedReason: task.stoppedReason || '',
    project: tags['zoom-loadtest.project'] || '',
    mode: tags['zoom-loadtest.mode'] || '',
    runId: tags['zoom-loadtest.run-id'] || '',
    meetingNumber: tags['zoom-loadtest.meeting-number'] || '',
    userId: tags['zoom-loadtest.user-id'] || '',
    userEmail: tags['zoom-loadtest.user-email'] || '',
    userType: tags['zoom-loadtest.user-type'] || '',
    stats: null,
    labels: tags,
    taskArn,
    launchType: task.launchType || '',
    platformVersion: task.platformVersion || ''
  };
}

async function describeTaskArns(client, taskArns) {
  if (!taskArns.length) return [];
  const described = [];
  for (const batch of chunked(taskArns, 100)) {
    const result = await client.send(new DescribeTasksCommand({
      cluster: config.ecs.cluster,
      tasks: batch,
      include: ['TAGS']
    }));
    described.push(...(result.tasks || []));
  }
  return described;
}

async function listTaskArns(client, desiredStatus) {
  const taskArns = [];
  let nextToken = '';
  do {
    const result = await client.send(new ListTasksCommand({
      cluster: config.ecs.cluster,
      family: config.ecs.taskFamily || undefined,
      desiredStatus,
      nextToken: nextToken || undefined
    }));
    taskArns.push(...(result.taskArns || []));
    nextToken = result.nextToken || '';
  } while (nextToken);
  return taskArns;
}

export async function dockerLogin() {
  return {
    ok: true,
    note: 'ECS Fargate pulls from ECR using the task execution role; docker login is not used.'
  };
}

export async function startContainers(input) {
  requireEcsConfig();
  const client = ecsClient();
  const mode = input.mode === 'start' ? 'start' : 'join';
  const project = input.project || config.ecs.project;
  const runId = input.runId || new Date().toISOString().replace(/[-:TZ.]/g, '').slice(0, 14);
  const count = Number(input.count || 1);
  const indexes = Array.from({ length: count }, (_, index) => index + 1);
  const started = [];

  async function runOne(index) {
    const name = `${project}-${mode}-${runId}-${index}`;
    const result = await client.send(new RunTaskCommand({
      cluster: config.ecs.cluster,
      taskDefinition: config.ecs.taskDefinition,
      launchType: config.ecs.launchType,
      platformVersion: config.ecs.platformVersion,
      count: 1,
      startedBy: runId.slice(0, 36),
      enableECSManagedTags: true,
      propagateTags: 'TASK_DEFINITION',
      networkConfiguration: networkConfiguration(),
      overrides: {
        cpu: config.ecs.taskCpu,
        memory: config.ecs.taskMemory,
        containerOverrides: [{
          name: config.ecs.containerName,
          environment: environmentForTask(input, mode, index)
        }]
      },
      tags: tagList({
        'zoom-loadtest': 'true',
        'zoom-loadtest.name': name,
        'zoom-loadtest.project': project,
        'zoom-loadtest.mode': mode === 'start' ? 'startmeeting' : 'joinmeeting',
        'zoom-loadtest.run-id': runId,
        'zoom-loadtest.started-at': new Date().toISOString(),
        'zoom-loadtest.meeting-number': input.meetingNumber,
        'zoom-loadtest.user-id': input.userId,
        'zoom-loadtest.user-email': input.userEmail,
        'zoom-loadtest.user-type': input.userType
      })
    }));

    if (result.failures?.length) {
      throw new HttpError(500, 'ECS RunTask failed', { failures: result.failures, index });
    }

    const task = result.tasks?.[0] || {};
    return {
      name,
      containerId: shortTaskId(task.taskArn),
      taskArn: task.taskArn,
      image: '',
      project,
      runId,
      mode,
      meetingNumber: input.meetingNumber,
      userEmail: input.userEmail || ''
    };
  }

  for (const batch of chunked(indexes, runTaskConcurrency)) {
    started.push(...await Promise.all(batch.map(runOne)));
  }

  return started;
}

export async function listContainers() {
  requireEcsConfig();
  const client = ecsClient();
  const running = await listTaskArns(client, 'RUNNING');
  const stopped = await listTaskArns(client, 'STOPPED');
  const tasks = await describeTaskArns(client, [...new Set([...running, ...stopped])]);
  return tasks
    .map(taskToContainer)
    .filter(task => task.project === config.ecs.project || task.labels['zoom-loadtest'] === 'true');
}

export async function killContainers({ target = 'all', project = '' } = {}) {
  requireEcsConfig();
  const client = ecsClient();
  const containers = await listContainers();
  const selectedProject = project || config.ecs.project;
  const selected = containers.filter(container => {
    if (target && target !== 'all' && target !== 'join' && target !== 'start') {
      return container.id === target || container.fullId === target || container.name === target || container.taskArn === target;
    }
    if (container.project !== selectedProject) return false;
    if (target === 'join') return container.mode === 'joinmeeting';
    if (target === 'start') return container.mode === 'startmeeting';
    return true;
  });

  const stopped = [];
  for (const container of selected.filter(item => item.running)) {
    await client.send(new StopTaskCommand({
      cluster: config.ecs.cluster,
      task: container.taskArn,
      reason: 'Stopped by Zoom load-test manager'
    }));
    stopped.push(container.taskArn);
  }

  return [{ project: selectedProject, target, count: stopped.length, containerIds: stopped }];
}
