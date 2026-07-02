import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { DynamoDBClient } from '@aws-sdk/client-dynamodb';
import { DeleteCommand, DynamoDBDocumentClient, PutCommand, ScanCommand } from '@aws-sdk/lib-dynamodb';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const usersPath = path.join(__dirname, '..', '.data', 'custcreate-users.json');
const tableName = process.env.STATUS_TABLE_NAME || '';
const documentClient = tableName ? DynamoDBDocumentClient.from(new DynamoDBClient({})) : null;

async function readLocalUsers() {
  try {
    return JSON.parse(await fs.readFile(usersPath, 'utf8')).users || [];
  } catch (error) {
    if (error.code === 'ENOENT') return [];
    throw error;
  }
}

async function writeLocalUsers(users) {
  await fs.mkdir(path.dirname(usersPath), { recursive: true });
  await fs.writeFile(usersPath, `${JSON.stringify({ users }, null, 2)}\n`, 'utf8');
}

async function putUser(user) {
  if (!documentClient) return;
  await documentClient.send(new PutCommand({
    TableName: tableName,
    Item: { pk: 'managed-user', sk: user.id, ...user }
  }));
}

export async function listManagedUsers() {
  if (!documentClient) {
    return (await readLocalUsers()).sort((a, b) => String(a.email).localeCompare(String(b.email)));
  }
  const result = await documentClient.send(new ScanCommand({
    TableName: tableName,
    FilterExpression: 'pk = :pk',
    ExpressionAttributeValues: { ':pk': 'managed-user' }
  }));
  return (result.Items || [])
    .map(({ pk, sk, ...user }) => user)
    .sort((a, b) => String(a.email).localeCompare(String(b.email)));
}

export async function addManagedUser(user) {
  const now = new Date().toISOString();
  const record = {
    id: user.id || user.zoomUserId || user.email,
    zoomUserId: user.zoomUserId || user.id || user.email,
    email: user.email,
    firstName: user.firstName || '',
    lastName: user.lastName || '',
    type: user.type || 1,
    source: 'custCreate',
    createdAt: user.createdAt || now,
    updatedAt: now,
    meeting: user.meeting || null
  };
  if (documentClient) {
    await putUser(record);
  } else {
    const users = (await listManagedUsers()).filter(existing => existing.id !== record.id && existing.email !== record.email);
    users.push(record);
    await writeLocalUsers(users);
  }
  return record;
}

export async function updateManagedUser(id, patch) {
  const user = await findManagedUser(id);
  if (!user) return null;
  const updated = { ...user, ...patch, updatedAt: new Date().toISOString() };
  if (documentClient) {
    await putUser(updated);
  } else {
    const users = (await listManagedUsers()).filter(existing => existing.id !== user.id);
    users.push(updated);
    await writeLocalUsers(users);
  }
  return updated;
}

export async function removeManagedUser(id) {
  const user = await findManagedUser(id);
  if (!user) return { removed: 0 };
  if (documentClient) {
    await documentClient.send(new DeleteCommand({ TableName: tableName, Key: { pk: 'managed-user', sk: user.id } }));
  } else {
    await writeLocalUsers((await listManagedUsers()).filter(existing => existing.id !== user.id));
  }
  return { removed: 1 };
}

export async function findManagedUser(id) {
  return (await listManagedUsers()).find(user => user.id === id || user.email === id || user.zoomUserId === id) || null;
}

export function suggestManagedUser(domain) {
  const suffix = `${Date.now().toString(36)}${Math.random().toString(36).slice(2, 6)}`;
  return {
    email: `loadtest-${suffix}@${domain}`,
    firstName: 'Load',
    lastName: `Host${suffix.slice(-4).toUpperCase()}`,
    type: 2
  };
}
