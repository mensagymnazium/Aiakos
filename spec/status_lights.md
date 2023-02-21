# Status lights

The card reader panel contains three colored indicator lights.

## Ready (blue)

The blue light is active whenever the device is ready to accept cards.

| Status   | Meaning                                                       |
| -------- | ------------------------------------------------------------- |
| Off      | The device is busy and cannot temporarily process cards.      |
| Blinking | The card list is empty. Only the master key will be accepted. |
| On       | The device is ready to process cards.                         |

## Fail (red)

The red light is used to signalize various states that prevent normal operation.

| Status   | Meaning                                                    |
| -------- | ---------------------------------------------------------- |
| Off      | The device is operational                                  |
| Blinking | The device failed to connect to the internet.              |
| On       | The reader is temporarily disabled to prevent overheating. |

The red light will also briefly flash whenever a card not on the card list is scanned, as well as whenever the device fails to connect to the server.

## Green (OK)

The green light will flash briefly whenever the device is successfully unlocked. It has no other use.

## Device status

The red and blue light can be used together to get a overview of the current device state.

| Blue     | Red              | Meaning                                                                                                               |
| -------- | ---------------- | --------------------------------------------------------------------------------------------------------------------- |
| Off      | Off              | The device is busy doing network communication, or turned off.                                                        |
| Off      | On               | The lock is in cooldown mode to prevent it from overheating. It should be ready within 60 seconds.                    |
| Blinking | Off              | The device is connected to the network but either failed to connect to the server or downloaded an empty card list.   |
| Blinking | Blinking (short) | Downloading the card list from the server has failed. The server is unreachable or the file is malformed.             |
| Blinking | Blinking (long)  | The device failed to connect to the network or obtain a DHCP lease.                                                   |
| On       | Off              | The device has a non-empty card list and is ready to accept cards.                                                    |
| On       | Blinking (short) | The last attempt to download the card list failed. The device still has the old card list in memory.                  |
| On       | Blinking (long)  | The device got disconnected from the network or failed to renew its DHCP lease. It still has the card list in memory. |
