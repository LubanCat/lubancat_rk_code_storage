import hmac
from hashlib import sha1

class AuthIfo:
    mqttClientId = ''
    mqttUsername = ''
    mqttPassword = ''

    def calculate_sign_time(self, productKey, deviceName, deviceSecret, clientId, timeStamp):
        self.mqttClientId = clientId + "|securemode=2,signmethod=hmacsha1,timestamp=" + timeStamp + "|"
        self.mqttUsername = deviceName + "&" + productKey
        content = "clientId" + clientId + "deviceName" + deviceName + "productKey" + productKey + "timestamp" + timeStamp
        self.mqttPassword = hmac.new(deviceSecret.encode(), content.encode(), sha1).hexdigest()

    def calculate_sign(self, productKey, deviceName, deviceSecret, clientId):
        self.mqttClientId = clientId + "|securemode=2,signmethod=hmacsha1|"
        self.mqttUsername = deviceName + "&" + productKey
        content = "clientId" + clientId + "deviceName" + deviceName + "productKey" + productKey
        self.mqttPassword = hmac.new(deviceSecret.encode(), content.encode(), sha1).hexdigest()