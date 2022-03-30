/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */


import React from 'react';
import { View} from 'react-native';
class App extends React.Component {
  render() {
    console.log('HELLO WORLD');
    nativeLoggingHook("************ I'm in render()", 0);

    return (
      <View style={{width: 600, height: 600, backgroundColor: "green"}}>
        <View style={{width: 100, height: 100, backgroundColor: "pink"}} />
      </View>
    );
  }
}

nativeLoggingHook("I'm in app.js", 0);



export default App;
