// App.jsx
import React from 'react';
import { BrowserRouter as Router, Route, Routes } from 'react-router-dom';
import Navbar from './Navbar';
import TrayOccupancyPrediction from './TrayOccupancyPrediction';
import BirdPrediction from './BirdPrediction';
import "./App.css"

const App = () => {
  return (
    <Router>
      <Navbar />
      <Routes>
        <Route path="/tray-occupancy-prediction" element={<TrayOccupancyPrediction />} />
        <Route path="/bird-prediction" element={<BirdPrediction />} />
      </Routes>
    </Router>
  );
};

export default App;
