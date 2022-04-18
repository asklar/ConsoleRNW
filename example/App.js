/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */

import React from 'react';

import {StyleSheet, Text, useColorScheme, View} from 'react-native';

import {Colors} from 'react-native/Libraries/NewAppScreen';


const Section = ({children, title}) => {
  const isDarkMode = useColorScheme() === 'dark';
  return (
    <View style={styles.sectionContainer}>
      <Text
        style={[
          styles.sectionTitle,
          {
            color: isDarkMode ? Colors.white : Colors.black,
          },
        ]}>
        {title}
      </Text>
      <Text
        style={[
          styles.sectionDescription,
          {
            color: isDarkMode ? Colors.light : Colors.dark,
          },
        ]}>
        {children}
      </Text>
    </View>
  );
};

const App = () => {
  const isDarkMode = useColorScheme() === 'dark';

  const backgroundStyle = {
    backgroundColor: isDarkMode ? Colors.darker : Colors.lighter,
  };

  const [bgColor, setBgColor] = React.useState('red');
  return (
//    <View
//      style={{
//        width: 200,
//        height: 200,
//        backgroundColor: bgColor,
//        borderRadius: 100,
//      }}
//      onMouseEnter={e => {
//          setBgColor('green');
//        /*  alert(`ON MOUSE ENTER! ${JSON.stringify(e.nativeEvent)}`);*/
//      }}
//      onMouseLeave={e => {
//        setBgColor('red')
////        alert(`MOUSE LEAVE`);
//      }}
//      >
      <Text style={{ width: 200, height: 50, margin: 20, backgroundColor: "red"}}>Hello world</Text>
    //</View>
  );
};

const styles = StyleSheet.create({
  sectionContainer: {
    marginTop: 32,
    paddingHorizontal: 24,
  },
  sectionTitle: {
    fontSize: 24,
    fontWeight: '600',
  },
  sectionDescription: {
    marginTop: 8,
    fontSize: 18,
    fontWeight: '400',
  },
  highlight: {
    fontWeight: '700',
  },
});

export default App;
