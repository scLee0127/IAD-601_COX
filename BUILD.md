# CMake 빌드

필요 도구: CMake 3.22 이상, Ninja, Arm GNU Toolchain
(`arm-none-eabi-gcc`, `arm-none-eabi-objcopy`, `arm-none-eabi-size`).
도구의 `bin` 디렉터리를 PATH에 추가한 터미널에서 프로젝트 루트를 연다.
STM32 VS Code 확장에서 제공하는 빌드 터미널도 사용할 수 있다.

```sh
cmake --preset Debug
cmake --build --preset Debug

cmake --preset Release
cmake --build --preset Release

# VNH7070 검증용 활성 빌드
cmake --preset Release-Motor
cmake --build --preset Release-Motor
```

결과는 `build/Debug/` 또는 `build/Release/`의
`IAD601_COX.elf`, `IAD601_COX.hex`, `IAD601_COX.bin`, `IAD601_COX.map`이다.
`compile_commands.json`도 같은 디렉터리에 생성된다.
현재 `.clangd`는 `build/Debug`를 참조한다.

운영체제나 도구 설치 경로가 바뀌어 기존 캐시를 재사용할 수 없다면
해당 `build/Debug` 또는 `build/Release` 디렉터리를 삭제하고 다시 구성한다.

사용자 소스는 루트 `CMakeLists.txt`의 `target_sources`에 추가한다.
CubeMX가 생성하는 `cmake/stm32cubemx/CMakeLists.txt`와 분리하여 관리한다.
확장자가 없는 `Core/Src/mymotor_success`는 빌드 대상에 포함하지 않는다.

Debug/Release 빌드는 STM32 GNU 14.3.1로 검증했다.
`mymotor.c:434`의 `&&`/`||` 괄호 경고는 남아 있다.
실제 보드 동작은 별도 검증이 필요하며, 호출되지 않는 함수는 기존
`--gc-sections` 설정에 따라 최종 펌웨어에서 제거될 수 있다.

모터 기능은 `Core/Inc/global.h`의 `BOARD_MOTOR_ENABLE=0`으로 비활성화되어 있다.
모터 제어/PID 및 TIM3/TIM14 PWM 코드는 컴파일에서 제외하며, GUI/Modbus의
모터 PWM 상태는 0, 리미트 스위치 상태는 OFF로 반환한다. 모터 설정값 저장 형식은 유지한다.
기능을 복원하려면 `BOARD_MOTOR_ENABLE`을 1로 변경하고 다시 빌드한다.
동일 목적의 `Debug-Motor` 프리셋도 제공되지만, `-O0` Flash 예산을
초과할 수 있으므로 링크 결과를 확인한다.
