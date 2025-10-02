import type { WiFiNetwork } from '../types/wifi';

interface WiFiScannerProps {
  networks: WiFiNetwork[];
  scanning: boolean;
  onScan: () => void;
  onSelect: (network: WiFiNetwork) => void;
}

export function WiFiScanner({ networks, scanning, onScan, onSelect }: WiFiScannerProps) {
  const getSignalStrength = (rssi: number) => {
    if (rssi >= -50) return 'Excellent';
    if (rssi >= -60) return 'Good';
    if (rssi >= -70) return 'Fair';
    return 'Weak';
  };

  const getEncryptionType = (enc: number) => {
    const types = ['Open', 'WEP', 'WPA', 'WPA2', 'WPA/WPA2', 'WPA2 Enterprise', 'WPA3'];
    return types[enc] || 'Unknown';
  };

  return (
    <div class="w-full max-w-md">
      <div class="flex justify-between items-center mb-4">
        <h2 class="text-xl font-bold">Available Networks</h2>
        <button
          onClick={onScan}
          disabled={scanning}
          class="px-4 py-2 bg-blue-600 text-white rounded-lg disabled:opacity-50 disabled:cursor-not-allowed hover:bg-blue-700 transition"
        >
          {scanning ? 'Scanning...' : 'Scan'}
        </button>
      </div>

      <div class="space-y-2">
        {networks.length === 0 && !scanning && (
          <p class="text-gray-500 text-center py-8">No networks found. Click Scan to search.</p>
        )}

        {networks.map((network) => (
          <button
            key={network.bssid || network.ssid}
            onClick={() => onSelect(network)}
            class="w-full p-4 bg-white border border-gray-200 rounded-lg hover:border-blue-500 hover:shadow transition text-left"
          >
            <div class="flex justify-between items-start">
              <div class="flex-1">
                <div class="font-semibold text-gray-900">{network.ssid}</div>
                <div class="text-sm text-gray-500 mt-1">
                  {getEncryptionType(network.encryption)}
                </div>
              </div>
              <div class="text-right">
                <div class="text-sm font-medium text-gray-700">
                  {getSignalStrength(network.rssi)}
                </div>
                <div class="text-xs text-gray-500 mt-1">{network.rssi} dBm</div>
              </div>
            </div>
          </button>
        ))}
      </div>
    </div>
  );
}
