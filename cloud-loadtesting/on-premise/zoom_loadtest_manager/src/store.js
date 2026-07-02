import fs from 'node:fs/promises';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const dataDir = path.join(__dirname, '..', '.data');
const usersPath = path.join(dataDir, 'custcreate-users.json');

async function readJson(filePath, fallback) {
  try {
    return JSON.parse(await fs.readFile(filePath, 'utf8'));
  } catch (error) {
    if (error.code === 'ENOENT') return fallback;
    throw error;
  }
}

async function writeJson(filePath, value) {
  await fs.mkdir(path.dirname(filePath), { recursive: true });
  await fs.writeFile(filePath, `${JSON.stringify(value, null, 2)}\n`, 'utf8');
}

export async function listManagedUsers() {
  const data = await readJson(usersPath, { users: [] });
  return data.users.sort((a, b) => String(a.email).localeCompare(String(b.email)));
}

export async function addManagedUser(user) {
  const users = await listManagedUsers();
  const now = new Date().toISOString();
  const id = user.id || user.zoomUserId || user.email;
  const record = {
    id,
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

  const next = users.filter(existing => existing.id !== record.id && existing.email !== record.email);
  next.push(record);
  await writeJson(usersPath, { users: next });
  return record;
}

export async function updateManagedUser(id, patch) {
  const users = await listManagedUsers();
  const index = users.findIndex(user => user.id === id || user.email === id || user.zoomUserId === id);
  if (index === -1) return null;
  users[index] = {
    ...users[index],
    ...patch,
    updatedAt: new Date().toISOString()
  };
  await writeJson(usersPath, { users });
  return users[index];
}

export async function removeManagedUser(id) {
  const users = await listManagedUsers();
  const next = users.filter(user => user.id !== id && user.email !== id && user.zoomUserId !== id);
  await writeJson(usersPath, { users: next });
  return {
    removed: users.length - next.length
  };
}

export async function findManagedUser(id) {
  const users = await listManagedUsers();
  return users.find(user => user.id === id || user.email === id || user.zoomUserId === id) || null;
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
