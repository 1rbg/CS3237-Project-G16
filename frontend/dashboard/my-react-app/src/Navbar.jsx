// Navbar.jsx
import React from 'react';
import { Link, NavLink } from 'react-router-dom';
import './Navbar.css';
import { LuBird } from "react-icons/lu";
import { ImCross } from "react-icons/im";
import { PiHandWavingLight } from "react-icons/pi";

const Navbar = () => {
  return (
    <nav className="navbar">
      <h1 className="navbar-logo">BYEBYEBIRDIE <PiHandWavingLight /><PiHandWavingLight /><LuBird /></h1>
      <ul className="navbar-links">
        <li>
          <NavLink 
            to="/tray-occupancy-prediction" 
            className={({ isActive }) => isActive ? 'active-link' : ''}
          >
            Tray Occupancy Prediction
          </NavLink>
        </li>
        <li>
          <NavLink 
            to="/bird-prediction" 
            className={({ isActive }) => isActive ? 'active-link' : ''}
          >
            Bird Prediction
          </NavLink>
        </li>
      </ul>
    </nav>
  );
};

export default Navbar;
