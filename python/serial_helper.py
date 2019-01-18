#!/usr/bin/env python3
import serial
import serial.tools.list_ports
import sys

device_vid = 0x0B07
device_pid = 0x07A5

def list_serial_ports():
    return serial.tools.list_ports.comports()

def select_serial_port(ports=None):
    if ports == None:
        ports = list_serial_ports()
    if len(ports) == 0:
        raise RuntimeError('No serial port available')
    elif len(ports) == 1:
        port = ports[0]
    else:
        # p = 0
        # for port in ports:
            # print(p, port)
            # p += 1
        # while True:
            # try:
                # choice = int(input('Select a port: '))
                # if choice in list(range(len(ports))):
                    # port = ports[choice]
                    # break
            # except KeyboardInterrupt:
                # raise
            # except:
                # continue
        devs = []
        for dev in ports:
            if dev.vid == device_vid and dev.pid == device_pid:
                devs.append(dev)
        if len(devs)) == 1:
            port = devs[0]
        else:
            p = 0
            for port in ports:
                print(p, port)
                p += 1
            while True:
                try:
                    choice = int(input('Select a port: '))
                    if choice in list(range(len(ports))):
                        port = ports[choice]
                        break
                except KeyboardInterrupt:
                    raise
                except:
                    continue
    return port.device

def main():
    ports = list_serial_ports()
    port = select_serial_port(ports)
    print('Found:', port)
    ser = serial.Serial(port)
    ser.close()

if __name__ == '__main__':
    main()