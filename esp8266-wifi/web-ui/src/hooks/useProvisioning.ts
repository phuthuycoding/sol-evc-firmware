import { useState, useEffect } from 'preact/hooks';
import type { ProvisioningStatus } from '../types/wifi';
import { ProvisioningState } from '../types/wifi';
import { api } from '../services/api';

export function useProvisioning(wifiConnected: boolean) {
  const [state, setState] = useState<ProvisioningState>(ProvisioningState.IDLE);
  const [status, setStatus] = useState<ProvisioningStatus>({ provisioned: false });
  const [error, setError] = useState<string | null>(null);

  const startProvisioning = async () => {
    if (!wifiConnected) {
      setError('WiFi not connected');
      return;
    }

    setState(ProvisioningState.WAITING_PROVISION);
    setError(null);

    try {
      await api.subscribeProvisioning();

      const checkInterval = setInterval(async () => {
        try {
          const data = await api.getProvisioningStatus();
          setStatus(data);

          if (data.provisioned) {
            setState(ProvisioningState.PROVISIONED);
            clearInterval(checkInterval);
          }
        } catch (err) {
          setError((err as Error).message);
          setState(ProvisioningState.ERROR);
          clearInterval(checkInterval);
        }
      }, 2000);

      setTimeout(() => {
        clearInterval(checkInterval);
        if (state === ProvisioningState.WAITING_PROVISION) {
          setState(ProvisioningState.ERROR);
          setError('Provisioning timeout');
        }
      }, 60000);
    } catch (err) {
      setError((err as Error).message);
      setState(ProvisioningState.ERROR);
    }
  };

  useEffect(() => {
    if (wifiConnected && state === ProvisioningState.IDLE) {
      setState(ProvisioningState.IDLE);
    }
  }, [wifiConnected]);

  return { state, status, error, startProvisioning };
}
