import { ProvisioningState } from '../types/wifi';
import type { ProvisioningStatus } from '../types/wifi';

interface ProvisioningFlowProps {
  state: ProvisioningState;
  status: ProvisioningStatus;
  error: string | null;
  onStart: () => void;
}

export function ProvisioningFlow({ state, status, error, onStart }: ProvisioningFlowProps) {
  return (
    <div class="w-full max-w-md bg-white border border-gray-200 rounded-lg p-6">
      <h3 class="text-lg font-bold mb-4">Device Provisioning</h3>

      {state === ProvisioningState.IDLE && (
        <div class="text-center">
          <p class="text-gray-600 mb-4">
            Connect to server to receive MQTT credentials
          </p>
          <button
            onClick={onStart}
            class="px-6 py-2 bg-green-600 text-white rounded-lg hover:bg-green-700 transition"
          >
            Start Provisioning
          </button>
        </div>
      )}

      {state === ProvisioningState.WAITING_PROVISION && (
        <div class="text-center">
          <div class="inline-block animate-spin rounded-full h-12 w-12 border-b-2 border-blue-600 mb-4"></div>
          <p class="text-gray-600">
            Waiting for server to send credentials...
          </p>
          <p class="text-sm text-gray-500 mt-2">
            Subscribed to provision topic
          </p>
        </div>
      )}

      {state === ProvisioningState.PROVISIONED && (
        <div class="space-y-3">
          <div class="flex items-center justify-center text-green-600 mb-2">
            <svg class="w-16 h-16" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M5 13l4 4L19 7"></path>
            </svg>
          </div>
          <p class="text-center text-green-600 font-semibold mb-4">
            Provisioning Complete!
          </p>

          <div class="bg-gray-50 rounded-lg p-3 space-y-2">
            <div>
              <label class="text-xs font-medium text-gray-500">MQTT Broker</label>
              <div class="text-sm font-mono text-gray-900">{status.mqttBroker}</div>
            </div>
            <div>
              <label class="text-xs font-medium text-gray-500">Username</label>
              <div class="text-sm font-mono text-gray-900">{status.mqttUsername}</div>
            </div>
            <div>
              <label class="text-xs font-medium text-gray-500">Password</label>
              <div class="text-sm font-mono text-gray-900">{'•'.repeat(status.mqttPassword?.length || 0)}</div>
            </div>
          </div>

          <p class="text-sm text-gray-500 text-center mt-4">
            Device will restart to apply configuration
          </p>
        </div>
      )}

      {state === ProvisioningState.ERROR && (
        <div class="text-center">
          <div class="text-red-600 mb-4">
            <svg class="w-16 h-16 mx-auto" fill="none" stroke="currentColor" viewBox="0 0 24 24">
              <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M6 18L18 6M6 6l12 12"></path>
            </svg>
          </div>
          <p class="text-red-600 font-semibold mb-2">Provisioning Failed</p>
          <p class="text-sm text-gray-600 mb-4">{error}</p>
          <button
            onClick={onStart}
            class="px-6 py-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 transition"
          >
            Retry
          </button>
        </div>
      )}
    </div>
  );
}
