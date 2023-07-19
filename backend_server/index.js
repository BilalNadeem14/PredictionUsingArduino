const http = require('http');
const WebSocket = require('ws');
const noble = require('@abandonware/noble')

const server = http.createServer();
const wss = new WebSocket.Server({ server });

const jsonServiceUuid = '19b10000e8f2537e4f6cd104768a1214';
const jsonCharacteristicUuid = '19b10001e8f2537e4f6cd104768a1214';

wss.on('connection', (ws) => {
    console.log('WebSocket client connected');

    ws.on('message', (message) => {
        console.log(`Received message: ${message}`);
    });

    ws.on('close', () => {
        console.log('WebSocket client disconnected');
    });
});

server.listen(8080, () => {
    console.log('WebSocket server started on port 8080');
});

noble.on('stateChange', (state) => {
    if (state === 'poweredOn') {
        noble.startScanning([jsonServiceUuid]);
    } else {
        noble.stopScanning();
    }
});

noble.on('discover', (peripheral) => {
    noble.stopScanning();

    console.log(`Connecting to '${peripheral.advertisement.localName}'...`);

    peripheral.connect((error) => {
        if (error) {
            console.error('Error connecting to peripheral:', error);
            return;
        }

        console.log(`Connected to '${peripheral.advertisement.localName}'`);

        peripheral.discoverServices([jsonServiceUuid], (error, services) => {
            if (error) {
                console.error('Error discovering services:', error);
                return;
            }

            const jsonService = services[0];

            jsonService.discoverCharacteristics([jsonCharacteristicUuid], (error, characteristics) => {
                if (error) {
                    console.error('Error discovering characteristics:', error);
                    return;
                }

                const jsonCharacteristic = characteristics[0];

                jsonCharacteristic.subscribe((error) => {
                    if (error) {
                        console.error('Error subscribing to characteristic:', error);
                        return;
                    }

                    console.log('Subscribed to JSON characteristic notifications');
                });

                jsonCharacteristic.on('data', (data, isNotification) => {
                    const jsonData = data.toString();
                    console.log(`Received JSON data: ${jsonData}`);

                    wss.clients.forEach((client) => {
                        if (client.readyState === WebSocket.OPEN) {
                            client.send(jsonData);
                        }
                    });
                });
            });
        });
    });

    peripheral.once('disconnect', () => {
        console.log(`Disconnected from '${peripheral.advertisement.localName}'`);
        noble.startScanning([jsonServiceUuid]);
    });
});
