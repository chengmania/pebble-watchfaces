var Clay = require('pebble-clay');
var clayConfig = require('./config');
var ZONES = require('./zones');

// Clay handles `showConfiguration` and `webviewclosed` itself, and sends the
// settings to the watch as an AppMessage keyed by the messageKeys in config.js.
var clay = new Clay(clayConfig);

var REFRESH_MS = 30 * 60 * 1000;   // ~30 minutes, per the spec
var refreshedOnLaunch = false;
var LOCATION_OPTIONS = { timeout: 15000, maximumAge: 10 * 60 * 1000 };

function log(message) {
  console.log('LCD Watch: ' + message);
}

// Clay persists the chosen settings to localStorage under 'clay-settings'.
// Note: clay.getSettings() is NOT the way to read them back - it requires the
// config page's response string and throws without one.
function storedSettings() {
  try {
    return JSON.parse(localStorage.getItem('clay-settings') || '{}');
  } catch (e) {
    log('could not read stored settings: ' + e);
    return {};
  }
}

// Fetch the temperature in the unit the watch is going to display.
function wantsCelsius() {
  return String(storedSettings().UNIT) === '1';
}

// --- second clock -----------------------------------------------------------

function zoneEntry(zone) {
  for (var i = 0; i < ZONES.length; i++) {
    if (ZONES[i][0] === zone) {
      return ZONES[i];
    }
  }
  return ZONES[0];   // UTC
}

function zoneForLabel(label) {
  for (var i = 0; i < ZONES.length; i++) {
    if (ZONES[i][1] === label) {
      return ZONES[i][0];
    }
  }
  return null;
}

// The watch is the authority on the settings: it keeps them in persistent
// storage, which survives the app being reinstalled, whereas this side's
// localStorage does not. Seed our copy from the watch so the config page
// opens on what the user actually chose rather than on config.js defaults.
function adoptWatchSettings(payload) {
  var settings = storedSettings();
  var numeric = ['THEME', 'CLOCK_24H', 'UNIT', 'SLOT_A', 'SLOT_B', 'SLOT_C', 'SLOT_D'];

  numeric.forEach(function(key) {
    if (typeof payload[key] !== 'undefined') {
      settings[key] = String(payload[key]);
    }
  });

  ['BATTERY_BAR', 'SEPARATORS'].forEach(function(key) {
    if (typeof payload[key] !== 'undefined') {
      settings[key] = !!payload[key];
    }
  });

  if (payload.TZ_LABEL) {
    var zone = zoneForLabel(String(payload.TZ_LABEL));
    if (zone) {
      settings.TZ_ZONE = zone;
    }
  }

  try {
    localStorage.setItem('clay-settings', JSON.stringify(settings));
    log('adopted settings from watch (city ' + settings.TZ_ZONE + ')');
  } catch (e) {
    log('could not store settings: ' + e);
  }

  refreshedOnLaunch = true;
  refreshAll();
}

function selectedZone() {
  return String(storedSettings().TZ_ZONE || 'UTC');
}

// Does this runtime honour the `timeZone` option? Some engines accept the
// option and silently ignore it, which would report every city as the host's
// own time. Probing two zones twelve hours apart catches that.
var tzSupport = null;   // 'intl' | 'locale' | 'none', resolved once

function detectTimezoneSupport() {
  if (tzSupport) {
    return tzSupport;
  }

  try {
    if (typeof Intl !== 'undefined' && Intl.DateTimeFormat
        && new Intl.DateTimeFormat('en-US', { timeZone: 'UTC' }).formatToParts) {
      tzSupport = 'intl';
      return tzSupport;
    }
  } catch (e) { /* fall through */ }

  try {
    var now = new Date();
    var a = now.toLocaleString('en-US', { timeZone: 'UTC' });
    var b = now.toLocaleString('en-US', { timeZone: 'Asia/Tokyo' });
    if (a !== b) {
      tzSupport = 'locale';
      return tzSupport;
    }
  } catch (e2) { /* fall through */ }

  log('this runtime cannot resolve IANA zones; '
      + 'falling back to standard-time offsets (no DST)');
  tzSupport = 'none';
  return tzSupport;
}

// Minutes to add to UTC for `zone`, right now. Derived from a formatted local
// time rather than a table, so DST is handled wherever the runtime's timezone
// database is correct.
function offsetMinutes(zone) {
  var now = new Date();
  var support = detectTimezoneSupport();

  if (support === 'intl') {
    try {
      var dtf = new Intl.DateTimeFormat('en-US', {
        timeZone: zone, hour12: false,
        year: 'numeric', month: '2-digit', day: '2-digit',
        hour: '2-digit', minute: '2-digit', second: '2-digit'
      });
      var parts = {};
      dtf.formatToParts(now).forEach(function(part) { parts[part.type] = part.value; });
      // Some runtimes render midnight as hour 24.
      var asUTC = Date.UTC(+parts.year, +parts.month - 1, +parts.day,
                           (+parts.hour) % 24, +parts.minute, +parts.second);
      return Math.round((asUTC - now.getTime()) / 60000);
    } catch (e) {
      log('Intl failed for ' + zone + ': ' + e);
    }
  }

  if (support === 'locale') {
    try {
      var local = new Date(now.toLocaleString('en-US', { timeZone: zone }));
      var utc = new Date(now.toLocaleString('en-US', { timeZone: 'UTC' }));
      if (!isNaN(local.getTime()) && !isNaN(utc.getTime())) {
        return Math.round((local.getTime() - utc.getTime()) / 60000);
      }
    } catch (e2) {
      log('toLocaleString failed for ' + zone + ': ' + e2);
    }
  }

  // Last resort: the zone's standard-time offset. An hour off during that
  // zone's DST months, but far better than reporting the host's own time.
  return zoneEntry(zone)[3];
}

function sendTimezone() {
  // With no stored city, say nothing: the watch keeps the city it already has.
  // Sending a default here would silently reset the user's choice whenever the
  // phone-side settings store is empty, e.g. after reinstalling the app.
  var stored = storedSettings().TZ_ZONE;
  if (!stored) {
    log('no city chosen yet; leaving the watch\'s second clock alone');
    return;
  }

  var zone = String(stored);
  var entry = zoneEntry(zone);
  var minutes = offsetMinutes(zone);

  Pebble.sendAppMessage(
    { TZ_OFFSET: minutes, TZ_LABEL: entry[1] },
    function() { log('sent tz ' + entry[1] + ' ' + minutes + ' min'); },
    function(e) { log('failed to send tz: ' + (e && e.error && e.error.message)); }
  );
}

// --- weather ----------------------------------------------------------------

// TEMP_UNIT describes THIS reading, and is deliberately not UNIT: the user's
// unit preference belongs to the config page, and a weather refresh must not
// overwrite it (the phone's copy of the settings may have been cleared).
// Open-Meteo returns sunrise/sunset as local ISO strings ("2026-10-25T07:12").
// Send minutes after midnight, which is all the watch needs to print them.
function localMinutes(isoString) {
  var m = /T(\d{2}):(\d{2})/.exec(String(isoString));
  return m ? (parseInt(m[1], 10) * 60 + parseInt(m[2], 10)) : -1;
}

// WMO weather codes, collapsed to words that fit a slot.
function conditionWord(code) {
  if (code === 0) { return 'CLEAR'; }
  if (code <= 2)  { return 'FAIR'; }
  if (code === 3) { return 'CLOUDY'; }
  if (code <= 48) { return 'FOG'; }
  if (code <= 57) { return 'DRIZZLE'; }
  if (code <= 67) { return 'RAIN'; }
  if (code <= 77) { return 'SNOW'; }
  if (code <= 82) { return 'SHOWERS'; }
  if (code <= 86) { return 'SNOW'; }
  return 'STORM';
}

function sendTemp(temperature, celsius, data) {
  var message = { TEMP: Math.round(temperature), TEMP_UNIT: celsius ? 1 : 0 };

  var code = data.current && data.current.weather_code;
  if (typeof code === 'number') {
    message.CONDITION = conditionWord(code);
  }

  if (data.daily && data.daily.sunrise && data.daily.sunset) {
    var rise = localMinutes(data.daily.sunrise[0]);
    var set = localMinutes(data.daily.sunset[0]);
    if (rise >= 0) { message.SUNRISE = rise; }
    if (set >= 0)  { message.SUNSET = set; }
  }

  Pebble.sendAppMessage(
    message,
    function() { log('sent temp ' + Math.round(temperature)); },
    function(e) { log('failed to send temp: ' + e.error.message); }
  );
}

function onPosition(position) {
  var celsius = wantsCelsius();
  // Open-Meteo needs no API key and will convert units for us.
  var url = 'https://api.open-meteo.com/v1/forecast'
    + '?latitude=' + position.coords.latitude.toFixed(3)
    + '&longitude=' + position.coords.longitude.toFixed(3)
    + '&current=temperature_2m,weather_code'
    + '&daily=sunrise,sunset&forecast_days=1&timezone=auto'
    + '&temperature_unit=' + (celsius ? 'celsius' : 'fahrenheit');

  var xhr = new XMLHttpRequest();
  xhr.timeout = 20000;

  xhr.onload = function() {
    if (xhr.status !== 200) {
      log('weather HTTP ' + xhr.status);
      return;
    }
    try {
      var data = JSON.parse(xhr.responseText);
      var temperature = data.current && data.current.temperature_2m;
      if (typeof temperature === 'number') {
        sendTemp(temperature, celsius, data);
      } else {
        log('weather response had no temperature');
      }
    } catch (e) {
      log('could not parse weather response: ' + e);
    }
  };

  // The watch keeps showing its last reading (or "--") on any failure.
  xhr.onerror = function() { log('weather request failed'); };
  xhr.ontimeout = function() { log('weather request timed out'); };

  xhr.open('GET', url);
  xhr.send();
}

function onLocationError(error) {
  // The error object is not always populated, so do not assume a message.
  log('no location: ' + ((error && error.message) || 'unavailable'));
}

function refresh() {
  navigator.geolocation.getCurrentPosition(onPosition, onLocationError, LOCATION_OPTIONS);
}

// Recomputed on every refresh as well as on launch, so a DST change is picked
// up within one refresh cycle without the watch needing to know about it.
function refreshAll() {
  sendTimezone();
  setTimeout(refresh, 1000);   // stagger, so the two messages do not collide
}

// A settings dump always carries THEME; anything else is not one.
Pebble.addEventListener('appmessage', function(e) {
  var payload = (e && e.payload) || {};
  if (typeof payload.THEME !== 'undefined') {
    adoptWatchSettings(payload);
  }
});

Pebble.addEventListener('ready', function() {
  log('PebbleKit JS ready');

  // Ask the watch for its settings. refreshAll() still runs on a timer in case
  // the watch does not answer, so a missed reply only costs us the reseed.
  Pebble.sendAppMessage({ READY: 1 }, null, function(err) {
    log('READY ping failed: ' + (err && err.error && err.error.message));
  });

  // Fallback only: if the watch answered, adoptWatchSettings has already
  // refreshed and this would just repeat the same two messages.
  setTimeout(function() {
    if (!refreshedOnLaunch) {
      refreshAll();
    }
  }, 3000);
  setInterval(refreshAll, REFRESH_MS);
});

// Clay sends the new settings itself; follow up with a fetch so a unit change
// is reflected without waiting for the next scheduled refresh.
Pebble.addEventListener('webviewclosed', function(e) {
  if (e && e.response) {
    setTimeout(refreshAll, 1500);
  }
});
