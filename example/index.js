/**
 * @format
 */

// We need to import something from react-native, so that react-native is included in the bundle correctly.
import {NativeModules} from 'react-native'; 

const J = 'Java';
const S = 'Script';
nativeConsole.log(`Hello from ${J}${S}!`);

nativeConsole.exit();