import {LogBox} from 'react-native';

// We dont support showing UI -- so remove the LogBox
LogBox.uninstall();

import './App';


const J = 'Java';
const S = 'Script';
nativeConsole.log(`Hello from ${J}${S}!`);