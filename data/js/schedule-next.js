// Switch to HTML hydration: the server returns a <tbody> fragment at /status.html

let failStreak = 0;
const BASE_MS = 2000;
const MAX_MS = 15000;
const REQ_TIMEOUT_MS = MAX_MS;

let generation = 0; // increments per tick
let inFlightCtrl = null; // AbortController for the current request
let nextTimer = null; // the only allowed pending timer

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
    const r = await fetch("/status.html", { cache: "no-store", signal: ctrl.signal });
    if (!r.ok) throw new Error("HTTP " + r.status);

    const html = await r.text();

    // If a newer tick started while we awaited, drop this result
    if (myGen !== generation) return;

    const tb = document.querySelector("#t tbody");
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

export function scheduleNext(ms) {
  if (nextTimer) {
    clearTimeout(nextTimer);
  }
  nextTimer = setTimeout(tick, ms);
}
