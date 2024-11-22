import requests
import pandas as pd
import matplotlib.pyplot as plt
from statsmodels.tsa.statespace.sarimax import SARIMAX
from statsmodels.tsa.stattools import adfuller

# Step 1: Fetch data from the API 
# if you are testing the code this will not work because our server is taken down (so instead comment the code below from 9 to 23 and uncomment 25 to 43 it will generate data to run the model)
try:
    response = requests.get("http://13.250.30.243/getdata")  # Replace with your API endpoint
    response.raise_for_status()  
    data_json = response.json()  

    data = pd.DataFrame(data_json)
    data['timestamp'] = pd.to_datetime(data['timestamp'])
    data.set_index('timestamp', inplace=True)
    data = data.asfreq('10min')  # Change frequency to 10 minutes
    data['occupancy'] = data['occupancy'].fillna(0) 

    print("Data fetched and processed successfully!")
except requests.exceptions.RequestException as e:
    print(f"Error fetching data: {e}")
    exit()

# # Generate timestamps for 3 days from 12 a.m. to 11:50 p.m. at 10-minute intervals
# timestamps = pd.date_range(start='2024-11-05 00:00', end='2024-11-15 23:50', freq='10min')

# # Define occupancy levels with closed hours filled with 0
# occupancy = []
# for current_time in timestamps.time:
#     if current_time >= pd.Timestamp("08:00").time() and current_time < pd.Timestamp("21:00").time():
#         if (current_time >= pd.Timestamp("12:00").time() and current_time <= pd.Timestamp("14:00").time()) or \
#            (current_time >= pd.Timestamp("18:00").time() and current_time <= pd.Timestamp("20:00").time()):
#             occupancy.append(np.random.randint(5, 9))  # Peak occupancy
#         else:
#             occupancy.append(np.random.randint(1, 5))  # Non-peak occupancy
#     else:
#         occupancy.append(0)  # Closed hours

# # Create the DataFrame
# data = pd.DataFrame({'timestamp': timestamps, 'occupancy': occupancy})
# data.set_index('timestamp', inplace=True)
# data = data.asfreq('10min') 

# Step 2: Check if the series is stationary
result = adfuller(data['occupancy'])
print(f'p-value: {result[1]}')
if result[1] > 0.05:
    print("Series is not stationary. Consider differencing.")

# Step 3: Fit a SARIMA model
# Adjust seasonal_period to 144 (10-minute intervals in a day)
model = SARIMAX(data['occupancy'],
                order=(1, 1, 1),
                seasonal_order=(1, 1, 1, 144),
                enforce_stationarity=False,
                enforce_invertibility=False)

sarima_model = model.fit(disp=False, low_memory=True)
print(sarima_model.summary())

# Step 4: Forecast the next 144 periods (1 day at 10-minute intervals)
forecast = sarima_model.get_forecast(steps=144)
forecast_ci = forecast.conf_int()

# Step 5: Plot the forecast
plt.figure(figsize=(10, 6))
plt.plot(data['occupancy'], label='Observed')
plt.plot(forecast.predicted_mean, label='Forecast', color='orange')
plt.fill_between(forecast_ci.index,
                 forecast_ci.iloc[:, 0],
                 forecast_ci.iloc[:, 1], color='pink', alpha=0.3)
plt.legend()
plt.title("Occupancy Forecast (10-minute intervals)")
plt.xlabel("Timestamp")
plt.ylabel("Occupancy")
plt.grid(True)
plt.savefig('occupancy_forecast_10min.png')
plt.close()

# Step 6: Save forecast data to a CSV file
forecast_timestamps = forecast.predicted_mean.index
forecast_occupancy = forecast.predicted_mean.values

forecast_df = pd.DataFrame({
    'timestamp': forecast_timestamps,
    'occupancy': forecast_occupancy
})

forecast_df.to_csv('forecasted_occupancy_10min.csv', index=False)
print("Forecast data saved to 'forecasted_occupancy_10min.csv'")
