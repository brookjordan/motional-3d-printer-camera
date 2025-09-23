/**
 * @fileoverview Error message management module
 */

const symbolToErrorElement = new Map<symbol, HTMLElement>();
const symbolToInfoElement = new Map<symbol, HTMLElement>();

/**
 * Adds an error message with a unique symbol ID
 */
export function addMessage(id: symbol, message: string): void {
  const errorDiv = document.getElementById("error-messages");
  if (!errorDiv) return;

  // Reuse existing error element or create new one
  let errorP = symbolToErrorElement.get(id);
  if (errorP) {
    // Update existing element text
    errorP.textContent = `${message} since ${new Date().toString()}`;
  } else {
    // Create new error message
    errorP = document.createElement("p");
    errorP.textContent = `${message} since ${new Date().toString()}`;
    errorDiv.appendChild(errorP);
    symbolToErrorElement.set(id, errorP);
  }
}

/**
 * Removes an error message by symbol ID
 */
export function removeMessage(id: symbol): void {
  const errorElement = symbolToErrorElement.get(id);
  if (errorElement) {
    errorElement.remove();
    symbolToErrorElement.delete(id);
  }
}

/**
 * Adds an info message with a unique symbol ID
 */
export function addInfo(id: symbol, message: string): void {
  const infoDiv = document.getElementById("info-messages");
  if (!infoDiv) return;

  // Reuse existing info element or create new one
  let infoP = symbolToInfoElement.get(id);
  if (infoP) {
    // Update existing element text
    infoP.textContent = `${message}: ${new Date().toString()}`;
  } else {
    // Create new info message
    infoP = document.createElement("p");
    infoP.textContent = `${message}: ${new Date().toString()}`;
    infoDiv.appendChild(infoP);
    symbolToInfoElement.set(id, infoP);
  }
}

/**
 * Removes an info message by symbol ID
 */
export function removeInfo(id: symbol): void {
  const infoElement = symbolToInfoElement.get(id);
  if (infoElement) {
    infoElement.remove();
    symbolToInfoElement.delete(id);
  }
}