/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */

import React from 'react';

import {Text, Image, Button, View, requireNativeComponent } from 'react-native';

const NativeButton = requireNativeComponent("NativeButton");

const App = () => {
    const [color, setColor] = React.useState('red');
    return (
        <>
            <Text style={{ fontFamily: "Segoe UI", fontSize: 14, marginLeft: 24, marginBottom: 12 }}>Hello world!</Text>
            <Image source={{ uri: "react.png" }} style={{width: 200, height: 160}}/>
            <View style={{ backgroundColor: 'yellow', flexDirection: 'row' }}>
                <NativeButton title="this is a long title in a native button"
                    fontFamily="Segoe UI" fontSize={22}
                    onClick={() => { console.log('clicked'); alert('clicked'); }} />
            </View>
        </>
    );
};

export default App;
