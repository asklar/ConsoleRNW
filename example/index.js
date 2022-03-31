import {AppRegistry, LogBox} from 'react-native';

// We dont support showing UI -- so remove the LogBox
LogBox.uninstall();

//import {App} from './App';

import React from 'react';
import {View} from 'react-native';
class App extends React.Component {
  render() {
    console.log('HELLO WORLD');
    nativeLoggingHook("************ I'm in render()", 0);

    return (
      <View style={{width: 600, height: 600, backgroundColor: "green", borderRadius: 20}}>
        <View style={{width: 100, height: 100, backgroundColor: "yellow", borderRadius: 20}} />
      </View>
    );
  }
}

import {name as appName} from './app.json';

AppRegistry.registerComponent(appName, () => App);
