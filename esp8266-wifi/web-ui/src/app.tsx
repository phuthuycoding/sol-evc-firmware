import { useState, useEffect } from 'preact/hooks';
import { WiFiScanner } from './components/WiFiScanner';
import { WiFiConnectForm } from './components/WiFiConnectForm';
import { ProvisioningFlow } from './components/ProvisioningFlow';
import { useWiFi } from './hooks/useWiFi';
import { useProvisioning } from './hooks/useProvisioning';
import type { WiFiNetwork } from './types/wifi';
import { ProvisioningState } from './types/wifi';

export function App() {
  const { networks, status, scanning, connecting, error: wifiError, scan, connect } = useWiFi();
  const { state, status: provStatus, error: provError, startProvisioning } = useProvisioning(status.connected);
  const [selectedNetwork, setSelectedNetwork] = useState<WiFiNetwork | null>(null);

  useEffect(() => {
    scan();
  }, []);

  useEffect(() => {
    if (status.connected && state === ProvisioningState.IDLE) {
      startProvisioning();
    }
  }, [status.connected]);

  const handleConnect = async (ssid: string, password: string) => {
    await connect(ssid, password);
    setSelectedNetwork(null);
  };

  return (
    <div class="min-h-screen bg-gray-50 py-8 px-4">
      <div class="max-w-4xl mx-auto">
        <header class="text-center mb-8">
          <h1 class="text-3xl font-bold text-gray-900 mb-2">SolEVC Setup</h1>
          <p class="text-gray-600">Configure your charging station</p>
        </header>

        {status.connected && (
          <div class="mb-6 p-4 bg-green-50 border border-green-200 rounded-lg">
            <div class="flex items-center gap-2 text-green-700">
              <svg class="w-5 h-5" fill="currentColor" viewBox="0 0 20 20">
                <path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.707-9.293a1 1 0 00-1.414-1.414L9 10.586 7.707 9.293a1 1 0 00-1.414 1.414l2 2a1 1 0 001.414 0l4-4z" clip-rule="evenodd"></path>
              </svg>
              <span class="font-semibold">Connected to {status.ssid}</span>
            </div>
            <div class="text-sm text-green-600 mt-1">
              IP: {status.ip} | Signal: {status.rssi} dBm
            </div>
          </div>
        )}

        {(wifiError || provError) && (
          <div class="mb-6 p-4 bg-red-50 border border-red-200 rounded-lg text-red-700">
            {wifiError || provError}
          </div>
        )}

        <div class="flex flex-col items-center gap-6">
          {!status.connected && !selectedNetwork && (
            <WiFiScanner
              networks={networks}
              scanning={scanning}
              onScan={scan}
              onSelect={setSelectedNetwork}
            />
          )}

          {!status.connected && selectedNetwork && (
            <WiFiConnectForm
              network={selectedNetwork}
              connecting={connecting}
              onConnect={handleConnect}
              onCancel={() => setSelectedNetwork(null)}
            />
          )}

          {status.connected && (
            <ProvisioningFlow
              state={state}
              status={provStatus}
              error={provError}
              onStart={startProvisioning}
            />
          )}
        </div>

        <footer class="text-center mt-12 text-sm text-gray-500">
          <p>SolEVC Charging Station © 2025</p>
        </footer>
      </div>
    </div>
  );
}
