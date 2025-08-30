const img = document.getElementById("img");

export const rotateImg = () => {
  img.src = `/i/latest.jpg?_=${Date.now()}`;
  setTimeout(rotateImg, 950);
};
