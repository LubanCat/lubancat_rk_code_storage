#!/bin/bash -e

board_info() {
    case $1 in
        0000)
            BOARD_NAME='LubanCat1'
            PIN_ID='1'
            ;;
        0100)
            BOARD_NAME='LubanCat1N'
            PIN_ID='2'
            ;;
        0200)
            BOARD_NAME='LubanCat Zero N'
            PIN_ID='0'
            ;;
        0300)
            BOARD_NAME='LubanCat Zero W'
            PIN_ID='0'
            ;;
        0400)
            BOARD_NAME='LubanCat2'
            PIN_ID='3'
            ;;
        0500)
            BOARD_NAME='LubanCat2N'
            ;;
        0600)
            BOARD_NAME='LubanCat2IO'
            ;;
    esac

    echo "板子名称：" $BOARD_NAME
    echo "板子排号：" $PIN_ID
}


get_index(){
    ADC_RAW=$1
    INDEX=0xff
    declare -a ADC_INDEX=(229 344 460 595 732 858 975 1024)

    for i in 00 01 02 03 04 05 06 07; do
        if [ $ADC_RAW -lt ${ADC_INDEX[$i]} ]; then
            INDEX=$i
            break
        fi	
    done 
}

board_id() {
    ADC_CH2_RAW=$(cat /sys/bus/iio/devices/iio\:device0/in_voltage2_raw)
    ADC_CH3_RAW=$(cat /sys/bus/iio/devices/iio\:device0/in_voltage3_raw)

    echo "ADC_CH2_RAW:"$ADC_CH2_RAW
    echo "ADC_CH3_RAW:"$ADC_CH3_RAW

    get_index $ADC_CH2_RAW
    ADC_CH2_INDEX=$INDEX
    get_index $ADC_CH3_RAW
    ADC_CH3_INDEX=$INDEX

    BOARD_ID=$ADC_CH2_INDEX$ADC_CH3_INDEX
    # echo "BOARD_ID:"$BOARD_ID

}

screen_test(){
    echo "屏幕测试"

    SPI_PIN="42"

    case $PIN_ID in 
    0)
        SPI_PIN="103"
        ;;
    esac

    echo "SPI_D/C引脚:"$SPI_PIN 

    /home/cat/lubancat-test/test/i2c_oled/i2c_oled /dev/i2c-3 &
    /home/cat/lubancat-test/test/i2c_oled/i2c_oled /dev/i2c-5 &
    /home/cat/lubancat-test/test/spi_oled/spi_oled /dev/spidev3.1 $SPI_PIN &
    sleep 1s
    /home/cat/lubancat-test/test/spi_oled/spi_oled /dev/spidev3.0 $SPI_PIN &
    echo "------------------------------"
}

gpio_test(){
    echo "gpio测试"
    declare -a GPIO_PIN=(0 0 0 0 0 0 0 0 0 0 0 0)
    declare -a GPIO_PIN_L2=(0 0 0 0 0 0 0 0 0 0 0 0)
    declare -a GPIO_PIN_L3=(8 101 102 103 152 110 117 95 96 18 40 41)
    declare -a GPIO_PIN_L0=(34 35 36 37 39 40 41 83 84 42 101 102)
    declare -a GPIO_PIN_L1=(36 101 102 103 113 110 117 114 115 84 40 41)

    case $PIN_ID in
        0)
            for ((i=0;i<12;i++))
            do
                GPIO_PIN[$i]=${GPIO_PIN_L0[$i]}
            done
            ;;
        1)
            for ((i=0;i<12;i++))
            do
                GPIO_PIN[$i]=${GPIO_PIN_L1[$i]}
            done
            ;;
        2)
            for ((i=0;i<12;i++))
            do
                GPIO_PIN[$i]=${GPIO_PIN_L2[$i]}
            done
            ;;
        3)
            for ((i=0;i<12;i++))
            do
                GPIO_PIN[$i]=${GPIO_PIN_L3[$i]}
            done
            ;;
    esac
    
    for ((i=0;i<12;i++))
    do
        /home/cat/lubancat-test/test/gpio ${GPIO_PIN[$i]} 0
    done
    sleep 1s
    for ((i=0;i<12;i++))
    do
        /home/cat/lubancat-test/test/gpio ${GPIO_PIN[$i]} 1
        sleep 0.3s
        /home/cat/lubancat-test/test/gpio ${GPIO_PIN[$i]} 0
    done

    echo "------------------------------"
}

pwm_test(){
    echo "pwm测试"
    echo "------------------------------"
    for j in 1 2 3 4; do
    echo "pwm"$j
        for i in 10000 30000 60000 80000 100000 400000 600000 900000 0; do
            /home/cat/lubancat-test/test/pwm $j 1000000 $i
            sleep 0.1s
        done
    done
}

uart_test(){
    echo "uart测试"
    echo "------------------------------"

    DEV_TTY="/dev/ttyS3"

    case $PIN_ID in 
    0)
        DEV_TTY="/dev/ttyS8"
        ;;
    esac

    echo "串口名称:"$DEV_TTY 

    time1=$(date)
    echo $time1 > $DEV_TTY
    echo "send" > $DEV_TTY

    echo "接收结果"
    cat $DEV_TTY  &
    sleep 2s
    sudo killall cat
    echo "~~~~~~~~~~~~~~~~~"
}

test_start(){
    version="1.0.0"
    fix_time="2022/10/25"
    echo "版本号："$version
    echo "创建时间："$fix_time
}


test_finish(){
    echo "测试完成"
}


test_start
board_id
board_info ${BOARD_ID}
screen_test
gpio_test
pwm_test
uart_test
test_finish