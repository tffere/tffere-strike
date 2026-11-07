// Keep startup failures visible even when a game dependency cannot be imported.
const button = document.getElementById('start');
const title = document.getElementById('overlay-title');
const copy = document.getElementById('overlay-copy');
button.disabled = true;
button.textContent = 'LOADING 3D GAME…';
function fail(message) {
  title.textContent = 'GAME COULD NOT START';
  copy.textContent = message;
  button.textContent = 'REFRESH AFTER FIXING THE ERROR';
  button.disabled = true;
}
if (location.protocol === 'file:') {
  fail('Open a terminal in this project, run npm ci and then npm start. Open http://localhost:4174 in your browser. Opening index.html directly cannot load the game.');
} else {
  import('./game.js').then(() => {
    button.disabled = false;
    button.innerHTML = 'DEPLOY INTO DEPOT <span>→</span>';
  }).catch((error) => {
    console.error('Game startup failed:', error);
    fail(`The 3D game could not load. Check that npm ci completed successfully, run npm start, and open http://localhost:4174. If dependencies are installed, enable browser graphics acceleration and check the Console. Details: ${error.message}`);
  });
}
