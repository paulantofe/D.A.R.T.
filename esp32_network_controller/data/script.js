/* ------------------ Stream Connection ------------------ */
// This section ensures that the stream connection is secured.
// The image loads in browser RAM and displays only when connected.
// If there is no connection, browser tries to connect every two seconds.

const streamImg = document.getElementById("stream");
let isStreamRunning = false;

function tryConnectVideo() {
  if (isStreamRunning) return;

  const tempImg = new Image();

  tempImg.onload = () => {
    isStreamRunning = true;
    streamImg.src = tempImg.src;
    streamImg.alt = 'Live Stream';
  };

  tempImg.onerror = () => {
    console.log('Waiting video signal...');
    streamImg.alt = 'Video Offline';

    setTimeout(() => {
      tryConnectVideo();
    }, 2000);
  };

  tempImg.src = "/stream?" + new Date().getTime();
}

streamImg.onerror = () => {
  if (!isStreamRunning) return;
  
  isStreamRunning = false;
  streamImg.removeAttribute('src');
  tryConnectVideo();
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

const cmdStatus = document.getElementById("cmd-status");

const keys = {
  w: false,
  a: false,
  s: false,
  d: false,
};

const buttonMap = {
  w: document.getElementById("btn-forward"),
  a: document.getElementById("btn-left"),
  s: document.getElementById("btn-backward"),
  d: document.getElementById("btn-right"),
};

let currentY = null;
let currentX = null;
let lastSentCmd = '';
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

    cmdStatus.innerText = label;

    fetch('/cmd', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: `val=${cmd}`
    })
    .then(response => {
        if (!response.ok) throw new Error("Not responding");
        console.log(`Sent command: ${cmd}`);
    })
    .catch(error => {
        console.log('Communication Error:', error);
    });
  }
}

function setKey(key, state) {
  keys[key] = state;

  if (state) {
    if ((key === 'w' || key === 's') && !currentY) currentY = key;
    if ((key === 'a' || key === 'd') && !currentX) currentX = key;

    buttonMap[key].classList.add("pressed");
  } else {
    if (key === currentY) {
      currentY = keys.w ? 'w' : (keys.s ? 's' : null);
    }
    if (key === currentX) {
      currentX = keys.a ? 'a' : (keys.d ? 'd' : null);
    }

    buttonMap[key].classList.remove("pressed");
  }

  updateCommand();
}

document.addEventListener("keydown", (e) => {
  const key = e.key.toLowerCase();

  if (keys.hasOwnProperty(key) && !keys[key]) {
    setKey(key, true);
  }
});

document.addEventListener("keyup", (e) => {
    const key = e.key.toLowerCase();

    if (keys.hasOwnProperty(key)) {
      setKey(key, false);
    }
});

["w", "a", "s", "d"].forEach((key) => {
  const btn = buttonMap[key];

  btn.addEventListener("pointerdown", (e) => {
    e.preventDefault();
    setKey(key, true);
  });

  btn.addEventListener("pointerup", (e) => {
    e.preventDefault();
    setKey(key, false);
  });

  btn.addEventListener("pointerleave", (e) => {
    e.preventDefault();
    setKey(key, false);
  });
});

/* ------------------------------------------------------------------------- */


/* ------------------ Status Panel and Failsafe Heartbeat ------------------ */
// This section requests JSON from esp32 every two seconds to get battery and WiFi info, then updates the UI Status Panel accordingly.
// This very request is also the heartbeat signal to test esp32-browser connection.

const batStatus = document.getElementById('battery-status');
const wifiStatus = document.getElementById('wifi-status');

function updateStatus() {
  fetch('/status')
  .then(response => {
    if (!response.ok) { throw new Error('Status Offline'); }
    return response.json();
  })
  .then(data => {
    isConnected = true;
    batStatus.innerText = data.battery;
    wifiStatus.innerText = data.rssi;
  })
  .catch(error => {
    isConnected = false;
    console.log('Telemetry Error: ', error);
    batStatus.innerText = 'N/A';
    wifiStatus.innerText = 'Disconnected';

    cmdStatus.innerText = 'N/A'; 
    lastSentCmd = '';
  });
}

updateStatus();
setInterval(updateStatus, 2000);

/* ------------------------------------------------------------------------ */

