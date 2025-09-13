// @ts-check

/**
 * @fileoverview Updates help text to indicate live diagnostics mode
 */

/**
 * Updates the help text element to show live diagnostics status
 * @returns {void}
 */
export const updateText = () => {
  const helpElement = document.getElementById("help");
  if (helpElement) {
    helpElement.innerHTML = "Live-updating ESP diagnostics.";
  }
};
