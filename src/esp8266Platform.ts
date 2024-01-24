import express from 'express';

import { API, DynamicPlatformPlugin, Logger, PlatformAccessory, PlatformConfig, Service, Characteristic } from 'homebridge';

import { PLATFORM_NAME, PLUGIN_NAME } from './settings';
import { ESP8266LightAccessory } from './esp8266Acessory';

export class ESP8266Platform implements DynamicPlatformPlugin {
  public readonly Service: typeof Service = this.api.hap.Service;
  public readonly Characteristic: typeof Characteristic = this.api.hap.Characteristic;

  public readonly accessories: PlatformAccessory[] = [];

  public server;

  constructor(
    public readonly log: Logger,
    public readonly config: PlatformConfig,
    public readonly api: API,
  ) {
    this.log.debug('Finished initializing platform:', this.config.name);

    this.api.on('didFinishLaunching', () => {
      log.debug('Executed didFinishLaunching callback');

      this.createHttpService();
      this.discoverDevices();
    });
  }

  configureAccessory(accessory: PlatformAccessory) {
    this.log.info('Loading accessory from cache:', accessory.displayName);

    this.accessories.push(accessory);
  }


  discoverDevices() {
    for (const device of this.config.accessories) {
      const uuid = this.api.hap.uuid.generate(device.mac);
      const existingAccessory = this.accessories.find(accessory => accessory.UUID === uuid);

      if (existingAccessory) {
        this.log.info('Restoring existing accessory from cache:', existingAccessory.displayName);
        new ESP8266LightAccessory(this, existingAccessory);
      } else {
        this.addAccessory(device);
      }
    }
  }

  async createHttpService() {
    this.server = express();
    this.server.listen(18081, () => this.log.info('Http server listening on 18081...'));

    this.server.get('/id/:mac', (req, res) => {
      const device = this.config.accessories.find(accessory => accessory.mac === req.params.mac);
      if (device) {
        const uuid = this.api.hap.uuid.generate(device.mac);
        res.send(uuid);
      } else {
        res.status(404).send('Not found');
      }
    });
  }

  addAccessory(device) {
    const uuid = this.api.hap.uuid.generate(device.mac);
    this.log.info('Adding accessory mac %s (%s)', device.mac, uuid);
    if (device.accessoryType === 'basic_light') {
      const accessory = new this.api.platformAccessory(device.displayName, uuid);

      accessory.context.device = device;

      new ESP8266LightAccessory(this, accessory);

      this.api.registerPlatformAccessories(PLUGIN_NAME, PLATFORM_NAME, [accessory]);
    } else {
      this.log.error('Unexpected accessory type %s', device.accessoryType);
    }
    return uuid;
  }
}
