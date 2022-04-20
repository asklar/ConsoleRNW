/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */

import React from 'react';

import {Text, Button, View} from 'react-native';
const App = () => {
  const [bgColor, setBgColor] = React.useState('red');
    return (
        <Button  title="Click me" />);
/*
    <View
      style={{
        width: 200,
        height: 200,
        backgroundColor: bgColor,
              borderRadius: 100,
              borderColor: "#802060",
        borderWidth: 4,
      }}
      onMouseEnter={e => {
          setBgColor('green');
        /*  alert(`ON MOUSE ENTER! ${JSON.stringify(e.nativeEvent)}`);* /
      }}
      onMouseLeave={e => {
        setBgColor('red')
//        alert(`MOUSE LEAVE`);
      }}
      >
      <Text style={{ width: 150, height: 50, margin: 20, backgroundColor: "#00bb20", 
          borderColor: "green"
          }}>Hello world</Text>
      <Button title="Click me" />
    </View>
);
*/

};

export default App;
