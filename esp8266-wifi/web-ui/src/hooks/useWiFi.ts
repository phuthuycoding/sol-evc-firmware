import { useState, useEffect } from 'preact/hooks';
import type { WiFiNetwork, WiFiStatus } from '../types/wifi';
import { api } from '../services/api';

export function useWiFi() {
  const [networks, setNetworks] = useState<WiFiNetwork[]>([]);
  const [status, setStatus] = useState<WiFiStatus>({ connected: false });
  const [scanning, setScanning] = useState(false);
  const [connecting, setConnecting] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const scan = async () => {
    setScanning(true);
    setError(null);
    try {
      const data = await api.scanWiFi();
      setNetworks(data.sort((a, b) => b.rssi - a.rssi));
    } catch (err) {
      setError((err as Error).message);
    } finally {
      setScanning(false);
    }
  };

  const connect = async (ssid: string, password: string) => {
    setConnecting(true);
    setError(null);
    try {
      await api.connectWiFi(ssid, password);
      await checkStatus();
    } catch (err) {
      setError((err as Error).message);
    } finally {
      setConnecting(false);
    }
  };

  const checkStatus = async () => {
    try {
      const data = await api.getWiFiStatus();
      setStatus(data);
    } catch (err) {
      setError((err as Error).message);
    }
  };

  useEffect(() => {
    checkStatus();
    const interval = setInterval(checkStatus, 5000);
    return () => clearInterval(interval);
  }, []);

  return { networks, status, scanning, connecting, error, scan, connect };
}
