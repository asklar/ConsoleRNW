import {AppRegistry, LogBox, View} from 'react-native';
import React from 'react';


// We dont support showing UI -- so remove the LogBox
LogBox.uninstall();

// import {App} from './App';

class App  extends React.Component {

  render() {
    console.log("HELLO WORLD");
    nativeLoggingHook("************ I'm in render()", 0);
    
    return (
      <View style={{width: 400, height: 600}}/>
    );
  }
}

import {name as appName} from './app.json';

console.log("BEFORE");
AppRegistry.registerComponent(appName, () => App);
console.log("AFTER  ");

nativeLoggingHook("FOO", 0);