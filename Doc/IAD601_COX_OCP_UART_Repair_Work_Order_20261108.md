# IAD-601_COX 방향별 CS 선택 및 UART2 디버그 점검 작업지시서

개정일은 2026-11-08이다. 기준은 IAD-601_COX_20261108.zip이다. 모터 방향별 current-sense 채널 선택 최소 수정에 UART2 디버그 통신 점검·필요한 최소 수정을 추가한다. Codex는 IAD-601_COX 소스와 이 문서만으로 작업한다.

## 1. 수정 대상

Core/Src/mygpio.c의 MYGPIO_Motor_Apply(int _nDirection, float _nDuty).

기존 단일 샘플·즉시 OCP 판정과 현재 프로젝트의 전류 환산·임계값·CUr 설정을 기준으로 한다. 이번 변경의 대상은 VNH7070의 SEL0 출력이다. 앞선 문서의 고정 raw3800/3.062V 비교 및 CUr 비적용 지시는 이번 방향 선택 작업에서 적용하지 않는다.

## 2. 핀과 처리

| 실제 출력 | INA/PB14 | INB/PB12 | SEL0/PB15 |
|---|---|---|---|
| 하드웨어 양의 방향 | HIGH | LOW | HIGH |
| 하드웨어 음의 방향 | LOW | HIGH | LOW |
| 정지 | LOW | LOW | LOW |

기존 NO/NC 반전 이후 hw_direction을 사용한다. hw_direction>0이면 SEL0를 HIGH, hw_direction<0이면 LOW로 쓴다.

변경은 아래 두 곳이다.

1. MYGPIO_Motor_Apply()의 방향 변경 블록에서 INA/INB를 설정한 다음, 기존 지연 루프 전에 아래 코드를 추가한다.

```c
/* Select CS feedback for the high-side switch actually driving. */
HAL_GPIO_WritePin(MYGPIO_MOTOR_SEL0_GPIO_PORT,
                 MYGPIO_MOTOR_SEL0_GPIO_NUM,
                 (hw_direction > 0) ? GPIO_PIN_SET : GPIO_PIN_RESET);
```

2. 방향 변경 블록 밖에서 매 호출마다 SEL0를 LOW로 쓰던 한 줄을 제거한다. 다음 PWM 호출은 기존 위치를 사용한다.

```c
MYPWM_Set_Duty(MOTOR_PWM_CHANNEL_M0, _nDuty);
```

정지 분기의 SEL0 LOW 처리는 유지한다. 따라서 SEL0 선택은 PWM 인가 전 기존 지연 구간보다 먼저 완료되고, 같은 방향 듀티 갱신에서는 선택 채널을 유지한다.

## 3. 작업 범위

모터 소스 diff는 MYGPIO_Motor_Apply()의 위 두 변경으로 제한한다. 새로운 ADC 구조체·스냅샷·timestamp·sequence·valid·필터·연속 판정·기동 유예·보호 임계값 변경을 포함하지 않는다.

UART2 작업은 아래 6절을 따른다. 함께 제공한 mygpio.c에는 SEL0 변경만 있으며 UART 수정은 포함되어 있지 않다. 모터 수정과 UART 점검·수정은 diff와 검증 결과를 구분해 보고한다.

현재 분석된 COX의 CS 전류 환산식과 정지 PWM·반전 리미트에는 앞선 검토 사항이 남아 있다. 이번 변경 결과는 방향별 채널 선택에 대한 것이며, 실제 전류 A 보정 또는 전체 모터 보호 검증의 완료를 의미하지 않는다.

## 4. 검증

- 기존 소스의 NO/NC 기본 설정에서 양의 방향은 INA/INB/SEL0=1/0/0으로 발생해 요구 조합과 불일치하는 것을 호스트 GPIO 스텁으로 재현했다.
- 수정된 함수는 NO/NC 두 설정, 논리 양·음 방향, 같은 방향 듀티 변경, 정지를 조합한 16개 확인을 통과했다.
- 호스트 시험은 GPIO와 PWM 호출 결과를 확인한 것이며 실제 ADC 전류값, 드라이버 보호 및 기구 동작을 확인한 실기 시험은 아니다.
- Codex는 실제 프로젝트에서 모터 활성·비활성 ARM 빌드를 수행하고, 보드에서 PB12/PB14/PB15를 확인한다. CS/PA6이 양방향에서 올바르게 반응하는지 기존 ADC 진단 경로로 확인한다.

## 5. 적용 파일

함께 제공한 mygpio.c를 대상 프로젝트의 Core/Src/mygpio.c와 diff로 대조해 위 변경만 적용한다. 사용자의 이후 수정이 있는 저장소에는 전체 파일 덮어쓰기 대신 해당 함수의 diff만 반영한다.

## 6. 추가 작업: UART2 디버그 통신 점검 및 최소 수정

### 타깃 파일 경로 및 함수명

| 파일 | 함수·설정 | 작업 |
|---|---|---|
| Core/Inc/myuart.h | MYUART_INSTANCE_DEBUG, UART_BAUD_115200 | 디버그 USART2, RS485 USART1 매핑 확인 |
| Core/Src/myuart.c | MYUART_Debug_Init(), _myuart_uart2_debug_init(), _myuart_convert_baudrate() | 디버그 속도 115200bps 정합화, 초기화 확인 |
| Core/Inc/main.h | DEBUG_UART_TX/RX 핀 | TX=PA2, RX=PA3 확인 |
| Core/Src/stm32f0xx_hal_msp.c | HAL_UART_MspInit()의 USART2 분기 | USART2/GPIOA 클럭 및 PA2/PA3 AF1 확인 |
| Core/Src/main.c | main(), Print_Banner(), __io_putchar() | 초기화 순서·배너·문자 송신 확인 |
| Core/Src/syscalls.c | _write() | 최종 ELF의 printf 리타깃 연결 확인 |
| Core/Inc/global.h | BOARD_DEBUG_MSG_ENABLE, MYUTIL_Printf | 실제 빌드에서 로그 활성 여부 확인 |
| IAD601_COX.ioc | USART2, PA2/PA3 | 115200, 8N1, TX/RX를 런타임 설정과 일치 |

### 현재 로직의 문제점/한계

- 기준 소스의 MYUART_Debug_Init()는 UART_BAUD_230400을 사용한다. PC가 115200으로 열려 있으면 정상 해독 조건이 맞지 않는다. 다만 이것만으로 실기 무출력의 원인을 확정하지 않는다.
- USART2 클럭 활성화, GPIOA PA2/PA3 AF1, TX_RX/8N1 설정은 이미 구현되어 있다. 정상인 부분은 유지한다. .ioc에는 USART2 baud가 명시되어 있지 않아 런타임과 일치하도록 기록한다.
- main()은 SystemClock_Config() → MYGPIO_Init() → MYUART_Debug_Init() → Print_Banner() → 환경 로드 순서다. MYUART_Init()에서 재호출하더라도 gState 가드로 중복 초기화를 피한다. 이 순서와 배너 1회 출력을 유지한다.
- 출력 경로는 MYUTIL_Printf → printf → syscalls.c의 _write → main.c의 __io_putchar → HAL_UART_Transmit(&UartHandler_Debug, ...)다. setvbuf(stdout, NULL, _IONBF, 0)도 이미 있다. 중복 _write 구현이나 새 출력 계층을 추가하지 않는다.
- 현재 debug 송신은 polling 방식이다. UART2 NVIC나 DMA 미설정만으로 배너 무출력의 원인이라고 판단하지 않는다. RX 인터랙티브 명령 기능은 이번 요구사항에 포함하지 않는다.

### 반영해야 할 구체적인 수정 및 점검 순서

1. MYUART_Debug_Init()의 `_myuart_uart2_debug_init(UART_BAUD_230400)`을 `_myuart_uart2_debug_init(UART_BAUD_115200)`으로 변경한다. 변환 함수의 UART_BAUD_115200 반환값이 115200인지 확인한다. 이미 변경되어 있으면 재수정하지 않는다. USART1 속도와 저장 옵션은 유지한다.
2. .ioc의 USART2 설정을 115200bps, 8 data bits, no parity, 1 stop bit, no flow control, TX/RX로 명시한다. CubeMX UI 또는 해당 프로젝트 버전에 맞는 키를 사용하고, 사용자 정의 초기화와 충돌하지 않게 한다. 이 작업만을 위해 프로젝트 전체를 재생성하지 않는다.
3. HAL_UART_Init() 성공, Instance==USART2, GPIOA/USART2 클럭 활성, PA2/PA3 AF1/AF_PP를 확인한다. 이후 GPIO 초기화가 PA2/PA3을 다른 용도로 덮어쓰는지도 확인한다. 불일치가 확인된 부분만 수정한다.
4. 실기 점검용 빌드에서만 MYUART_Debug_Init() 직후, Print_Banner() 전에 짧은 고정 문자열을 HAL_UART_Transmit()으로 1회 직접 보낸다. 예: `static const uint8_t probe[] = "UART2 DIRECT OK\r\n";` 및 `HAL_UART_Transmit(&UartHandler_Debug, (uint8_t *)probe, sizeof(probe)-1U, 100U);`. 반환 상태는 디버거로 확인 가능한 지역 변수로 보관한다. 송신 실패를 같은 UART printf로 반복 보고하지 않는다. 무한 재시도·추가 대기 루프를 넣지 않는다.
5. 직접 송신은 보이는데 배너가 안 보이면 최종 ELF/MAP에서 _write 및 __io_putchar의 실제 연결 대상과 syscalls.c 포함 여부를 확인한다. MYUTIL_Printf 확장·조건부 컴파일·stdout 설정도 확인한다. 잘못 연결된 경우에만 기존 출력 경로를 최소 수정한다. 직접 송신도 안 보이면 printf 변경보다 핀·클럭·실제 baud·배선 점검을 먼저 진행한다.
6. 필요하면 디버거에서 USART2 BRR, CR1(UE/TE/RE), ISR, RCC 및 GPIOA MODER/AFRL을 확인한다. 실제 peripheral clock을 기준으로 baud를 검산한다. 고정 문자열이 실패하는 UART에 긴 진단 로그를 의존하지 않는다.
7. 점검용 직접 송신 문자열과 상태 변수는 원인 확인 후 제거하거나 기존 진단 빌드 조건 아래에만 남긴다. 최종 정상 경로는 기존 부팅 배너 1회다. 동작 점검을 위해 주기 로그나 모터 제어 루프의 출력량을 늘리지 않는다.

공식 동작 근거: [ST STM32 HAL UART API 안내](https://community.st.com/stm32-mcus-60/faq-stm32-hal-uart-driver-api-and-callbacks-29)는 HAL_UART_Transmit()을 polling 모드로 설명한다. 기존 방식의 송신 확인에 DMA/IRQ 구조 전환은 필요하지 않다.

### 실기 완료 기준

| 점검 | 조건 | 정상 기준 |
|---|---|---|
| PC 설정 | 115200bps, 8N1, flow control 없음 | 재부팅 전 터미널을 열고 배너 확인 |
| 배선 | MCU PA2(TX) → 3.3V TTL USB-UART RX, 공통 GND | 보드 커넥터 실핀과 신호 경로 확인 |
| 직접 송신 | 초기화 직후 1회 고정 문자열 | HAL_OK 및 PC 정상 문자열 수신 |
| 출력 경로 | 기존 printf/Print_Banner | 부팅당 배너 1회, 문자 깨짐 없음 |
| 연속 동작 | 기존 로그가 발생하는 동작 수행 | 모터 정지·양방향 구동 시 기존 로그 수신 여부 확인 |
| 회귀 | USART1 RS485, PA8 DE | 기존 역할·설정·송신 경로 유지 |

PA2에서 신호가 나오고 PC에 보이지 않으면 커넥터·변환기·PC 설정을 확인한다. PA2에서도 신호가 없으면 초기화 진입·레지스터·리셋 반복을 확인한다. 계측기가 없으면 디버거와 USB-UART 수신 결과까지만 기록하고 파형 확인은 미실행으로 남긴다. UART2 RX 명령 파서는 추가하지 않는다.

### STM32F030 환경의 메모리/오버헤드 제약

64KB Flash/8KB SRAM 범위에서 기존 polling 경로를 유지한다. 새 DMA·링 버퍼·태스크·동적 할당·주기 진단 로그를 추가하지 않는다. 부팅 시 1회 짧은 송신으로 점검하고, 최종 빌드에서 Flash/SRAM 및 설정 저장 영역 침범 여부를 확인한다. 긴 송신 timeout 반복으로 IWDG 리셋이나 제어 지연을 만들지 않는다.

## 7. Codex 실행 프롬프트

COX에 두 작업을 수행하라. (1) 단일 샘플·즉시 OCP 판정 구조를 유지한 모터 방향별 CS 채널 선택 최소 수정, (2) UART2 디버그 통신 점검 및 필요한 최소 수정이다. 참고 펌웨어 없이 이 문서와 대상 저장소만 사용하라. 사용자의 기존 변경을 보존하라.

대상은 Core/Src/mygpio.c의 MYGPIO_Motor_Apply()다. 기존 NO/NC 변환 이후 hw_direction을 사용해 양의 방향은 INA/INB/SEL0=1/0/1, 음의 방향은 0/1/0, 정지는 0/0/0이 되도록 하라. 방향 변경 블록에서 INA/INB 설정 다음과 기존 지연 루프 전에 SEL0를 설정하고, 블록 밖에서 매 호출 SEL0를 LOW로 쓰는 코드를 제거하라.

모터 diff는 위 함수의 SEL0 선택에 한정한다. 기존 ADC·전류 환산·OCP 임계값·CUr·PID·램프·시간 처리·저장 구조를 변경하지 않는다. 이전 문서의 raw3800 고정 임계값 변경은 이번 지시에 포함하지 않는다.

UART2는 위 6절을 그대로 수행하라. MYUART_Debug_Init()의 속도를 115200으로 정합화하고 .ioc도 일치시켜라. PA2 TX/PA3 RX AF1, 클럭, HAL 초기화, 기존 printf 리타깃과 부팅 배너 연결을 확인하라. 직접 HAL 송신 → printf → 부팅 배너 순서의 실기 점검 절차를 제공하고, 보드가 연결되어 있을 때만 실측하라. 기존 polling 송신을 유지하고, DMA·링 버퍼·수신 명령 기능은 추가하지 않는다. USART1 RS485 설정은 변경하지 않는다. 실기 연결이 없으면 정상 통신을 단정하지 않는다.

NO/NC 두 설정에서 양방향, 같은 방향 듀티 갱신, 정지를 검증하고 가능한 모터 활성·비활성 ARM 빌드를 실행하라. 변경 파일·함수, 모터/UART별 diff, 빌드 및 자원 사용량, 실제 검증 결과와 미실행 실기를 구분해 보고하라. 추가 구조 변경은 수행하지 않는다.
