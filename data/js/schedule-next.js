const isObj = (v) => v && typeof v === "object" && !Array.isArray(v);

function renderObject(obj) {
  const table = document.createElement("table");
  table.classList.add("sub");
  const tbody = document.createElement("tbody");

  Object.entries(obj).forEach(([key, value]) => {
    const row = document.createElement("tr");
    const headCell = document.createElement("th");
    headCell.textContent = key;
    const dataCell = document.createElement("td");
    dataCell.innerHTML = renderValue(key, value, true);
    row.append(headCell, dataCell);
    tbody.append(row);
  });

  table.append(tbody);
  return table.outerHTML;
}

function renderArrayOfObjects(arr) {
  if (!arr.length) {
    const muted = document.createElement("em");
    muted.className = "muted";
    muted.textContent = "empty";
    return muted.outerHTML;
  }

  const cols = [...new Set(arr.flatMap((o) => Object.keys(o)))];
  let thead = document.createElement("thead");
  let headRow = document.createElement("tr");
  thead.append(headRow);
  cols.forEach((c) => {
    let th = document.createElement("th");
    th.textContent = c;
    headRow.append(th);
  });
  let tBody = document.createElement("tbody");
  arr.forEach((row) => {
    const tr = document.createElement("tr");
    cols.forEach((c) => {
      const td = document.createElement("td");
      td.innerHTML = renderValue(c, row[c]);
      tr.append(td);
    });
    tBody.append(tr);
  });
  const table = document.createElement("table");
  table.classList.add("sub");
  table.append(thead);
  table.append(tBody);

  return table.outerHTML;
}

function renderValue(key, v) {
  if (key === "partitions" && Array.isArray(v)) return renderArrayOfObjects(v);
  if (key === "ota" && isObj(v)) return renderObject(v);
  if (Array.isArray(v)) {
    if (v.length && isObj(v[0])) return renderArrayOfObjects(v);
    return (
      `<table class='sub'><tbody>` +
      v.map((x, i) => `<tr><td>${i}</td><td>${String(x)}</td></tr>`).join("") +
      `</tbody></table>`
    );
  }
  if (isObj(v)) return renderObject(v);
  return String(v);
}

function row(k, vHtml) {
  const big = vHtml.includes("<table");
  return (
    `<tr><td>${k}</td><td class="${big ? "has-table" : ""}">` +
    vHtml +
    `</td></tr>`
  );
}

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
    const r = await fetch("/json", { cache: "no-store", signal: ctrl.signal });
    if (!r.ok) throw new Error("HTTP " + r.status);

    const j = await r.json();

    // If a newer tick started while we awaited, drop this result
    if (myGen !== generation) return;

    const tb = document.querySelector("#t tbody");
    tb.innerHTML = "";
    Object.keys(j).forEach((k) => {
      const html = renderValue(k, j[k]);
      tb.insertAdjacentHTML("beforeend", row(k, html));
    });

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
