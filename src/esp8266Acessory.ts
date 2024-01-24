import { Service, CharacteristicValue } from 'homebridge';
import type { PlatformAccessory } from 'homebridge';

import { ESP8266Platform } from './esp8266Platform';

export class ESP8266LightAccessory {
  private service: Service;
  private onState = false;
  private lastSet = Date.now();
  private lastPull = 0;

  constructor(
    private readonly platform: ESP8266Platform,
    private readonly accessory: PlatformAccessory,
  ) {
    this.accessory.getService(this.platform.Service.AccessoryInformation)!
      .setCharacteristic(this.platform.Characteristic.Manufacturer, 'Folar-Co')
      .setCharacteristic(this.platform.Characteristic.Model, 'Candles')
      .setCharacteristic(this.platform.Characteristic.SerialNumber, '1');

    this.service = this.accessory.getService(this.platform.Service.Lightbulb) || this.accessory.addService(this.platform.Service.Lightbulb);

    this.service.setCharacteristic(this.platform.Characteristic.Name, accessory.context.device.displayName);
    this.service.getCharacteristic(this.platform.Characteristic.On)
      .onSet(this.setOn.bind(this))
      .onGet(this.getOn.bind(this));

    this.setupRoutes();
  }

  async setupRoutes() {
    const route = '/status/' + this.accessory.UUID;
    this.platform.log.info('Setting up route: %s', route);
    this.platform.server.get(route, async (req, res) => {
      const data = await this.getOn() as boolean;
      this.platform.log.info('%s requesting status: %s', this.accessory.UUID, data);
      this.lastPull = Date.now();
      res.send(data);
    });
  }

  async setOn(value: CharacteristicValue) {
    this.onState = value as boolean;
    this.lastSet = Date.now();
  }

  async getOn(): Promise<CharacteristicValue> {
    return this.onState;
  }
}
