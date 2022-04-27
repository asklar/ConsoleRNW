/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */

import React from 'react';

import {Text, Image, Button, View, requireNativeComponent } from 'react-native';
import { NativeButton } from './nativeButton';

const App = () => {
    const [color, setColor] = React.useState('red');
    return (
        <>
            <Text style={{ fontFamily: "Segoe UI", fontSize: 11, 
            marginLeft: 24, marginBottom: 12, marginRight: 22, textAlign: 'right'}}>Hello world!</Text>
            <Image source={{ uri: "react.png" }} style={{width: 200, height: 160}}/>
            <View style={{ backgroundColor: 'yellow', flexDirection: 'row' }}>
                <NativeButton title="Click me" style={{margin: 8}}
                    fontFamily="Segoe UI" fontSize={10}
                    onClick={() => { console.log('clicked'); alert('clicked'); }} />
            </View>
        </>
    );
};

export default App;
