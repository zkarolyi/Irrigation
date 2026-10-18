'use strict';

// Single-page UI for the irrigation controller.
// Data comes from the JSON API (/api/*); live updates arrive over SSE (/events).

const state = {
  scheduled: true,
  channels: [],
  names: [],
  durationMin: 1,
  durationMax: 90,
  durationDef: 20,
  deadline: 0, // Date.now() based end of the running channel, 0 = idle
  loaded: false
};
let scheduleData = null; // last /api/schedules response
let online = true;

// ---------- helpers ----------

// Creates an element: h('div', {class: 'x', onclick: fn}, 'text', childNode, ...)
function h(tag, attrs, ...children) {
  const el = document.createElement(tag);
  for (const [key, value] of Object.entries(attrs || {})) {
    if (value === false || value == null) continue;
    if (key.startsWith('on')) el.addEventListener(key.slice(2), value);
    else if (key === 'value') el.value = value;
    else el.setAttribute(key, value === true ? '' : value);
  }
  for (const child of children.flat()) {
    if (child == null || child === false) continue;
    el.append(child instanceof Node ? child : document.createTextNode(String(child)));
  }
  return el;
}

const $ = (id) => document.getElementById(id);
const pad = (n) => String(n).padStart(2, '0');
const formatTime = (minutes) => pad(Math.floor(minutes / 60)) + ':' + pad(minutes % 60);
const formatLeft = (seconds) => pad(Math.floor(seconds / 60)) + ':' + pad(seconds % 60);

function storageGet(key, fallback) {
  try { const v = localStorage.getItem(key); return v === null ? fallback : v; } catch (e) { return fallback; }
}
function storageSet(key, value) {
  try { localStorage.setItem(key, value); } catch (e) { /* private mode */ }
}

async function api(path, body) {
  const options = body === undefined ? {} : {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body)
  };
  const response = await fetch(path, options);
  let data = null;
  try { data = await response.json(); } catch (e) { /* empty body */ }
  if (!response.ok) throw new Error((data && data.error) || 'Request failed (' + response.status + ')');
  return data;
}

let toastTimer = 0;
function toast(message, isError) {
  const el = $('toast');
  el.textContent = message;
  el.classList.toggle('error', !!isError);
  el.hidden = false;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { el.hidden = true; }, isError ? 5000 : 2500);
}

function confirmDialog(text) {
  const dialog = $('confirm');
  if (!dialog.showModal) return Promise.resolve(confirm(text));
  $('confirmText').textContent = text;
  dialog.returnValue = '';
  return new Promise((resolve) => {
    dialog.addEventListener('close', () => resolve(dialog.returnValue === 'ok'), { once: true });
    dialog.showModal();
  });
}

function channelName(i) {
  return state.names[i] || 'Channel ' + (i + 1);
}

// ---------- state ----------

function applyState(data) {
  state.scheduled = data.scheduled;
  state.channels = data.channels;
  state.names = data.names;
  state.durationMin = data.durationMin;
  state.durationMax = data.durationMax;
  state.durationDef = data.durationDef;
  setRemaining(data.remaining);
  state.loaded = true;
}

function setRemaining(seconds) {
  state.deadline = seconds > 0 ? Date.now() + seconds * 1000 : 0;
}

async function loadState() {
  try {
    applyState(await api('/api/state'));
    if (currentRoute().name === 'control') renderControl();
    else updateStatus();
  } catch (e) {
    toast(e.message, true);
  }
}

// ---------- control view ----------

function renderControl() {
  const view = $('view');
  if (!state.loaded) {
    view.replaceChildren(h('p', { class: 'muted' }, 'Loading…'));
    return;
  }
  const saved = parseInt(storageGet('duration', state.durationDef), 10);
  const duration = isNaN(saved) ? state.durationDef : saved;

  view.replaceChildren(
    h('section', { class: 'card status' },
      h('div', { class: 'status-line' },
        h('span', { id: 'modeBadge', class: 'badge' }),
        h('span', { id: 'timeLeft', class: 'time-left' })),
      h('label', { class: 'switch-row' },
        h('span', {}, 'Scheduled mode'),
        h('input', { type: 'checkbox', id: 'modeSwitch', class: 'switch', onchange: onModeChange }))),
    h('section', { class: 'card' },
      h('label', { for: 'duration', class: 'field-label' }, 'Manual run time (minutes)'),
      h('div', { class: 'stepper' },
        h('button', { type: 'button', class: 'btn', 'aria-label': 'Less', onclick: () => stepDuration(-5) }, '−'),
        h('input', {
          type: 'number', id: 'duration', inputmode: 'numeric',
          min: state.durationMin, max: state.durationMax, value: duration,
          onchange: () => stepDuration(0)
        }),
        h('button', { type: 'button', class: 'btn', 'aria-label': 'More', onclick: () => stepDuration(5) }, '+'))),
    h('ul', { class: 'channels', id: 'channels' },
      state.channels.map((_, i) => h('li', {},
        h('button', { type: 'button', class: 'channel', 'data-ch': i, onclick: () => onChannelClick(i) },
          h('span', { class: 'num' }, i + 1),
          h('span', { class: 'name' }, channelName(i)),
          h('span', { class: 'state' }))))),
    h('button', { type: 'button', class: 'btn danger wide', onclick: () => onChannelClick(-1) }, 'All off')
  );
  updateStatus();
  updateChannels();
}

function updateStatus() {
  const badge = $('modeBadge');
  if (!badge) return;
  badge.textContent = state.scheduled ? 'Scheduled' : 'Manual';
  badge.classList.toggle('manual', !state.scheduled);
  $('modeSwitch').checked = state.scheduled;
  updateCountdown();
}

function updateCountdown() {
  const el = $('timeLeft');
  if (!el) return;
  const seconds = state.deadline ? Math.max(0, Math.round((state.deadline - Date.now()) / 1000)) : 0;
  el.textContent = seconds > 0 ? formatLeft(seconds) + ' left' : '';
}

function updateChannels() {
  const list = $('channels');
  if (!list) return;
  list.querySelectorAll('.channel').forEach((button) => {
    const on = state.channels[button.dataset.ch] === 1;
    button.setAttribute('aria-pressed', on);
    button.querySelector('.state').textContent = on ? 'On' : 'Off';
  });
}

function stepDuration(delta) {
  const input = $('duration');
  let value = parseInt(input.value, 10);
  if (isNaN(value)) value = state.durationDef;
  value = Math.min(state.durationMax, Math.max(state.durationMin, value + delta));
  input.value = value;
  storageSet('duration', value);
}

async function onChannelClick(i) {
  const duration = parseInt($('duration').value, 10);
  const starting = i >= 0 && state.channels[i] !== 1;
  if (starting && (isNaN(duration) || duration < state.durationMin || duration > state.durationMax)) {
    toast('Run time must be ' + state.durationMin + '–' + state.durationMax + ' minutes', true);
    return;
  }
  // Optimistic: only one channel runs at a time; SSE and the response confirm
  const previous = { channels: state.channels.slice(), scheduled: state.scheduled, deadline: state.deadline };
  state.channels = state.channels.map((_, j) => (starting && j === i ? 1 : 0));
  if (starting) {
    state.scheduled = false;
    state.deadline = Date.now() + duration * 60000;
  } else if (!state.scheduled) {
    state.deadline = 0;
  }
  updateChannels();
  updateStatus();
  try {
    applyState(await api('/api/channel', { ch: i + 1, duration: duration || state.durationDef }));
  } catch (e) {
    Object.assign(state, previous);
    toast(e.message, true);
  }
  updateChannels();
  updateStatus();
}

async function onModeChange(event) {
  const scheduled = event.target.checked;
  try {
    applyState(await api('/api/mode', { scheduled }));
  } catch (e) {
    toast(e.message, true);
  }
  updateChannels();
  updateStatus();
}

// ---------- schedules view ----------

async function loadSchedules() {
  scheduleData = await api('/api/schedules');
  return scheduleData;
}

function scheduleTotal(schedule) {
  const minutes = schedule.channelDurations.reduce((sum, d) => sum + d, 0);
  return Math.round(minutes * schedule.weight / 100);
}

async function renderSchedules() {
  const view = $('view');
  let data;
  try {
    data = await loadSchedules();
  } catch (e) {
    view.replaceChildren(h('p', { class: 'muted' }, 'Could not load schedules: ' + e.message));
    return;
  }
  if (currentRoute().name !== 'schedules') return;

  const fileInput = h('input', { type: 'file', accept: '.json,application/json', hidden: true, onchange: onImport });
  view.replaceChildren(
    h('div', { class: 'row space' },
      h('h2', {}, 'Schedules'),
      h('a', { href: '#/schedule/new', class: 'btn primary' }, '+ Add')),
    data.schedules.length === 0
      ? h('p', { class: 'muted card' }, 'No schedules yet.')
      : h('ul', { class: 'schedules' }, data.schedules.map((s) => h('li', { class: 'card schedule' },
          h('div', { class: 'sched-time' }, formatTime(s.startTime)),
          h('div', { class: 'sched-meta' },
            data.days[s.daysToRun] + ' · ' + s.weight + '% · ' + scheduleTotal(s) + ' min'),
          s.next && h('div', { class: 'next', title: 'Runs today and the next 3 days' },
            [...s.next].map((c, d) => h('span', { class: c === '!' ? 'dot on' : 'dot' , title: ['Today', 'Tomorrow', '+2 days', '+3 days'][d] }))),
          h('div', { class: 'actions' },
            h('a', { href: '#/schedule/' + s.id, class: 'btn' }, 'Edit'),
            h('button', { type: 'button', class: 'btn danger', onclick: () => onDelete(s) }, 'Delete'))))),
    h('div', { class: 'row end' },
      h('a', { href: '/api/export', class: 'btn', download: 'schedules.json' }, 'Export'),
      h('button', { type: 'button', class: 'btn', onclick: () => fileInput.click() }, 'Import'),
      fileInput)
  );
}

async function onDelete(schedule) {
  if (!await confirmDialog('Delete the ' + formatTime(schedule.startTime) + ' schedule?')) return;
  try {
    await api('/api/schedule/delete', { id: schedule.id });
    toast('Schedule deleted');
  } catch (e) {
    toast(e.message, true);
  }
  renderSchedules();
}

async function onImport(event) {
  const file = event.target.files[0];
  if (!file) return;
  try {
    let parsed;
    try { parsed = JSON.parse(await file.text()); } catch (e) { throw new Error('Not a valid JSON file'); }
    const result = await api('/api/import', parsed);
    toast('Imported ' + result.count + ' schedule(s)');
  } catch (e) {
    toast('Import failed: ' + e.message, true);
  }
  renderSchedules();
}

// ---------- schedule editor ----------

function options(values, selected, label) {
  const list = values.includes(selected) ? values : values.concat([selected]).sort((a, b) => a - b);
  return list.map((v) => h('option', { value: v, selected: v === selected }, label(v)));
}

async function renderEditor(id) {
  const view = $('view');
  try {
    if (!scheduleData) await loadSchedules();
    if (!state.loaded) applyState(await api('/api/state'));
  } catch (e) {
    view.replaceChildren(h('p', { class: 'muted' }, 'Could not load: ' + e.message));
    return;
  }
  const isNew = id === 'new';
  const existing = isNew ? null : scheduleData.schedules.find((s) => s.id === Number(id));
  if (!isNew && !existing) {
    view.replaceChildren(h('p', { class: 'muted card' }, 'Schedule not found.'), h('a', { href: '#/schedules', class: 'btn' }, 'Back'));
    return;
  }
  const s = existing || { id: -1, startTime: 360, daysToRun: 0, weight: 100, channelDurations: new Array(8).fill(0) };
  const hours = Array.from({ length: 24 }, (_, i) => i);
  const weights = [50, 75, 100, 125, 150];

  const totalEl = h('span', { class: 'muted' });
  const updateTotal = () => {
    const form = $('editor');
    const minutes = [...form.querySelectorAll('input[name=duration]')].reduce((sum, el) => sum + (parseInt(el.value, 10) || 0), 0);
    totalEl.textContent = 'Total: ' + Math.round(minutes * Number(form.weight.value) / 100) + ' min';
  };

  view.replaceChildren(
    h('h2', {}, isNew ? 'New schedule' : 'Edit schedule'),
    h('form', { id: 'editor', class: 'card form', onsubmit: onSave, oninput: updateTotal, novalidate: true },
      h('div', { class: 'field' },
        h('label', { for: 'hour' }, 'Start time'),
        h('div', { class: 'row' },
          h('select', { id: 'hour', name: 'hour' }, options(hours, Math.floor(s.startTime / 60), pad)),
          h('span', {}, ':'),
          h('select', { id: 'minute', name: 'minute', 'aria-label': 'Minute' }, options([0, 15, 30, 45], s.startTime % 60, pad)))),
      h('div', { class: 'field' },
        h('label', { for: 'days' }, 'Days to run'),
        h('select', { id: 'days', name: 'days' },
          scheduleData.days.map((d, i) => h('option', { value: i, selected: i === s.daysToRun }, d)))),
      h('div', { class: 'field' },
        h('label', { for: 'weight' }, 'Weight'),
        h('select', { id: 'weight', name: 'weight' }, options(weights, s.weight, (v) => v + '%'))),
      h('fieldset', { class: 'durations' },
        h('legend', {}, 'Minutes per channel'),
        s.channelDurations.map((d, i) => h('label', { class: 'duration-row' },
          h('span', {}, (i + 1) + '. ' + channelName(i)),
          h('input', { type: 'number', name: 'duration', inputmode: 'numeric', min: 0, max: state.durationMax, value: d })))),
      h('div', { class: 'row space' },
        totalEl,
        h('div', { class: 'row' },
          h('a', { href: '#/schedules', class: 'btn' }, 'Cancel'),
          h('button', { type: 'submit', class: 'btn primary' }, 'Save'))))
  );
  $('editor').dataset.id = s.id;
  updateTotal();
}

async function onSave(event) {
  event.preventDefault();
  const form = event.target;
  const durations = [...form.querySelectorAll('input[name=duration]')].map((el) => Number(el.value));
  const bad = durations.findIndex((d) => !Number.isInteger(d) || d < 0 || d > state.durationMax);
  if (bad >= 0) {
    toast('Channel ' + (bad + 1) + ': enter 0–' + state.durationMax + ' minutes', true);
    form.querySelectorAll('input[name=duration]')[bad].focus();
    return;
  }
  const body = {
    id: Number(form.dataset.id),
    startTime: Number(form.hour.value) * 60 + Number(form.minute.value),
    daysToRun: Number(form.days.value),
    weight: Number(form.weight.value),
    channelDurations: durations
  };
  try {
    await api('/api/schedule/save', body);
    toast('Schedule saved');
    location.hash = '#/schedules';
  } catch (e) {
    toast(e.message, true);
  }
}

// ---------- routing ----------

function currentRoute() {
  const parts = location.hash.replace(/^#\/?/, '').split('/');
  if (parts[0] === 'schedules') return { name: 'schedules' };
  if (parts[0] === 'schedule') return { name: 'editor', id: parts[1] || 'new' };
  return { name: 'control' };
}

function render() {
  const route = currentRoute();
  document.querySelectorAll('[data-nav]').forEach((a) => {
    const active = a.dataset.nav === route.name || (route.name === 'editor' && a.dataset.nav === 'schedules');
    if (active) a.setAttribute('aria-current', 'page'); else a.removeAttribute('aria-current');
  });
  if (route.name === 'schedules') renderSchedules();
  else if (route.name === 'editor') renderEditor(route.id);
  else renderControl();
  window.scrollTo(0, 0);
}

// ---------- live updates ----------

function setOnline(value) {
  online = value;
  $('offline').hidden = value;
}

function connectEvents() {
  const source = new EventSource('/events');
  let wasOffline = false;
  source.addEventListener('open', () => {
    if (wasOffline || !online) loadState();
    wasOffline = false;
    setOnline(true);
  });
  source.addEventListener('error', () => {
    wasOffline = true;
    setOnline(false);
    // The browser retries by itself unless the connection was closed for good
    if (source.readyState === EventSource.CLOSED) setTimeout(connectEvents, 5000);
  });
  source.addEventListener('status', (e) => {
    const status = JSON.parse(e.data);
    state.scheduled = status.scheduled;
    setRemaining(status.remaining);
    updateStatus();
  });
  source.addEventListener('channels', (e) => {
    state.channels = [...e.data].map((c) => (c === '1' ? 1 : 0));
    updateChannels();
  });
}

window.addEventListener('hashchange', render);
setInterval(updateCountdown, 1000);
render();
loadState();
connectEvents();
