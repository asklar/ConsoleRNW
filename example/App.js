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
    return (
        <Button title="Click me!" 
        onPress={() => { alert('clicked!'); }} />);
};

export default App;
