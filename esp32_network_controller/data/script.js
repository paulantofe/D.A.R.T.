/* ------------------ Stream Connection ------------------ */
// This section manages the MJPEG video stream connection
// The stream element stays transparent until the first valid frame arrives.
// If the connection drops or fails, it retries every two seconds.

const streamImg = document.getElementById('stream');
let isStreamRunning = false;
let streamRetryTimer = null;

function tryConnectVideo() {
  if (isStreamRunning) { return; }

  clearTimeout(streamRetryTimer);
  streamImg.src = '/stream?' + Date.now();
}

streamImg.onload = () => {
  isStreamRunning = true;
  streamImg.classList.add('active');
};

streamImg.onerror = () => {
  isStreamRunning = false;
  streamImg.classList.remove('active');
  if (streamImg.hasAttribute('src')) {
    streamImg.removeAttribute('src');
  }

  console.log('Waiting Video Signal...');
  clearTimeout(streamRetryTimer);
  
  streamRetryTimer = setTimeout(tryConnectVideo, 2000);
};

window.onload = () => {
  tryConnectVideo();
};

/* ------------------------------------------------------- */


/* ------------------ User Input Parsing and Send Command ------------------ */
// This section sanitizes user key input, maps it to one command and sends it to esp32 via POST method.
// Conflicting Key Logic: - First Key Wins (e.g. 'W' first then 'S' -> 'W').
//                        - Second Command Memory (e.g. 'W' hold first then 'S' hold -> 'W', followed by 'W' release -> 'S').
// If there is no connection, there is no attempt to send command.

// This variable is updated by "Status Panel and Failsafe Heartbeat" Section.
let isConnected = false;

const cmdStatus = document.getElementById('cmd-status');

const keys = {
  w: false,
  a: false,
  s: false,
  d: false,
};

const buttonMap = {
  w: document.getElementById('btn-forward'),
  a: document.getElementById('btn-left'),
  s: document.getElementById('btn-backward'),
  d: document.getElementById('btn-right'),
};

let currentY = null;
let currentX = null;
let lastSentCmd = '0';

function updateCommand() {
  if (!isConnected) { return; }
  
  let cmd = '0';
  let label = 'STOP';

  const fwd = (currentY === 'w');
  const bwd = (currentY === 's');
  const left = (currentX === 'a');
  const right = (currentX === 'd');

  if (fwd && left) { cmd = 'Q'; label = 'F_Left'; }
  else if (fwd && right) { cmd = 'E'; label = 'F_Right'; }
  else if (bwd && left) { cmd = 'Z'; label = 'B_Left'; }
  else if (bwd && right) { cmd = 'C'; label = 'B_Right'; }
  else if (fwd) { cmd = 'W'; label = 'Forward'; }
  else if (bwd) { cmd = 'S'; label = 'Backward'; }
  else if (left) { cmd = 'A'; label = 'Left'; }
  else if (right) { cmd = 'D'; label = 'Right'; }

  if (cmd !== lastSentCmd) {
    lastSentCmd = cmd;

    fetch('/control', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: `val=${cmd}`
    })
    .then(response => {
        if (!response.ok) { throw new Error('Not responding'); }
        cmdStatus.innerText = label;
        console.log(`Sent command: ${cmd}`);
    })
    .catch(error => {
        console.log('Communication Error:', error);
        lastSentCmd = '0';
        cmdStatus.innerText = 'N/A';
    });
  }
}

function setKey(key, state) {
  keys[key] = state;

  if (state) {
    if ((key === 'w' || key === 's') && !currentY) { currentY = key; }
    if ((key === 'a' || key === 'd') && !currentX) { currentX = key; }

    buttonMap[key].classList.add('pressed');
  } else {
    if (key === currentY) {
      currentY = keys.w ? 'w' : (keys.s ? 's' : null);
    }
    if (key === currentX) {
      currentX = keys.a ? 'a' : (keys.d ? 'd' : null);
    }

    buttonMap[key].classList.remove('pressed');
  }

  updateCommand();
}

document.addEventListener('keydown', (e) => {
  const key = e.key.toLowerCase();

  if (keys.hasOwnProperty(key) && !keys[key]) {
    setKey(key, true);
  }
});

document.addEventListener('keyup', (e) => {
  const key = e.key.toLowerCase();

  if (keys.hasOwnProperty(key)) {
    setKey(key, false);
  }
});

['w', 'a', 's', 'd'].forEach((key) => {
  const btn = buttonMap[key];

  btn.addEventListener('pointerdown', (e) => {
    e.preventDefault();
    setKey(key, true);
  });

  ['pointerup', 'pointerleave', 'pointercancel'].forEach((eventType) => {
    btn.addEventListener(eventType, (e) => {
      e.preventDefault();
      if (keys[key]) { setKey(key, false); }
    });
  });
});

window.addEventListener('blur', () => {
  ['w', 'a', 's', 'd'].forEach(k => { if (keys[k]) setKey(k, false); });
});

/* ------------------------------------------------------------------------- */


/* ------------------ Status Panel and Failsafe Heartbeat ------------------ */
// This section requests JSON from esp32 every two seconds to get battery, WiFi and mode info, then updates the UI Status Panel accordingly.
// This very request is also the heartbeat signal to test esp32-browser connection.

const batStatus = document.getElementById('battery-status');
const wifiStatus = document.getElementById('wifi-status');
const modeStatus = document.getElementById('mode-status');

function updateStatus() {
  const controller = new AbortController();
  const timeoutId = setTimeout(() => controller.abort(), 1500);
  
  fetch('/status', {signal: controller.signal})
  .then(response => {
    if (!response.ok) { throw new Error('Status Offline'); }
    return response.json();
  })
  .then(data => {
    const wasOffline = !isConnected;

    isConnected = true;
    batStatus.innerText = data.battery;
    wifiStatus.innerText = data.rssi;
    modeStatus.innerText = data.mode;

    if (data.mode === 'Driver') {
      if (cmdStatus.innerText === 'N/A') { 
        cmdStatus.innerText = 'STOP'; 
      }

      modeStatus.classList.remove('spectator');
      modeStatus.classList.add('driver');
    }
    else {
      cmdStatus.innerText = 'N/A';
      modeStatus.classList.remove('driver');
      modeStatus.classList.add('spectator');
    }
    if (wasOffline) { updateCommand(); }
  })
  .catch(error => {
    isConnected = false;
    console.log('Telemetry Error: ', error);
    batStatus.innerText = 'N/A';
    wifiStatus.innerText = 'Disconnected';
    modeStatus.innerText = 'N/A';

    modeStatus.classList.remove('driver', 'spectator');

    cmdStatus.innerText = 'N/A'; 
    lastSentCmd = '0';
  })
  .finally(() => {
    clearTimeout(timeoutId);
    setTimeout(updateStatus, 2000);
  });
}

updateStatus();

/* ------------------------------------------------------------------------ */

