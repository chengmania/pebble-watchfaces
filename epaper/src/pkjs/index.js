var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

function settings() {
  var s = {};
  try {
    s = JSON.parse(localStorage.getItem('clay-settings')) || {};
  } catch (e) {
    s = {};
  }
  return s;
}

function num(v, fallback) {
  var n = parseInt(v, 10);
  return isNaN(n) ? fallback : n;
}

function isFahrenheit() {
  var s = settings();
  return num(s.UNIT, 1) !== 0;
}

function refreshMinutes() {
  return num(settings().WX_REFRESH, 30);
}

// --------------------------------------------------------------- weather

// Open-Meteo WMO weather codes -> our icon ids.
var WX_CLEAR = 0, WX_PARTLY = 1, WX_CLOUDY = 2, WX_RAIN = 3,
    WX_SNOW = 4, WX_STORM = 5, WX_FOG = 6;

function iconForCode(code) {
  if (code === 0) return WX_CLEAR;
  if (code === 1 || code === 2) return WX_PARTLY;
  if (code === 3) return WX_CLOUDY;
  if (code === 45 || code === 48) return WX_FOG;
  if (code >= 95) return WX_STORM;
  if ((code >= 71 && code <= 77) || code === 85 || code === 86) return WX_SNOW;
  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) return WX_RAIN;
  return WX_CLOUDY;
}

// "2026-10-25T06:58" -> minutes since midnight
function isoToMinutes(iso) {
  if (!iso) return -1;
  var t = iso.split('T')[1];
  if (!t) return -1;
  var p = t.split(':');
  return parseInt(p[0], 10) * 60 + parseInt(p[1], 10);
}

function fetchWeather(lat, lon) {
  var fahrenheit = isFahrenheit();
  var url = 'https://api.open-meteo.com/v1/forecast' +
    '?latitude=' + lat.toFixed(3) +
    '&longitude=' + lon.toFixed(3) +
    '&current=temperature_2m,weather_code' +
    '&daily=sunrise,sunset' +
    '&timezone=auto&forecast_days=1' +
    (fahrenheit ? '&temperature_unit=fahrenheit' : '');

  inFlight = true;
  var req = new XMLHttpRequest();
  req.timeout = 20000;
  req.open('GET', url, true);
  req.onload = function () {
    inFlight = false;
    if (req.status !== 200) {
      console.log('InkSeg: weather HTTP ' + req.status);
      retryLater();
      return;
    }
    var data;
    try {
      data = JSON.parse(req.responseText);
    } catch (e) {
      console.log('InkSeg: bad weather JSON');
      retryLater();
      return;
    }
    if (!data || !data.current || !data.daily) {
      console.log('InkSeg: weather payload missing fields');
      retryLater();
      return;
    }
    var msg = {
      // State the unit this reading is in, so the watch never has to guess.
      UNIT: fahrenheit ? 1 : 0,
      TEMP: Math.round(data.current.temperature_2m),
      WX_ICON: iconForCode(data.current.weather_code),
      SUNRISE: isoToMinutes(data.daily.sunrise[0]),
      SUNSET: isoToMinutes(data.daily.sunset[0])
    };
    console.log('InkSeg: weather ' + JSON.stringify(msg));
    retry = 0;
    Pebble.sendAppMessage(msg, null, function () {
      console.log('InkSeg: weather send failed');
      retryLater();
    });
  };
  req.onerror = function () {
    inFlight = false;
    console.log('InkSeg: weather request failed');
    retryLater();
  };
  req.ontimeout = function () {
    inFlight = false;
    console.log('InkSeg: weather request timed out');
    retryLater();
  };
  req.send();
}

function rememberPosition(lat, lon) {
  try {
    localStorage.setItem('inkseg-pos', JSON.stringify({ lat: lat, lon: lon }));
  } catch (e) { /* storage is best-effort */ }
}

function lastPosition() {
  try {
    return JSON.parse(localStorage.getItem('inkseg-pos'));
  } catch (e) {
    return null;
  }
}

function updateWeather() {
  if (inFlight) return;
  navigator.geolocation.getCurrentPosition(
    function (pos) {
      rememberPosition(pos.coords.latitude, pos.coords.longitude);
      fetchWeather(pos.coords.latitude, pos.coords.longitude);
    },
    function (err) {
      console.log('InkSeg: location unavailable (' + ((err && err.message) || 'no detail') + ')');
      // Fall back to the last fix we saw, so the face still updates indoors.
      var p = lastPosition();
      if (p) {
        fetchWeather(p.lat, p.lon);
      } else {
        retryLater();
      }
    },
    { timeout: 15000, maximumAge: 10 * 60 * 1000 }
  );
}

// The watch drives the refresh cadence (it survives a JS restart, a timer
// here does not), so all this side does is honour a request and retry a
// failed one a few times before giving up until the next request.
var RETRY_DELAYS = [20 * 1000, 60 * 1000, 180 * 1000];
var retry = 0;
var inFlight = false;

function retryLater() {
  if (retry >= RETRY_DELAYS.length) {
    console.log('InkSeg: giving up until the next request');
    return;
  }
  var delay = RETRY_DELAYS[retry++];
  console.log('InkSeg: retrying in ' + (delay / 1000) + 's');
  setTimeout(updateWeather, delay);
}

// --------------------------------------------------------------- events

Pebble.addEventListener('ready', function () {
  updateWeather();
});

// The watch asks for a refresh when its own interval has elapsed.
Pebble.addEventListener('appmessage', function (e) {
  if (e && e.payload && e.payload.FETCH) {
    retry = 0;
    updateWeather();
  }
});

Pebble.addEventListener('showConfiguration', function () {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) return;
  var dict = clay.getSettings(e.response);
  Pebble.sendAppMessage(dict, function () {
    retry = 0;
    updateWeather();
  }, function () {
    console.log('InkSeg: settings send failed');
  });
});
