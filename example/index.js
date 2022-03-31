import {AppRegistry, LogBox} from 'react-native';

// We dont support showing UI -- so remove the LogBox
LogBox.uninstall();

/**
 * @format
 */

 import App from './App';
 import {name as appName} from './app.json';
 
 AppRegistry.registerComponent(appName, () => App);
 