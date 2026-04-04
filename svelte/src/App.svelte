<script>
  import { onMount } from 'svelte';
  import { writable } from 'svelte/store';
  import { Home, Wifi, Radio, Settings, RefreshCw, Thermometer, Wind, Sun, Activity, Droplets } from 'lucide-svelte';

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

  async function fetchData(endpoint, store) {
    try {
      const res = await fetch(`${API_BASE}${endpoint}`);
      if (!res.ok) throw new Error(`Failed to fetch ${endpoint}`);
      const data = await res.json();
      store.set(data);
    } catch (err) {
      console.error(err);
      if (endpoint === '/status') error = err.message;
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
    <div class="brand">Room133</div>
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
    <div class="spacer"></div>
    <button on:click={restart} class="danger">
      <RefreshCw size={20} /> Restart
    </button>
  </nav>

  <main class="content">
    {#if loading}
      <div class="loading">Loading...</div>
    {:else if error}
      <div class="error">{error}</div>
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
              <div class="stat" class:active={$status.motion}>
                <Activity /> {$status.motion ? 'Motion Detected' : 'Clear'}
              </div>
            </div>
          {/if}

          {#if $status.features.haveMotion}
            <div class="card">
              <h3>Motion 2</h3>
              <div class="stat" class:active={$status.motion2}>
                <Activity /> {$status.motion2 ? 'Motion Detected' : 'Clear'}
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
          <button on:click={() => sendConfig('/wifi', $wifi)}>Save</button>
        </div>
      {:else if currentPage === 'mqtt'}
        <div class="card form">
          <h3>MQTT Setup</h3>
          <label><input type="checkbox" bind:checked={$mqtt.enabled} /> Enabled</label>
          <label>Broker <input bind:value={$mqtt.hostAddr} /></label>
          <label>User <input bind:value={$mqtt.authUser} /></label>
          <label>Password <input type="password" bind:value={$mqtt.authPassword} /></label>
          <button on:click={() => sendConfig('/mqtt', $mqtt)}>Save</button>
        </div>
      {:else if currentPage === 'config'}
        <div class="card form">
          <h3>Parameters</h3>
          {#each Object.keys($config) as key}
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
  .sidebar { width: 240px; background: #1f2937; color: white; display: flex; flex-direction: column; padding: 1rem; gap: 0.5rem; }
  .brand { font-size: 1.5rem; font-weight: bold; margin-bottom: 2rem; text-align: center; }
  .sidebar button { 
    display: flex; align-items: center; gap: 0.75rem; width: 100%; padding: 0.75rem; 
    background: transparent; border: none; color: #9ca3af; text-align: left; border-radius: 0.5rem;
  }
  .sidebar button:hover { background: #374151; color: white; }
  .sidebar button.active { background: #059669; color: white; }
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
  .form button { background: #059669; color: white; border: none; padding: 0.75rem 1.5rem; border-radius: 0.5rem; margin-top: 1rem; }
</style>
