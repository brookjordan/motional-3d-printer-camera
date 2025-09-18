// @ts-check

/**
 * @fileoverview Updates help text to indicate live diagnostics mode
 */

/**
 * Updates the help text element to show live diagnostics status
 * @returns {void}
 */
const updateText = () => {
  const helpElement = document.getElementById("help");
  if (helpElement) {
    helpElement.innerHTML = "Live-updating ESP diagnostics.";
  }
};

// @ts-check
/**
 * @fileoverview Handles periodic status table updates with exponential backoff and visibility awareness
 */

/** @type {number} Current consecutive failure count */
let failStreak = 0;

/** @type {number} Base interval between updates in milliseconds */
const BASE_MS = 2000;

/** @type {number} Maximum interval between updates in milliseconds */
const MAX_MS = 15000;

/** @type {number} Request timeout in milliseconds */
const REQ_TIMEOUT_MS = MAX_MS;

/** @type {number} Generation counter to prevent race conditions */
let generation = 0;

/** @type {AbortController | null} Controller for the current in-flight request */
let inFlightCtrl = null;

/** @type {number | null} Timer ID for the next scheduled update */
let nextTimer = null;

/**
 * Performs a single status update tick with error handling and race condition prevention
 * @returns {Promise<void>}
 */
async function tick() {
  const myGen = ++generation;

  // Cancel any older in-flight request
  if (inFlightCtrl) {
    try {
      inFlightCtrl.abort("superseded");
    } catch {}
  }

  const start = performance.now();
  const ctrl = new AbortController();
  inFlightCtrl = ctrl;

  // Hard timeout for this request
  const hardTm = setTimeout(() => ctrl.abort("timeout"), REQ_TIMEOUT_MS);

  try {
    const r = await fetch("/status.html", {
      cache: "no-store",
      signal: ctrl.signal,
    });
    if (!r.ok) throw new Error("HTTP " + r.status);

    const html = await r.text();

    // If a newer tick started while we awaited, drop this result
    if (myGen !== generation) return;

    const tb = document.querySelector("#diagnostics-table tbody");
    if (tb) {
      // Response contains a full <tbody>...</tbody> — replace the node
      tb.outerHTML = html;
    }

    failStreak = 0;
  } catch (e) {
    // Ignore aborts from supersession/visibility changes
    if (e?.name !== "AbortError") {
      console.warn("poll error:", e);
      failStreak++;
    }
  } finally {
    clearTimeout(hardTm);

    // Only the *current* generation may schedule the next tick
    if (myGen === generation) {
      const base = Math.max(0, BASE_MS - (performance.now() - start));
      const backoff = Math.min(
        MAX_MS,
        BASE_MS * Math.pow(2, Math.max(0, failStreak - 1))
      );
      const wait = failStreak ? backoff : base;
      scheduleNext(wait);
    }
  }
}

// Visibility-aware: pause when hidden, resume immediately when visible
document.addEventListener("visibilitychange", () => {
  if (document.visibilityState === "visible") {
    // Start a fresh generation immediately
    scheduleNext(0);
  } else {
    // Stop any pending work
    if (nextTimer) {
      clearTimeout(nextTimer);
      nextTimer = null;
    }
    if (inFlightCtrl) {
      try {
        inFlightCtrl.abort("page hidden");
      } catch {}
    }
  }
});

/**
 * Schedules the next status update after the specified delay
 * @param {number} milliseconds - Delay in milliseconds before the next update
 * @returns {void}
 */
function scheduleNext(milliseconds) {
  if (nextTimer) {
    clearTimeout(nextTimer);
  }
  nextTimer = setTimeout(tick, milliseconds);
}

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
const rotateImg = () => {
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

// @ts-check


// Initialize the web interface
updateText();
scheduleNext(0);
rotateImg();
