import evdev

device = evdev.InputDevice('/dev/input/event6')
print(device)
for event in device.read_loop():
    print(event)
