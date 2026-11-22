const WS_URL = `ws://${window.location.hostname}/ws`;
const STREAM_URL = `http://${window.location.hostname}:81/stream`;

/* ------------------ DOM Elements ------------------ */

const stream = document.getElementById('stream');
const streamStatus = document.getElementById('stream-status');
const videoContainer = document.querySelector('.video-container');

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

/* -------------------------------------------------- */


/* ------------------ Application State ------------------ */

let ws = null;
let sessionAuthorized = false;
let canControl = false;
let isUnloading = false;

let wsReconnectTimer = null;
let heartbeatTimer = null;
let retryStreamTimer = null;
let streamConnected = false;

/* ------------------------------------------------------- */


/* ------------------ Stream (Port 81) ------------------ */

function tryConnectStream() {
  if (isUnloading || !sessionAuthorized) return;

  console.log('[STREAM] Connecting...');
  clearTimeout(retryStreamTimer);
  stream.setAttribute('src', `${STREAM_URL}?t=${Date.now()}`);
}

function stopStream(statusLabel = 'N/A') {
  clearTimeout(retryStreamTimer);
  streamConnected = false;
  stream.classList.remove('active');
  videoContainer?.classList.remove('stream-active');
  streamStatus?.classList.remove('live');
  if (streamStatus?.lastElementChild) {
    streamStatus.lastElementChild.textContent = statusLabel;
  }
  stream.removeAttribute('src');
}

stream.addEventListener('load', () => {
  streamConnected = true;
  stream.classList.add('active');
  videoContainer?.classList.add('stream-active');
  streamStatus?.classList.add('live');
  if (streamStatus?.lastElementChild) {
    streamStatus.lastElementChild.textContent = 'Live';
  }
  console.log('[STREAM] Online.');
});

stream.addEventListener('error', () => {
  if (isUnloading || !sessionAuthorized) return;

  const wasConnected = streamConnected;
  stopStream(wasConnected ? 'Reconnecting' : 'N/A');

  clearTimeout(retryStreamTimer);
  retryStreamTimer = setTimeout(tryConnectStream, 2000);
});

/* ------------------------------------------------------ */


/* ------------------ WebSocket (Port 80) ------------------ */

function connectWebSocket() {
  if (isUnloading) return;

  clearTimeout(wsReconnectTimer);
  ws = new WebSocket(WS_URL);

  ws.addEventListener('message', (event) => {
    let data;
    try {
      data = JSON.parse(event.data);
    } catch {
      return;
    }

    if (data.type === 'ready') {
      sessionAuthorized = true;
      console.log('[WS] Session acquired.');

      clearInterval(heartbeatTimer);
      heartbeatTimer = setInterval(() => {
        if (ws && ws.readyState === WebSocket.OPEN) {
          ws.send('P');
        }
      }, 500);

      tryConnectStream();
      return;
    }

    if (data.type === 'status') {
      updateTelemetryUI(data);
      return;
    }

    if (data.type === 'error') {
      console.warn(`[WS] Error (${data.code}): ${data.msg}`);
    }
  });

  ws.addEventListener('close', (event) => {
    clearInterval(heartbeatTimer);
    sessionAuthorized = false;
    canControl = false;
    resetControlUI();

    if (event.code === 4003) {
      console.warn('[WS] Rover is already controlled by another client.');
      stopStream('Busy');
      return;
    }

    stopStream('Reconnecting');
    if (!isUnloading) {
      wsReconnectTimer = setTimeout(connectWebSocket, 1500);
    }
  });

  ws.addEventListener('error', () => {
    ws.close();
  });
}

/* --------------------------------------------------------- */


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
const headlightStatus = document.getElementById('light-status');

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
  '-1,1': 'C'  // backward-right
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
};

const pressedKeys = new Set();
let headlightOn = false;

function parseCommand() {
  const up = pressedKeys.has('w');
  const left = pressedKeys.has('a');
  const down = pressedKeys.has('s');
  const right = pressedKeys.has('d');

  const vertical = up === down ? 0 : up ? 1 : -1;
  const horizontal = left === right ? 0 : right ? 1 : -1;

  return commands[`${vertical},${horizontal}`];
}

function sendCommand(command, nextHeadlightState) {
  if (!sessionAuthorized || !ws || ws.readyState !== WebSocket.OPEN) {
    console.error('[CONTROL] Rejected or failed: WebSocket is not connected.');
    return;
  }

  ws.send(command);

  headlightOn = nextHeadlightState;
  headlightStatus.textContent = headlightOn ? 'On' : 'Off';
  commandStatus.textContent = commandsMessage[command[0]];
  headlightButton.classList.toggle('pressed', headlightOn);
}

function updateCommand(nextHeadlightState = headlightOn) {
  const motorCmd = parseCommand();
  const headlightCmd = nextHeadlightState ? HEADLIGHT_ON_COMMAND : HEADLIGHT_OFF_COMMAND;
  const command = `${motorCmd}${headlightCmd}`;

  sendCommand(command, nextHeadlightState);
}

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

function clearPressedInputs() {
  const hadPressedKeys = pressedKeys.size > 0;

  pressedKeys.clear();

  for (const button of Object.values(buttons)) {
    button?.classList.remove('pressed');
  }

  if (hadPressedKeys) {
    updateCommand();
  }
}

function resetControlUI() {
  pressedKeys.clear();

  for (const button of Object.values(buttons)) {
    button?.classList.remove('pressed');
  }

  headlightOn = false;
  if (headlightStatus) headlightStatus.textContent = 'N/A';
  headlightButton?.classList.remove('pressed');
  if (commandStatus) commandStatus.textContent = 'N/A';
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
  if (!button) continue;

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

headlightButton?.addEventListener('pointerdown', (event) => {
  toggleHeadlight(event);
});

window.addEventListener('blur', clearPressedInputs);

/* ------------------------------------------------------------------------- */


/* ------------------ Telemetry ------------------ */

function updateTelemetryUI(json) {
  canControl =
    json.Link_State === 'online' &&
    json.System_State !== 'systemfault' &&
    json.System_State !== 'offline';

  if (!canControl || json.System_State === 'failsafe' || json.Link_State !== 'online') {
    resetControlUI();
  } else {
    if (headlightStatus) headlightStatus.textContent = headlightOn ? 'On' : 'Off';
  }

  if (statusContainers.link) {
    statusContainers.link.classList.toggle('online', json.Link_State === 'online');
    statusContainers.link.classList.toggle('checking', json.Link_State !== 'online');
    statusContainers.link.textContent = json.Link_State;
  }
  if (statusContainers.wifi) statusContainers.wifi.textContent = json.RSSI;
  if (statusContainers.battery) statusContainers.battery.textContent = json.Battery;
  if (statusContainers.system) {
    statusContainers.system.textContent =
      json.System_State === 'idle' || json.System_State === 'moving'
        ? 'Online'
        : json.System_State;
  }
  if (statusContainers.status) statusContainers.status.textContent = json.Speed;
}

/* ----------------------------------------------- */


/* ------------------ Lifecycle ------------------ */

window.addEventListener('load', connectWebSocket);

document.addEventListener('visibilitychange', () => {
  if (document.hidden) clearPressedInputs();
});

window.addEventListener('pagehide', () => {
  isUnloading = true;
  clearTimeout(wsReconnectTimer);
  clearInterval(heartbeatTimer);
  stopStream('N/A');

  if (ws && ws.readyState === WebSocket.OPEN) {
    if (sessionAuthorized) {
      const headlightCmd = headlightOn ? HEADLIGHT_ON_COMMAND : HEADLIGHT_OFF_COMMAND;
      ws.send(`X${headlightCmd}`);
    }
    ws.close();
  }
  window.stop();
});

/* ----------------------------------------------- */