# jsmartsw_fw

This is the firmware repository for the jSmartSW firmware. Information here is limited to the software development aspect.

For functionality and day-to-day usage, please refer to the [main project repository](https://github.com/jeffqchen/jSmartSW)

## Environment

### Hardware
Raspberry Pi Pico

### Software:
Arduino IDE v2.3.5

## Libraries Used

- [Adafruit_SSD1306](https://github.com/adafruit/Adafruit_SSD1306)
- [FastLED](https://github.com/FastLED/FastLED)
- [TinyUSB](https://github.com/hathach/tinyusb)

## How To Build

Configure the Arduino IDE with the following settings:

Tools ->
`Board`: `Raspberry Pi Pico`
`Port`: The corresponding port when you plug in the Pi Pico into your computer
`Flash Size`: `2MB (no FS)`
`Upload Method`: `Default (UF2)` (If you are connecting the Pi Pico to your computer via a USB cable.)
`USB Stack`: `Adafruit TinyUSB Host (native)`

## How To Upload

Hold the `BOOTSEL` button on the Pi Pico and then plug it into the computer. Then choose the new port that appears in the `Tools` -> `Port menu`. Then click the `Upload` button.
