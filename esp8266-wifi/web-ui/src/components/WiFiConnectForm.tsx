import { useState } from 'preact/hooks';
import type { WiFiNetwork } from '../types/wifi';

interface WiFiConnectFormProps {
  network: WiFiNetwork;
  connecting: boolean;
  onConnect: (ssid: string, password: string) => void;
  onCancel: () => void;
}

export function WiFiConnectForm({ network, connecting, onConnect, onCancel }: WiFiConnectFormProps) {
  const [password, setPassword] = useState('');
  const [showPassword, setShowPassword] = useState(false);

  const handleSubmit = (e: Event) => {
    e.preventDefault();
    if (password.trim()) {
      onConnect(network.ssid, password);
    }
  };

  const requiresPassword = network.encryption !== 0;

  return (
    <div class="w-full max-w-md bg-white border border-gray-200 rounded-lg p-6">
      <h3 class="text-lg font-bold mb-4">Connect to WiFi</h3>

      <div class="mb-4">
        <label class="block text-sm font-medium text-gray-700 mb-1">Network</label>
        <div class="p-3 bg-gray-50 rounded border border-gray-200">
          <div class="font-semibold">{network.ssid}</div>
          <div class="text-sm text-gray-500 mt-1">
            Signal: {network.rssi} dBm
          </div>
        </div>
      </div>

      <form onSubmit={handleSubmit}>
        {requiresPassword && (
          <div class="mb-4">
            <label class="block text-sm font-medium text-gray-700 mb-1">Password</label>
            <div class="relative">
              <input
                type={showPassword ? 'text' : 'password'}
                value={password}
                onInput={(e) => setPassword((e.target as HTMLInputElement).value)}
                class="w-full px-3 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-blue-500 focus:border-blue-500 pr-10"
                placeholder="Enter WiFi password"
                required
                disabled={connecting}
              />
              <button
                type="button"
                onClick={() => setShowPassword(!showPassword)}
                class="absolute right-2 top-1/2 -translate-y-1/2 text-gray-500 hover:text-gray-700"
              >
                {showPassword ? '🙈' : '👁️'}
              </button>
            </div>
          </div>
        )}

        <div class="flex gap-3">
          <button
            type="button"
            onClick={onCancel}
            disabled={connecting}
            class="flex-1 px-4 py-2 border border-gray-300 rounded-lg hover:bg-gray-50 disabled:opacity-50 transition"
          >
            Cancel
          </button>
          <button
            type="submit"
            disabled={connecting || (requiresPassword && !password.trim())}
            class="flex-1 px-4 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 disabled:opacity-50 disabled:cursor-not-allowed transition"
          >
            {connecting ? 'Connecting...' : 'Connect'}
          </button>
        </div>
      </form>
    </div>
  );
}
