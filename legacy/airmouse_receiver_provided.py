import socket
import smbus
import time
import RPi.GPIO as GPIO

# ==========================================
# ★ 수신부 IP 설정 (본인 환경에 맞게 수정!)
SERVER_IP = '192.168.0.15'  
SERVER_PORT = 8080

# 핀 번호 설정 (작성자님 확정)
# 스위치
BTN_L, BTN_R, BTN_S, BTN_M = 17, 18, 27, 22
# 74HC595
DS, SH_CP, ST_CP = 6, 19, 13
# 출력
BUZZER, LED = 24, 23

# I2C MPU6050 설정
bus = smbus.SMBus(1)
Device_Address = 0x68
PWR_MGMT_1 = 0x6B
SMPLRT_DIV = 0x19
CONFIG = 0x1A
GYRO_CONFIG = 0x1B
INT_ENABLE = 0x38
GYRO_ZOUT_H = 0x47
GYRO_YOUT_H = 0x45

# GPIO 초기화
GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM)
for btn in [BTN_L, BTN_R, BTN_S, BTN_M]:
    GPIO.setup(btn, GPIO.IN, pull_up_down=GPIO.PUD_UP)
GPIO.setup([DS, SH_CP, ST_CP, BUZZER, LED], GPIO.OUT)

# ==========================================
# ★ 수정된 숫자 패턴 (작성자님 배선: Q7=A ~ Q0=DP)
# 비트 순서: A B C D E F G DP
# ==========================================
num_patterns = [
    0b11111100, # 0 (A,B,C,D,E,F 켜짐)
    0b01100000, # 1 (B,C 켜짐)
    0b11011010, # 2 (A,B,D,E,G 켜짐)
    0b11110010, # 3 (A,B,C,D,G 켜짐)
    0b01100110, # 4 (F,G,B,C 켜짐)
    0b10110110, # 5 (A,C,D,F,G 켜짐)
    0b10111110, # 6 (A,C,D,E,F,G 켜짐)
    0b11100000, # 7 (A,B,C 켜짐)
    0b11111110, # 8 (전부 켜짐)
    0b11110110  # 9 (A,B,C,D,F,G 켜짐)
]

sensitivity = 1

def MPU_Init():
    bus.write_byte_data(Device_Address, SMPLRT_DIV, 7)
    bus.write_byte_data(Device_Address, PWR_MGMT_1, 1)
    bus.write_byte_data(Device_Address, CONFIG, 0)
    bus.write_byte_data(Device_Address, GYRO_CONFIG, 24)
    bus.write_byte_data(Device_Address, INT_ENABLE, 1)

def read_raw_data(addr):
    try:
        high = bus.read_byte_data(Device_Address, addr)
        low = bus.read_byte_data(Device_Address, addr+1)
        val = (high << 8) | low
        if val > 32768: val -= 65536
        return val
    except:
        return 0

def shift_out(val):
    GPIO.output(ST_CP, False)
    for i in range(8):
        # MSB First 전송 (Bit 7 -> Q7 = A)
        bit = (val >> (7 - i)) & 1 
        GPIO.output(DS, bit)
        GPIO.output(SH_CP, True)
        time.sleep(0.0001)
        GPIO.output(SH_CP, False)
    GPIO.output(ST_CP, True)

def feedback():
    """클릭 시 피드백"""
    GPIO.output(BUZZER, True)
    GPIO.output(LED, True)
    time.sleep(0.05)
    GPIO.output(BUZZER, False)
    GPIO.output(LED, False)

# ==========================================
# 메인 루프
# ==========================================
print("=== 에어마우스 송신부 시작 ===")
MPU_Init()
shift_out(num_patterns[1]) # 초기값 1 표시

client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    print(f"서버({SERVER_IP}) 연결 시도...")
    client_socket.connect((SERVER_IP, SERVER_PORT))
    print(">>> 연결 성공! <<<")
    # 연결 성공음
    GPIO.output(BUZZER, True)
    time.sleep(0.3)
    GPIO.output(BUZZER, False)
except:
    print("오프라인 모드 (연결 실패)")
    client_socket = None

try:
    while True:
        # 1. 센서값 읽기
        gz = read_raw_data(GYRO_ZOUT_H)
        gy = read_raw_data(GYRO_YOUT_H)

        status = "MOVE"
        
        # 2. 민감도 버튼 (누르면 1->2->3 변경)
        if GPIO.input(BTN_M) == 0:
            sensitivity = (sensitivity % 3) + 1
            print(f"민감도 변경: {sensitivity}")
            shift_out(num_patterns[sensitivity]) # 화면 갱신
            feedback()
            time.sleep(0.2) # 디바운싱

        # 3. 마우스 버튼
        if GPIO.input(BTN_L) == 0:
            status = "LEFT"
            feedback()
        elif GPIO.input(BTN_R) == 0:
            status = "RIGHT"
            feedback()
        elif GPIO.input(BTN_S) == 0:
            status = "SCROLL"
            GPIO.output(LED, True)
        else:
            GPIO.output(LED, False)

        # 4. 데이터 전송
        msg = f"{sensitivity},{status},{gz},{gy}\n"
        if client_socket:
            try:
                client_socket.send(msg.encode('utf-8'))
            except:
                break
        
        time.sleep(0.02) # 전송 속도 제어

except KeyboardInterrupt:
    GPIO.cleanup()