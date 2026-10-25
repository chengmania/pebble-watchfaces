// Second-clock cities. One entry per zone:
//   [IANA zone, watch label, menu name, standard-time offset in minutes]
// The watch label is what gets drawn next to the small clock, so it is kept to
// the three characters the layout has room for.
// UTC is pinned first; the rest are grouped west to east.
//
// The offset is STANDARD time only and exists purely as a last-resort
// fallback for JS runtimes that cannot resolve an IANA zone (see index.js).
// It is an hour off in a zone's DST months; the Intl path is what makes the
// second clock DST-correct.
module.exports = [
  ['UTC',                            'UTC', 'UTC', 0],

  ['Pacific/Honolulu',               'HNL', 'Honolulu', -600],
  ['America/Anchorage',              'ANC', 'Anchorage', -540],
  ['America/Los_Angeles',            'LAX', 'Los Angeles', -480],
  ['America/Denver',                 'DEN', 'Denver', -420],
  ['America/Chicago',                'CHI', 'Chicago', -360],
  ['America/New_York',               'NYC', 'New York', -300],
  ['America/Toronto',                'TOR', 'Toronto', -300],
  ['America/Mexico_City',            'MEX', 'Mexico City', -360],
  ['America/Sao_Paulo',              'SAO', 'Sao Paulo', -180],
  ['America/Argentina/Buenos_Aires', 'BUE', 'Buenos Aires', -180],

  ['Europe/London',                  'LON', 'London', 0],
  ['Europe/Paris',                   'PAR', 'Paris', 60],
  ['Europe/Berlin',                  'BER', 'Berlin', 60],
  ['Europe/Rome',                    'ROM', 'Rome', 60],
  ['Europe/Istanbul',                'IST', 'Istanbul', 180],
  ['Europe/Moscow',                  'MOS', 'Moscow', 180],
  ['Africa/Cairo',                   'CAI', 'Cairo', 120],
  ['Africa/Lagos',                   'LOS', 'Lagos', 60],
  ['Africa/Nairobi',                 'NBO', 'Nairobi', 180],
  ['Africa/Johannesburg',            'JNB', 'Johannesburg', 120],

  ['Asia/Dubai',                     'DXB', 'Dubai', 240],
  ['Asia/Kolkata',                   'BOM', 'Mumbai / Kolkata', 330],
  ['Asia/Bangkok',                   'BKK', 'Bangkok', 420],
  ['Asia/Singapore',                 'SIN', 'Singapore', 480],
  ['Asia/Hong_Kong',                 'HKG', 'Hong Kong', 480],
  ['Asia/Shanghai',                  'SHA', 'Shanghai', 480],
  ['Asia/Tokyo',                     'TYO', 'Tokyo', 540],
  ['Asia/Seoul',                     'SEL', 'Seoul', 540],
  ['Australia/Sydney',               'SYD', 'Sydney', 600],
  ['Pacific/Auckland',               'AKL', 'Auckland', 720]
];
