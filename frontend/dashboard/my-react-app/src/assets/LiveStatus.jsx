import React from "react";
import "./LiveStatus.css";

const LiveStatus = () => {
	return (
		<div className="live-status">
			<span className="blinking-text">
				<ul>
					<li>LIVE</li>
				</ul>
			</span>
		</div>
	);
};

export default LiveStatus;
