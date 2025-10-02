import type { WiFiNetwork, WiFiStatus, ProvisioningStatus } from '../types/wifi';

const API_BASE = '/api';

export const api = {
  async scanWiFi(): Promise<WiFiNetwork[]> {
    const res = await fetch(`${API_BASE}/wifi/scan`);
    if (!res.ok) throw new Error('Scan failed');
    return res.json();
  },

  async connectWiFi(ssid: string, password: string): Promise<{ success: boolean }> {
    const res = await fetch(`${API_BASE}/wifi/connect`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ ssid, password }),
    });
    if (!res.ok) throw new Error('Connection failed');
    return res.json();
  },

  async getWiFiStatus(): Promise<WiFiStatus> {
    const res = await fetch(`${API_BASE}/wifi/status`);
    if (!res.ok) throw new Error('Failed to get status');
    return res.json();
  },

  async subscribeProvisioning(): Promise<{ success: boolean }> {
    const res = await fetch(`${API_BASE}/provision/subscribe`, { method: 'POST' });
    if (!res.ok) throw new Error('Subscribe failed');
    return res.json();
  },

  async getProvisioningStatus(): Promise<ProvisioningStatus> {
    const res = await fetch(`${API_BASE}/provision/status`);
    if (!res.ok) throw new Error('Failed to get provisioning status');
    return res.json();
  },
};
