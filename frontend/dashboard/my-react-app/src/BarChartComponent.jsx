import React, { useEffect, useState } from 'react';
import { VictoryChart, VictoryBar, VictoryAxis, VictoryTooltip, VictoryLegend } from 'victory';

const BarChartComponent = ({ actualData, predictedData, labels, glow }) => {
  const data = labels.map((label, index) => ({
    x: label,
    actual: Math.round(actualData[index]),
    predicted: Math.round(predictedData[index]),
    label: label
  }));

  // State for controlling opacity pulse effect
  const [pulseOpacity, setPulseOpacity] = useState(1);

  // Toggle opacity for a glow-like pulsing effect
  useEffect(() => {
    const interval = setInterval(() => {
      setPulseOpacity((prev) => (prev === 1 ? 0.3 : 1));
    }, 1000); // Change every 1 second
    return () => clearInterval(interval);
  }, []);

  return (
    <div style={{ width: '100%', height: '78vh' }}>
      <VictoryChart
        domainPadding={20}
        width={window.innerWidth}
        height={window.innerHeight}
        animate={{
          duration: 1000,
          onLoad: { duration: 500 },
        }}
      >
        {/* X and Y Axes */}
        <VictoryAxis />
        <VictoryAxis dependentAxis />

        {/* Predicted Data Bars */}
        <VictoryBar
          data={data}
          x="x"
          y="predicted"
          style={{
            data: {
              fill: ({ datum }) =>
                glow.includes(datum.x) ? "rgba(200, 200, 200, 1)" : "rgba(54, 162, 235, 1)", // Gray fill for glow months
              width: 40,
            },
          }}
          labels={({ datum }) => datum.label}
          labelComponent={<VictoryTooltip />}
        />

        {/* Actual Data Bars with Conditional Glow Effect */}
        <VictoryBar
          data={data}
          x="x"
          y="actual"
          style={{
            data: {
              fill: "rgba(255, 99, 132, 1)", // Constant red fill
              fillOpacity: ({ datum }) => (glow.includes(datum.x) ? pulseOpacity : 1), // Apply pulsing opacity for glow months
              width: 40,
            },
          }}
          labels={({ datum }) => datum.label}
          labelComponent={<VictoryTooltip />}
        />

        {/* Legend */}
        <VictoryLegend
          x={100}
          y={-5}
          title="Legend"
          centerTitle
          orientation="horizontal"
          data={[
            { name: "Actual", symbol: { fill: "rgba(255, 99, 132, 1)" } },
            { name: "Predicted", symbol: { fill: "rgba(54, 162, 235, 1)" } },
          ]}
        />
      </VictoryChart>
    </div>
  );
};

export default BarChartComponent;
