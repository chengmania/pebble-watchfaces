var SLOT_OPTIONS = [
  { label: 'Off', value: '0' },
  { label: 'Weather', value: '1' },
  { label: 'Sunrise', value: '2' },
  { label: 'Sunset', value: '3' },
  { label: 'Sunrise + Sunset', value: '4' },
  { label: 'Heart rate', value: '5' },
  { label: 'Steps', value: '6' },
  { label: 'Battery', value: '7' },
  { label: 'Date', value: '8' }
];

var RING_OPTIONS = [
  { label: 'Steps', value: '0' },
  { label: 'Battery', value: '1' },
  { label: 'Off', value: '2' }
];

function slotItem(key, label, defaultValue) {
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
    defaultValue: 'Arc Storm'
  },
  {
    type: 'text',
    defaultValue: 'Electric-blue cyberpunk watchface.'
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Theme' },
      {
        type: 'select',
        messageKey: 'THEME',
        label: 'Theme',
        defaultValue: '0',
        options: [{ label: 'Arc Storm (blue)', value: '0' }]
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Top row' },
      slotItem('TOP_1', 'Left', '1'),
      slotItem('TOP_2', 'Centre', '2'),
      slotItem('TOP_3', 'Right', '3')
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Bottom row' },
      slotItem('BOT_1', 'Left', '5'),
      slotItem('BOT_2', 'Centre', '6'),
      slotItem('BOT_3', 'Right', '7')
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Rings' },
      {
        type: 'select',
        messageKey: 'RING_OUTER',
        label: 'Outer ring',
        defaultValue: '0',
        options: RING_OPTIONS
      },
      {
        type: 'select',
        messageKey: 'RING_INNER',
        label: 'Inner ring',
        defaultValue: '1',
        options: RING_OPTIONS
      },
      {
        type: 'input',
        messageKey: 'STEP_GOAL',
        label: 'Step goal',
        defaultValue: '10000',
        attributes: { type: 'number', min: 100, step: 100 }
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Units & format' },
      {
        type: 'radiogroup',
        messageKey: 'UNIT',
        label: 'Temperature',
        defaultValue: '1',
        options: [
          { label: '°F', value: '1' },
          { label: '°C', value: '0' }
        ]
      },
      {
        type: 'radiogroup',
        messageKey: 'CLOCK_24H',
        label: 'Time format',
        defaultValue: '0',
        options: [
          { label: 'Follow watch', value: '0' },
          { label: '12 hour', value: '1' },
          { label: '24 hour', value: '2' }
        ]
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Effects' },
      {
        type: 'radiogroup',
        messageKey: 'ANIM_MODE',
        label: 'Animations',
        defaultValue: '0',
        options: [
          { label: 'Full', value: '0' },
          { label: 'Calm (none)', value: '1' }
        ]
      },
      {
        type: 'radiogroup',
        messageKey: 'HR_PULSE',
        label: 'Heart pulse',
        defaultValue: '0',
        options: [
          { label: 'On flick', value: '0' },
          { label: 'Always', value: '1' }
        ]
      },
      {
        type: 'select',
        messageKey: 'WX_REFRESH',
        label: 'Weather refresh',
        defaultValue: '30',
        options: [
          { label: '15 minutes', value: '15' },
          { label: '30 minutes', value: '30' },
          { label: '60 minutes', value: '60' }
        ]
      }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
