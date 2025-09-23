/**
 * @fileoverview Handles automatic image rotation with exponential backoff on errors
 */

import { addMessage, removeMessage, addInfo } from "./error-messages.js";

const canvasElement = document.getElementById("img-canvas");
const imgLoadingElement = document.getElementById("img-loading");

if (!(canvasElement instanceof HTMLCanvasElement)) {
  throw new Error("Canvas element not found");
}
if (!(imgLoadingElement instanceof HTMLImageElement)) {
  throw new Error("Loading image element not found");
}

const canvas = canvasElement;
const ctx = canvas.getContext("2d")!;
const imgLoading = imgLoadingElement;

const IMAGE_ERROR = Symbol("image-error");
const IMAGE_INFO = Symbol("image-info");

const IMG_MIN_MS: number = 15000;
const IMG_MAX_MS: number = 60000;

let imgBackoff: number = IMG_MIN_MS;
let lastLoadedUrl: string = "";
let imageErrorShowing: boolean = false;
let isLoading: boolean = false;
let isFirstCall: boolean = true;

/**
 * Continuously rotates the camera image with exponential backoff on failures
 */
export const rotateImg = (): void => {
  // Don't start new request if one is already loading
  if (isLoading) {
    setTimeout(rotateImg, imgBackoff);
    return;
  }
  
  // Delay only the first call to avoid double-load on page load
  if (isFirstCall) {
    isFirstCall = false;
    setTimeout(rotateImg, imgBackoff);
    return;
  }
  
  isLoading = true;
  const url = `/i/latest.jpg?_=${Date.now()}`;
  const tmp = new Image();
  let done = false;
  
  // Show loading image
  imgLoading.src = url;
  imgLoading.style.opacity = "1";
  
  // Timeout to prevent stuck loading
  const timeout = setTimeout(() => {
    if (!done) {
      done = true;
      console.warn("Image load timeout");
      isLoading = false;
      setTimeout(rotateImg, imgBackoff);
    }
  }, 30000); // 30 second timeout

  tmp.onload = () => {
    if (done) return;
    done = true;
    clearTimeout(timeout);
    
    // Only update timestamp if image actually changed
    if (tmp.src !== lastLoadedUrl) {
      addInfo(IMAGE_INFO, "Last image");
      lastLoadedUrl = tmp.src;
    }
    
    // Draw loaded image to canvas and size canvas to match
    canvas.width = tmp.width;
    canvas.height = tmp.height;
    ctx.drawImage(tmp, 0, 0);
    
    // Hide loading image
    imgLoading.style.opacity = "0";
    
    removeMessage(IMAGE_ERROR);
    imageErrorShowing = false;
    imgBackoff = IMG_MIN_MS; // success -> reset
    isLoading = false;
    setTimeout(rotateImg, imgBackoff);
  };

  tmp.onerror = () => {
    if (done) return;
    done = true;
    clearTimeout(timeout);
    // Only show error message if not already showing
    if (!imageErrorShowing) {
      addMessage(IMAGE_ERROR, "Image is no longer live");
      imageErrorShowing = true;
    }
    imgBackoff = Math.min(IMG_MAX_MS, Math.floor(imgBackoff * 1.7));
    isLoading = false;
    setTimeout(rotateImg, imgBackoff);
  };

  tmp.src = url;
};
