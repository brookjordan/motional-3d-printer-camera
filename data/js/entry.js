// @ts-check

/**
 * @fileoverview Main entry point for ESP32-S3 camera web interface
 */

import { updateText } from "./update-text.js";
import { scheduleNext } from "./schedule-next.js";
import { rotateImg } from "./rotate-image.js";

// Initialize the web interface
updateText();
scheduleNext(0);
rotateImg();
