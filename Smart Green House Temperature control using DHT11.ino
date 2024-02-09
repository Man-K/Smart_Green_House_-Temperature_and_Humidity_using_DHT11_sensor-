import Adafruit_DHT
import json
import time
from paho.mqtt import client as mqtt_client
from gpiozero import LED
import RPi.GPIO as GPIO

broker = '192.168.100.131'
port = 1883
topic_publish = 'topic/satu'
topic_subscribe = 'topic/kedua'
client_id = 'raspberry_pi'
username = ''
password = ''

pin_led_hijau = 17  # Pin GPIO untuk LED hijau
pin_led_kuning = 18  # Pin GPIO untuk LED kuning
pin_led_merah = 27  # Pin GPIO untuk LED merah
pin_dht11 = 4  # Pin GPIO untuk sensor DHT11

led_hijau = LED(pin_led_hijau)
led_kuning = LED(pin_led_kuning)
led_merah = LED(pin_led_merah)

GPIO.setmode(GPIO.BCM)
GPIO.setup(pin_led_hijau, GPIO.OUT)
GPIO.setup(pin_led_kuning, GPIO.OUT)
GPIO.setup(pin_led_merah, GPIO.OUT)

def get_status(suhu):
    if 20 <= suhu <= 28:
        return "Aman"
    elif 28 <= suhu <= 30: #<= temperature >= 31:
        return "Waspada"
    else:
        return "Berbahaya"

def control_led(suhu):
    Kondisi = get_status(suhu)

    # Matikan semua LED terlebih dahulu
    led_hijau.off()
    led_kuning.off()
    led_merah.off()

    # Nyalakan LED sesuai dengan warna lampu
    if Kondisi == "Aman":
        GPIO.output(pin_led_hijau, GPIO.HIGH)
    elif Kondisi == "Waspada":
        GPIO.output(pin_led_kuning, GPIO.HIGH)
    elif Kondisi == "Berbahaya":
        GPIO.output(pin_led_merah, GPIO.HIGH)
    else:
        print("Unknown condition. LEDs are turned off.")

def connect_mqtt():
    def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected to MQTT Broker!")
        else:
            print("Failed to connect, return code %d\n" % rc)

    client = mqtt_client.Client(client_id)
    client.username_pw_set(username, password)
    client.on_connect = on_connect
    client.connect(broker, port)
    return client

def read_dht11():
    # Membaca data suhu dan kelembapan dari sensor DHT11
    sensor = Adafruit_DHT.DHT11
    kelembapan, suhu = Adafruit_DHT.read_retry(sensor, pin_dht11)
    return kelembapan, suhu

def publish_suhu(client, suhu, kelembapan, Kondisi):
    msg = json.dumps({
        "suhu": suhu,
        "kelembapan": kelembapan,
        "Kondisi": Kondisi
    })
    result = client.publish(topic_publish, msg)
    status = result.rc
    if status == 0:
        print(f"Sent Suhu: {suhu:.2f}°C, Kelembapan: {suhu:.2f}%, status: {Kondisi:}, to topic {topic_publish}")
    else:
        print(f"Failed to send message to topic {topic_publish}")

def on_message(client, userdata, msg):
    print(f"Received {str(msg.payload)} from topic {msg.topic}")

def publishSubscribe(client):
    client.on_message = on_message
    client.subscribe(topic_subscribe)
    while True:
        kelembapan, suhu = read_dht11()
        Kondisi = get_status(suhu)  # Mendapatkan warna lampu berdasarkan suhu
        if kelembapan is not None and suhu is not None and Kondisi is not None:
            print(f"Suhu: {suhu:.2f}°C, Kelembapan: {kelembapan:.2f}%, Kondisi: {Kondisi}")
            publish_suhu(client, suhu, kelembapan, Kondisi)
            control_led(suhu)  # Kontrol LED sesuai dengan suhu
        else:
            print("Failed to read data from DHT11. Retrying...")
        time.sleep(2)

def run():
    client = connect_mqtt()
    client.loop_start()
    publishSubscribe(client)

if _name_ == '_main_':
    run()