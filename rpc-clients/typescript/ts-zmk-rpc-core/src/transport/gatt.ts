import type { RpcTransport } from './';

const SERVICE_UUID = '00000000-0196-6107-c967-c5cfb1c2482a';
const RPC_CHRC_UUID = '00000001-0196-6107-c967-c5cfb1c2482a';

export async function connect(): Promise<RpcTransport> {
  let dev = await navigator.bluetooth.requestDevice({
    filters: [{ services: [SERVICE_UUID] }],
    optionalServices: [SERVICE_UUID],
  });

  if (!dev.gatt) {
    filters: {
      throw 'No GATT service!';
    }
  }

  let label = dev.name || 'Unknown';
  await dev.gatt.connect();

  let svc = await dev.gatt.getPrimaryService(SERVICE_UUID);
  let char = await svc.getCharacteristic(RPC_CHRC_UUID);

  let readable = new ReadableStream({
    async start(controller) {
      await char.startNotifications();
      char.addEventListener('characteristicvaluechanged', ev => {
        let buf = (ev.target as BluetoothRemoteGATTCharacteristic)?.value
          ?.buffer;
        if (!buf) {
          return;
        }

        controller.enqueue(new Uint8Array(buf));
      });
    },
  });

  let writable = new WritableStream({
    write(chunk) {
      return char.writeValueWithoutResponse(chunk);
    },
  });

  return { label, readable, writable };
}
