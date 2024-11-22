import React, { useState, useEffect } from "react";
import Papa from "papaparse";
import BarChartComponent from "./BarChartComponent";
import LiveStatus from "./assets/LiveStatus";

const TrayOccupancyPrediction = () => {
	// State for labels, actual data, and predicted data
	const [labels, setLabels] = useState([]);
	const [actualData, setActualData] = useState([
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	]);
	const [predictedData, setPredictedData] = useState([]);
	const [glow, setGlow] = useState([]);

	const generateLabels = (currentTime) => {
		currentTime.setMinutes(0, 0, 0); // Set time to start of the hour (00 minutes)

		const newLabels = [];
		for (let i = 0; i < 12; i++) {
			// 12 intervals of 5 minutes in an hour
			const labelTime = new Date(currentTime);
			labelTime.setMinutes(currentTime.getMinutes() + i * 5); // Increment minutes by 5
			newLabels.push(
				labelTime.toLocaleTimeString([], {
					hour: "2-digit",
					minute: "2-digit",
				})
			);
		}

		return newLabels;
	};

	// Function to fetch and parse the CSV file
	const fetchCSVData = (filePath) => {
		console.log(filePath);
		return new Promise((resolve, reject) => {
			Papa.parse(filePath, {
				download: true,
				header: true,
				dynamicTyping: true,
				complete: (result) => {
					resolve(result.data); // Resolve with the parsed data
				},
				error: (error) => reject(error),
			});
		});
	};

	// Function to filter the occupancy data based on the current hour
	const filterDataByCurrentHour = (data, currentTime) => {
		console.log(data, "hejfkdjasl");

		const currentHour = currentTime.getHours();
		const startOfHour = new Date(currentTime.setMinutes(0, 0, 0)); // Start of the current hour

		// Filter the data to get occupancy values for the current hour with 5-minute intervals
		const filteredData = data.filter((entry) => {
			const timestamp = new Date(entry.timestamp);
			return timestamp.getHours() === currentHour;
		});

		// Ensure the filtered data has the correct 5-minute intervals (0, 5, 10, 15, ...)
		const occupancyData = [];
		for (let i = 0; i < 12; i++) {
			const labelTime = new Date(startOfHour);
			labelTime.setMinutes(i * 5); // Set the 5-minute interval

			// Find the closest timestamp in the data for the given interval
			const entry = filteredData.find((entry) => {
				const timestamp = new Date(entry.timestamp);
				return timestamp.getMinutes() === labelTime.getMinutes();
			});

			// If entry is found, get the occupancy value, ensuring it's within the range [0, 8]
			const occupancyValue = entry ? entry.occupancy : 0;
			const cappedOccupancyValue = Math.max(
				0,
				Math.min(occupancyValue, 8)
			); // Cap the value between 0 and 8
			occupancyData.push(cappedOccupancyValue);
		}

		return occupancyData;
	};

	const fetchActualOutput = async (currentTime) => {
		try {
			const response = await fetch(
				"https://byebyebirdie-delta.vercel.app/output"
			); // Replace with your actual endpoint
			if (!response.ok) {
				throw new Error("Failed to fetch actual output");
			}

			// Assuming the API returns a single value for 'output'
			const data = await response.json();
			const actualOutput = data.output; // The actual output value
			console.log(actualOutput);
			// Get current time and round it to the last 5-minute interval
			const currentMinutes = currentTime.getMinutes();
			const minutesToLastFiveMin = currentMinutes % 5;
			const lastFiveMinuteInterval = new Date(currentTime);
			lastFiveMinuteInterval.setMinutes(
				currentMinutes - minutesToLastFiveMin,
				0,
				0
			);

			// Create an array of 12 points, set the most recent to actual output and the rest to 0
			const occupancyData = new Array(12).fill(0);
			const lastFiveMinIndex = Math.floor(
				lastFiveMinuteInterval.getMinutes() / 5
			);

			// Set the actual output at the last 5-minute point
			occupancyData[lastFiveMinIndex] = actualOutput;
			console.log(occupancyData);
			// Update state with the new occupancy data
			return occupancyData;
		} catch (error) {
			console.error("Error fetching actual output:", error);
		}
	};

	// Polling function to check every 5 minutes
	useEffect(() => {
		const intervalId = setInterval(async () => {
			const currentTime = new Date();
			const currentMinutes = currentTime.getMinutes();
			const minutesToLastFiveMin = currentMinutes % 5;

			// Adjust to the last 5-minute interval
			const lastFiveMinuteInterval = new Date(currentTime);
			lastFiveMinuteInterval.setMinutes(
				currentMinutes - minutesToLastFiveMin
			);
			lastFiveMinuteInterval.setSeconds(0); // Set seconds to 0

			// Extract and format hour and minutes
			const hour = String(lastFiveMinuteInterval.getHours()).padStart(
				2,
				"0"
			);
			const minutes = String(
				lastFiveMinuteInterval.getMinutes()
			).padStart(2, "0");
			const timeString = `${hour}:${minutes}`;
			setGlow([timeString]);
			fetchActualOutput(new Date(currentTime))
				.then((data) => {
					setActualData(data);
				})
				.catch((error) => console.error("Error ", error));
			fetchCSVData("/occupancy_data.csv")
				.then((data) => {
					console.log(data, "hey");
					const filteredOccupancyData = filterDataByCurrentHour(
						data,
						new Date(currentTime)
					);
					setLabels(generateLabels(new Date(currentTime)));
					setPredictedData(filteredOccupancyData); // Use your prediction logic here
				})
				.catch((error) =>
					console.error("Error fetching CSV data:", error)
				);
		}, 2000); // Poll every 5 minutes (300,000 milliseconds)

		// Cleanup interval on component unmount
		return () => clearInterval(intervalId);
	}, []);

	useEffect(() => {
		// Initial setup when the component mounts

		const currentTime = new Date();
		const currentMinutes = currentTime.getMinutes();
		const minutesToLastFiveMin = currentMinutes % 5;

		// Adjust to the last 5-minute interval
		const lastFiveMinuteInterval = new Date(currentTime);
		lastFiveMinuteInterval.setMinutes(
			currentMinutes - minutesToLastFiveMin
		);
		lastFiveMinuteInterval.setSeconds(0); // Set seconds to 0

		// Extract and format hour and minutes
		const hour = String(lastFiveMinuteInterval.getHours()).padStart(2, "0");
		const minutes = String(lastFiveMinuteInterval.getMinutes()).padStart(
			2,
			"0"
		);
		const timeString = `${hour}:${minutes}`;
		setGlow([timeString]);
		fetchActualOutput(new Date(currentTime))
			.then((data) => {
				setActualData(data);
			})
			.catch((error) => console.error("Error ", error));
		fetchCSVData("/occupancy_data.csv")
			.then((data) => {
				console.log(data, "hey");
				const filteredOccupancyData = filterDataByCurrentHour(
					data,
					new Date(currentTime)
				);
				setLabels(generateLabels(new Date(currentTime)));
				setPredictedData(filteredOccupancyData); // Use your prediction logic here
			})
			.catch((error) => console.error("Error fetching CSV data:", error));
	}, []);

    const today = new Date().toLocaleDateString(undefined, {
		year: 'numeric',
		month: 'long',
		day: 'numeric',
	});

	return (
		<div style={{ height: "100%" }}>
			<div
				style={{
					paddingLeft: "1%",
					display: "flex",
					justifyContent: "center",
					alignItems: "center",
				}}
			>
				<h4>Tray Occupancy Prediction -{today}</h4>
				<LiveStatus />
			</div>

			<BarChartComponent
				actualData={actualData}
				predictedData={predictedData}
				labels={labels}
				glow={glow}
			/>
		</div>
	);
};

export default TrayOccupancyPrediction;
