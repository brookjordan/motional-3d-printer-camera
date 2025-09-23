/**
 * @fileoverview Handles periodic status table updates with exponential backoff and visibility awareness
 */

import { addMessage, removeMessage, addInfo } from "./error-messages.js";

const DATA_ERROR = Symbol("data-error");
const DATA_INFO = Symbol("data-info");

let dataErrorShowing: boolean = false;
let failStreak: number = 0;

const BASE_MS: number = 15000;
const MAX_MS: number = 60000;
const REQ_TIMEOUT_MS: number = MAX_MS;

let generation: number = 0;
let inFlightCtrl: AbortController | null = null;
let nextTimer: number | null = null;

/**
 * Performs a single status update tick with error handling and race condition prevention
 */
async function tick(): Promise<void> {
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

  // Show info message when data fetch starts
  addInfo(DATA_INFO, "Last data");

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

    removeMessage(DATA_ERROR);
    dataErrorShowing = false;

    failStreak = 0;
  } catch (e) {
    // Ignore aborts from supersession/visibility changes
    if ((e as Error)?.name !== "AbortError") {
      console.warn("poll error:", e);
      failStreak++;

      // Only show error message if not already showing
      if (!dataErrorShowing) {
        addMessage(DATA_ERROR, "Data is no longer live");
        dataErrorShowing = true;
      }
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
 */
export function scheduleNext(milliseconds: number): void {
  if (nextTimer) {
    clearTimeout(nextTimer);
  }
  nextTimer = setTimeout(tick, milliseconds);
}
