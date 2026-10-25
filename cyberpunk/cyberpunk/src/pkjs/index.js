var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

// Clay stores every value as a string; the watch wants plain integers.
var INT_KEYS = [
  'THEME', 'TOP_1', 'TOP_2', 'TOP_3', 'BOT_1', 'BOT_2', 'BOT_3',
  'RING_OUTER', 'RING_INNER', 'STEP_GOAL', 'UNIT', 'CLOCK_24H',
  'ANIM_MODE', 'HR_PULSE', 'WX_REFRESH'
];

// Weather icon ids, matching the WeatherIcon enum in slots.h.
var WX_CLEAR = 0, WX_CLOUDY = 1, WX_RAIN = 2, WX_SNOW = 3, WX_STORM = 4, WX_FOG = 5;

var s_refreshTimer = null;
var s_unitIsF = true;

function settingValue(settings, key, fallback) {
  var raw = settings[key];
  if (raw === undefined || raw === null) {
    return fallback;
  }
  if (typeof raw === 'object' && raw.value !== undefined) {
    raw = raw.value;
  }
  var n = parseInt(raw, 10);
  return isNaN(n) ? fallback : n;
}

function toWatchDict(settings) {
  var dict = {};
  INT_KEYS.forEach(function (key) {
    var n = settingValue(settings, key, null);
    if (n !== null) {
      dict[key] = n;
    }
  });
  return dict;
}

function iconForCode(code) {
  if (code === 0 || code === 1) { return WX_CLEAR; }
  if (code === 2 || code === 3) { return WX_CLOUDY; }
  if (code === 45 || code === 48) { return WX_FOG; }
  if (code >= 95) { return WX_STORM; }
  if ((code >= 71 && code <= 77) || code === 85 || code === 86) { return WX_SNOW; }
  if ((code >= 51 && code <= 67) || (code >= 80 && code <= 82)) { return WX_RAIN; }
  return WX_CLOUDY;
}

// "2026-10-25T07:12" -> minutes since local midnight.
function isoToMinutes(iso) {
  if (!iso) { return -1; }
  var t = iso.indexOf('T');
  if (t < 0) { return -1; }
  var parts = iso.substring(t + 1).split(':');
  var h = parseInt(parts[0], 10);
  var m = parseInt(parts[1], 10);
  if (isNaN(h) || isNaN(m)) { return -1; }
  return h * 60 + m;
}

function sendWeather(json) {
  var dict = {};
  if (json.current && typeof json.current.temperature_2m === 'number') {
    dict.TEMP = Math.round(json.current.temperature_2m);
    dict.WX_ICON = iconForCode(json.current.weather_code);
  }
  if (json.daily && json.daily.sunrise && json.daily.sunset) {
    var rise = isoToMinutes(json.daily.sunrise[0]);
    var set = isoToMinutes(json.daily.sunset[0]);
    if (rise >= 0 && set >= 0) {
      dict.SUNRISE = rise;
      dict.SUNSET = set;
    }
  }
  if (Object.keys(dict).length === 0) {
    return;
  }
  Pebble.sendAppMessage(dict, function () {
    console.log('Weather sent: ' + JSON.stringify(dict));
  }, function (e) {
    console.log('Weather send failed: ' + JSON.stringify(e));
  });
}

function fetchWeather(lat, lon) {
  var url = 'https://api.open-meteo.com/v1/forecast' +
    '?latitude=' + lat +
    '&longitude=' + lon +
    '&current=temperature_2m,weather_code' +
    '&daily=sunrise,sunset' +
    '&timezone=auto&forecast_days=1' +
    '&temperature_unit=' + (s_unitIsF ? 'fahrenheit' : 'celsius');

  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function () {
    if (req.status !== 200) {
      console.log('Open-Meteo HTTP ' + req.status);
      return;
    }
    try {
      sendWeather(JSON.parse(req.responseText));
    } catch (err) {
      console.log('Open-Meteo parse error: ' + err);
    }
  };
  req.onerror = function () {
    console.log('Open-Meteo request failed');
  };
  req.send();
}

function updateWeather() {
  navigator.geolocation.getCurrentPosition(
    function (pos) {
      fetchWeather(pos.coords.latitude.toFixed(3), pos.coords.longitude.toFixed(3));
    },
    function (err) {
      console.log('Geolocation unavailable: ' + ((err && err.message) || 'unknown'));
    },
    { timeout: 15000, maximumAge: 10 * 60 * 1000 }
  );
}

function applySettings(settings) {
  s_unitIsF = settingValue(settings, 'UNIT', 1) !== 0;
  var refreshMin = settingValue(settings, 'WX_REFRESH', 30);

  if (s_refreshTimer) {
    clearInterval(s_refreshTimer);
  }
  s_refreshTimer = setInterval(updateWeather, refreshMin * 60 * 1000);
}

Pebble.addEventListener('showConfiguration', function () {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e || !e.response) {
    return;
  }
  var settings = clay.getSettings(e.response, false);
  sendSettings(settings);
  applySettings(settings);
  updateWeather();
});

// clay.getSettings() only parses a webview response; on launch the saved
// values have to come straight out of localStorage. The watch keeps its own
// persisted copy, so nothing is sent back to it here.
function storedSettings() {
  try {
    return JSON.parse(localStorage.getItem('clay-settings')) || {};
  } catch (err) {
    console.log('Could not read stored settings: ' + err);
    return {};
  }
}

Pebble.addEventListener('ready', function () {
  console.log('Arc Storm PKJS ready');
  applySettings(storedSettings());
  updateWeather();
});
