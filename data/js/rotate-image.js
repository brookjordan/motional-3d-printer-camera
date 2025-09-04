const img = document.getElementById("img");

let imgBackoff = 1000; // start at 1s, back off on errors
const IMG_MIN_MS = 800;
const IMG_MAX_MS = 15000;

export const rotateImg = () => {
  const url = `/i/latest.jpg?_=${Date.now()}`;
  const tmp = new Image();
  let done = false;
  tmp.onload = () => {
    if (done) return;
    done = true;
    img.src = url;
    imgBackoff = IMG_MIN_MS; // success -> reset
    setTimeout(rotateImg, imgBackoff);
  };
  tmp.onerror = () => {
    if (done) return;
    done = true;
    imgBackoff = Math.min(IMG_MAX_MS, Math.floor(imgBackoff * 1.7));
    setTimeout(rotateImg, imgBackoff);
  };
  tmp.src = url;
};
