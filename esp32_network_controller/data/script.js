const keys = {
  w: false,
  a: false,
  s: false,
  d: false,
};

const cmdStatus = document.getElementById("cmd-status");
const buttonMap = {
  w: document.getElementById("btn-forward"),
  a: document.getElementById("btn-left"),
  s: document.getElementById("btn-backward"),
  d: document.getElementById("btn-right"),
};

let currentY = null;
let currentX = null;
function updateCommand() {
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

  cmdStatus.innerText = label;
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
