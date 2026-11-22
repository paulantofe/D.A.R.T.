/**
 * @file     script.js
 * @brief    Browser controls and status dashboard logic for the ESP32 server.
 */

/* ------------------ Stream Connection ------------------ */

const STREAM_URL = `http://${window.location.hostname}:81/stream`;

const stream = document.getElementById('stream');
const streamStatus = document.getElementById('stream-status');
const videoContainer = document.querySelector('.video-container');

let retryStreamTimer = null;
let streamConnected = false;
let isUnloading = false;

function tryConnectStream() {
  if (isUnloading) return;

  console.log('[STREAM] Connecting stream...');
  clearTimeout(retryStreamTimer);
  stream.setAttribute('src', `${STREAM_URL}?t=${Date.now()}`);
}

function disconnectStream() {
  isUnloading = true;
  clearTimeout(retryStreamTimer);
  stream.removeAttribute('src');
  window.stop(); 
}

stream.addEventListener('load', () => {
  streamConnected = true;
  stream.classList.add('active');
  videoContainer.classList.add('stream-active');
  streamStatus.classList.add('live');
  streamStatus.lastElementChild.textContent = 'Live';

  console.log('[STREAM] Stream is online.');
});

stream.addEventListener('error', () => {
  if (isUnloading) return;

  stream.classList.remove('active');
  videoContainer.classList.remove('stream-active');
  streamStatus.classList.remove('live');

  streamStatus.lastElementChild.textContent =
    streamConnected ? 'Reconnecting' : 'N/A';

  console.error(
    'Stream error: ' +
    (streamConnected ? 'lost stream connection.' : 'failed to connect stream.')
  );

  stream.removeAttribute('src');

  clearTimeout(retryStreamTimer);
  retryStreamTimer = setTimeout(tryConnectStream, 2000);
});

window.addEventListener('load', tryConnectStream);
window.addEventListener('pagehide', disconnectStream);

/* ------------------------------------------------------- */


/* ------------------ User Input Parsing and Send Command ------------------ */

const HEADLIGHT_ON_COMMAND = 'F';
const HEADLIGHT_OFF_COMMAND = 'f';

const buttons = {
  'w': document.getElementById('btn-forward'),
  'a': document.getElementById('btn-left'),
  's': document.getElementById('btn-backward'),
  'd': document.getElementById('btn-right'),
};

const headlightButton = document.getElementById('btn-speedlight-on');
const commandStatus = document.getElementById('cmd-status');

const keyAliases = {
  w: 'w', W: 'w',
  a: 'a', A: 'a',
  s: 's', S: 's',
  d: 'd', D: 'd',
  f: 'f', F: 'f',
  ArrowUp: 'w',
  ArrowLeft: 'a',
  ArrowDown: 's',
  ArrowRight: 'd',
};

const commands = {
  '1,-1': 'Q', // forward-left
  '1,0':  'W', // forward
  '1,1':  'E', // forward-right
  '0,-1': 'A', // left
  '0,0':  'X', // stop
  '0,1':  'D', // right
  '-1,-1': 'Z', // backward-left
  '-1,0':  'S', // backward
  '-1,1': 'C' // backward-right
};

const commandsMessage = {
  'Q': 'Forward-Left',
  'W': 'Forward',
  'E': 'Forward-Right',
  'A': 'Left',
  'X': 'Stop',
  'D': 'Right',
  'Z': 'Backward-Left',
  'S': 'Backward',
  'C': 'Backward-Right'
}

const pressedKeys = new Set();
let headlightOn = false;
let commandQueue = Promise.resolve();

function parseCommand() {
  const up = pressedKeys.has('w');
  const left = pressedKeys.has('a');
  const down = pressedKeys.has('s');
  const right = pressedKeys.has('d');

  const vertical = up === down ? 0 : up ? 1 : -1;
  const horizontal = left === right ? 0 : right ? 1 : -1;

  return commands[`${vertical},${horizontal}`];
}

async function sendCommand(command, nextHeadlightState) {
  const response = await fetch('/control', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded',
    },
    body: new URLSearchParams({ val: command }),
  });

  if (!response.ok) {
    throw new Error(`Control request failed: ${response.status}`);
  }

  headlightOn = nextHeadlightState;
  headlightStatus.textContent = headlightOn ? 'On' : 'Off';
  commandStatus.textContent = commandsMessage[command[0]];
  headlightButton.classList.toggle('pressed', headlightOn);
}

function updateCommand(nextHeadlightState = headlightOn) {
  const motorCmd = parseCommand();
  const headlightCmd = nextHeadlightState ? HEADLIGHT_ON_COMMAND : HEADLIGHT_OFF_COMMAND;
  const command = `${motorCmd}${headlightCmd}`;

  commandQueue = commandQueue
    .then(() => sendCommand(command, nextHeadlightState))
    .catch((error) => {
      console.error('[CONTROL] Rejected or failed: ', error);
    });
}

const headlightStatus = document.getElementById('light-status');

function toggleHeadlight(event) {
  if (!canControl) return;
  
  event.preventDefault();
  updateCommand(!headlightOn);
}

function pressCommand(key, event) {
  if (!canControl) return;
  
  event.preventDefault();
  pressedKeys.add(key);
  buttons[key].classList.add('pressed');
  updateCommand();
}

function releaseCommand(key, event) {
  if (!pressedKeys.has(key)) return;
  
  event.preventDefault();
  pressedKeys.delete(key);
  buttons[key].classList.remove('pressed');
  updateCommand();
}

window.addEventListener('keydown', (event) => {
  const key = keyAliases[event.key];

  if (!key || event.repeat) {
    return;
  }

  if (key === 'f') {
    toggleHeadlight(event);
    return;
  }
  
  pressCommand(key, event);
});

window.addEventListener('keyup', (event) => {
  const key = keyAliases[event.key];

  if (!key || key === 'f') {
    return;
  }

  releaseCommand(key, event);
});

for (const [key, button] of Object.entries(buttons)) {
  button.addEventListener('pointerdown', (event) => {
    button.setPointerCapture(event.pointerId);
    pressCommand(key, event);
  });

  button.addEventListener('pointerup', (event) => {
    releaseCommand(key, event);
  });

  button.addEventListener('pointercancel', (event) => {
    releaseCommand(key, event);
  });
}

headlightButton.addEventListener('pointerdown', (event) => {
  toggleHeadlight(event);
});

window.addEventListener('blur', () => {
  const hadPressedKeys = pressedKeys.size > 0;

  pressedKeys.clear();

  for (const button of Object.values(buttons)) {
    button.classList.remove('pressed');
  }
  
  if (hadPressedKeys) {
    updateCommand();
  }
});

/* ------------------------------------------------------------------------- */


/* ------------------ Status Panel and Failsafe Heartbeat ------------------ */

let canControl = false;

const statusContainers = {
  mode: document.getElementById('mode-status'),
  mode_dot: document.getElementById('mode-dot'),
  wifi: document.getElementById('wifi-status'),
  battery: document.getElementById('battery-status'),
  system: document.getElementById('system-status'),
  clients: document.getElementById('stream-clients-status'),
  link: document.getElementById('hardware-status'),
  status: document.getElementById('speed-status')
};

function updateStatus(json) {
  if (!json) {
    return;
  }

  canControl = json.Link_State === 'online' &&
             json.System_State !== 'systemfault' &&
             json.System_State !== 'offline';
             
  if (!canControl || json.System_State === 'failsafe' || json.Client_Mode !== 'Driver') {
    headlightOn = false;
    headlightStatus.textContent = 'N/A';
    headlightButton.classList.remove('pressed');
    commandStatus.textContent = 'N/A';
  }

  if (json.Client_Mode === 'Driver') {
    statusContainers.mode.classList.remove('spectator');
    statusContainers.mode.classList.add('driver');
    statusContainers.mode_dot.classList.add('driver');
    headlightStatus.textContent = headlightOn ? 'On' : 'Off';
    commandStatus.textContent = 'Stop';
  } else {
    statusContainers.mode.classList.remove('driver');
    statusContainers.mode_dot.classList.remove('driver');
    statusContainers.mode.classList.add('spectator');
  }

  if (json.Link_State === 'online') {
    statusContainers.link.classList.remove('checking');
    statusContainers.link.classList.add('online');
  } else {
    statusContainers.link.classList.remove('online');
    statusContainers.link.classList.add('checking');
  }

  statusContainers.mode.textContent = json.Client_Mode;
  statusContainers.wifi.textContent = json.RSSI;
  statusContainers.battery.textContent = json.Battery;
  if (json.System_State === 'idle' || json.System_State === 'moving') {
    statusContainers.system.textContent = 'Online';
  } else {
    statusContainers.system.textContent = json.System_State;
  }
  statusContainers.clients.textContent = json.Active_Clients;
  statusContainers.status.textContent = json.Speed;
  statusContainers.link.textContent = json.Link_State;
}

async function pollStatus() {
  try {
    const response = await fetch('/status');

    if (!response.ok) {
      throw new Error(`Status request failed: ${response.status}`);
    }

    const status = await response.json();

    updateStatus(status);
  } catch (error) {
    console.error('[STATUS] Poll failed: ', error);
  } finally {
    setTimeout(pollStatus, 1000);
  }
}

pollStatus();

/* ------------------------------------------------------------------------- */
