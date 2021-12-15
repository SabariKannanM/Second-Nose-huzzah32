# Second-Nose
## Problem:
Around 3.2 percent of Americans experience smell disorders like Anosmia and Hyposmia.
The undisposed waste in our homes starts to decompose after a few days and releases harmful gases like methane, sulphur dioxide, nitrogen dioxide, etc.
When these gases are inhaled by humans, it causes headache, mood swing, breathing problems, lung disease, etc. Since people with anosmia have no smell sense, they are in high risk to experiencing the above said health issues.

## Solution:
To build a Second-Nose, a device that is installed in the dust bins which tracks the air quality in it and waste level, and notifies the user via a mobile app to dispose the waste once the air gets polluted or when the bin is full. The device is connected to the internet and the data is sent through MQTT protocol. The Second-Nose consists of a BME688 gas sensor to detect the IAQ, a time of flight sensor to detect the waste level, an Adafruit Huzzah32 to get the sensor data and send the data through internet, an OLED display to display the data locally, and a battery to power the system. One of the outputs from the BME688 sensor is the IAQ which indicates the pollution level in the air. This device generates alert when the air is lightly polluted (IAQ: 101 - 150), moderately polluted (IAQ: 151 - 200) and heavily polluted (IAQ: 201 - 250).

## Mobile app features:
* Display the Index of air quality (IAQ) in the bin.
* Display the waste level.
* Give notification to dispose the waste.

## Optional features:
Estimating the air quality beforehand with the knowledge of previous ambient temperature, pressure and air quality inside the bin using the environmental sensor BME688.

## Workflow:
* STEP 1: Research previous projects and study the datasheet of the components.
* STEP 2: Establish an communication between the BME688 sensor and the Adafruit Huzzah32.
* STEP 3: Establish an communication between the OLED display and the Adafruit Huzzah32.
* STEP 4: Establish an communication between the VL53L1X time of flight sensor and the Adafruit Huzzah32.
* STEP 5: Record sensor data from various dustbins at different temperature (room, kitchen and balcony)
* STEP 6: Send the sensor data to the MQTT server.
* STEP 7: Create a mobile app to view the bin status.
* STEP 8: Build the final prototype

## Time plan:
* WEEK 21: Research
* WEEK 22: Get data from BME688 sensor, start developing mobile app
* WEEK 23: Display data on the OLED display, work on the mobile app
* WEEK 24: Send data to the MQTT server, work on the mobile app
* WEEK 25: Get data from the time of flight sensor, work on the mobile app
* WEEK 26: Test the Second-Nose
* WEEK 27: Final adjustments
* WEEK 28: Final prototype

## Evaluation plan:
If the Second-Nose is capable of sending the IAQ and waste level to the user's mobile phone through the internet, and gives notifications to dispose the waste when the pollution level is light to high, then the project would be successful.

## BOM:
* [BOM link](https://octopart.com/bom-tool/PDJ5Hqcs)

## Result:
The Second-Nose is successfully sending the IAQ and waste level in the bin to the user's mobile phone through the internet, and gives notifications to dispose the waste when the bin is full or the air is polluted (IAQ > 100).
* IAQ plot
<img src="https://github.com/ubilab-ss21/Second-Nose-huzzah32/blob/master/result_IAQ_plot.png" title="IAQ plot">

* App screen
<img src="https://github.com/ubilab-ss21/smart-bin-huzzah32/blob/master/result_app_screen.PNG" width="350" title="App screen">

* App notofications
<img src="https://github.com/ubilab-ss21/smart-bin-huzzah32/blob/master/result_app_notification.PNG" width="350" title="App notofications">

## Reference:
* [Newfangled Immaculate Trash Can Tracking System](https://www.academia.edu/download/62231884/5_IJRASET2652323-2820200229-105349-5h9jnt.pdf)
* [Smart dustbin based on IOT](https://www.researchgate.net/publication/325116872_Smart_dustbin_based_on_IOT)
* [Quick Statistics About Taste and Smell](https://www.nidcd.nih.gov/health/statistics/quick-statistics-taste-smell)
* [BME688 datasheet](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme688-ds000.pdf)
