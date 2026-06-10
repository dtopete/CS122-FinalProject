import serial
import serial.tools.list_ports
import time

def find_pico_port():
    """Automatically finds the COM port the Pico is connected to."""
    ports = serial.tools.list_ports.comports()
    for port in ports:
        # Picos often show up as "USB Serial Device" or have "Pico" in the description
        if "USB Serial" in port.description or "Pico" in port.description:
            return port.device
        
        # Alternatively, fallback to standard macOS/Linux tty/cu formats
        if "usbmodem" in port.device or "ttyACM" in port.device:
            return port.device
            
    return None

def main():
    print("Searching for Raspberry Pi Pico...")
    port = find_pico_port()
    
    # If the auto-detect fails, hardcode your port here (e.g., port = "COM3" or "/dev/cu.usbmodem1101")
    # port = "COM3" 

    if not port:
        print("Error: Could not find Pico. Check your USB connection and CMakeLists.txt.")
        return

    print(f"Pico found on port {port}! Opening serial connection...\n")

    try:
        # Pico standard USB baudrate doesn't strictly matter for virtual COM, but 115200 is standard
        with serial.Serial(port, 115200, timeout=1) as ser:
            time.sleep(1) # Give it a second to connect
            
            while True:
                if ser.in_waiting > 0:
                    # Read the line, decode it to text, and strip trailing whitespace/newlines
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(line)
                        
    except serial.SerialException as e:
        print(f"Serial connection error: {e}")
    except KeyboardInterrupt:
        print("\nExiting monitor...")

if __name__ == "__main__":
    main()