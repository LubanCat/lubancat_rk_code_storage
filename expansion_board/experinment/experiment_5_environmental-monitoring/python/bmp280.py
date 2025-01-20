'''
# BMP280 / BME280 over SPI

Homepage: https://www.learningtopi.com/python-modules-applications/bmx280_spi/

BMP280/BME280 python3 driver using the SPI bus to control and read data.

General GPIO Pin (NOT an SPI CS) used to pull the signal pin low.  The SPI CS pins do not operate properly with the SPI kernel driver.

BMP280 datasheet: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf

BME280 datasheet: https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf

BMP280/BME280 Pinout:
    SCL = SCK (SPI Clock)
    SDO = MISO (sensor out to board in)
    SDA = MOSI (sensir in to board out)
    CSB = CS (select)

Parts:
- BMP280 (Temp and pressure) - https://amzn.to/3YVwblE
- BME280 (Temp, pressure and humidity) - https://amzn.to/3JIxtMr

## Usage:


    # Import bmx280 SPI class
    from bmx280_spi import Bmx280Spi, MODE_NORMAL

    # initialize device
    # cs_chip and cs_pin from "gpioinfo".  gpiod used for platform compatibility.
    bmx = Bmx280Spi(spiBus=0, cs_chip=0, cs_pin=26)
    bmx.set_power_mode(MODE_NORMAL)
    bmx.set_sleep_duration_value(3)
    bmx.set_temp_oversample(1)
    bmx.set_pressure_oversample(1)
    bmx.set_filter(0)
    reading = bmx.update_reading() # returns instance of Bmx280Readings
    print(reading)
    # --or--
    print(reading.temp_c, reading.temp_f, reading.pressure_psi)

## Testing
Included in the module is a basic test script that can be executed with the following:

    python3 -m dht11_spi [gpio]

Additional test options are available for interval, run time, dht22.  Documentation is available using the "--help" option.

### Example Output

    DHT11: 105/105 (100.0%): Temps (min/avg/max): 73.54/75.2/75.34 deg FHumidity (min/avg/max): 17.0/17.0/17.0 %
    DHT22: 112/112 (100.0%): Temps (min/avg/max): 74.48/74.51/74.66 deg FHumidity (min/avg/max): 14.1/14.21/16.0 %

MIT License

Copyright (c) 2023 LearningToPi

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

'''
import spidev
import logging_handler
from threading import Lock
from time import time, sleep

DEBUG = False

SPI_MIN_HZ =      1_000
SPI_MAX_HZ = 10_000_000

BMX_MODEL_IDS = {
    '0x58': 'BMP280',
    '0x60': 'BME280'
}

WRITE_MASK =         0b10000000
WRITE_RETRY =        3 # number of write retries
WRITE_RETRY_DELAY =  1 # delay before retries

REG_ID =             0xD0

# REG_CONFIG and bit masks
REG_CONFIG =         0xF5
REG_CONFIG_STANDBY = 0b11100000
REG_CONFIG_FILTER =  0b00011110
REG_CONFIG_SPI_3W =  0b00000001
STANDBY_VALUES = (0.5, 62.5, 125, 250, 500, 1000, 2000, 4000) # ms between reads in normal mode, index corresponds to values 0-7

# REG_CTRL_MEAS and bit masks
REG_CTRL_MEAS =                0xF4
REG_CTRL_MEAS_TEMP_OVERSMPL =  0b11100000
REG_CTRL_MEAS_PRESS_OVERSMPL = 0b00011100
REG_CTRL_MEAS_POWER =          0b00000011
REG_CTRL_HUMID =               0xF2
REG_CTRL_HUMID_OVERSMPL =      0b00000111
OVERSAMPLE_VALUES =            (0, 1, 2, 4, 8, 16, 16, 16) # 3 bits value 0-7 match to sampling table.  Last 3 values all mean 16
MODE_STANDBY = 0
MODE_FORCED = 1
MODE_NORMAL = 3
MODE_VALUES = (MODE_STANDBY, MODE_FORCED, MODE_FORCED, MODE_NORMAL)

# REG_STATUS and bit masks
REG_STATUS = 0xF3
REG_STATUS_MEASURE = 0b00001000
REG_STATUS_UPDATE =  0b00000001

# REG_RESET
REG_RESET =       0xE0
REG_RESET_VALUE = 0xB6

# REG_TEMP
REG_TEMP =       0xFA
REG_TEMP_COUNT = 3

# REG_PRESSURE
REG_PRESS =  0xF7
REG_PRESS_COUNT = 3

# REG_HUMID
REG_HUMID = 0xFD
REG_HUMID_COUNT = 2

# REG_TRIM_TEMP/PRESS/HUMID - Hard coded trimming 
REG_TRIM_TEMP_T1 = 0x88 # unsigned short
REG_TRIM_TEMP_T2 = 0x8A # SIGNED short
REG_TRIM_TEMP_T3 = 0x8C # SIGNED short
REG_TRIM_PRESS_P1 = 0x8E # unsigned short
REG_TRIM_PRESS_P2 = 0x90 # SIGNED short
REG_TRIM_PRESS_P3 = 0x92 # SIGNED short
REG_TRIM_PRESS_P4 = 0x94 # SIGNED short
REG_TRIM_PRESS_P5 = 0x96 # SIGNED short
REG_TRIM_PRESS_P6 = 0x98 # SIGNED short
REG_TRIM_PRESS_P7 = 0x9A # SIGNED short
REG_TRIM_PRESS_P8 = 0x9C # SIGNED short
REG_TRIM_PRESS_P9 = 0x9E # SIGNED short
REG_TRIM_HUMID_H1 = 0XA1 # unsigned Char
REG_TRIM_HUMID_H2 = 0XE1 # SIGNED short
REG_TRIM_HUMID_H3 = 0XE3 # unsigned short
REG_TRIM_HUMID_H4_H5 = 0XE4 # SIGNED short - 12 bits H4, 12bits H5 in 3 registers
REG_TRIM_HUMID_H6 = 0xE7 # signed Char

REG_TRIM_PRESS = 0x8E
REG_TRIM_PRESS_COUNT = 18

class BMX280_Exception(Exception):
    ''' Error for a BME280 or BMP280 sensor connected via an SPI bus '''
    pass

# Attempt import of different gpio libraries
try:
    import gpiod
    GPIO_LIBRARY = 'gpiod'
except:
    raise BMX280_Exception("Could not load gpio library.  Supported libraries: gpiod")


class Bmx280Readings:
    ''' Class to represent a reading performed on BME280 or BMP280 sensor '''
    def __init__(self, temp, pressure, humidity=None):
        self.temp = temp
        self.pressure = pressure
        self.humidity = round(humidity, 4) if humidity is not None else None

    def __str__(self):
        ''' Return the class as a printable string '''
        return f"Temp: {self.temp_c}degC ({self.temp_f}degF), Pressure: {self.pressure_pa}Pa ({self.pressure_psi}psi) ({self.pressure_atm}atm)" + (f", Humidity: {self.humidity}%" if self.humidity is not None else "")

    @property
    def pressure_pa(self):
        ''' Return pressure in Pa (Pascal) '''
        return self.pressure

    @property
    def pressure_psi(self):
        ''' Return pressure in psi (pounds per sq inch) '''
        return round(self.pressure / 6895, 4)

    @property
    def pressure_atm(self):
        ''' Return pressure in atm (standard atmosphere) '''
        return round(self.pressure / 101300, 6)

    @property
    def temp_c(self):
        ''' Return temp in degrees celcius '''
        return self.temp

    @property
    def temp_f(self):
        ''' return temp in degrees Fahrenheit '''
        return round(self.temp * 9 / 5 + 32, 4)


class Bmx280Spi:
    ''' Class to represent a BME280 or BMP280 sensor connected via an SPI bus '''
    def __init__(self, spiBus:int, cs_pin:int, cs_chip=None, set_spi_hz=True, logger=None,
                temp_enable=True, pressure_enable=True, humidity_enable=True, filter=0,
                temp_oversample=1, pressure_oversample=1, humidity_oversample=1,
                mode=MODE_FORCED, sleep_duration=0, cs=0):
        self._lock = Lock()
        self._log_extra = f"SPI{spiBus}.{cs_pin}"
        self._logger = logger if logger is not None else logging_handler.create_logger('DEBUG' if DEBUG else 'INFO')
        self._logger.info(f"{self.info_str}: Opening")
        self._spi = spidev.SpiDev()
        self._spi.open(spiBus, cs)
        
        # check and lower or raise the speed if needed
        if self._spi.max_speed_hz > SPI_MAX_HZ and set_spi_hz:
            self._logger.info(f"{self.info_str}: Changing SPI Bus Speed from {self._spi.max_speed_hz} to {SPI_MAX_HZ}")
            self._spi.max_speed_hz = SPI_MAX_HZ
        elif self._spi.max_speed_hz < SPI_MIN_HZ and set_spi_hz:
            self._logger.info(f"{self.info_str}: Changing SPI Bus Speed from {self._spi.max_speed_hz} to {SPI_MIN_HZ}")
            self._spi_max_speed_hz = SPI_MIN_HZ
        if self._spi.max_speed_hz < SPI_MIN_HZ or self._spi.max_speed_hz > SPI_MAX_HZ:
            self._logger.error(f"{self.info_str}:  SPI Bus Speed is: {self._spi.max_speed_hz} range is: {SPI_MIN_HZ}-{SPI_MAX_HZ}")
            raise BMX280_Exception(f'SPI Bus {spiBus} Speed is: {self._spi.max_speed_hz} range is: {SPI_MIN_HZ}-{SPI_MAX_HZ}')

        # Create Soft CS
        if GPIO_LIBRARY == 'gpiod':
            self._cs = gpio_SPI_Cs(chip=cs_chip, pin=cs_pin)
        else:
            raise BMX280_Exception('Unable to open CS. GPIO Library error.')

        # get sensor model
        id_reg = self._read_reg(REG_ID)
        if str(hex(id_reg)) in BMX_MODEL_IDS:
            self.model = BMX_MODEL_IDS[str(hex(id_reg))]
            self._logger.info(f"{self.info_str}: Sensor model is {self.model}")
        else:
            raise BMX280_Exception(f'Unable to identify device from ID Reg {id_reg} ({str(hex(id_reg))}).  Supported models are: {BMX_MODEL_IDS}')

        # get trim values for temp and pressure
        self._trim_temp = [
            unsigned_short(self._read_reg(REG_TRIM_TEMP_T1, 2), reverse=True),
            signed_short(self._read_reg(REG_TRIM_TEMP_T2, 2), reverse=True),
            signed_short(self._read_reg(REG_TRIM_TEMP_T3, 2), reverse=True)
        ]
        self._trim_press = [
            unsigned_short(self._read_reg(REG_TRIM_PRESS_P1, 2), reverse=True),
            signed_short(self._read_reg(REG_TRIM_PRESS_P2, 2), reverse=True),
            signed_short(self._read_reg(REG_TRIM_PRESS_P3, 2), reverse=True),
            signed_short(self._read_reg(REG_TRIM_PRESS_P4, 2), reverse=True),
            signed_short(self._read_reg(REG_TRIM_PRESS_P5, 2), reverse=True),
            signed_short(self._read_reg(REG_TRIM_PRESS_P6, 2), reverse=True),
            signed_short(self._read_reg(REG_TRIM_PRESS_P7, 2), reverse=True),
            signed_short(self._read_reg(REG_TRIM_PRESS_P8, 2), reverse=True),
            signed_short(self._read_reg(REG_TRIM_PRESS_P9, 2), reverse=True)
        ]
        self._logger.debug(f"{self.info_str}: Temp Trim: {self._trim_temp}")
        self._logger.debug(f"{self.info_str}: Pressure Trim: {self._trim_press}")

        # if BME280, get humidity try
        if self.model == 'BME280':
            # Get the 3 registers with H4 and H5 and split up into 2x 12bit values
            H4_H5_reg = self._read_reg(REG_TRIM_HUMID_H4_H5, 3)
            H4_val = (H4_H5_reg[0] << 4) + (H4_H5_reg[1] & 0x0F)
            H5_val = ((H4_H5_reg[2] & 0b11110000) << 4) + ((H4_H5_reg[1] >> 4) & 0x0F)
            self._trim_humid = [
                self._read_reg(REG_TRIM_HUMID_H1), # unsigned char
                signed_short(self._read_reg(REG_TRIM_HUMID_H2, 2), reverse=True),
                self._read_reg(REG_TRIM_HUMID_H3), # unsighed char
                signed_short([], bits=12, value=H4_val),
                signed_short([], bits=12, value=H5_val),
                signed_short([], bits=8, value=self._read_reg(REG_TRIM_HUMID_H6))
            ]
            self._logger.debug(f"{self.info_str}: Humidity Trim: {self._trim_humid}")

        # init the sensor
        self.init(temp_enable=temp_enable, pressure_enable=pressure_enable, humidity_enable=humidity_enable,
                  temp_oversample=temp_oversample, pressure_oversample=pressure_oversample, 
                  humidity_oversample=humidity_oversample, mode=mode, sleep_duration=sleep_duration, filter=filter)

    def init(self, temp_enable=True, pressure_enable=True, humidity_enable=True, filter=0,
                temp_oversample=1, pressure_oversample=1, humidity_oversample=1,
                mode=MODE_FORCED, sleep_duration=0) -> bool:
        ''' Initialize the sensor with the bulk settings 
        Parameters:
            temp_enable (bool:True) - Enables the temperature sensor
            humidity_enable (bool:True) - Enables the humidity sensor (if available)
            pressure_enable (bool:True) - Enables the pressure sensor
            filter (int:0) - Enables the filter (averages readings to reduce fluctuations)
                Supports 0, 2, 4, 8 and 16.  Formula is data_filtered = (previous_data * (filter - 1) + new_data) / filter
            temp_oversample (int:1) - Sets the oversampling rate for temp sensor (0,1,2,4,8) (0 disables the sensor)
            humidity_oversample (int:1) - Sets the oversampling rate for humidity sensor (0,1,2,4,8) (0 disables the sensor)
            pressure_oversample (int:1) - Sets the oversampling rate for pressure sensor (0,1,2,4,8) (0 disables the sensor)
            mode (int:1) - MODE_STANDBY = 0, MODE_FORCED = 1, MODE_NORMAL = 3

        Returns:
            True - Settings were successful
            False - An error occured
        '''
        if not self.set_filter(filter):
            return False
        if not self.set_sleep_duration_value(sleep_duration):
            return False
        if temp_enable:
            if not self.set_temp_oversample(temp_oversample if temp_oversample >= 1 else 1):
                return False
        else:
            if not self.set_temp_oversample(0):
                return False
        if pressure_enable:
            if not self.set_pressure_oversample(pressure_oversample if pressure_oversample >= 1 else 1):
                return False
        else:
            if not self.set_pressure_oversample(0):
                return False
        if self.model == 'BME280':
            if humidity_enable:
                if not self.set_humidity_oversample(humidity_oversample if humidity_oversample >= 1 else 1):
                    return False
            else:
                if not self.set_humidity_oversample(0):
                    return False
        if not self.set_power_mode(mode):
            return False
        return True

    def _read_reg(self, reg:int, count=1) -> int:
        ''' Read a register and return the value '''
        with self._lock:
            self._cs.low()
            self._spi.xfer([reg])
            value = self._spi.readbytes(count)
            self._cs.high()
            self._logger.debug(f"{self.info_str}: Read Register: {reg}, value: {value}")
            return value[0] if isinstance(value, list) and len(value) == 1 else value

    def _write_single_reg(self, reg:int, value:int, retries=WRITE_RETRY, retry_delay=WRITE_RETRY_DELAY, check_reply=True) -> bool:
        '''Write a single register.  Returns True/False if successful (tested by reading back from the sensor) '''
        for x in range(retries):
            with self._lock:
                self._logger.debug(f"{self.info_str}: Writing Register: {reg}, value: {value}")
                self._cs.low()
                self._spi.xfer([reg & ~WRITE_MASK, value])
                self._cs.high()
            if check_reply:
                updated_value = self._read_reg(reg)
                if value == updated_value:
                    return True
                self._logger.warning(f"{self.info_str}: Write error to reg {reg}. Sending {value}, device returned {updated_value}. Retry {x+1}/{retries} after {retry_delay} sec")
                sleep(retry_delay)

    def get_filter(self) -> int:
        ''' Read the filter coefficient from the sensor '''
        return self._read_reg(REG_CONFIG) & REG_CONFIG_FILTER

    def get_sleep_duration_value(self) -> int:
        ''' Return the sleep duration between measurements when in normal mode value (0-7) '''
        return (self._read_reg(REG_CONFIG) & REG_CONFIG_STANDBY) >> get_trailing_bits(REG_CONFIG_STANDBY)

    def get_sleep_duration_ms(self) -> float:
        ''' Return the sleep duration between measurements when in mormal mode in ms '''
        return STANDBY_VALUES[(self._read_reg(REG_CONFIG) & REG_CONFIG_STANDBY) >> get_trailing_bits(REG_CONFIG_STANDBY)]

    def get_spi_3w(self) -> bool:
        ''' Return the SPI 3 Wire configuration '''
        return self._read_reg(REG_CONFIG) & REG_CONFIG_SPI_3W

    def set_filter(self, value:int) -> bool:
        ''' Write the filter coefficient to the sensor.  Returns True/False if successful (tested by reading back from the sensor) '''
        # get the current config 
        config = self._read_reg(REG_CONFIG)
        sleep_duration = config & REG_CONFIG_STANDBY
        spi3w = config & REG_CONFIG_SPI_3W
        self._logger.debug(f"{self.info_str}: Setting IIR Filter to: {value}.  Existing Filter: {(config & REG_CONFIG_FILTER) >> get_trailing_bits(REG_CONFIG_FILTER)}")
        return self._write_single_reg(REG_CONFIG, sleep_duration + set_bits(value, REG_CONFIG_FILTER) + spi3w)

    def set_sleep_duration_value(self, value) -> bool:
        ''' Write the sleep duration between measurements when in nomal mode value (0-7) '''
        if 0 <= value <= 7:
            config = self._read_reg(REG_CONFIG)
            iir_filter = config & REG_CONFIG_FILTER
            spi3w = config & REG_CONFIG_SPI_3W
            self._logger.debug(f"{self.info_str}: Setting sleep duration to: {value}.  Existing Filter: {(config & REG_CONFIG_STANDBY) >> get_trailing_bits(REG_CONFIG_STANDBY)}")
            return self._write_single_reg(REG_CONFIG, set_bits(value, REG_CONFIG_STANDBY) + iir_filter + spi3w)
        raise BMX280_Exception(f'Standby Sleep Duration of {value} outside supported range of 0-7')

    def set_sleep_duration_ms(self, value:float) -> float:
        ''' Write the sleep diration between measurements when in normal mode in ms.  Sets to closest match rounding down.
            Values: (0.5, 62.5, 125, 250, 500, 1000, 2000, 4000) '''
        sleep_ms = STANDBY_VALUES[0]
        for x in range(len(STANDBY_VALUES)):
            sleep_ms = STANDBY_VALUES[x] if STANDBY_VALUES[x] < value else sleep_ms
        self._logger.debug(f"{self.info_str}: Received sleep duration of {value}, setting to {sleep_ms}")
        return self.get_sleep_duration_ms()

    def get_temp_oversample(self) -> int:
        ''' Return the oversampling rate for temperature sensor '''
        return OVERSAMPLE_VALUES[(self._read_reg(REG_CTRL_MEAS) & REG_CTRL_MEAS_TEMP_OVERSMPL) >> get_trailing_bits(REG_CTRL_MEAS_TEMP_OVERSMPL)]

    def get_pressure_oversample(self) -> int:
        ''' Return the oversampling rate for the pressure sensor '''
        return OVERSAMPLE_VALUES[(self._read_reg(REG_CTRL_MEAS) & REG_CTRL_MEAS_PRESS_OVERSMPL) >> get_trailing_bits(REG_CTRL_MEAS_PRESS_OVERSMPL)]

    def get_humidity_oversample(self) -> int:
        '''Return the oversampling rate for the humidity sensor '''     
        if self.model == 'BME280':
            return OVERSAMPLE_VALUES[(self._read_reg(REG_CTRL_HUMID) & REG_CTRL_HUMID_OVERSMPL) >> get_trailing_bits(REG_CTRL_HUMID_OVERSMPL)]
        self._logger.warning(f"{self.info_str}: Device model {self.model} does not support the humidity sensor.")
        return None

    def get_power_mode(self) -> int:
        ''' Get the current power mode and return.  Use constants for validation (POWER_STANDBY, POWER_FORCED, POWER_NORMAL).  0 = Standby, 1/2 = Forced, 3 = Normal '''
        return MODE_VALUES[(self._read_reg(REG_CTRL_MEAS) & REG_CTRL_MEAS_POWER) >> get_trailing_bits(REG_CTRL_MEAS_POWER)]

    def set_power_mode(self, value) -> bool:
        ''' Write the power mode. 0 = Standby, 1/2 = Forced, 3 = Normal '''
        if value in MODE_VALUES:
            ctrl_meas = self._read_reg(REG_CTRL_MEAS)
            press_ovsmpl = ctrl_meas & REG_CTRL_MEAS_PRESS_OVERSMPL
            temp_ovsmpl = ctrl_meas & REG_CTRL_MEAS_TEMP_OVERSMPL
            self._logger.debug(f"{self.info_str}: Setting Power Mode to: {value}.  Existing: {(ctrl_meas & REG_CTRL_MEAS_POWER) >> get_trailing_bits(REG_CTRL_MEAS_POWER)}")
            if value == MODE_FORCED:
                self._write_single_reg(REG_CTRL_MEAS, temp_ovsmpl + press_ovsmpl + (value if value in MODE_VALUES else 1), check_reply=False if value == MODE_FORCED else True)
                return True
            return self._write_single_reg(REG_CTRL_MEAS, temp_ovsmpl + press_ovsmpl + (value if value in MODE_VALUES else 1), check_reply=False if value == MODE_FORCED else True)

    def set_temp_oversample(self, value) -> bool:
        ''' Write the oversample rate for the temp sensor.  Values 0,1,2,4,8,16 supported.  Returns True/False if successful '''
        if value in OVERSAMPLE_VALUES:
            ctrl_meas = self._read_reg(REG_CTRL_MEAS)
            press_ovsmpl = ctrl_meas & REG_CTRL_MEAS_PRESS_OVERSMPL
            power_mode = ctrl_meas & REG_CTRL_MEAS_POWER
            self._logger.debug(f"{self.info_str}: Setting Temp Oversample to: {value}.  Existing: {(ctrl_meas & REG_CTRL_MEAS_TEMP_OVERSMPL) >> get_trailing_bits(REG_CTRL_MEAS_TEMP_OVERSMPL)}")
            return self._write_single_reg(REG_CTRL_MEAS, set_bits(OVERSAMPLE_VALUES.index(value), REG_CTRL_MEAS_TEMP_OVERSMPL) + press_ovsmpl + power_mode)
        raise BMX280_Exception(f'Temp oversample of {value} outside supported vaules of 0,1,2,4,8,16')

    def set_pressure_oversample(self, value) -> bool:
        ''' Write the oversample rate for the pressure sensor.  Values 0,1,2,4,8,16.  Returns True/False if successful '''
        if value in OVERSAMPLE_VALUES:
            ctrl_meas = self._read_reg(REG_CTRL_MEAS)
            temp_ovsmpl = ctrl_meas & REG_CTRL_MEAS_TEMP_OVERSMPL
            power_mode = ctrl_meas & REG_CTRL_MEAS_POWER
            self._logger.debug(f"{self.info_str}: Setting Pressure Oversample to: {value}.  Existing: {(ctrl_meas & REG_CTRL_MEAS_PRESS_OVERSMPL) >> get_trailing_bits(REG_CTRL_MEAS_PRESS_OVERSMPL)}")
            return self._write_single_reg(REG_CTRL_MEAS, temp_ovsmpl + set_bits(OVERSAMPLE_VALUES.index(value), REG_CTRL_MEAS_PRESS_OVERSMPL) + power_mode)
        raise BMX280_Exception(f'Pressure oversample of {value} outside supported range of 0,1,2,4,8,16')

    def set_humidity_oversample(self, value) -> bool:
        ''' Write the oversample rate for the humidity sensor.  Values 0,1,2,4,8,16. Returns True/False if successful '''
        if self.model == 'BME280':
            if value in OVERSAMPLE_VALUES:
                ctrl_humid = self._read_reg(REG_CTRL_HUMID)
                other_data = ctrl_humid & (~REG_CTRL_HUMID_OVERSMPL)
                self._logger.debug(f"{self.info_str}: Setting Humidity Oversample to: {value}. Existing: {(ctrl_humid & REG_CTRL_HUMID_OVERSMPL) >> get_trailing_bits(REG_CTRL_HUMID_OVERSMPL)}")
                reg_humid_write =  self._write_single_reg(REG_CTRL_HUMID, other_data + set_bits(OVERSAMPLE_VALUES.index(value), REG_CTRL_HUMID_OVERSMPL))
                if not reg_humid_write:
                    return reg_humid_write
                # Humidity control update requires a write to the CTRL_MEAS register as well, read and write the temp oversample to force
                return self.set_temp_oversample(self.get_temp_oversample())
            raise BMX280_Exception(f'Humidity oversample of {value} outside supported range of 0,1,2,4,8,16')
        self._logger.warning(f"{self.info_str}: Device model {self.model} does not support the humidity sensor.")
        return None

    @property
    def measuring(self) -> bool:
        ''' Returns True/False if the sensor is currently in process of taking a measurement '''
        return (self._read_reg(REG_STATUS) & REG_STATUS_MEASURE) >> get_trailing_bits(REG_STATUS_MEASURE)

    @property
    def updating(self) -> bool:
        ''' Returns True/False if the sensor is currently writing an update '''
        return (self._read_reg(REG_STATUS) & REG_STATUS_UPDATE) >> get_trailing_bits(REG_STATUS_UPDATE)

    def reset_device(self):
        ''' Send a reset command to the device '''
        self._write_single_reg(REG_RESET, REG_RESET_VALUE, check_reply=False)

    def update_readings(self, timeout=2):
        ''' Update the temp/pressure/humidity readings (depending on platform)
            If in standby - set to force and get reading
            If in normal - collect the last readings from the register
            Returns an instance of Bmx280Readings containing the readings '''
        if self.get_power_mode() == MODE_STANDBY:
            # set to forced and wait for the sensor to complete
            cancel_time = time() + timeout
            self.set_power_mode(MODE_FORCED)
            while time() < cancel_time and self.get_power_mode() != MODE_STANDBY:
                sleep(.1)
            
        temp, fine_temp = calculate_temp(self._read_reg(REG_TEMP, REG_TEMP_COUNT), self._trim_temp)
        pressure = calculate_pressure(self._read_reg(REG_PRESS, REG_PRESS_COUNT), self._trim_press, fine_temp)
        humidity = calculate_humidity(self._read_reg(REG_HUMID, REG_HUMID_COUNT), self._trim_humid, fine_temp) if self.model == 'BME280' else None

        return Bmx280Readings(temp=round(temp, 4), pressure=round(pressure, 2), humidity=humidity)

    @property
    def info_str(self):
        ''' Returns the info string for the class (used in logging commands) '''
        return f"{self.__class__.__name__} ({self._log_extra})"

class gpio_SPI_Cs:
    ''' Class to represent a soft CS pin for the SPI bus (separate from the hardware CS pins) 
        per: https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#spi-software
        the spi-bcm2835 Linux SPI driver does not use the hardware CS lines!
    '''
    def __init__(self, chip, pin):
        chip = gpiod.Chip(str(chip), gpiod.Chip.OPEN_BY_NUMBER)
        self._pin = chip.get_line(int(pin))
        self._pin.request(consumer="gpio", type=gpiod.LINE_REQ_DIR_OUT, default_vals=[0])
        #pin_config.consumer = 'SPI_Soft_CS'
        #pin_config.request_type = gpiod.LINE_REQ_DIR_OUT
        #self._pin.request(pin_config)
        self.high()

    def __del__(self):
        self.close()

    def close(self):
        self._pin.release()

    @property
    def state(self):
        ''' Return current CS state '''
        return self._pin.get_value()

    def high(self):
        ''' Set the CS to on '''
        self._pin.set_value(1)
        
    def low(self):
        ''' Set the CS to off '''
        self._pin.set_value(0)


def set_bits(value, mask) -> int:
    ''' Take a value and offset it to fit in the mask '''
    # get bit length of the value passed to make sure it fits
    value_bit_len = len(bin(value).split('b')[1])
    # get bit len of mask
    mask_bit_len = len(bin(mask).split('b')[1].replace('0', ''))
    mask_training_bits = get_trailing_bits(mask)
    if value_bit_len > mask_bit_len:
        raise BMX280_Exception(f"Error setting bit into mask.  {value} ({bin(value)}) doesn't fit in {mask} ({bin(mask)})")
    return value << mask_training_bits

def get_trailing_bits(mask) -> int:
    ''' get the number of trailing bits in a mask ''' 
    mask_bit_len = len(bin(mask).split('b')[1].replace('0', ''))
    return len(bin(mask).split('b')[1]) - mask_bit_len


def unsigned_short(registers:list, reverse=False) -> int:
    ''' Takes a pair of registers and returns an unsigned integer. If reverse, registers are read in reverse order ''' 
    reg = registers
    if reverse:
        reg = registers.copy()
        reg.reverse()
    if len(reg) != 2:
        raise BMX280_Exception(f'Unable to convert list of registers to an unsigned short. Received {reg}.  Requires list of 2.')
    return (reg[0] << 8 ) | reg[1]


def signed_short(registers:list, reverse=False, bits=16, value=None) -> int:
    ''' Takes a pair of registers and returns a signed integer using the specified number of bits. If reverse, registers are read in reverse order '''
    reg = registers
    if reverse:
        reg = registers.copy()
        reg.reverse()
    if len(reg) != 2 and value is None:
        raise BMX280_Exception(f'Unable to convert list of registers to an signed short. Received {reg}.  Requires list of 2.')
    unsigned = unsigned_short(reg) if value is None else value
    if unsigned & (1 << (bits-1)):
        unsigned -= 1 << bits
    return unsigned


def combine_20bit(registers:list) -> int:
    '''Takes 3 registers and returns a 20bit integer.  The 3rd register uses only the first 4 bits ''' 
    if len(registers) != 3:
        raise BMX280_Exception(f'Unable to convert list of registers to an 20bit int. Received {registers}.  Requires list of 3.')
    # drop everything down 4 bits to account for the unused bits in the 3rd register
    value = (registers[0] << 12) | (registers[1] << 4) | (registers[2] >> 4)
    return value


def calculate_temp(registers:list, trim:list, tempraw=None) -> float:
    ''' Calculates the temperature using the trim values.  Return in C and fine temp (used for pressure calculation) ''' 
    temp_raw = combine_20bit(registers) if tempraw is None else tempraw
    if len(trim) != 3:
        raise BMX280_Exception(f"Unable to calculate temperature due to error with Trim values.  Received {trim} but require 3 register values.")
    var1 = (temp_raw / 16384.0 - trim[0] / 1024.0) * trim[1]
    #(adc_T/16384.0 - dig_T1/1024.0)*digT2
    var2 = ((temp_raw / 131072.0 - trim[0] / 8192.0) * (temp_raw / 131072.0 - trim[0] / 8192.0)) * trim[2]
    #((adc_T/131072.0 - dig_T1/8192.0)*(adc_T/131072.0 - dig_T1/8192.0))*dig_T3
    fine = var1 + var2
    temp = fine / 5120.0
    return temp, fine


def calculate_pressure(registers:list, trim:list, fine_temp:float) -> float:
    ''' Calculates the pressure using trim values and fine temp value.  Returns in Pa '''
    press_raw = combine_20bit(registers)
    if len(trim) != 9:
        raise BMX280_Exception(f"Unable to calculate pressure due to error with trim values.  Receivied {trim} but require 9 register values.")
    var1 = (fine_temp/2.0) - 64000.0
    var2 = var1 * var1 * trim[5] / 32768.0
    var2 = var2 + var1 * trim[4] * 2.0
    var2 = (var2 / 4.0) + (trim[3] * 65536.0)
    var1 = (trim[2] * var1 * var1 / 524288.0 + trim[1] * var1) / 524288.0
    var1 = (1.0 + var1 / 32768.0) * trim[0]
    p = 1048576.0 - press_raw
    p = (p - (var2 / 4096.0)) * 6250.0 / var1
    var1 = trim[8] * p * p / 2147483648.0
    var2 = p * trim[7] / 32768.0
    p = p + (var1 + var2 + trim[6]) / 16.0
    return p


def calculate_humidity(registers:list, trim:list, fine_temp:float) -> float:
    ''' Calculates the humidity using trim values and fine temp value.  Returns in % '''
    humid_raw = unsigned_short(registers)
    #var1 = humid_raw - (trim[3] * 64.0 + trim[4] / 16384.0 * (fine_temp - 76800))
    #var1 *= trim[1] / 65536.0 * (1.0 + trim[5] / 67108864.0 * var1 * (1.0 + trim[2] / 67108864.0 * var1))
    #var1 *= 1.0 - trim[0] * var1 / 524288.0
    #return var1 / 100
    var1 = fine_temp - 76800
    #var1 = (((((humid_raw << 14) - (trim[3] << 20) - (trim[4] * var1)) + (16384)) >> 15) * (((((((var1 * trim[5])) >> 10) * (((var1 * trim[2]) >> 11) + \
    #        (32768))) >> 10) + (2097152)) * trim[1] + 8192) >> 14))

    var1 = (((((humid_raw << 14) - ((trim[3]) << 20) - ((trim[4]) * var1)) + (16384)) / 2**15) * (((((((var1 * (trim[5])) / 2**10) * (((var1 * (trim[2])) / 2**11) + (32768))) / 2**10) + (2097152)) * (trim[1]) + 8192) /  2**14))
    var1 = (var1 - (((((var1 /  2**15) * (var1 / 2**15)) / 2**7) * (trim[0])) /2**4))
    var1 = 0 if var1 < 0 else var1
    var1 = 419430400 if var1 > 419430400 else var1
    return var1 / 2**12 / 1024
