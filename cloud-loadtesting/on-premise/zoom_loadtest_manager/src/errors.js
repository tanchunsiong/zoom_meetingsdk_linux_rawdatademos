export class HttpError extends Error {
  constructor(status, message, details = undefined) {
    super(message);
    this.name = 'HttpError';
    this.status = status;
    this.details = details;
  }
}

export function requireField(value, name) {
  if (value === undefined || value === null || String(value).trim() === '') {
    throw new HttpError(400, `${name} is required`);
  }
  return String(value).trim();
}

export function asPositiveInteger(value, name, fallback = undefined) {
  if ((value === undefined || value === null || value === '') && fallback !== undefined) {
    return fallback;
  }
  const parsed = Number.parseInt(value, 10);
  if (!Number.isFinite(parsed) || parsed < 1) {
    throw new HttpError(400, `${name} must be a positive integer`);
  }
  return parsed;
}
