// Get current sensor readings when the page loads  
window.addEventListener('load', getReadings);

// send immediately on load and then once per minute
window.addEventListener('load', async () => {
  sendClientTimeToDevice();
  setInterval(sendClientTimeToDevice, 60 * 1000);
});

// Safely close EventSource on page unload if it was created
window.addEventListener('beforeunload', function () {
  if (typeof source !== 'undefined' && source && typeof source.close === 'function') {
    source.close();
    console.log("EventSource connection closed");
  }
});

// Create Temperature Gauge
var gaugeTemp = new RadialGauge({
  renderTo: 'gauge-temperature',
  width: 250,
  height: 250,
  units: "°C",
  title: "Temperature",
  minValue: -40,
  maxValue: 40,
  majorTicks: [
    -40,
    -30,
    -20,
    -10,
    0,
    10,
    20,
    30,
    40
  ],
  minorTicks: 2,
  strokeTicks: true,
  highlights: [
    {
      "from": -40,
      "to": 0,
      "color": "rgba(0,0, 255, .3)"
    },
    {
      "from": 30,
      "to": 40,
      "color": "rgba(255, 0, 0, .3)"
    }
  ],
  ticksAngle: 225,
  startAngle: 67.5,
  colorMajorTicks: "#ddd",
  colorMinorTicks: "#ddd",
  colorTitle: "#eee",
  colorUnits: "#ccc",
  colorNumbers: "#eee",
  colorPlate: "#222",
  borderShadowWidth: 0,
  borders: true,
  needleType: "arrow",
  needleWidth: 2,
  needleCircleSize: 7,
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 1500,
  animationRule: "linear",
  borderInnerWidth: 0,
  borderMiddleWidth: 0,
  borderOuterWidth: 10,
  colorBorderOuter: "#ccc",
  colorBorderOuterEnd: "#ccc",
  colorNeedleShadowDown: "#333",
  colorNeedleCircleOuter: "#333",
  colorNeedleCircleOuterEnd: "#111",
  colorNeedleCircleInner: "#111",
  colorNeedleCircleInnerEnd: "#222",
  valueBoxBorderRadius: 0,
  colorValueBoxRect: "#222",
  colorValueBoxRectEnd: "#333",
  valueInt: 1,
  valueDec: 1
}).draw();

// Create Humidity Gauge
var gaugeHum = new RadialGauge({
  renderTo: 'gauge-humidity',
  width: 250,
  height: 250,
  units: "% rH",
  title: "Humidity",
  minValue: 0,
  maxValue: 100,
  majorTicks: [
    0,
    10,
    20,
    30,
    40,
    50,
    60,
    70,
    80,
    90,
    100
  ],
  minorTicks: 5,
  strokeTicks: true,
  highlights: [
    {
      "from": 0,
      "to": 35,
      "color": "rgba(255,0, 0, .3)"
    },
    {
      "from": 65,
      "to": 100,
      "color": "rgba(0, 0, 255, .3)"
    }
  ],
  ticksAngle: 225,
  startAngle: 67.5,
  colorMajorTicks: "#ddd",
  colorMinorTicks: "#ddd",
  colorTitle: "#eee",
  colorUnits: "#ccc",
  colorNumbers: "#eee",
  colorPlate: "#222",
  borderShadowWidth: 0,
  borders: true,
  needleType: "arrow",
  needleWidth: 2,
  needleCircleSize: 7,
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 1500,
  animationRule: "linear",
  borderInnerWidth: 0,
  borderMiddleWidth: 0,
  borderOuterWidth: 10,
  colorBorderOuter: "#ccc",
  colorBorderOuterEnd: "#ccc",
  colorNeedleShadowDown: "#333",
  colorNeedleCircleOuter: "#333",
  colorNeedleCircleOuterEnd: "#111",
  colorNeedleCircleInner: "#111",
  colorNeedleCircleInnerEnd: "#222",
  valueBoxBorderRadius: 0,
  colorValueBoxRect: "#222",
  colorValueBoxRectEnd: "#333",
  valueInt: 1,
  valueDec: 0
}).draw();

// Create Average Wind Speed Gauge
var gaugeWindavg = new RadialGauge({
  renderTo: 'gauge-windavg',
  width: 250,
  height: 250,
  units: "m/s",
  title: "Wind Speed (avg)",
  minValue: 0,
  maxValue: 30,
  majorTicks: [
    0,
    5,
    10,
    20,
    30
  ],
  minorTicks: 2,
  strokeTicks: true,
  highlights: [
    {
      "from": 8,
      "to": 17.2,
      "color": "rgba(255, 128, 0, .3)"
    },
    {
      "from": 17.2,
      "to": 30,
      "color": "rgba(255, 0, 0, .3)"
    }
  ],
  ticksAngle: 225,
  startAngle: 67.5,
  colorMajorTicks: "#ddd",
  colorMinorTicks: "#ddd",
  colorTitle: "#eee",
  colorUnits: "#ccc",
  colorNumbers: "#eee",
  colorPlate: "#222",
  borderShadowWidth: 0,
  borders: true,
  needleType: "arrow",
  needleWidth: 2,
  needleCircleSize: 7,
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 1500,
  animationRule: "linear",
  borderInnerWidth: 0,
  borderMiddleWidth: 0,
  borderOuterWidth: 10,
  colorBorderOuter: "#ccc",
  colorBorderOuterEnd: "#ccc",
  colorNeedleShadowDown: "#333",
  colorNeedleCircleOuter: "#333",
  colorNeedleCircleOuterEnd: "#111",
  colorNeedleCircleInner: "#111",
  colorNeedleCircleInnerEnd: "#222",
  valueBoxBorderRadius: 0,
  colorValueBoxRect: "#222",
  colorValueBoxRectEnd: "#333",
  valueInt: 1,
  valueDec: 1
}).draw();

// Create Gust Wind Speed Gauge
var gaugeWindgust = new RadialGauge({
  renderTo: 'gauge-windgust',
  width: 250,
  height: 250,
  units: "m/s",
  title: "Wind Speed (Gusts)",
  minValue: 0,
  maxValue: 30,
  majorTicks: [
    0,
    5,
    10,
    20,
    30
  ],
  minorTicks: 2,
  strokeTicks: true,
  highlights: [
    {
      "from": 8,
      "to": 17.2,
      "color": "rgba(255, 128, 0, .3)"
    },
    {
      "from": 17.2,
      "to": 30,
      "color": "rgba(255, 0, 0, .3)"
    }
  ],
  ticksAngle: 225,
  startAngle: 67.5,
  colorMajorTicks: "#ddd",
  colorMinorTicks: "#ddd",
  colorTitle: "#eee",
  colorUnits: "#ccc",
  colorNumbers: "#eee",
  colorPlate: "#222",
  borderShadowWidth: 0,
  borders: true,
  needleType: "arrow",
  needleWidth: 2,
  needleCircleSize: 7,
  needleCircleOuter: true,
  needleCircleInner: false,
  animationDuration: 1500,
  animationRule: "linear",
  borderInnerWidth: 0,
  borderMiddleWidth: 0,
  borderOuterWidth: 10,
  colorBorderOuter: "#ccc",
  colorBorderOuterEnd: "#ccc",
  colorNeedleShadowDown: "#333",
  colorNeedleCircleOuter: "#333",
  colorNeedleCircleOuterEnd: "#111",
  colorNeedleCircleInner: "#111",
  colorNeedleCircleInnerEnd: "#222",
  valueBoxBorderRadius: 0,
  colorValueBoxRect: "#222",
  colorValueBoxRectEnd: "#333",
  valueInt: 1,
  valueDec: 1
}).draw();

var gaugeWinddir = new RadialGauge({
  renderTo: 'gauge-winddir',
  width: 250,
  height: 250,
  title: "Wind Direction",
  minValue: 0,
  maxValue: 360,
  majorTicks: [
    "N",
    "NE",
    "E",
    "SE",
    "S",
    "SW",
    "W",
    "NW",
    "N"
  ],
  minorTicks: 9,
  ticksAngle: 360,
  startAngle: 180,
  strokeTicks: false,
  highlights: false,
  colorPlate: "#222",
  colorMajorTicks: "#f5f5f5",
  colorMinorTicks: "#ddd",
  colorNumbers: "#ccc",
  colorNeedle: "rgba(240, 128, 128, 1)",
  colorNeedleEnd: "rgba(255, 160, 122, .9)",
  valueBox: false,
  valueTextShadow: false,
  colorCircleInner: "#fff",
  colorNeedleCircleOuter: "#ccc",
  needleCircleSize: 15,
  needleCircleOuter: false,
  animationRule: "linear",
  needleType: "line",
  needleStart: 75,
  needleEnd: 99,
  needleWidth: 3,
  borders: true,
  borderInnerWidth: 0,
  borderMiddleWidth: 0,
  borderOuterWidth: 10,
  colorBorderOuter: "#ccc",
  colorBorderOuterEnd: "#ccc",
  colorNeedleShadowDown: "#222",
  borderShadowWidth: 0,
  animationDuration: 1500
}).draw();

// Max. values: 50, 50, 100, 100
var gaugeRainh = new LinearGauge({
  renderTo: 'gauge-rainh',
  width: 90,
  height: 250,
  title: "Past 60 mins",
  units: "mm",
  minValue: 0,
  maxValue: 50,
  majorTicks: [
    "0",
    "10",
    "20",
    "30",
    "40",
    "50"
  ],
  minorTicks: 5,
  strokeTicks: true,
  highlights: false,
  colorPlate: "#fff",
  colorBar: "#f5f5f5",
  colorBarProgress: "#327ac0",
  borderShadowWidth: 0,
  borders: false,
  needleType: "arrow",
  needleWidth: 2,
  animationDuration: 1500,
  animationRule: "linear",
  tickSide: "left",
  numberSide: "left",
  needleSide: "left",
  barStrokeWidth: 4,
  barBeginCircle: false,
  valueInt: 1,
  valueDec: 1
}).draw();

var gaugeRaind = new LinearGauge({
  renderTo: 'gauge-raind',
  width: 90,
  height: 250,
  title: "Today",
  units: "mm",
  minValue: 0,
  maxValue: 50,
  majorTicks: [
    "0",
    "10",
    "20",
    "30",
    "40",
    "50"
  ],
  minorTicks: 5,
  strokeTicks: true,
  highlights: false,
  colorPlate: "#fff",
  colorBar: "#f5f5f5",
  colorBarProgress: "#327ac0",
  borderShadowWidth: 0,
  borders: false,
  needleType: "arrow",
  needleWidth: 2,
  animationDuration: 1500,
  animationRule: "linear",
  tickSide: "left",
  numberSide: "left",
  needleSide: "left",
  barStrokeWidth: 4,
  barBeginCircle: false,
  valueInt: 1,
  valueDec: 1
}).draw();

var gaugeRainw = new LinearGauge({
  renderTo: 'gauge-rainw',
  width: 90,
  height: 250,
  title: "Week",
  units: "mm",
  minValue: 0,
  maxValue: 100,
  majorTicks: [
    "0",
    "20",
    "40",
    "60",
    "80",
    "100"
  ],
  minorTicks: 10,
  strokeTicks: true,
  highlights: false,
  colorPlate: "#fff",
  colorBar: "#f5f5f5",
  colorBarProgress: "#327ac0",
  borderShadowWidth: 0,
  borders: false,
  needleType: "arrow",
  needleWidth: 2,
  animationDuration: 1500,
  animationRule: "linear",
  tickSide: "left",
  numberSide: "left",
  needleSide: "left",
  barStrokeWidth: 4,
  barBeginCircle: false,
  valueInt: 1,
  valueDec: 1
}).draw();

var gaugeRainm = new LinearGauge({
  renderTo: 'gauge-rainm',
  width: 90,
  height: 250,
  title: "Month",
  units: "mm",
  minValue: 0,
  maxValue: 100,
  majorTicks: [
    "0",
    "20",
    "40",
    "60",
    "80",
    "100"
  ],
  minorTicks: 10,
  strokeTicks: true,
  highlights: false,
  colorPlate: "#fff",
  colorBar: "#f5f5f5",
  colorBarProgress: "#327ac0",
  borderShadowWidth: 0,
  borders: false,
  needleType: "arrow",
  needleWidth: 2,
  animationDuration: 1500,
  animationRule: "linear",
  tickSide: "left",
  numberSide: "left",
  needleSide: "left",
  barStrokeWidth: 4,
  barBeginCircle: false,
  valueInt: 1,
  valueDec: 1
}).draw();

/* Limit value to prevent overflow of gauge */
function limitValue(value, min, max) {
  return Math.min(Math.max(value, min), max);
}

/* update "last update" element (pass ms since epoch or omit to use now) */
function updateLastUpdate() {
  const el = document.getElementById('last-update');
  if (!el) return;
  el.textContent = formatTimestamp(Date.now());
}

// Function to get current readings on the webpage when it loads for the first time
function getReadings() {
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      if (myObj != null) {
        if (myObj.ws_temp_c !== undefined) {
          var temp = myObj.ws_temp_c;
          gaugeTemp.value = temp;
        }
        if (myObj.ws_humidity !== undefined) {
          var hum = myObj.ws_humidity;
          gaugeHum.value = hum;
        }
        if (myObj.ws_wind_avg_ms !== undefined) {
          var windavg = myObj.ws_wind_avg_ms;
          gaugeWindavg.value = windavg;
        }
        if (myObj.ws_wind_gust_ms !== undefined) {
          var windgust = myObj.ws_wind_gust_ms;
          gaugeWindgust.value = windgust;
        }
        if (myObj.ws_wind_dir_deg !== undefined) {
          var winddir = myObj.ws_wind_dir_deg;
          gaugeWinddir.value = winddir;
        }
        if (myObj.ws_rain_h !== undefined) {
          var rainh = myObj.ws_rain_h;
          gaugeRainh.value = limitValue(rainh, -1, 52);
        }
        if (myObj.ws_rain_d !== undefined) {
          var raind = myObj.ws_rain_d;
          gaugeRaind.value = limitValue(raind, -1, 52);
        }
        if (myObj.ws_rain_w !== undefined) {
          var rainw = myObj.ws_rain_w;
          gaugeRainw.value = limitValue(rainw, -1, 104);
        }
        if (myObj.ws_rain_m !== undefined) {
          var rainm = myObj.ws_rain_m;
          gaugeRainm.value = limitValue(rainm, -1, 104);
        }
      }
    }
  };
  xhr.open("GET", "/readings", true);
  xhr.send();

}

// Send current unix time (seconds) to device
// This is needed for ESP in WiFi AP mode without access to internet
// and thus NTP server.
function sendClientTimeToDevice() {
  const epoch = Math.floor(Date.now() / 1000);
  fetch(`/settime?epoch=${epoch}`)
    .then(r => r.text())
    .then(txt => console.log('settime:', epoch, txt))
    .catch(err => console.error('settime error', err));
}

if (!!window.EventSource) {
  var source = new EventSource('/events');

  source.addEventListener('open', function (e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function (e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);

  source.addEventListener('message', function (e) {
    console.log("message", e.data);
  }, false);

  source.addEventListener('new_readings', function (e) {
    console.log("new_readings", e.data);
    try {
      var myObj = JSON.parse(e.data);
      console.log(myObj);
      if (myObj != null) {
        if (myObj.ws_temp_c !== undefined) {
          gaugeTemp.value = myObj.ws_temp_c;
        }
        if (myObj.ws_humidity !== undefined) {
          gaugeHum.value = myObj.ws_humidity;
        }
        if (myObj.ws_wind_avg_ms !== undefined) {
          gaugeWindavg.value = myObj.ws_wind_avg_ms;
        }
        if (myObj.ws_wind_gust_ms !== undefined) {
          gaugeWindgust.value = myObj.ws_wind_gust_ms;
        }
        if (myObj.ws_wind_dir_deg !== undefined) {
          gaugeWinddir.value = myObj.ws_wind_dir_deg;
        }
        if (myObj.ws_rain_h !== undefined) {
          gaugeRainh.value = limitValue(myObj.ws_rain_h, -1, 52);
        }
        if (myObj.ws_rain_d !== undefined) {
          gaugeRaind.value = limitValue(myObj.ws_rain_d, -1, 52);
        }
        if (myObj.ws_rain_w !== undefined) {
          gaugeRainw.value = limitValue(myObj.ws_rain_w, -1, 104);
        }
        if (myObj.ws_rain_m !== undefined) {
          gaugeRainm.value = limitValue(myObj.ws_rain_m, -1, 104);
        }
      }
    } catch (error) {
      console.error("Error parsing JSON:", error);
    }
  }, false);
}

