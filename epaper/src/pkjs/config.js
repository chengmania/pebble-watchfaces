module.exports = [
  {
    type: 'heading',
    defaultValue: 'InkSeg'
  },
  {
    type: 'text',
    defaultValue: 'A stark 1-bit e-paper watchface.'
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Appearance' },
      {
        type: 'select',
        messageKey: 'INVERT',
        label: 'Invert',
        defaultValue: '0',
        options: [
          { label: 'Black on white', value: '0' },
          { label: 'White on black', value: '1' }
        ]
      },
      {
        type: 'select',
        messageKey: 'DATE_FMT',
        label: 'Date format',
        defaultValue: '0',
        options: [
          { label: 'SUNDAY / OCTOBER 25', value: '0' },
          { label: 'SUN 25 OCT', value: '1' }
        ]
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Time' },
      {
        type: 'select',
        messageKey: 'CLOCK_24H',
        label: 'Time format',
        defaultValue: '0',
        options: [
          { label: '12 hour', value: '0' },
          { label: '24 hour', value: '1' }
        ]
      },
      {
        type: 'toggle',
        messageKey: 'LEADING_ZERO',
        label: 'Leading zero (24h)',
        defaultValue: true
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Data rows' },
      { type: 'toggle', messageKey: 'SHOW_STEPS', label: 'Steps', defaultValue: true },
      { type: 'toggle', messageKey: 'SHOW_BATTERY', label: 'Battery', defaultValue: true },
      {
        type: 'toggle',
        messageKey: 'SHOW_HR',
        label: 'Heart rate monitor',
        description: 'Shows the heart rate row and asks the watch for regular readings. Off hides the row and leaves the sensor on the system\'s own battery-saving schedule.',
        defaultValue: true
      },
      { type: 'toggle', messageKey: 'SHOW_WX', label: 'Weather', defaultValue: true },
      { type: 'toggle', messageKey: 'SHOW_SUN', label: 'Sunrise / sunset', defaultValue: true }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'World time' },
      {
        type: 'toggle',
        messageKey: 'SHOW_WORLD',
        label: 'Second time zone',
        description: 'Shown in small type under the battery.',
        defaultValue: true
      },
      {
        type: 'input',
        messageKey: 'WORLD_LABEL',
        label: 'Label',
        defaultValue: 'UTC',
        attributes: { maxlength: 5, placeholder: 'e.g. TOK' }
      },
      {
        type: 'select',
        messageKey: 'WORLD_OFFSET',
        label: 'UTC offset',
        defaultValue: '0',
        options: [
          { label: 'UTC-11:00', value: '-660' },
          { label: 'UTC-10:00 Honolulu', value: '-600' },
          { label: 'UTC-09:00 Anchorage', value: '-540' },
          { label: 'UTC-08:00 Los Angeles', value: '-480' },
          { label: 'UTC-07:00 Denver', value: '-420' },
          { label: 'UTC-06:00 Chicago', value: '-360' },
          { label: 'UTC-05:00 New York', value: '-300' },
          { label: 'UTC-04:00 Halifax', value: '-240' },
          { label: 'UTC-03:00 Sao Paulo', value: '-180' },
          { label: 'UTC-01:00 Azores', value: '-60' },
          { label: 'UTC+00:00 London', value: '0' },
          { label: 'UTC+01:00 Berlin', value: '60' },
          { label: 'UTC+02:00 Athens', value: '120' },
          { label: 'UTC+03:00 Moscow', value: '180' },
          { label: 'UTC+03:30 Tehran', value: '210' },
          { label: 'UTC+04:00 Dubai', value: '240' },
          { label: 'UTC+05:00 Karachi', value: '300' },
          { label: 'UTC+05:30 Mumbai', value: '330' },
          { label: 'UTC+06:00 Dhaka', value: '360' },
          { label: 'UTC+07:00 Bangkok', value: '420' },
          { label: 'UTC+08:00 Singapore', value: '480' },
          { label: 'UTC+09:00 Tokyo', value: '540' },
          { label: 'UTC+09:30 Adelaide', value: '570' },
          { label: 'UTC+10:00 Sydney', value: '600' },
          { label: 'UTC+12:00 Auckland', value: '720' }
        ]
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Weather' },
      {
        type: 'select',
        messageKey: 'UNIT',
        label: 'Units',
        defaultValue: '1',
        options: [
          { label: 'Fahrenheit', value: '1' },
          { label: 'Celsius', value: '0' }
        ]
      },
      {
        type: 'select',
        messageKey: 'WX_REFRESH',
        label: 'Refresh every',
        defaultValue: '30',
        options: [
          { label: '15 minutes', value: '15' },
          { label: '30 minutes', value: '30' },
          { label: '60 minutes', value: '60' }
        ]
      }
    ]
  },
  { type: 'submit', defaultValue: 'Save' }
];
