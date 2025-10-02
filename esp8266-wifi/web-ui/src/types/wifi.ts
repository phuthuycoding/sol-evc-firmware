export interface WiFiNetwork {
  ssid: string;
  rssi: number;
  encryption: number;
  bssid?: string;
}

export interface WiFiStatus {
  connected: boolean;
  ssid?: string;
  ip?: string;
  rssi?: number;
}

export interface ProvisioningStatus {
  provisioned: boolean;
  mqttUsername?: string;
  mqttPassword?: string;
  mqttBroker?: string;
}

export enum ProvisioningState {
  IDLE = 'idle',
  SCANNING = 'scanning',
  CONNECTING = 'connecting',
  WAITING_PROVISION = 'waiting_provision',
  PROVISIONED = 'provisioned',
  ERROR = 'error',
}
