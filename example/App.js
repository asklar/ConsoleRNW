/**
 * @format
 */


function sendInput(c) {
  try {
    nativeConsole.log(`<${c.toUpperCase()}>`);

    if (c === 'e') {
      throw new Error('We throw an error on "e"!');
    }
    if (c === 'q') {
        nativeConsole.exit();
    }
  } catch (e) {
    // If we throw within a JSI method, we appear to crash.  Catching errors and pushing
    // them to a  timer means they run within a loop that redbox will catch them.
      setTimeout(() => {throw e}, 1);
  }
}

// This function is called by native
global.sendInput = sendInput;


