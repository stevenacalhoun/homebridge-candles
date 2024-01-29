import { Service, CharacteristicValue } from 'homebridge';
import type { PlatformAccessory } from 'homebridge';

import { ESP8266Platform } from './esp8266Platform';

export class ESP8266LightAccessory {
  private service: Service;
  private onState = false;
  private lastButtonPushTime = 0;
  private lastFetchTime = Date.now();

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
      let currentLightStatus = await this.getOn() as boolean;

      const requestLightStatus = req.query.currentLightStatus === 'true';
      const requestButtonPushTime = parseInt(req.query.lastButtonPushTime);

      this.platform.log.debug('GET %s', req.originalUrl);

      if (currentLightStatus !== requestLightStatus) {
        // The lights have been more recently set via the button, update status
        // on the homebridge side
        if (this.lastButtonPushTime !== requestButtonPushTime) {
          this.platform.log.info('Button used on %s updating homebridge status to %s', this.accessory.UUID, requestLightStatus ? 'on' : 'off');

          currentLightStatus = requestLightStatus;
          this.setOn(currentLightStatus);
          this.lastButtonPushTime = requestButtonPushTime;
        } else {
          this.platform.log.info('Setting %s to %s', this.accessory.UUID, currentLightStatus ? 'on' : 'off');
        }
      }

      // Send new status
      res.send(currentLightStatus);
      this.lastFetchTime = Date.now();
    });
  }

  async setOn(value: CharacteristicValue) {
    this.onState = value as boolean;
  }

  async getOn(): Promise<CharacteristicValue> {
    return this.onState;
  }
}
