/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */

debugger;

import React from 'react';

import {
  View,
} from 'react-native';

nativeLoggingHook("I'm in app.js", 0);

class App  extends React.Component {

  render() {
    console.log("HELLO WORLD");
    nativeLoggingHook("************ I'm in render()", 0);
    debugger;
    return (
    < View style={{width: 400, height: 600}}/>
    );
  }
}


export default App;
