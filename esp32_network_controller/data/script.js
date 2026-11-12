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
  clearTimeout(retryStreamTimer);
  stream.setAttribute('src', `/stream?${Date.now()}`);
}

stream.addEventListener('load', () => {
  streamConnected = true;
  stream.classList.add('active');
  videoContainer.classList.add('stream-active');
  streamStatus.classList.add('live');
  streamStatus.lastElementChild.textContent = 'Live';
});

stream.addEventListener('error', () => {
  stream.classList.remove('active');
  videoContainer.classList.remove('stream-active');
  streamStatus.classList.remove('live');

  streamStatus.lastElementChild.textContent =
    streamConnected ? 'Reconnecting' : 'N/A';

  stream.removeAttribute('src');

  retryStreamTimer = setTimeout(tryConnectStream, 2000);
});

window.addEventListener('load', () => {
  tryConnectStream();
});

/* ------------------------------------------------------- */


/* ------------------ User Input Parsing and Send Command ------------------ */


/* ------------------------------------------------------------------------- */


/* ------------------ Status Panel and Failsafe Heartbeat ------------------ */



/* ------------------------------------------------------------------------- */