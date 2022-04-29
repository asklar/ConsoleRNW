/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */

import React from 'react';

import {Text, Image, TextInput, Button, View, requireNativeComponent } from 'react-native';
import { NativeButton } from './nativeButton';

const App = () => {
    const [color, setColor] = React.useState('red');
    const [text, setText] = React.useState('text input field');
    return (
        <>
            <Text style={{ fontFamily: "Segoe UI", fontSize: 11, 
            marginLeft: 24, marginBottom: 12, marginRight: 22, textAlign: 'right'}}>Hello world!</Text>
            <Image source={require('./react.png')}/>
            <TextInput style={{ margin: 20}} onChangeText={(e) => { alert(JSON.stringify(e.nativeEvent)); }} value={text} /> 
            <View style={{ backgroundColor: color, flexDirection: 'row', margin: 8 }} onMouseEnter={() => { setColor('yellow'); }} onMouseLeave={() => { setColor('red'); }}>
                <NativeButton title="Click me" style={{margin: 8}}
                    fontFamily="Segoe UI" fontSize={10}
                    onClick={() => { console.log('clicked'); alert('clicked'); }} />
            </View>
        </>
    );
};

export default App;
