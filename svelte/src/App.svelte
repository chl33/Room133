<script>
  import { onMount } from 'svelte';
  import { writable } from 'svelte/store';
  import { Home, Wifi, Radio, Settings, RefreshCw, Thermometer, Wind, Sun, Activity, Droplets, CheckCircle2, XCircle } from 'lucide-svelte';

  const API_BASE = '/api';

  let currentPage = 'home';
  let loading = true;
  let error = null;

  const status = writable({
    temperature: 0,
    humidity: 0,
    light: 0,
    motion: false,
    motion2: false,
    leak: 0,
    leak_filtered: 0,
    mqttConnected: false,
    software: '',
    hardware: '',
    board: '',
    features: {
      haveMotionLight: false,
      haveMotion: false,
      haveLeak: false,
      haveOled: false,
      boardV10: false
    }
  });

  const wifi = writable({ essId: '', board: '', wifiPassword: '' });
  const mqtt = writable({ hostAddr: '', port: 1883, authUser: '', authPassword: '', enabled: false });
  const config = writable({});
  const isOnline = writable(true);

  async function fetchData(endpoint, store) {
    try {
      const res = await fetch(`${API_BASE}${endpoint}`);
      if (!res.ok) throw new Error(`Failed to fetch ${endpoint}`);
      const data = await res.json();
      store.set(data);
      if (endpoint === '/status') isOnline.set(true);
    } catch (err) {
      console.error(err);
      if (endpoint === '/status') {
        error = err.message;
        isOnline.set(false);
      }
    }
  }

  async function loadAll() {
    loading = true;
    await Promise.all([
      fetchData('/status', status),
      fetchData('/wifi', wifi),
      fetchData('/mqtt', mqtt),
      fetchData('/config', config)
    ]);
    loading = false;
  }

  onMount(() => {
    loadAll();
    const interval = setInterval(() => fetchData('/status', status), 5000);
    return () => clearInterval(interval);
  });

  async function sendConfig(endpoint, data) {
    try {
      const res = await fetch(`${API_BASE}${endpoint}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
      });
      if (!res.ok) throw new Error('Update failed');
      alert('Settings saved');
    } catch (err) {
      alert(err.message);
    }
  }

  async function restart() {
    if (confirm('Restart device?')) fetch(`${API_BASE}/restart`, { method: 'POST' });
  }
</script>

<div class="app">
  <nav class="sidebar">
    <div class="brand">
      Room133
      {#if $status.board}
        <div class="hostname">{$status.board}</div>
      {/if}
    </div>

    <div class="connection-status">
      <div class="status-item" class:online={$isOnline}>
        {#if $isOnline}
          <CheckCircle2 size={14} /> <span>Device Online</span>
        {:else}
          <XCircle size={14} /> <span>Device Offline</span>
        {/if}
      </div>
      <div class="status-item" class:online={$status.mqttConnected}>
        {#if $status.mqttConnected}
          <CheckCircle2 size={14} /> <span>MQTT Connected</span>
        {:else}
          <XCircle size={14} /> <span>MQTT Offline</span>
        {/if}
      </div>
    </div>

    <div class="nav-links">
      <button class:active={currentPage === 'home'} on:click={() => currentPage = 'home'}>
        <Home size={20} /> Overview
      </button>
      <button class:active={currentPage === 'wifi'} on:click={() => currentPage = 'wifi'}>
        <Wifi size={20} /> WiFi
      </button>
      <button class:active={currentPage === 'mqtt'} on:click={() => currentPage = 'mqtt'}>
        <Radio size={20} /> MQTT
      </button>
      <button class:active={currentPage === 'config'} on:click={() => currentPage = 'config'}>
        <Settings size={20} /> Settings
      </button>
    </div>

    <div class="spacer"></div>

    <div class="version-info">
      <div>SW: {$status.software || 'v0.0.0'}</div>
      <div>HW: {$status.hardware || 'Room133'}</div>
    </div>

    <button on:click={restart} class="danger-btn">
      <RefreshCw size={20} /> Restart
    </button>
  </nav>

  <main class="content">
    {#if loading}
      <div class="loading">Loading...</div>
    {:else if error && !$isOnline}
      <div class="error">
        <h2>Connection Lost</h2>
        <p>Attempting to reconnect to {$status.hardware || 'device'}...</p>
      </div>
    {:else}
      {#if currentPage === 'home'}
        <div class="grid">
          <div class="card">
            <h3>Climate</h3>
            <div class="stat"><Thermometer /> {$status.temperature.toFixed(1)}°C</div>
            <div class="stat"><Wind /> {$status.humidity.toFixed(1)}% RH</div>
          </div>

          {#if $status.features.haveMotionLight}
            <div class="card">
              <h3>Light & Motion</h3>
              <div class="stat"><Sun /> {$status.light.toFixed(1)}% Light</div>
              <div class="stat" class:active={$status.motion === 'ON'}>
                <Activity /> {$status.motion === 'ON' ? 'Motion Detected' : 'Clear'}
              </div>
            </div>
          {/if}

          {#if $status.features.haveMotion}
            <div class="card">
              <h3>Motion 2</h3>
              <div class="stat" class:active={$status.motion2 === 'ON'}>
                <Activity /> {$status.motion2 === 'ON' ? 'Motion Detected' : 'Clear'}
              </div>
            </div>
          {/if}

          {#if $status.features.haveLeak}
            <div class="card">
              <h3>Leak Sensor</h3>
              <div class="stat" class:warning={$status.leak_filtered > 10}>
                <Droplets /> {$status.leak_filtered.toFixed(1)}%
              </div>
            </div>
          {/if}
        </div>
      {:else if currentPage === 'wifi'}
        <div class="card form">
          <h3>WiFi Setup</h3>
          <label>SSID <input bind:value={$wifi.essId} /></label>
          <label>Password <input type="password" bind:value={$wifi.wifiPassword} /></label>
          <label>Hostname <input bind:value={$wifi.board} /></label>
          <button on:click={() => sendConfig('/wifi', $wifi)}>Save & Reboot</button>
        </div>
      {:else if currentPage === 'mqtt'}
        <div class="card form">
          <h3>MQTT Setup</h3>
          <label class="checkbox-label">
            <input type="checkbox" bind:checked={$mqtt.enabled} />
            <span>Enable MQTT Updates</span>
          </label>
          <label>Broker <input bind:value={$mqtt.hostAddr} /></label>
          <label>User <input bind:value={$mqtt.authUser} /></label>
          <label>Password <input type="password" bind:value={$mqtt.authPassword} /></label>
          <button on:click={() => sendConfig('/mqtt', $mqtt)}>Save</button>
        </div>
      {:else if currentPage === 'config'}
        <div class="card form">
          <h3>Parameters</h3>
          {#each Object.keys($config).filter(k => !k.endsWith('OutMin') && !k.endsWith('OutMax')) as key}
            <label>{key} <input bind:value={$config[key]} /></label>
          {/each}
          <button on:click={() => sendConfig('/config', $config)}>Save</button>
        </div>
      {/if}
    {/if}
  </main>
</div>

<style>
  .app { display: flex; height: 100vh; width: 100vw; background: #f0f2f5; color: #1a1a1a; }

  .sidebar {
    width: 240px; background: #1f2937; color: white; display: flex;
    flex-direction: column; padding: 1rem; gap: 1rem;
  }

  .brand { font-size: 1.5rem; font-weight: bold; text-align: center; margin-bottom: 0.5rem; }
  .hostname { font-size: 0.875rem; color: #10b981; font-weight: normal; margin-top: 0.25rem; }

  .connection-status {
    background: #111827;
    padding: 0.75rem;
    border-radius: 0.5rem;
    font-size: 0.75rem;
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
  }

  .status-item { display: flex; align-items: center; gap: 0.5rem; color: #9ca3af; }
  .status-item.online { color: #10b981; }

  .nav-links { display: flex; flex-direction: column; gap: 0.5rem; }

  .sidebar button {
    display: flex; align-items: center; gap: 0.75rem; width: 100%; padding: 0.75rem;
    background: transparent; border: none; color: #9ca3af; text-align: left; border-radius: 0.5rem;
    cursor: pointer; transition: all 0.2s;
  }

  .sidebar button:hover { background: #374151; color: white; }
  .sidebar button.active { background: #059669; color: white; }

  .danger-btn { color: #f87171 !important; }
  .danger-btn:hover { background: #7f1d1d !important; color: white !important; }

  .version-info {
    font-size: 0.7rem;
    color: #6b7280;
    padding: 0.5rem;
    border-top: 1px solid #374151;
  }

  .spacer { flex: 1; }

  .content { flex: 1; padding: 2rem; overflow-y: auto; }

  .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 1.5rem; }

  .card { background: white; padding: 1.5rem; border-radius: 0.75rem; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
  .card h3 { margin-top: 0; color: #374151; border-bottom: 1px solid #eee; padding-bottom: 0.5rem; }

  .stat { display: flex; align-items: center; gap: 1rem; font-size: 1.25rem; margin: 1rem 0; color: #4b5563; }
  .stat.active { color: #059669; font-weight: bold; }
  .stat.warning { color: #dc2626; font-weight: bold; }

  .form label { display: block; margin-bottom: 1rem; color: #4b5563; }
  .form input { width: 100%; padding: 0.5rem; border: 1px solid #d1d5db; border-radius: 0.25rem; margin-top: 0.25rem; }

  .checkbox-label {
    display: flex !important;
    flex-direction: row !important;
    align-items: center;
    gap: 0.75rem;
    cursor: pointer;
  }

  .checkbox-label input { width: auto; margin: 0; }

  .form button {
    background: #059669; color: white; border: none; padding: 0.75rem 1.5rem;
    border-radius: 0.5rem; margin-top: 1rem; width: 100%; font-weight: bold; cursor: pointer;
  }

  .loading, .error {
    display: flex; flex-direction: column; align-items: center;
    justify-content: center; height: 60vh; color: #6b7280;
  }
</style>
