/**
 * @file     script.js
 * @brief    Browser controls and status dashboard logic for the ESP32 server.
 */

/* ------------------ Stream Connection ------------------ */

const stream = document.getElementById('stream');
const streamStatus = document.getElementById('stream-status');
const videoContainer = document.querySelector('.video-container');

let retryStreamTimer = null;
let streamConnected = false;

function tryConnectStream() {
  console.log('[STREAM] Connecting stream...');
  clearTimeout(retryStreamTimer);
  stream.setAttribute('src', `/stream?${Date.now()}`);
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
  stream.classList.remove('active');
  videoContainer.classList.remove('stream-active');
  streamStatus.classList.remove('live');

  streamStatus.lastElementChild.textContent =
    streamConnected ? 'Reconnecting' : 'N/A';

  console.error('Stream error: ' + 
    (streamConnected ? 'lost stream connection. ' : 'failed to connect stream. '),
  );

  stream.removeAttribute('src');

  retryStreamTimer = setTimeout(tryConnectStream, 2000);
});

window.addEventListener('load', () => {
  tryConnectStream();
});

/* ------------------------------------------------------- */


/* ------------------ User Input Parsing and Send Command ------------------ */

const buttons = {
  'w': document.getElementById('btn-forward'),
  'a': document.getElementById('btn-left'),
  's': document.getElementById('btn-backward'),
  'd': document.getElementById('btn-right'),
};

const keyAliases = {
  w: 'w',
  a: 'a',
  s: 's',
  d: 'd',
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

const pressedKeys = new Set();

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

async function sendCommand(command) {
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
}

function updateCommand() {
  const command = parseCommand();

  commandQueue = commandQueue
    .then(() => sendCommand(command))
    .catch((error) => {
      console.error('[CONTROL] Failed to send command: ', error);
    });
}

function pressCommand(key, event) {
  event.preventDefault();
  pressedKeys.add(key);
  buttons[key].classList.add('pressed');
  updateCommand();
}

function releaseCommand(key, event) {
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

  pressCommand(key, event);
});

window.addEventListener('keyup', (event) => {
  const key = keyAliases[event.key];

  if (!key) {
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

window.addEventListener('blur', () => {
  pressedKeys.clear();
  for (const button of Object.values(buttons)) {
    button.classList.remove('pressed');
  }
  updateCommand();
});

/* ------------------------------------------------------------------------- */


/* ------------------ Status Panel and Failsafe Heartbeat ------------------ *//* ------------------------------------------------------------------------- */