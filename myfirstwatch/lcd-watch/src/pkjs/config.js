var ZONES = require('./zones');

// Clay settings page. Values are sent as strings and parsed on the watch,
// so every `value` here must match the matching enum in src/c/lcd-watch.c.

var SLOT_OPTIONS = [
  { label: 'Temperature', value: '0' },
  { label: 'Steps',       value: '1' },
  { label: 'Date',        value: '2' },
  { label: 'Day',         value: '3' },
  { label: 'Battery',     value: '4' },
  { label: 'Off',           value: '5' },
  { label: 'Heart rate',    value: '6' },
  { label: 'Active minutes', value: '7' },
  { label: 'Distance',      value: '8' },
  { label: 'Calories',      value: '9' },
  { label: 'Sunrise',       value: '10' },
  { label: 'Sunset',        value: '11' },
  { label: 'Condition',     value: '12' }
];

function slot(key, label, defaultValue) {
  return {
    type: 'select',
    messageKey: key,
    label: label,
    defaultValue: defaultValue,
    options: SLOT_OPTIONS
  };
}

module.exports = [
  {
    type: 'heading',
    defaultValue: 'LCD Watch'
  },
  {
    type: 'text',
    defaultValue: 'A retro LCD watchface with four complication slots and a second clock.'
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Appearance' },
      {
        type: 'select',
        messageKey: 'THEME',
        label: 'Theme',
        defaultValue: '0',
        options: [
          { label: 'Classic Grey',           value: '0' },
          { label: 'Classic Grey Inverted',  value: '2' },
          { label: 'Retro Amber',            value: '1' },
          { label: 'Retro Amber Inverted',   value: '3' },
          { label: 'LCD Green',              value: '4' },
          { label: 'LCD Green Inverted',     value: '5' },
          { label: 'Blue Backlight',         value: '6' },
          { label: 'Blue Backlight Inverted', value: '7' },
          { label: 'Red LED',                value: '8' },
          { label: 'Red LED Inverted',       value: '9' }
        ]
      },
      {
        type: 'select',
        messageKey: 'CLOCK_24H',
        label: 'Time format',
        defaultValue: '0',
        options: [
          { label: 'Follow watch setting', value: '0' },
          { label: '12 hour',              value: '1' },
          { label: '24 hour',              value: '2' }
        ]
      },
      {
        type: 'toggle',
        messageKey: 'SEPARATORS',
        label: 'Panel separators',
        description: 'Frame lines between the panels. Off makes the face one continuous display.',
        defaultValue: true
      },
      {
        type: 'toggle',
        messageKey: 'BATTERY_BAR',
        label: 'Battery bar',
        description: 'A thin charge bar across the top of the face.',
        defaultValue: true
      },
      {
        type: 'select',
        messageKey: 'UNIT',
        label: 'Temperature units',
        defaultValue: '0',
        options: [
          { label: 'Fahrenheit (°F)', value: '0' },
          { label: 'Celsius (°C)',    value: '1' }
        ]
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Complication slots' },
      {
        type: 'text',
        defaultValue: 'A and B sit above the time, C and D below it.'
      },
      slot('SLOT_A', 'Slot A (top left)',     '0'),
      slot('SLOT_B', 'Slot B (top right)',    '1'),
      slot('SLOT_C', 'Slot C (bottom left)',  '3'),
      slot('SLOT_D', 'Slot D (bottom right)', '2')
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Second clock' },
      {
        type: 'select',
        messageKey: 'TZ_ZONE',
        label: 'City',
        defaultValue: 'UTC',
        options: ZONES.map(function(zone) {
          return { label: zone[2] + ' (' + zone[1] + ')', value: zone[0] };
        })
      }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
