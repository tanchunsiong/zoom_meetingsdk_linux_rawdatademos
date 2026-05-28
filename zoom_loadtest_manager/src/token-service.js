import { config } from './config.js';
import { HttpError } from './errors.js';

function firstString(...values) {
  for (const value of values) {
    if (typeof value === 'string' && value.trim()) return value.trim();
  }
  return '';
}

export async function getMeetingSdkToken({ meetingNumber, role }) {
  const response = await fetch(config.zoom.tokenEndpoint, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json'
    },
    body: JSON.stringify({
      meetingNumber: String(meetingNumber),
      role: Number(role)
    })
  });

  const text = await response.text();
  let body = {};
  try {
    body = text ? JSON.parse(text) : {};
  } catch {
    body = { raw: text };
  }

  if (!response.ok) {
    throw new HttpError(response.status, 'Meeting SDK token service request failed', body);
  }

  const data = body.data ?? body.result ?? body.payload ?? {};
  const token = firstString(
    body.signature,
    body.token,
    body.jwtToken,
    body.jwt_token,
    data.signature,
    data.token,
    data.jwtToken,
    data.jwt_token
  );

  if (!token) {
    throw new HttpError(502, 'Meeting SDK token service response did not include signature/token', body);
  }

  return {
    token,
    meetingNumber: firstString(body.meetingNumber, body.meeting_number, data.meetingNumber, data.meeting_number),
    meetingPassword: firstString(body.meetingPassword, body.meeting_password, body.password, data.meetingPassword, data.meeting_password, data.password),
    raw: body
  };
}
