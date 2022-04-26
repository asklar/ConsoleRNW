/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */

import React from 'react';

import {Text, Image, Button, View} from 'react-native';
const App = () => {
    const [color, setColor] = React.useState('red');
    return (
        /*<>
        <View style={{
            width: 200, height: 50,
            backgroundColor: color,
            borderRadius: 50,
        }} onMouseEnter={() => { setColor('green'); }} />
        */<Image source="react.png" />
        //</>
    );
};

export default App;
