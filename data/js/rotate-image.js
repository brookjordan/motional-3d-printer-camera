// @ts-check
/**
 * @fileoverview Handles automatic image rotation with exponential backoff on errors
 */

const imgElement = document.getElementById("img");
const img = imgElement instanceof HTMLImageElement ? imgElement : null;

/** @type {number} Minimum interval between image updates in milliseconds */
const IMG_MIN_MS = 2000;

/** @type {number} Maximum interval between image updates in milliseconds */
const IMG_MAX_MS = 15000;

/** @type {number} Current backoff interval, starts at minimum and increases on errors */
let imgBackoff = IMG_MIN_MS;

/**
 * Continuously rotates the camera image with exponential backoff on failures
 * @returns {void}
 */
export const rotateImg = () => {
  const url = `/i/latest.jpg?_=${Date.now()}`;
  const tmp = new Image();
  let done = false;

  tmp.onload = () => {
    if (done) return;
    done = true;
    if (img) {
      img.src = url;
    }
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
